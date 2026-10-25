/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/28 05:42:35 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/25 20:47:25 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../bonus/includes/Bot.hpp"
#include "../includes/Server.hpp"
#include "../includes/Utils.hpp"
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
	time_t	now;

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

void	Server::close_connections(close_type type, int fd)
{
	std::map<int, Client>::iterator it;
	
	if (type == CLIENT_DISCONNECT)
	{
		this->_bot.cleanup(CLIENT_DISCONNECTED, fd, NULL); // clear client memory and requests
		it = this->_clients.find(fd);
		if (it == this->_clients.end()) // client not found
			return ;
		if (this->_epoll_fd != -1)
			epoll_ctl(this->_epoll_fd, EPOLL_CTL_DEL, fd, NULL);
		close(fd);
		this->_clients.erase(it);
		std::cout << "\n\033[1;31m- Client " << fd << " disconnected\033[0m\n\n";
	}
	else if (type == SERVER_SHUTDOWN)
	{
		this->_bot.cleanup(BOT_SHUTDOWN, -1, NULL); // clear all requests and memory
		for (it = this->_clients.begin(); it != this->_clients.end(); ++it)
			close(it->first);   // close all client sockets
		this->_clients.clear(); // rmv all clients
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
	int 		current_fd;

	this->_epoll_fd = epoll_create1(0);
	if (this->_epoll_fd < 0)
		throw std::runtime_error("IRC: epoll_create1() failed.");
	this->_bot.set_epoll_fd(this->_epoll_fd); // share epoll with ollama bot
	event.events = EPOLLIN;
	event.data.fd = this->_listen_fd;
	if (epoll_ctl(this->_epoll_fd, EPOLL_CTL_ADD, this->_listen_fd, &event) < 0)
		throw std::runtime_error("IRC: epoll_ctl() failed.");
	while (alive)
	{
		ready = epoll_wait(this->_epoll_fd, events, 10, 1000);
		if (ready < 0)
		{
			if (!alive)
				break ;
			throw std::runtime_error("IRC: epoll_wait() failed.");
		}
		for (int i = 0; i < ready; i++)
		{
			current_fd = events[i].data.fd;
			this->handle_socket_event(current_fd, events[i].events);
		}
	}
}

// EPOLLIN on listening socket -> a new client connection is waiting -> accept it
// EPOLLIN on client socket -> has incoming data ready to be received -> read data FROM client
// EPOLLOUT on client socket -> is ready to send pending data -> write data TO client
void	Server::handle_socket_event(int fd, int events)
{
	if (fd == this->_listen_fd)
	{
		if (events & EPOLLIN)
			this->accept_client(); // accept new client connection and add to epoll
	}
	else if (this->_bot.has_request(fd))
		this->_bot.handle_bot_event(*this, fd, events); // handle ollama bot event
	else if (this->_clients.find(fd) != this->_clients.end())
	{
		if (events & (EPOLLERR | EPOLLHUP))
			this->close_connections(CLIENT_DISCONNECT, fd); // disconnect client if socket has error
		else
		{
			if (events & EPOLLIN)
				this->handle_client_data(fd, EPOLLIN);
			if ((events & EPOLLOUT) && this->_clients.find(fd) != this->_clients.end())
				this->handle_client_data(fd, EPOLLOUT);
		}
	}
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

// recv() - receive data FROM client through client socket
// send() - send pending data TO client through client socket
void	Server::handle_client_data(int client_fd, int event)
{
	epoll_event	ev;
	Client 		&client = this->_clients[client_fd];
	ssize_t		bytes;
	char		buffer[1024];
	
	if (event == EPOLLIN)
	{
		std::string &recv_buffer = client.get_recv_buffer();
		bytes = recv(client_fd, buffer, sizeof(buffer), 0);
		if (bytes == 0)
		{
			this->close_connections(CLIENT_DISCONNECT, client_fd);
			return ;
		}
		if (bytes < 0)
			return ; // recv failed or would block ->ignore this event 
		recv_buffer.append(buffer, bytes); // append bytes ->recv() may not receive all data at once 
		log_tcp(TCP_RECEIVE, client_fd, bytes, std::string(buffer, bytes));
		this->process_msg(client, recv_buffer);
	}
	else if (event == EPOLLOUT)
	{
		std::string &send_buffer = client.get_send_buffer();
		if (send_buffer.empty()) // no pending data to send to client
			return ;
		bytes = send(client_fd, send_buffer.c_str(), send_buffer.size(), MSG_NOSIGNAL);
		if (bytes <= 0)
			return ; // send failed or would block -> keep pending data
		log_tcp(TCP_SEND, client_fd, bytes, send_buffer.substr(0, bytes));
		send_buffer.erase(0, bytes); // rmv only bytes successfully sent
		if (send_buffer.empty()) // if all pending data was sent
		{
			if (client.get_quit())
				this->close_connections(CLIENT_DISCONNECT, client_fd); // QUIT was received -> disconnect client
			else
			{
				ev.events = EPOLLIN; // no pending data ->monitor socket for reading only
				ev.data.fd = client_fd;
				if (epoll_ctl(this->_epoll_fd, EPOLL_CTL_MOD, client_fd, &ev) < 0)
					this->close_connections(CLIENT_DISCONNECT, client_fd);
			}
		}
	}
}

// irc msgs end with \r\n, which separates one complete msg from the next one
// incomplete msgs remain in client_buffer till more data is received with recv()
void	Server::process_msg(Client &client, std::string &client_buffer)
{
	std::vector<std::string> params;
	std::string 			 msg;
	std::string 			 cmd;
	size_t					 pos;

	pos = client_buffer.find("\r\n");
	while (pos != std::string::npos)
	{
		msg = client_buffer.substr(0, pos);
		client_buffer.erase(0, pos + 2);
		if (!msg.empty())
		{
			parse_msg(msg, cmd, params);
			log_tcp(IRC_CMD, client.get_client_fd(), msg.size() + 2, cmd);
			this->execute_cmd(client, cmd, params);
			if (client.get_quit())
				return ;
		}
		pos = client_buffer.find("\r\n");
	}
}

void	Server::send_msg(Client &client, const std::string &msg)
{
	epoll_event	event;
	int			client_fd;

	client_fd = client.get_client_fd();
	client.get_send_buffer() += msg; // append msg to send buffer
	if (msg.size() < 2 || msg.substr(msg.size() - 2) != "\r\n") // if msg does not end with \r\n
		client.get_send_buffer() += "\r\n"; // end msg with \r\n
	client.get_send_buffer() += "\r\n"; 
	event.events = EPOLLIN | EPOLLOUT; // monitor socket for reading and writing
	event.data.fd = client_fd;
	if (epoll_ctl(this->_epoll_fd, EPOLL_CTL_MOD, client_fd, &event) < 0)
		throw std::runtime_error("IRC: epoll_ctl() failed.");
}

void Server::send_error(Client &client, int error, const std::string &param)
{
	std::map<int, std::string> errors; // each error code has a corresponding error msg
	std::string				   nick;

	nick = client.get_nick();
	if (nick.empty())
		nick = "*";
	errors[ERR_NOSUCHNICK] = "401 " + nick + " " + param + " :No such nick/channel";
	errors[ERR_NOORIGIN] = "409 " + nick + " :No origin specified";
	errors[ERR_NORECIPIENT] = "411 " + nick + " :No recipient given (" + param + ")";
	errors[ERR_NOTEXTTOSEND] = "412 " + nick + " :No text to send";
	errors[ERR_UNKNOWNCOMMAND] = "421 " + nick + " " + param + " :Unknown command";
	errors[ERR_NONICKNAMEGIVEN] = "431 " + nick + " :No nickname given";
	errors[ERR_ERRONEOUSNICKNAME] = "432 " + nick + " " + param + " :Erroneous nickname";
	errors[ERR_NICKNAMEINUSE] = "433 " + nick + " " + param + " :Nickname is already in use";
	errors[ERR_USERONCHANNEL] = "443 " + nick + " " + param + " :is already on channel";
	errors[ERR_NOTREGISTERED] = "451 " + nick + " :You have not registered";
	errors[ERR_NEEDMOREPARAMS] = "461 " + nick + " " + param + " :Not enough parameters";
	errors[ERR_ALREADYREGISTRED] = "462 " + nick + " :You may not reregister";
	errors[ERR_PASSWDMISMATCH] = "464 " + nick + " :Password incorrect";
    errors[ERR_CHANNELISFULL] = "471 " + nick + " " + param + " :Cannot join channel (+l)";
    errors[ERR_INVITEONLYCHAN] = "473 " + nick + " " + param + " :Cannot join channel (+i)";
    errors[ERR_BADCHANNELKEY] = "475 " + nick + " " + param + " :Cannot join channel (+k)";
	errors[ERR_BADCHANMASK] = "476 " + nick + " " + param + " :Bad Channel Mask";
	errors[ERR_PASSALREADYACCEPTED] = "601 " + nick + " :PASS already accepted";
	errors[ERR_PASSREQUIRED] = "602 " + nick + " :PASS must be accepted before NICK";
	errors[ERR_NICKREQUIRED] = "603 " + nick + " :NICK must be set before USER";
	errors[ERR_INVALIDUSERMODE] = "604 " + nick + " :Invalid USER mode: only numbers are allowed";
	errors[ERR_MIKUUNAVAILABLE] = "605 " + nick + " :Miku is unavailable";
	if (errors.find(error) == errors.end())
		return ; // error code is not in map -> ignore it
	this->send_msg(client, ":" + this->_name + " " + errors[error]);
}
