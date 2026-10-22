/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/28 05:42:35 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/22 13:31:49 by pedro            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Channels.hpp"
#include "../includes/Server.hpp"
#include "../includes/Utils.hpp"
#include "../bonus/includes/Bot.hpp"
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

// port: where server will listen for incoming tcp connections
// pass: password clients will need during irc registration
Server::Server(const std::string &port, const std::string &pass)
{
	int value;
	time_t now;

	if (port.empty())
		throw std::runtime_error("IRC: invalid port!");
	value = 0;
	for (size_t i = 0; i < port.size(); i++)
	{
		if (port[i] < '0' || port[i] > '9')
			throw std::runtime_error("IRC: port must contain only numbers!");
		if (value > 6553)
			throw std::runtime_error("IRC: port must be between 1024 and 65535!");
		value = value * 10 + (port[i] - '0');
		if (value > 65535)
			throw std::runtime_error("IRC: port must be between 1024 and 65535!");
	}
	if (value < 1024)
		throw std::runtime_error("IRC: privileged port! Use a port above 1023!");
	if (pass.empty())
		throw std::runtime_error("IRC: a server password is required!");
	this->_port = value;
	this->_pass = pass;
	this->_listen_fd = -1;
	this->_epoll_fd = -1;
	this->_name = "ircserv";
	this->_bot.start_request(STDOUT_FILENO, "");
	now = time(NULL);
	this->_date = ctime(&now);
	if (!this->_date.empty())
		this->_date.erase(this->_date.size() - 1);
}

Server::~Server()
{
    std::map<int, Client>::iterator it;

    for (it = this->_clients.begin(); it != this->_clients.end(); ++it)
    {
        close(it->first);
    }
    if (this->_epoll_fd != -1)
		close(this->_epoll_fd);
    if (this->_listen_fd != -1)
        close(this->_listen_fd);
    std::cout << "[SERVER] Closed!" << std::endl;
}

