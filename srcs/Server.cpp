/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/28 05:42:35 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/11/09 22:37:05 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Server.hpp"
#include "../includes/Channels.hpp"
#include "../includes/Utils.hpp"
#include "../bonus/includes/Bot.hpp"
#include <arpa/inet.h>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

Server::Server(const std::string &port, const std::string &pass)
{
	time_t now;

	this->_port = validate_port(port);
	if (pass.empty())
		throw std::runtime_error("IRC: a server password is required!");
	this->_pass = pass;
	this->_listen_fd = -1;
	this->_epoll_fd = -1;
	this->_name = "ircserv";
	now = time(NULL);
	this->_date = ctime(&now);
	if (!this->_date.empty())
		this->_date.erase(this->_date.size() - 1);
	this->_opers["admin"] = "1234";
}

// close all client connections
Server::~Server()
{
	this->close_connections(SERVER_SHUTDOWN, -1);
}

std::string Server::getName(void) const
{
	return (this->_name);
}

std::map<int, Client> &Server::getClients(void)
{
	return (this->_clients);
}

void Server::close_connections(close_type type, int fd)
{
	std::map<std::string, Channel>::iterator chanit;
	std::map<int, Client>::iterator 		 it;

	if (type == CLIENT_DISCONNECT)
	{
		it = this->_clients.find(fd);
		if (it == this->_clients.end())
			return ;
		this->_bot.cleanup(CLIENT_DISCONNECTED, fd, NULL); // rmv client's pending bot requests
		for (chanit = this->_channels.begin(); chanit != this->_channels.end(); ++chanit)
		{
			if (chanit->second.getClients().count(fd))
				chanit->second.leaveChannel(it->second); // rmv client from all channels
		}
		if (this->_epoll_fd != -1)
			epoll_ctl(this->_epoll_fd, EPOLL_CTL_DEL, fd, NULL); // rmv socket from epoll and close it
		close(fd);
		this->_clients.erase(it);
		std::cout << "\n\033[1;31m- Client " << fd << " disconnected\033[0m\n\n";
	}
	else if (type == SERVER_SHUTDOWN) // close entire server and all client connections
	{
		this->_bot.cleanup(BOT_SHUTDOWN, -1, NULL); // close all bot requests
		for (it = this->_clients.begin(); it != this->_clients.end(); ++it)
			close(it->first); // close all client sockets
		this->_clients.clear();
		this->_channels.clear();
		if (this->_listen_fd != -1)
		{
			close(this->_listen_fd);
			this->_listen_fd = -1;
		}
		if (this->_epoll_fd != -1)
		{
			close(this->_epoll_fd);
			this->_epoll_fd = -1;
		}
		std::cout << "[SERVER] Closed!" << std::endl;
	}
}

// listen() changes socket into a listening socket ready to accept connections
void	Server::start_socket()
{
	sockaddr_in	server_addr;
	int			socket_opt;

	this->_listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (this->_listen_fd < 0)
		throw std::runtime_error("IRC: socket() failed.");
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(this->_port);
	socket_opt = 1;
	if (setsockopt(this->_listen_fd, SOL_SOCKET, SO_REUSEADDR, &socket_opt,
			sizeof(socket_opt)) < 0)
		throw std::runtime_error("IRC: setsockopt() failed.");
	if (fcntl(this->_listen_fd, F_SETFL, O_NONBLOCK) < 0)
		throw std::runtime_error("IRC: fcntl() failed.");
	if (bind(this->_listen_fd, (sockaddr *)&server_addr,
			sizeof(server_addr)) < 0)
		throw std::runtime_error("IRC: bind() failed.");
	if (listen(this->_listen_fd, SOMAXCONN) < 0)
		throw std::runtime_error("IRC: listen() failed.");
	std::cout << "\n[SERVER] Listening on all interfaces (0.0.0.0:" << this->_port << ")" << std::endl;
}

// give bot same epoll fd used by server, bot registers its sockets w/out creating other epoll
// epoll_wait wait till at least one registered socket have events -> return nb of ready events
void Server::monitor_epoll_events()
{
	epoll_event event;
	epoll_event events[10];
	int 		ready;

	this->_epoll_fd = epoll_create1(0);
	if (this->_epoll_fd < 0)
		throw std::runtime_error("IRC: epoll_create1() failed.");
	this->_bot.set_epoll_fd(this->_epoll_fd);
	event.events = EPOLLIN; // monitor listening socket for new connections
	event.data.fd = this->_listen_fd;
	if (epoll_ctl(this->_epoll_fd, EPOLL_CTL_ADD, this->_listen_fd, &event) < 0)
		throw std::runtime_error("IRC: epoll_ctl() failed.");
	while (alive)
	{
		ready = epoll_wait(this->_epoll_fd, events, 10, -1);
		if (ready < 0)
		{
			if (!alive)
				break ;
			throw std::runtime_error("IRC: epoll_wait() failed.");
		}
		for (int i = 0; i < ready; i++)
			this->handle_epoll_event(events[i].data.fd, events[i].events);
	}
}

