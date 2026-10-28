/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Bot.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/16 13:13:28 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/28 22:57:23 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/Server.hpp"
#include "../../includes/Utils.hpp"
#include "../includes/BotUtils.hpp"
#include "../includes/Bot.hpp"
#include <arpa/inet.h>
#include <cerrno>
#include <fcntl.h>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>

Bot::Bot()
{
	this->_epoll_fd = -1;
}

Bot::~Bot()
{
	this->cleanup(BOT_SHUTDOWN, -1, NULL); // clear all bot requests and memory
}

void	Bot::set_epoll_fd(int epoll_fd)
{
	this->_epoll_fd = epoll_fd;
}

bool	Bot::has_request(int bot_fd) const
{
	return (this->_requests.find(bot_fd) != this->_requests.end());
}

void Bot::clear_channel(const std::string &channel)
{
    this->_chan_memory.erase(channel);
}

void	Bot::cleanup(clear_type type, int fd, Server *server)
{
	std::map<int, t_request>::iterator it;
	int								   bot_fd;
	int								   client_fd;

	if (type == CLIENT_DISCONNECTED)
		this->_user_memory.erase(fd);
	else if (type == BOT_SHUTDOWN)
	{
		this->_user_memory.clear();
		this->_chan_memory.clear();
	}
	it = this->_requests.begin();
	while (it != this->_requests.end())
	{
		bot_fd = it->first; // bot fd associated with request
		client_fd = it->second.client_fd; // client fd associated with bot request
		if (type == BOT_SHUTDOWN || ((type == REQUEST_COMPLETED || type == REQUEST_FAILED) && bot_fd == fd) // if request completed or failed
			|| (type == CLIENT_DISCONNECTED && client_fd == fd)) // if client disconnected
		{
			++it;
			if (this->_epoll_fd != -1)
				epoll_ctl(this->_epoll_fd, EPOLL_CTL_DEL, bot_fd, NULL);
			close(bot_fd);
			this->_requests.erase(bot_fd);
			if (type == REQUEST_FAILED && server != NULL)
				server->send_response(client_fd, "Miku is unavailable.");
		}
		else
			++it;
	}
}

std::string Bot::create_request(int client_fd, const std::string &nick, const std::string &channel, const std::string &question)
{
	std::string content;
	std::string request;
	std::string memory;

	if (channel.empty())
		memory = this->_user_memory[client_fd];
	else
		memory = this->_chan_memory[channel];
	content = "{\"model\":\"qwen:latest\",";
	content += "\"messages\":[";
	content += "{\"role\":\"system\",\"content\":";
	content += "\"You are Miku, a japanese singer. Remember important facts from previous conversations. ";
	content += "Use previous messages to understand current conversation. ";
	content += "The user's current name is " + convert_json(nick, JSON_ESCAPE) + ". ";
	content += "If you do not know something say Miku Miku Beam.\"}";
	if (!memory.empty())
		content += "," + memory; // if there is previous conversation->add it to request
	content += ",{\"role\":\"user\",\"content\":\"";
	content += convert_json(nick + ": " + question, JSON_ESCAPE) + "\"}],"; // add current user question
	content += "\"stream\":false,\"think\":false,\"keep_alive\":\"10m\",\"options\":{\"num_predict\":512,\"num_ctx\":4096,\"temperature\":0.3}}";
	request = "POST /api/chat HTTP/1.1\r\n"; // http request to send to ollama
	request += "Host: 127.0.0.1:11434\r\n";
	request += "Content-Type: application/json\r\n";
	request += "Content-Length: " + nb_to_str(content.size()) + "\r\n" + "Connection: close\r\n\r\n"; // ollama knows when request ends
	request += content;
	return (request);
}

// create socket to connect to ollama
bool Bot::start_request(int client_fd, const std::string &nick, const std::string &channel, const std::string &question)
{
	sockaddr_in addr;
	epoll_event event;
	t_request 	req;
	int 		bot_fd;
	int 		res;

	if (this->_epoll_fd < 0)
		return (false);
	bot_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (bot_fd < 0)
		return (false);
	if (fcntl(bot_fd, F_SETFL, O_NONBLOCK) < 0)
	{
		close(bot_fd);
		return (false);
	}
	addr.sin_family = AF_INET;
	addr.sin_port = htons(11434);
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	res = connect(bot_fd, (sockaddr *)&addr, sizeof(addr));
	if (res < 0 && errno != EINPROGRESS)
	{
		close(bot_fd);
		return (false);
	}
	req.client_fd = client_fd;
	req.nick = nick;
	req.channel = channel;
	req.bytes_sent = 0;
	req.response_complete = false;
	req.question = question;
	req.send_buffer = this->create_request(client_fd, nick, channel, question); // create http request with client nick and channel
	req.recv_buffer = "";
	event.events = EPOLLOUT; // start monitoring socket for writing
	event.data.fd = bot_fd;
	if (epoll_ctl(this->_epoll_fd, EPOLL_CTL_ADD, bot_fd, &event) < 0)
	{
		close(bot_fd);
		return (false);
	}
	this->_requests[bot_fd] = req; // store request in map
	return (true);
}