void Server::disconnect_client(int client_fd)
{
    epoll_ctl(this->_epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
    close(client_fd);
    this->_clients.erase(client_fd);
    std::cout << "\n=====================================================" << std::endl;
	std::cout << "          CLIENT " << client_fd << " DISCONNECTED!" << std::endl;
	std::cout << "=====================================================" << std::endl;
}

// create ipv4 tcp listening socket (and return its fd) used by irc server 
// setsockopt() allow local address to be reused after server restart
// bind() associates socket with local ipv4 and server port
// listen() changes socket into a listening socket ready to accept connections
void Server::start_socket()
{
	int			socket_opt;
	sockaddr_in	server_addr;

	this->_listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (this->_listen_fd < 0)
		throw std::runtime_error("IRC: socket() failed.");
	server_addr.sin_family = AF_INET;                // use ipv4 address
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		// accept connections on any local ipv4 interface
	server_addr.sin_port = htons(this->_port);       // convert server port
	socket_opt = 1;
	if (setsockopt(this->_listen_fd, SOL_SOCKET, SO_REUSEADDR, &socket_opt,
			sizeof(socket_opt)) < 0)
		throw std::runtime_error("IRC: setsockopt() failed.");
	if (fcntl(this->_listen_fd, F_SETFL, O_NONBLOCK) < 0)
		// set listening socket non-blocking
		throw std::runtime_error("IRC: fcntl() failed.");
	if (bind(this->_listen_fd, reinterpret_cast<sockaddr *>(&server_addr),
			sizeof(server_addr)) < 0)
		throw std::runtime_error("IRC: bind() failed.");
	if (listen(this->_listen_fd, SOMAXCONN) < 0)
		throw std::runtime_error("IRC: listen() failed.");
	std::cout << "\n[SERVER] Listening on all interfaces (0.0.0.0:" << this->_port << ")" << std::endl;
}

// monitor listening and client sockets for events using epoll
// EPOLLIN on listening socket -> a new client connection is waiting -> accept it
// EPOLLIN on client socket -> has incoming data ready to be received -> read data FROM client
// EPOLLOUT on client socket -> is ready to send pending data -> write data TO client
// epoll_wait wait till at least one registered socket becomes ready -> return how many are ready
void Server::monitor_epoll_events()
{
	epoll_event	event;
	epoll_event	events[10];
	int			ready_events;
	int			current_fd;

	this->_epoll_fd = epoll_create1(0);
	if (this->_epoll_fd < 0)
		throw std::runtime_error("IRC: epoll_create1() failed.");
	this->_bot.set_epoll_fd(this->_epoll_fd); // set epoll fd in bot to monitor its sockets for events
	event.events = EPOLLIN; // monitor listening socket for incoming connections
	event.data.fd = this->_listen_fd; // store fd to identify which socket generated event
	if (epoll_ctl(this->_epoll_fd, EPOLL_CTL_ADD, this->_listen_fd, &event) < 0)
		throw std::runtime_error("IRC: epoll_ctl() failed.");
	while (running) // keep monitoring events till server is stopped
	{
		ready_events = epoll_wait(this->_epoll_fd, events, 10, -1);
		if (ready_events < 0)
		{
			if (!running)
				break ;
			throw std::runtime_error("IRC: epoll_wait() failed.");
		}
		for (int i = 0; i < ready_events; i++)
			// process every socket that became ready
		{
			current_fd = events[i].data.fd; // get fd that generated event
			if (current_fd == this->_listen_fd)
			{
				this->accept_client();
				continue;
			}
			if (this->_bot.has_request(current_fd))
			{
			    this->_bot.handle_event(current_fd, events[i].events);
			    this->_bot.send_responses(*this);
			    continue;
			}
			if (events[i].events & EPOLLIN)
				this->handle_client_data(current_fd, EPOLLIN);
			if (this->_clients.find(current_fd) == this->_clients.end())
				continue;
			if (events[i].events & EPOLLOUT)
				this->handle_client_data(current_fd, EPOLLOUT);
		}
	}
}

// accept pending connection from new client -> return a new socket fd for that client and store its address in client_addr
// add client socket to epoll to monitor it for incoming data
// sin_addr stores ipv4 address in network byte order, inet_ntoa() converts this address from binary to readable string
void Server::accept_client()
{
	epoll_event	event;
	sockaddr_in	client_addr;
	socklen_t	addr_len;
	int			client_fd;

	addr_len = sizeof(client_addr);
	client_fd = accept(this->_listen_fd, reinterpret_cast<sockaddr *>(&client_addr), &addr_len);
	if (client_fd < 0)
		throw std::runtime_error("IRC: accept() failed.");
	if (fcntl(client_fd, F_SETFL, O_NONBLOCK) < 0) // set socket non-blocking to not block event loop
	{
		close(client_fd);
		throw std::runtime_error("IRC: fcntl() failed.");
	}
	event.events = EPOLLIN;
	event.data.fd = client_fd;
	if (epoll_ctl(this->_epoll_fd, EPOLL_CTL_ADD, client_fd, &event) < 0)
	{
		close(client_fd);
		throw std::runtime_error("IRC: epoll_ctl() failed.");
	}
	this->_clients[client_fd] = Client(client_fd);                         
		// create and store client object using its socket fd
	this->_clients[client_fd].set_host_ip(inet_ntoa(client_addr.sin_addr));
		// convert client ipv4 address to string and store it
	std::cout << "\n=====================================================" << std::endl;
	std::cout << "                CLIENT " << client_fd << " CONNECTED!" << std::endl;
	std::cout << "=====================================================" << std::endl;
}

void Server::handle_client_data(int client_fd, int event)
{
    Client &client = this->_clients[client_fd];
    char buffer[1024];
    ssize_t bytes;

    if (event == EPOLLIN)
    {
		std::string &recv_buffer = client.get_recv_buffer();
        bytes = recv(client_fd, buffer, sizeof(buffer), 0); // receive data FROM client through client socket
        if (bytes <= 0)
        {
            if (bytes == 0) // connection closed by client
                this->disconnect_client(client_fd);
            return; // if recv failed ignore this event
        }
        recv_buffer.append(buffer, bytes); // append bytes -> recv() may not receive all data at once
        this->process_msg(client, recv_buffer); // process complete msgs currently in buffer
    }
    else if (event == EPOLLOUT)
    {
        std::string &send_buffer = client.get_send_buffer();
        if (send_buffer.empty()) // no pending data to send to client
            return;
        bytes = send(client_fd, send_buffer.c_str(), send_buffer.size(), MSG_NOSIGNAL); // send data TO client through client socket
        if (bytes <= 0)
            return; // send failed -> ignore this event
        send_buffer.erase(0, bytes); // rmv only bytes successfully sent
        if (send_buffer.empty()) // if all pending data was sent
        {
            if (client.get_quit())
                this->disconnect_client(client_fd); // if QUIT was received -> disconnect client
            else
                this->send_msg(client, "", EPOLLIN); // update epoll -> monitor only for incoming data now
        }
    }
}

// irc msgs end with \r\n, which separates one complete msg from the next one.
// incomplete msgs remain in client_buffer till more data is received with recv()
void Server::process_msg(Client &client, std::string &client_buffer)
{
	std::vector<std::string> params;
	std::string msg;
	std::string cmd;
	size_t pos;

	pos = client_buffer.find("\r\n");
	while (pos != std::string::npos)
	{
		msg = client_buffer.substr(0, pos);
		client_buffer.erase(0, pos + 2);
		if (!msg.empty())
		{
			parse_msg(msg, cmd, params);
			this->execute_cmd(client, cmd, params);
			if (client.get_quit())
				return;
		}
		pos = client_buffer.find("\r\n");
	}
}

void Server::send_msg(Client &client, const std::string &msg, int events)
{
	epoll_event event;
	int client_fd;

	client_fd = client.get_client_fd();
	client.get_send_buffer() += msg;
	event.events = events;
	event.data.fd = client_fd;
	if (epoll_ctl(this->_epoll_fd, EPOLL_CTL_MOD, client_fd, &event) < 0)
		throw std::runtime_error("IRC: epoll_ctl() failed.");
}

void Server::send_error(Client &client, int error, const std::string &param)
{
    std::map<int, std::string> errors; // each error code has a corresponding error msg
    std::string nick;

    nick = client.get_nick();
    if (nick.empty())
        nick = "*";
    errors[401] = "401 " + nick + " " + param + " :No such nick/channel";
    errors[409] = "409 " + nick + " :No origin specified";
    errors[411] = "411 " + nick + " :No recipient given (" + param + ")";
    errors[412] = "412 " + nick + " :No text to send";
    errors[421] = "421 " + nick + " " + param + " :Unknown command";
    errors[431] = "431 " + nick + " :No nickname given";
    errors[432] = "432 " + nick + " " + param + " :Erroneous nickname";
    errors[433] = "433 " + nick + " " + param + " :Nickname is already in use";
    errors[451] = "451 " + nick + " :You have not registered";
    errors[461] = "461 " + nick + " " + param + " :Not enough parameters";
    errors[462] = "462 " + nick + " :You may not reregister";
    errors[464] = "464 " + nick + " :Password incorrect";
    if (errors.find(error) == errors.end())
        return; // error code is not in map -> ignore it
    this->send_msg(client, ":" + this->_name + " " + errors[error] + "\r\n", EPOLLIN | EPOLLOUT);
}

// Miku bot
void Server::send_response(int client_fd, const std::string &msg)
{
    std::map<int, Client>::iterator it;

    it = this->_clients.find(client_fd); // find client with this fd
    if (it == this->_clients.end()) // client not found -> ignore response
        return;
    this->send_msg(it->second, "Miku :" + msg + "\r\n" + "\n", EPOLLIN | EPOLLOUT);
}

std::string Server::getName(void) const
{
	return (this->_name);
}