// EPOLLIN on listening socket -> a new client connection is waiting -> accept it
// EPOLLIN on client socket -> has incoming data ready to be received -> read data FROM client
// EPOLLOUT on client socket -> is ready to send pending data -> write data TO client
int Server::handle_epoll_event(int fd, int events)
{
	int ret;

	if (fd == this->_listen_fd)
	{
		if (events & EPOLLIN)
			this->accept_client(); // socket has pending data->accept new connection and add to epoll
		return (SOCKET_SUCCESS);
	}
	if (this->_bot.has_request(fd))
	{
		this->_bot.handle_bot_event(*this, fd, events); // handle ollama bot socket events
		return (SOCKET_SUCCESS);
	}
	if (this->_clients.find(fd) == this->_clients.end())
		return (SOCKET_ERROR); // ignore sockets dont belong to clients or bot
	if (events & (EPOLLERR | EPOLLHUP))
	{
		this->close_connections(CLIENT_DISCONNECT, fd); // disconnect clients with epoll error or hangup
		return (SOCKET_ERROR);
	}
	if (events & EPOLLIN)
	{
		ret = this->handle_socket_data(fd, EPOLLIN, CLIENT_SOCKET); // receive data from clients
		if (ret == SOCKET_ERROR)
		{
			this->close_connections(CLIENT_DISCONNECT, fd);
			return (SOCKET_ERROR);
		}
	}
	if ((events & EPOLLOUT) && this->_clients.find(fd) != this->_clients.end())
	{
		ret = this->handle_socket_data(fd, EPOLLOUT, CLIENT_SOCKET); // send data if client is still connected
		if (ret == SOCKET_ERROR)
		{
			this->close_connections(CLIENT_DISCONNECT, fd);
			return (SOCKET_ERROR);
		}
	}
	return (SOCKET_SUCCESS);
}

// accept() return a new fd for that client and store its address in client_addr
// sin_addr stores ipv4 address in network byte order, inet_ntoa() converts this address 
// from binary to readable string
void	Server::accept_client()
{
	epoll_event event;
	sockaddr_in	client_addr;
	socklen_t	addr_len;
	int			client_fd;

	addr_len = sizeof(client_addr);
	client_fd = accept(this->_listen_fd, (sockaddr *)(&client_addr), &addr_len);
	if (client_fd < 0)
		return ;
	if (fcntl(client_fd, F_SETFL, O_NONBLOCK) < 0)
	{
		close(client_fd);
		return ;
	}
	event.events = EPOLLIN;
	event.data.fd = client_fd;
	if (epoll_ctl(this->_epoll_fd, EPOLL_CTL_ADD, client_fd, &event) < 0) // add socket to epoll to monitor incoming data
	{
		close(client_fd);
		return ;
	}
	this->_clients[client_fd] = Client(client_fd);
	this->_clients[client_fd].set_host_ip(inet_ntoa(client_addr.sin_addr));
	std::cout << "\n\033[1;32m" << "+ CLIENT " << client_fd << " CONNECTED" << "\033[0m\n";
}


void Server::execute_cmd(Client &client, const std::string &cmd, const std::vector<std::string> &params)
{
	typedef int (Server::*Cmd)(Client &, const std::vector<std::string> &);
	std::map<std::string, Cmd>::iterator it;
	std::map<std::string, Cmd> 			 cmds;
	int 								 ret;

	cmds["PASS"] = &Server::pass_cmd;
	cmds["NICK"] = &Server::nick_cmd;
	cmds["USER"] = &Server::user_cmd;
	cmds["FAST"] = &Server::fast_cmd;
	cmds["JOIN"] = &Server::join_cmd;
	cmds["INVITE"] = &Server::invite_cmd;
	cmds["TOPIC"] = &Server::topic_cmd;
	cmds["MODE"] = &Server::mode_cmd;
	cmds["PART"] = &Server::part_cmd;
	cmds["KICK"] = &Server::kick_cmd;
	cmds["PRIVMSG"] = &Server::privmsg_cmd;
	cmds["WHO"] = &Server::who_cmd;
	cmds["WHOIS"] = &Server::whois_cmd;
	cmds["HELP"] = &Server::help_cmd;
	cmds["AWAY"] = &Server::away_cmd;
	cmds["OPER"] = &Server::oper_cmd;
	cmds["KILL"] = &Server::kill_cmd;
	cmds["PING"] = &Server::ping_cmd;
	cmds["QUIT"] = &Server::quit_cmd;
	it = cmds.find(cmd);
	if (it == cmds.end())
	{
		this->send_error(client, ERR_UNKNOWNCOMMAND, cmd);
		return ;
	}
	if (!client.get_registered() && cmd != "PASS" && cmd != "NICK" && cmd != "USER" 
		&& cmd != "FAST" && cmd != "HELP" && cmd != "PING" && cmd != "QUIT")
	{
		if (cmd != "NOTICE")
			this->send_error(client, ERR_NOTREGISTERED, cmd);
		return ;
	}
	ret = (this->*(it->second))(client, params);
	if (cmd == "QUIT")
		return ;
	if (ret != 0)
	{
		this->send_error(client, ret, get_error_param(cmd, ret, params));
		return ;
	}
	if (!client.get_registered() && client.can_register())
	{
		client.set_registered(true);
		std::cout << "\n\033[1;36m" << "✓ CLIENT " << client.get_client_fd() << " REGISTERED" << "\033[0m\n\n";
		this->send_notice(client, REGISTRATION, "");
	}
}
