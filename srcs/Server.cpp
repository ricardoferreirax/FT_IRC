/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/28 05:42:35 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/12 14:51:57 by rmedeiro         ###   ########.fr       */
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

// create and configure tcp listening socket used by irc server
// setsockopt() allow local address to be reused after server restart
// bind() associates socket with local ipv4 interfaces and server port
// listen() changes socket into a listening socket ready to accept connections | SOMAXCONN is max nb ofpending connections allowed in queue
// ss -ltnp | grep 6667 -> check if server is listening on the port
void Server::start_socket()
{
	int			socket_opt;
	sockaddr_in	server_addr;

	this->_listen_fd = socket(AF_INET, SOCK_STREAM, 0); // create ipv4 tcp socket and return its fd
	if (this->_listen_fd < 0)
		throw std::runtime_error("IRC: socket() failed.");
	server_addr.sin_family = AF_INET; // use ipv4 address
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // accept connections on any local ipv4 interface
	server_addr.sin_port = htons(this->_port); // convert server port
	socket_opt = 1;
	if (setsockopt(this->_listen_fd, SOL_SOCKET, SO_REUSEADDR, &socket_opt, sizeof(socket_opt)) < 0)
		throw std::runtime_error("IRC: setsockopt() failed.");
	if (fcntl(this->_listen_fd, F_SETFL, O_NONBLOCK) < 0) // set listening socket non-blocking
		throw std::runtime_error("IRC: fcntl() failed.");
	if (bind(this->_listen_fd, reinterpret_cast<sockaddr *>(&server_addr), sizeof(server_addr)) < 0)
		throw std::runtime_error("IRC: bind() failed.");
	if (listen(this->_listen_fd, SOMAXCONN) < 0)
		throw std::runtime_error("IRC: listen() failed.");
	std::cout << "\n[SERVER] Listening on all interfaces (0.0.0.0:" << this->_port << ")" << std::endl;
}