// send() - send remaining request data TO ollama through bot socket.
// recv() - receive response data FROM ollama through bot socket.
// req.send_buffer.c_str() + req.bytes_sent ->pointer to first byte of remaining data to send 
bool Bot::handle_request_data(int bot_fd, int event)
{
    t_request 	&req = this->_requests[bot_fd];
	epoll_event	epoll_event;
    char        buffer[4096];
    ssize_t     bytes;
    size_t      remaining;

	if (event == EPOLLOUT) // socket is ready for writing
	{
		remaining = req.send_buffer.size() - req.bytes_sent; // how many bytes still need to be sent
		if (remaining > 0)
		{
			bytes = send(bot_fd, req.send_buffer.c_str() + req.bytes_sent, remaining, MSG_NOSIGNAL);
			if (bytes <= 0)
				return (false);
			req.bytes_sent += bytes; // update sent bytes
		}
		if (req.bytes_sent == req.send_buffer.size())
		{
			epoll_event.events = EPOLLIN; // all request data sent ->monitor socket for reading
			epoll_event.data.fd = bot_fd;
			if (epoll_ctl(this->_epoll_fd, EPOLL_CTL_MOD, bot_fd, &epoll_event) < 0)
				return (false);
		}
	}
	else if (event == EPOLLIN) // socket is ready for reading
	{
		bytes = recv(bot_fd, buffer, sizeof(buffer), 0);
		if (bytes < 0)
			return (false);
		if (bytes == 0)
		{
			req.response_complete = true; // connection closed
			return (true);
		}
		req.recv_buffer.append(buffer, bytes);
	}
	return (true);
}

void Bot::process_bot_response(t_request &req, Server &server)
{
	std::string *memory;
	std::string answer;
	size_t start;
	size_t end;

	if (req.recv_buffer.find("HTTP/1.1 200") != 0)
		throw std::runtime_error("Ollama HTTP error.");
	start = req.recv_buffer.find("\"content\":\"");
	if (start == std::string::npos)
		throw std::runtime_error("No response from Ollama.");
	start += 11;
	end = start;
	while (end < req.recv_buffer.size()) // find where miku answer ends
	{
		if (req.recv_buffer[end] == '\\' && end + 1 < req.recv_buffer.size())
			end += 2; // if is an escape char and not the last char ->skip
		else if (req.recv_buffer[end] == '"')
			break ;
		else
			++end; // move to next char
	}
	if (end == req.recv_buffer.size()) // no closing quote found ->incomplete response
		throw std::runtime_error("Incomplete Ollama response.");
	answer = convert_json(req.recv_buffer.substr(start, end - start), JSON_UNESCAPE); // extract answer and convert json to normal text
	server.send_response(req.client_fd, answer); // send answer to client
	if (req.channel.empty())
		memory = &this->_user_memory[req.client_fd]; // if no channel ->store memory for this client
	else
		memory = &this->_chan_memory[req.channel];
	if (!memory->empty())
		*memory += ","; // separate previous msgs with comma
	*memory += "{\"role\":\"user\",\"content\":\"" + convert_json(req.nick + ": " + req.question, JSON_ESCAPE) + "\"}"; // add user question to memory
	*memory += ",{\"role\":\"assistant\",\"content\":\"" + convert_json(answer, JSON_ESCAPE) + "\"}"; // add miku answer to memory
}

bool Bot::handle_bot_event(Server &server, int bot_fd, int events)
{
	try
	{
		std::map<int, t_request>::iterator it; // each bot fd has a corresponding request

		it = this->_requests.find(bot_fd);
		if (it == this->_requests.end())
			return (false);
		if ((events & EPOLLERR) 
			|| ((events & EPOLLOUT) && !this->handle_request_data(bot_fd, EPOLLOUT))
			|| ((events & EPOLLIN) && !this->handle_request_data(bot_fd, EPOLLIN)))
			throw std::runtime_error("Ollama socket error :/");
		if (!it->second.response_complete)
			return (true); // response is not complete ->wait for more data
		this->process_bot_response(it->second, server);
		this->cleanup(REQUEST_COMPLETED, bot_fd, &server);
		return (true);
	}
	catch (const std::exception &e)
	{
		std::cerr << "Miku: " << e.what() << std::endl;
		try
		{
			this->cleanup(REQUEST_FAILED, bot_fd, &server);
		}
		catch (const std::exception &error)
		{
			std::cerr << "Miku cleanup: " << error.what() << std::endl;
		}
		return (false);
	}
}
