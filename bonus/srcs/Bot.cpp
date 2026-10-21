/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Bot.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/16 13:13:28 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/21 17:39:53 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/Server.hpp"
#include "../../includes/Utils.hpp"
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

void Bot::set_epoll_fd(int epoll_fd)
{
	this->_epoll_fd = epoll_fd;
}

bool Bot::has_request(int bot_fd) const
{
	return (this->_requests.find(bot_fd) != this->_requests.end());
}

void Bot::cleanup(clear_type type, int fd, Server *server)
{
	int	bot_fd;
	int	client_fd;

	std::map<int, t_request>::iterator it;
	if (type == CLIENT_DISCONNECTED)
		this->_memory.erase(fd); // clear client memory
	else if (type == BOT_SHUTDOWN)
		this->_memory.clear(); // clear all memory
	it = this->_requests.begin();
	while (it != this->_requests.end())
	{
		bot_fd = it->first;
		client_fd = it->second.client_fd;
		if (type == BOT_SHUTDOWN ||
			((type == REQUEST_COMPLETED || type == REQUEST_FAILED) && bot_fd == fd) // if request completed or failed
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

std::string Bot::create_request(int client_fd, const std::string &nick, const std::string &question)
{
	std::string content;
	std::string request;
	
	content = "{\"model\":\"qwen:latest\",";
	content += "\"messages\":[";
	content += "{\"role\":\"system\",\"content\":";
	content += "\"You are Miku, a japanese singer. ";
	content += "Always answer the latest user message. ";
	content += "The user's current IRC nickname is " + escape_json(nick) + ". ";
	content += "Address the user by their nickname. ";
	content += "If you do not know something, say Miku Miku Beam.\"}";
	if (!this->_memory[client_fd].empty())
	{
		content += ",";
		content += this->_memory[client_fd]; // add previous conversation of client
	}
	content += ",{\"role\":\"user\",\"content\":\"";
	content += escape_json(question) + "\"}"; // add current user question
	content += "],";
	content += "\"stream\":false,";
	content += "\"think\":false,";
	content += "\"keep_alive\":\"10m\",";
	content += "\"options\":{\"num_predict\":512,"; // limit response length
	content += "\"num_ctx\":4096,\"temperature\":0.3}}";
	request = "POST /api/chat HTTP/1.1\r\n"; // create http request to send to ollama
	request += "Host: 127.0.0.1:11434\r\n";
	request += "Content-Type: application/json\r\n";
	request += "Content-Length: " + nb_to_str(content.size()) + "\r\n"; // ollama knows when request ends
	request += "Connection: close\r\n\r\n";
	request += content;
	return (request);
}

// create socket to connect to ollama
bool Bot::start_request(int client_fd, const std::string &nick,
	const std::string &question)
{
	sockaddr_in	addr;
	epoll_event	event;
	t_request	req;
	int			bot_fd;
	int			res;

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
	req.bytes_sent = 0;
	req.response_complete = false;
	req.question = question;
	req.send_buffer = this->create_request(client_fd, nick, question); // create http request with client nick
	req.recv_buffer = ""; // initialize response buffer
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
	epoll_event	epoll_event;
	char		buffer[4096];
	ssize_t		bytes;
	size_t		remaining;

	t_request &req = this->_requests[bot_fd];
	if (event == EPOLLOUT) // socket is ready for writing ->send request to ollama
	{
		remaining = req.send_buffer.size() - req.bytes_sent; // how many bytes still need to be sent
		if (remaining > 0)
		{
			bytes = send(bot_fd, req.send_buffer.c_str() + req.bytes_sent, remaining, MSG_NOSIGNAL);
			if (bytes <= 0)
				return (false);
			req.bytes_sent += bytes; // add bytes to total bytes sent
		}
		if (req.bytes_sent == req.send_buffer.size()) // when entire request has sent -> wait for response
		{
			epoll_event.events = EPOLLIN;
			epoll_event.data.fd = bot_fd;
			if (epoll_ctl(this->_epoll_fd, EPOLL_CTL_MOD, bot_fd, &epoll_event) < 0)
				return (false);
		}
	}
	else if (event == EPOLLIN) // socket is ready for reading ->receive response from ollama
	{
		bytes = recv(bot_fd, buffer, sizeof(buffer), 0);
		if (bytes < 0)
			return (false);
		if (bytes == 0)
		{
			req.response_complete = true; // ollama closed connection ->http response has ended 
			return (true);
		}
		req.recv_buffer.append(buffer, bytes);
	}
	return (true);
}

void Server::send_response(int client_fd, const std::string &msg)
{
	std::map<int, Client>::iterator it;
	
	it = this->_clients.find(client_fd); // find client that sent question
	if (it == this->_clients.end()) // client disconnected before miku answer -> ignore response
		return ;
	this->send_msg(it->second, ":Miku!Hatsune@127.0.0.1 PRIVMSG " + it->second.get_nick() + " :" + msg + "\r\n", EPOLLIN | EPOLLOUT);
}

bool Bot::handle_bot_event(Server &server, int bot_fd, int events)
{
	try
	{
		t_request &req = this->_requests[bot_fd];
		std::string &memory = this->_memory[req.client_fd];
		std::string answer;
		
		if ((events & EPOLLERR) // socket has an error
			|| ((events & EPOLLOUT) && !this->handle_request_data(bot_fd, EPOLLOUT)) // socket is ready to send data but failed
			|| ((events & (EPOLLIN | EPOLLHUP)) && !this->handle_request_data(bot_fd, EPOLLIN))) // socket has data to read or connection closed but reading failed
			throw std::runtime_error("Ollama socket error :/");
		if (!req.response_complete)
			return (true); // response is not complete -> wait for more data
		answer = extract_response(req.recv_buffer);
		if (answer == "Ollama returned an error." || answer == "No response from Ollama.")
			throw std::runtime_error("Invalid Ollama response.");
		if (!memory.empty())
			memory += ","; // previous conversation ->add comma to separate msgs 
		memory += "{\"role\":\"user\",\"content\":\"" + escape_json(req.question) + "\"}"; // add user question to memory
		memory += ",{\"role\":\"assistant\",\"content\":\"" + escape_json(answer) + "\"}"; // add miku answer to memory
		server.send_response(req.client_fd, answer); // send answer to client
		this->cleanup(REQUEST_COMPLETED, bot_fd, &server);
		return (true);
	}
	catch (const std::exception &e)
	{
		std::cerr << "Miku: " << e.what() << std::endl;
		try
		{
			this->cleanup(REQUEST_FAILED, bot_fd, &server); // close failed request and notify client
		}
		catch (const std::exception &error)
		{
			std::cerr << "Miku cleanup: " << error.what() << std::endl;
		}
		return (false);
	}
}
