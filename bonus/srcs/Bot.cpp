/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Bot.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/16 13:13:28 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/11/25 18:07:20 by pedro            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/Server.hpp"
#include "../../includes/Utils.hpp"
#include "../includes/Bot.hpp"
#include "../includes/BotUtils.hpp"

Bot::Bot(std::string name, std::string persona_path)
	: _epoll_fd(-1), _name(name)
{	
	// mkdir(".logs", 0755);
	std::ifstream master_file(BOT_MASTER_FILE);
	if (!master_file.is_open())
		throw std::runtime_error("could not open master file: " + std::string(BOT_MASTER_FILE));
	std::ifstream persona_file(persona_path.c_str());
	if (!persona_file.is_open())
		throw std::runtime_error("could not open personality file: " + persona_path);
	std::ostringstream buffer;
	buffer << persona_file.rdbuf() << "\n" << master_file.rdbuf();
	if (buffer.str().empty())
		throw std::runtime_error("personality content is empty");
	this->_personality = buffer.str();
	this->log_debug("", "");
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

void Bot::clear_channel(const std::string &channel)
{
	this->_chan_memory.erase(channel);
}

void Bot::cleanup(clear_type type, int fd, Server *server)
{
	std::map<int, t_request>::iterator requestit;
	std::map<int, Client>::iterator client_it;
	int bot_fd, client_fd;
	if (type == CLIENT_DISCONNECTED)
		this->_user_memory.erase(fd);
	else if (type == BOT_SHUTDOWN || type == BOT_REMOVED)
	{
		this->_user_memory.clear();
		this->_chan_memory.clear();
	}
	requestit = this->_requests.begin();
	while (requestit != this->_requests.end())
	{
		bot_fd = requestit->first;
		client_fd = requestit->second.client_fd;
		if (type != BOT_SHUTDOWN && type != BOT_REMOVED && !(type == CLIENT_DISCONNECTED && client_fd == fd) && !((type == REQUEST_COMPLETED || type == REQUEST_FAILED) && bot_fd == fd))
		{
			requestit++;
			continue ;
		}
		if (this->_epoll_fd != -1)
			epoll_ctl(this->_epoll_fd, EPOLL_CTL_DEL, bot_fd, NULL);
		close(bot_fd);
		this->_requests.erase(requestit++);
		if ((type == REQUEST_FAILED || type == BOT_REMOVED) && server != NULL)
		{
			client_it = server->getClients().find(client_fd);
			if (client_it != server->getClients().end())
			{
				std::string msg;
				if (type == BOT_REMOVED)
					msg = this->getName() + " has been removed";
				else
					msg = this->getName() + " is unavailable.";
				server->send_notice(client_it->second, msg);
			}
		}
	}
}

t_request *Bot::get_request(int fd)
{
	std::map<int, t_request>::iterator it;
	it = this->_requests.find(fd);
	if (it == this->_requests.end())
		return (NULL);
	return (&it->second);
}

std::string Bot::create_request(Client client, const std::string &channel, const std::string &question)
{
	std::string content;
	std::string request;
	std::string memory;
	if (channel.empty())
		memory = this->_user_memory[client.get_client_fd()];
	else
		memory = this->_chan_memory[channel];
	content = "{\"model\":\"qwen3:latest\",";
	content += "\"messages\":[";
	content += "{\"role\":\"system\",\"content\":\"";
	content += convert_json(this->_personality, JSON_ESCAPE);
	content += " The user's current name is " + convert_json(client.get_nick(), JSON_ESCAPE) + ".";
	if(client.is_server_op())
		content += " The current user is a Server Operator";
	else
		content += " The current user is not a Server Operator";
	content += ".\"}";
	if (!memory.empty())
		content += "," + memory; // if there is previous conversation->add it to request
	content += ",{\"role\":\"user\",\"content\":\"";
	content += convert_json(client.get_nick() + ": " + question, JSON_ESCAPE) + "\"}],"; // add current user question
	content += "\"stream\":false,\"think\":false,\"keep_alive\":\"10m\",\"options\":{\"num_predict\":512,\"num_ctx\":4096,\"temperature\":0.3}}";
	request = "POST /api/chat HTTP/1.1\r\n"; // http request to send to ollama
	request += "Host: 127.0.0.1:11434\r\n";
	request += "Content-Type: application/json\r\n";
	request += "Content-Length: " + nb_to_str(content.size()) + "\r\n" + "Connection: close\r\n\r\n"; // ollama knows when request ends
	request += content;
	// std::cout << request << std::endl;
	this->log_debug("Request for " + client.get_nick(), convert_json(question, JSON_UNESCAPE));
	return (request);
}

// create socket to connect to ollama
bool Bot::start_request(Client client, const std::string &channel, const std::string &question)
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
	req.client_fd = client.get_client_fd();
	req.nick = client.get_nick();
	req.channel = channel;
	req.response_complete = false;
	req.question = question;
	req.send_buffer = this->create_request(client, channel, question); // create http request with client nick and channel
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

static bool	is_irc_cmd(const std::string &cmd)
{
	size_t	i;

	std::vector<std::string> cmds;
	cmds.push_back("JOIN");
	cmds.push_back("INVITE");
	cmds.push_back("TOPIC");
	cmds.push_back("MODE");
	cmds.push_back("PART");
	cmds.push_back("KICK");
	cmds.push_back("PRIVMSG");
	cmds.push_back("WHO");
	cmds.push_back("WHOIS");
	cmds.push_back("HELP");
	cmds.push_back("AWAY");
	cmds.push_back("PING");
	cmds.push_back("BOTADD");
	cmds.push_back("BOTDEL");
	cmds.push_back("BOTLIST");
	i = 0;
	while (i < cmds.size())
	{
		if (cmd == cmds[i])
			return (true);
		i++;
	}
	return (false);
}

void Bot::process_bot_response(t_request &req, Server &server)
{
	epoll_event	event;
	size_t		start;
	size_t		end;

	std::map<int, Client>::iterator client_it;
	std::string *memory;
	std::string answer;
	if (req.recv_buffer.find("HTTP/1.1 200") != 0)
		throw std::runtime_error("Ollama HTTP error.");
	start = req.recv_buffer.find("\"content\":\"");
	if (start == std::string::npos)
		throw std::runtime_error("No response from Ollama.");
	start += 11;
	end = start;
	while (end < req.recv_buffer.size())
	{
		if (req.recv_buffer[end] == '\\' && end + 1 < req.recv_buffer.size())
			end += 2;
		else if (req.recv_buffer[end] == '"')
			break ;
		else
			++end;
	}
	if (end == req.recv_buffer.size())
		throw std::runtime_error("Incomplete Ollama response.");
	answer = convert_json(req.recv_buffer.substr(start, end - start), JSON_UNESCAPE);
	this->log_debug("Response to " + req.nick, answer);
	client_it = server.getClients().find(req.client_fd);
	if (client_it != server.getClients().end())
	{
		Client &client = client_it->second;
		std::istringstream iss(answer);
		std::string line;
		while (std::getline(iss, line))
		{
			if (!line.empty() && line[line.size() - 1] == '\r')
				line.erase(line.size() - 1);
			if (line.empty())
				continue ;
			std::string cmd;
			std::vector<std::string> params;
			parse_msg(line, cmd, params);
			if (!cmd.empty() && is_irc_cmd(cmd))
				server.execute_cmd(client, cmd, params);
			else
				client.get_send_buffer() += ":" + this->_name + "!IRCserv@127.0.0.1 PRIVMSG " + client.get_nick() + " :" + line + "\r\n";
		}
		event.events = EPOLLIN | EPOLLOUT;
		event.data.fd = client.get_client_fd();
		if (epoll_ctl(server.get_epoll_fd(), EPOLL_CTL_MOD, event.data.fd, &event) < 0)
			throw std::runtime_error("IRC: epoll_ctl() failed.");
	}
	if (req.channel.empty())
		memory = &this->_user_memory[req.client_fd];
	else
		memory = &this->_chan_memory[req.channel];
	if (!memory->empty())
		*memory += ",";
	*memory += "{\"role\":\"user\",\"content\":\"" + convert_json(req.nick + ": " + req.question, JSON_ESCAPE) + "\"}";
	*memory += ",{\"role\":\"assistant\",\"content\":\"" + convert_json(answer, JSON_ESCAPE) + "\"}";
}

bool Bot::handle_bot_event(Server &server, int bot_fd, int events)
{
	int	status;

	try
	{
		std::map<int, t_request>::iterator it;
		it = this->_requests.find(bot_fd);
		if (it == this->_requests.end())
			return (false);
		if (events & EPOLLERR)
			throw std::runtime_error("Ollama socket error :/");
		if (events & EPOLLOUT)
		{
			status = server.handle_socket_data(bot_fd, EPOLLOUT, BOT_SOCKET); // send http request when socket is ready for writing
			if (status == SOCKET_ERROR)
				throw std::runtime_error("Ollama send error :/");
		}
		if (events & EPOLLIN)
		{
			status = server.handle_socket_data(bot_fd, EPOLLIN, BOT_SOCKET); // receive http response when socket is ready for reading
			if (status == SOCKET_ERROR)
				throw std::runtime_error("Ollama recv error :/");
		}
		if (!it->second.response_complete)
			return (true); // wait until entire http response has been received
		this->process_bot_response(it->second, server);
		this->cleanup(REQUEST_COMPLETED, bot_fd, &server); // rmv request from map and close bot socket
		return (true);                                     // response is complete and processed successfully
	}
	catch (const std::exception &e)
	{
		std::cerr << this->_name + ": " << e.what() << std::endl;
		try
		{
			this->cleanup(REQUEST_FAILED, bot_fd, &server); // rmv failed request and notify client.
		}
		catch (const std::exception &error)
		{
			std::cerr << this->_name + " cleanup: " << error.what() << std::endl;
		}
		return (false);
	}
}

std::string Bot::getName(void) const
{
	return (this->_name);
}