// monitor listening and client sockets for events using epoll
// listening socket uses EPOLLIN to detect new pending client connections
// client sockets use EPOLLIN to receive data and EPOLLOUT to send pending data
// epoll_wait wait till at least one registered socket becomes ready store it in events[] and return how many are ready
void Server::monitor_epoll_events()
{
	epoll_event	event;
	epoll_event	events[10];
	int			ready_events;
	int			current_fd;

	this->_epoll_fd = epoll_create1(0);
	if (this->_epoll_fd < 0)
		throw std::runtime_error("IRC: epoll_create1() failed.");
	event.events = EPOLLIN; // monitor listening socket for incoming connections
	event.data.fd = this->_listen_fd; // store listening fd to identify its events
	if (epoll_ctl(this->_epoll_fd, EPOLL_CTL_ADD, this->_listen_fd, &event) < 0) // add listening socket to epoll interest list
		throw std::runtime_error("IRC: epoll_ctl() failed.");
	while (running) // keep monitoring events until server is stopped
	{
		ready_events = epoll_wait(this->_epoll_fd, events, 10, -1);
		if (ready_events < 0)
		{
			if (!running)
				break;
			throw std::runtime_error("IRC: epoll_wait() failed.");
		}
		for (int i = 0; i < ready_events; i++) // process every socket that became ready
		{
			current_fd = events[i].data.fd; // get fd that generated this event
			if (current_fd == this->_listen_fd) // listening socket ready -> a new client connection is waiting
			{
				this->accept_client(); // accept client and add its socket to epoll
				continue; // skip to next event
			}
			if (events[i].events & EPOLLIN) // EPOLLIN -> client socket has incoming data ready to be received
				this->handle_client_data(current_fd, EPOLLIN); // read/receive data FROM client and process it
			if (this->_clients.find(current_fd) == this->_clients.end()) // client may have disconnected while processing incoming data
				continue; // skip to next event
			if (events[i].events & EPOLLOUT) // EPOLLOUT -> client socket is ready to send pending data
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

// accept pending connection from new client -> return a new socket fd for that client and store its address in client_addr
// get local ip address used by server for this connection and store it in server_addr
// add client socket to epoll to monitor it for incoming data
// sin_addr stores ipv4 address in network byte order, inet_ntoa() converts this address from binary to readable string
void Server::accept_client()
{
	epoll_event	event;
	sockaddr_in	client_addr;
	sockaddr_in	server_addr;
	socklen_t	addr_len;
	int			client_fd;

	addr_len = sizeof(client_addr); // size available to store client address
	client_fd = accept(this->_listen_fd, reinterpret_cast<sockaddr *>(&client_addr), &addr_len);
	if (client_fd < 0)
		throw std::runtime_error("IRC: accept() failed.");
	addr_len = sizeof(server_addr); // size available to store server local address
	if (getsockname(client_fd, reinterpret_cast<sockaddr *>(&server_addr), &addr_len) < 0)
	{
		close(client_fd);
		throw std::runtime_error("IRC: getsockname() failed.");
	}
	if (fcntl(client_fd, F_SETFL, O_NONBLOCK) < 0) // set socket non-blocking so it doesn't block the event loop
	{
		close(client_fd);
		throw std::runtime_error("IRC: fcntl() failed.");
	}
	event.events = EPOLLIN; // monitor client socket for incoming data
	event.data.fd = client_fd; // store client fd to identify which socket generated the event
	if (epoll_ctl(this->_epoll_fd, EPOLL_CTL_ADD, client_fd, &event) < 0) // add client socket to epoll
	{
		close(client_fd);
		throw std::runtime_error("IRC: epoll_ctl() failed.");
	}
	this->_clients[client_fd] = Client(client_fd); // create and store client object using its socket fd
	this->_clients[client_fd].set_host_ip(inet_ntoa(client_addr.sin_addr)); // convert client ipv4 address to string and store it
	std::cout << "\n=====================================================" << std::endl;
	std::cout << "                CLIENT " << client_fd << " CONNECTED!" << std::endl;
	std::cout << "=====================================================" << std::endl;
	std::cout << "[SERVER] Local connection accepted on " << inet_ntoa(server_addr.sin_addr) << ":" << ntohs(server_addr.sin_port) << std::endl;
	std::cout << "=====================================================\n" << std::endl;
}

// handle client socket i/o depending on event received from epoll
// EPOLLIN -> recv() reads/receives incoming data FROM client and stores it in recv_buffer
// EPOLLOUT -> send() writes/sends pending data FROM send_buffer TO client
void Server::handle_client_data(int client_fd, int event)
{
	char		buffer[1024]; // temp buffer used by recv() to store received data
	ssize_t		bytes; // nb of bytes received or sent

	if (event == EPOLLIN)
	{
		std::string	&recv_buffer = this->_clients[client_fd].get_recv_buffer();
		bytes = recv(client_fd, buffer, sizeof(buffer), 0); // server receives data FROM client through client socket
		if (bytes < 0)
		{
			std::cerr << "[CLIENT " << client_fd << "] recv() failed." << std::endl;
			return;
		}
		if (bytes == 0) // client closed its side of tcp connection
		{
			if (epoll_ctl(this->_epoll_fd, EPOLL_CTL_DEL, client_fd, NULL) < 0) // remove client socket from epoll
				throw std::runtime_error("IRC: epoll_ctl() failed.");
			close(client_fd);
			this->_clients.erase(client_fd);
			std::cout << "\n=====================================================" << std::endl;
			std::cout << "          CLIENT " << client_fd << " DISCONNECTED!" << std::endl;
			std::cout << "=====================================================" << std::endl;
			return;
		}
		recv_buffer.append(buffer, bytes); // append received bytes to client's receive buffer
		this->process_message(client_fd, recv_buffer); // process complete irc nsgs stored in recv_buffer
	}
	else if (event == EPOLLOUT)
	{
		std::string	&send_buffer = this->_clients[client_fd].get_send_buffer();
		if (send_buffer.empty()) // no pending data to send to client
			return;
		bytes = send(client_fd, send_buffer.c_str(), send_buffer.size(), 0); // server sends data TO client through client socket
		if (bytes < 0)
		{
			std::cerr << "[CLIENT " << client_fd << "] send() failed." << std::endl;
			return;
		}
		send_buffer.erase(0, bytes); // remove only bytes successfully sent
		if (send_buffer.empty()) // if all pending data was successfully sent
			this->update_epoll(client_fd, EPOLLIN); // stop monitoring EPOLLOUT till new data needs to be sent
	}
}

// process complete irc msgs stored in client's receive buffer
// irc msgs end with \r\n, which separates one complete msg from the next one
// incomplete msgs remain in client_buffer till more data is received with recv()
void Server::process_message(int client_fd, std::string &client_buffer)
{
	std::string	msg;
	std::string	cmd;
	std::string	params;
	size_t		pos;
	size_t		space;

	pos = client_buffer.find("\r\n"); // find end of first complete msg
	while (pos != std::string::npos) // process msgs while complete msgs exist
	{
		msg = client_buffer.substr(0, pos); // extract complete msg without \r\n
		client_buffer.erase(0, pos + 2); // remove processed msg and \r\n from buffer
		if (!msg.empty())
		{
			space = msg.find(' '); // find separator between cmd and params
			cmd = msg.substr(0, space); // extract cmd
			if (space != std::string::npos)
				params = msg.substr(space + 1); // extract everything after cmd as params
			else
				params = ""; // cmd has no parameters
			this->handle_cmd(client_fd, cmd, params); // execute cmd with its parameters
			if (this->_clients.count(client_fd) == 0) // cmd may have disconnected and removed client
				return;
		}
		pos = client_buffer.find("\r\n"); // search for next complete msg
	}
}

// dispatch received cmd to its corresponding cmd handler
// when registration is complete, mark client as registered and send welcome reply
void Server::handle_cmd(int client_fd, const std::string &cmd, const std::string &params)
{
	Client		&client = this->_clients[client_fd]; // get client that sent the cmd
	std::string	&send_buffer = client.get_send_buffer(); // get buffer used to queue responses to client

	if ((cmd == "PASS" || cmd == "USER") && client.get_registered()) // registered client cannot put pass or user again
	{
		send_buffer += ":" + this->_name + " 462 * :You may not reregister\r\n";
		this->update_epoll(client_fd, EPOLLIN | EPOLLOUT); // enable EPOLLOUT to send queued reply
		return;
	}
	if (cmd == "PRIVMSG" && !client.get_registered()) // client must be registered before sending privmsg
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
		this->handle_ping(client, params);
	else if (cmd == "QUIT")
	{
		this->handle_quit(client_fd, params); // disconnect and remove client
		return;
	}
	else
	{
		send_buffer += ":" + this->_name + " 421 * " + cmd + " :Unknown command\r\n";
		this->update_epoll(client_fd, EPOLLIN | EPOLLOUT);
		return;
	}
	if (!client.get_registered() && client.can_register()) // if registration is complete
	{
		client.set_registered(true); // mark client as fully registered
		std::cout << "=====================================================" << std::endl;
		std::cout << "          CLIENT " << client_fd << " REGISTERED!" << std::endl;
		std::cout << "=====================================================" << std::endl;
		send_buffer += ":" + this->_name + " 001 " + client.get_nick() + " :Welcome to the IRC server\r\n" + "\r\n";
		this->update_epoll(client_fd, EPOLLIN | EPOLLOUT);
	}
}
