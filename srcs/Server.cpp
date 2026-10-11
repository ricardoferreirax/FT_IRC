/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/28 05:42:35 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/11 22:07:55 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Server.hpp"
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <arpa/inet.h>

// port: where server will listen for incoming tcp connections
// pass: password clients will need during irc registration
Server::Server(int port, const std::string &pass)
{
	this->_name = "ircserv";
	this->_pass = pass;
	this->_port = port;
	this->_listen_fd = -1;
	this->_epoll_fd = -1;
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

// creates and prepares tcp listening socket used by irc server
void Server::start_socket()
{
	int	socket_opt;
	sockaddr_in server_addr;

    this->_listen_fd = socket(AF_INET, SOCK_STREAM, 0); // create ipv4 tcp listening socket
    if (this->_listen_fd < 0)
        throw std::runtime_error("IRC: socket() failed.");
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // socket accept connections on any local ipv4 network
    server_addr.sin_port = htons(this->_port); // listen on specified port
    socket_opt = 1;
    if (setsockopt(this->_listen_fd, SOL_SOCKET, SO_REUSEADDR, &socket_opt, sizeof(socket_opt)) < 0) // enable SO_REUSEADDR so listening address can be reused after restarting the server
        throw std::runtime_error("IRC: setsockopt() failed.");
    if (fcntl(this->_listen_fd, F_SETFL, O_NONBLOCK) < 0) // configure socket as non blocking while performing socket operations
        throw std::runtime_error("IRC: fcntl() failed.");
    if (bind(this->_listen_fd, reinterpret_cast<sockaddr *>(&server_addr), sizeof(server_addr)) < 0) // associate socket with configured address and port so it can receive incoming connections
        throw std::runtime_error("IRC: bind() failed.");
    if (listen(this->_listen_fd, SOMAXCONN) < 0)         // changes socket into a listening socket so it can receive incoming tcp connection
        throw std::runtime_error("IRC: listen() failed.");
    std::cout << "\n[SERVER] Listening on all interfaces (0.0.0.0:" << this->_port << ")" << std::endl;
}

// monitors sockets (listening and client) for events, when event occurs, corresponding socket is processed.
// listening socket is monitored for new connections, while client sockets are monitored for incoming and outgoing data.
// epoll_wait() blocks until one or more registered sockets become ready.
void Server::monitor_epoll_events()
{
	epoll_event	event;
	epoll_event	events[10];
	int			ready_events;
	int			current_fd;

	this->_epoll_fd = epoll_create1(0);
	if (this->_epoll_fd < 0)
		throw std::runtime_error("IRC: epoll_create1() failed.");
	event.events = EPOLLIN; // monitor for new incoming connections
	event.data.fd = this->_listen_fd; // store listening socket fd
	if (epoll_ctl(this->_epoll_fd, EPOLL_CTL_ADD, this->_listen_fd, &event) < 0) // add listening socket to epoll
		throw std::runtime_error("IRC: epoll_ctl() failed.");
	while (running) // monitor epoll events till server is stopped
	{
		ready_events = epoll_wait(this->_epoll_fd, events, 10, -1); // wait at least one socket has an event
		if (ready_events < 0)
		{
			if (!running) // server stopped (ctrl-c)
				break;
			throw std::runtime_error("IRC: epoll_wait() failed.");
		}
		for (int i = 0; i < ready_events; i++) // iterate all sockets that have events
		{
			current_fd = events[i].data.fd; // get socket fd that generated event
			if (current_fd == this->_listen_fd) // if listening socket has an event means a new client is waiting to connect
			{
				this->accept_client(); // accept new client and add it to epoll
				continue; // skip to next event
			}
			if (events[i].events & EPOLLIN) // if client socket has an event means it has incoming data to be read
				this->handle_client_data(current_fd, EPOLLIN); // read/receive data FROM client and process it
			if (this->_clients.find(current_fd) == this->_clients.end()) // if client disconnected while processing its data
				continue; // skip to next event
			if (events[i].events & EPOLLOUT) // if client socket has an event means it has pending data to be sent
				this->handle_client_data(current_fd, EPOLLOUT); // send/write pending data TO client
		}
	}
}

void Server::update_epoll(int client_fd, int events)
{
	epoll_event	event;

	event.events = events; // set EPOLLIN -> monitor for incoming data, EPOLLOUT -> monitor for pending data to be sent
	event.data.fd = client_fd; // store client fd to identify which client generated event
	if (epoll_ctl(this->_epoll_fd, EPOLL_CTL_MOD, client_fd, &event) < 0) // modify client socket in epoll to monitor for specified events
		throw std::runtime_error("IRC: epoll_ctl() failed.");
}

// accepts a new client and adds it to epoll
void Server::accept_client()
{
	epoll_event	event;
	sockaddr_in	client_addr;
	sockaddr_in	server_addr;
	socklen_t	addr_len;
	int			client_fd;

	addr_len = sizeof(client_addr);
	client_fd = accept(this->_listen_fd, reinterpret_cast<sockaddr *>(&client_addr), &addr_len); // accept connection from new client and return its socket fd, store its addr in client_addr
	if (client_fd < 0)
		throw std::runtime_error("IRC: accept() failed.");
	addr_len = sizeof(server_addr);
	if (getsockname(client_fd, reinterpret_cast<sockaddr *>(&server_addr), &addr_len) < 0) // get local address and port of client socket and store it in server_addr
	{
		close(client_fd);
		throw std::runtime_error("IRC: getsockname() failed.");
	}
	if (fcntl(client_fd, F_SETFL, O_NONBLOCK) < 0) // set client socket to non-blocking
	{
		close(client_fd);
		throw std::runtime_error("IRC: fcntl() failed.");
	}
	event.events = EPOLLIN; // monitor client for incoming data
	event.data.fd = client_fd; // store client fd to identify which client generated event
	if (epoll_ctl(this->_epoll_fd, EPOLL_CTL_ADD, client_fd, &event) < 0) // add client to epoll
	{
		close(client_fd);
		throw std::runtime_error("IRC: epoll_ctl() failed.");
	}
	this->_clients[client_fd] = Client(client_fd); // create and store client with its socket fd
	this->_clients[client_fd].set_host_ip(inet_ntoa(client_addr.sin_addr)); // store client ip address in client
	std::cout << "\n=====================================================" << std::endl;
	std::cout << "                CLIENT " << client_fd << " CONNECTED!" << std::endl;
	std::cout << "=====================================================" << std::endl;
	std::cout << "[SERVER] Local connection accepted on " << inet_ntoa(server_addr.sin_addr) << ":" << ntohs(server_addr.sin_port) << std::endl;
	std::cout << "=====================================================\n" << std::endl;
}

// handles incoming or outgoing client data depending on the event
// receives data FROM client     : recv() -> CLIENT -> SERVER 
// sends pending TO to client  : send() -> SERVER -> CLIENT
void Server::handle_client_data(int client_fd, int event)
{
	std::string	&recv_buffer = this->_clients[client_fd].get_recv_buffer();
	std::string	&send_buffer = this->_clients[client_fd].get_send_buffer();
	char		buffer[1024]; // temp buffer stores data received FROM client or data TO be sent to client
	ssize_t		bytes; // nb bytes received FROM client or sent TO client

	if (event == EPOLLIN)
	{
		bytes = recv(client_fd, buffer, sizeof(buffer), 0); // server receives/reads data FROM client through client socket
		if (bytes < 0)
		{
			std::cerr << "[CLIENT " << client_fd << "] recv() failed." << std::endl;
			return;
		}
		if (bytes == 0) // client closed connection, server closes client socket and removes it from epoll and clients map
		{
			if (epoll_ctl(this->_epoll_fd, EPOLL_CTL_DEL, client_fd, NULL) < 0)
				throw std::runtime_error("IRC: epoll_ctl() failed.");
			close(client_fd);
			this->_clients.erase(client_fd);
			std::cout << "\n=====================================================" << std::endl;
			std::cout << "          CLIENT " << client_fd << " DISCONNECTED!" << std::endl;
			std::cout << "=====================================================" << std::endl;
			return;
		}
		recv_buffer.append(buffer, bytes); // append received data FROM client to its recv_buffer
		this->process_message(client_fd, recv_buffer); // process complete messages in recv_buffer
	}
	else if (event == EPOLLOUT)
	{
		if (send_buffer.empty()) // no pending data to be sent TO client
			return;
		bytes = send(client_fd, send_buffer.c_str(), send_buffer.size(), 0); // server sends/writes data TO client through client socket
		if (bytes < 0)
		{
			std::cerr << "[CLIENT " << client_fd << "] send() failed." << std::endl;
			return;
		}
		send_buffer.erase(0, bytes); // remove bytes that send() has successfully sent
		if (send_buffer.empty()) // if all data was sent
			this->update_epoll(client_fd, EPOLLIN); // modify client socket in epoll to monitor only for incoming data
	}
}

void Server::process_message(int client_fd, std::string &client_buffer)
{
	std::string	msg;
	std::string	cmd;
	std::string	params;
	size_t		pos;
	size_t		space;

	pos = client_buffer.find("\r\n");
	while (pos != std::string::npos)
	{
		msg = client_buffer.substr(0, pos); // get complete message w/out "\r\n"
		client_buffer.erase(0, pos + 2); // remove message from buffer
		if (!msg.empty())
		{
			space = msg.find(' '); // space separates cmd from params
			cmd = msg.substr(0, space); // get cmd
			if (space != std::string::npos) // if space was found
				params = msg.substr(space + 1); // get params
			else
				params = ""; // no space was found -> no params
			this->handle_cmd(client_fd, cmd, params); // process msg executing its cmd with its params
			if (this->_clients.count(client_fd) == 0) // client disconnected -> don't send data
				return;
		}
		pos = client_buffer.find("\r\n"); // find next complete msg in buffer
	}
}

void Server::handle_cmd(int client_fd, const std::string &cmd, const std::string &params)
{
	Client &client = this->_clients[client_fd]; // get client that sent cmd
	std::string	&send_buffer = client.get_send_buffer(); // get client send_buffer to store server responses to be sent TO client

	if ((cmd == "PASS" || cmd == "USER") && client.get_registered()) // if is registered, it cannot send PASS or USER commands again
	{
		send_buffer += ":" + this->_name + " 462 * :You may not reregister\r\n";
		this->update_epoll(client_fd, EPOLLIN | EPOLLOUT);
		return;
	}
	if (cmd == "PRIVMSG" && !client.get_registered()) // if is not registered it cannot send PRIVMSG command
	{
		send_buffer += ":" + this->_name + " 451 * :You have not registered\r\n";
		this->update_epoll(client_fd, EPOLLIN | EPOLLOUT);
		return;
	}
	if (cmd == "PASS")
		this->handle_pass(client, params);
	else if (cmd == "NICK")
		this->handle_nick(client, params);
	else if (cmd == "USER")
		this->handle_user(client, params);
	else if (cmd == "PRIVMSG")
		this->handle_privmsg(client, params);
	else if (cmd == "PING")
		this->handle_ping(client_fd, params);
	else if (cmd == "QUIT")
	{
		this->handle_quit(client_fd, params);
		return;
	}
	else
	{
		send_buffer += ":" + this->_name + " 421 * " + cmd + " :Unknown command\r\n";
		this->update_epoll(client_fd, EPOLLIN | EPOLLOUT);
		return;
	}
	if (!client.get_registered() && client.can_register()) // if client is not registered but all commands to register
	{
		client.set_registered(true); // set client as registered
		std::cout << "=====================================================" << std::endl;
		std::cout << "          CLIENT " << client_fd << " REGISTERED!" << std::endl;
		std::cout << "=====================================================" << std::endl;
		send_buffer += ":" + this->_name + " 001 " + client.get_nick() + " :Welcome to the IRC server\r\n";
		this->update_epoll(client_fd, EPOLLIN | EPOLLOUT); // modify client socket in epoll to monitor for incoming data and pending data to be sent
	}
}
