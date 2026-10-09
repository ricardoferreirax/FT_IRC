/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/28 05:42:35 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/08 23:04:51 by rmedeiro         ###   ########.fr       */
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

// port: where server will listen for incoming tcp connections
// pass: password clients will need during irc registration
Server::Server(int port, const std::string &pass)
{
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
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);  // configure socket to accept connections on any local network interface
    server_addr.sin_port = htons(this->_port);       // configure socket to listen on specified port (convert to network byte order)
    socket_opt = 1;
    if (setsockopt(this->_listen_fd, SOL_SOCKET, SO_REUSEADDR, &socket_opt, sizeof(socket_opt)) < 0) // enable SO_REUSEADDR so listening address can be reused after restarting the server
        throw std::runtime_error("IRC: setsockopt() failed.");
    if (fcntl(this->_listen_fd, F_SETFL, O_NONBLOCK) < 0) // configure socket as non blocking while performing socket operations
        throw std::runtime_error("IRC: fcntl() failed.");
    if (bind(this->_listen_fd, reinterpret_cast<sockaddr *>(&server_addr), sizeof(server_addr)) < 0) // associate socket with configured address and port so it can receive incoming connections
        throw std::runtime_error("IRC: bind() failed.");
    if (listen(this->_listen_fd, SOMAXCONN) < 0)         // changes socket into a listening socket so it can receive incoming tcp connection
        throw std::runtime_error("IRC: listen() failed.");
    std::cout << "\n[SERVER] Listening on 0.0.0.0:" << this->_port << std::endl;
}

// monitors sockets (listening and client) for events, when the event occurs, corresponding socket is processed.
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
		for (int i = 0; i < ready_events; i++) // process every event
		{
			current_fd = events[i].data.fd; // get fd that generated event
			if (current_fd == this->_listen_fd) // event is from listening socket
			{
				this->accept_client(); // accept new client who is waiting connect
				continue; // process next event
			}
			if (events[i].events & EPOLLIN) // event is from client
				this->receive_client_data(current_fd); // process data sent by client to server
			if (this->_clients.find(current_fd) == this->_clients.end()) // client with current_fd don't exist on map (disconnected)
				continue; // process next event
			if (events[i].events & EPOLLOUT) // event is from client
				this->send_client_data(current_fd); // send pending data to client
		}
	}
}

// accepts a new client and adds it to epoll
void Server::accept_client()
{
	epoll_event	event;
	int				client_fd;
	// sockaddr_in temp_for_client;
	// socklen_t temp_for_client_len = sizeof(temp_for_client);
	t_ip temp_for_client;
	temp_for_client.len = sizeof(temp_for_client.data);

	client_fd = accept(this->_listen_fd, (struct sockaddr *)&temp_for_client.data, &temp_for_client.len); // accept connection from new client and return its socket fd
	if (client_fd < 0)
		throw std::runtime_error("IRC: accept() failed.");
	if (fcntl(client_fd, F_SETFL, O_NONBLOCK) < 0) // set client non-blocking
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
	this->_clients[client_fd] = Client(client_fd, temp_for_client); // create and store client with its socket fd
	std::cout << "\n==========================================" << std::endl;
    std::cout << "        [CLIENT " << client_fd << "] CONNECTED!" << std::endl;
    std::cout << "==========================================\n" << std::endl;
}

// receives data from client and processes complete messages
// recv() -> CLIENT -> SERVER
void Server::receive_client_data(int client_fd)
{
	char		buffer[1024];
	ssize_t		bytes; // nb of bytes received/read from client

	bytes = recv(client_fd, buffer, sizeof(buffer), 0); // server receives/reads data FROM client through client socket
	if (bytes < 0)
	{
		std::cerr << "[CLIENT " << client_fd << "] recv() failed." << std::endl;
		return;
	}
	if (bytes == 0) // recv() returns 0 when client disconnects
	{
		// std::cout << "recv(): " << recv(client_fd, buffer, sizeof(buffer), 0) << std::endl;
		this->disconnect_client(client_fd);
		return;
	}
	this->_clients[client_fd].get_recv_buffer().append(buffer, bytes); // append received data to client's buffer
	this->process_message(client_fd, this->_clients[client_fd].get_recv_buffer()); // process complete msgs in client's buffer
}

// sends pending data to client
// send() -> SERVER -> CLIENT
void Server::send_client_data(int client_fd)
{
	epoll_event	event;
	ssize_t		bytes;

	if (this->_clients[client_fd].get_send_buffer().empty()) // no pending data to send to client
		return;
	bytes = send(client_fd, this->_clients[client_fd].get_send_buffer().c_str(), this->_clients[client_fd].get_send_buffer().size(), 0); // server sends/writes data TO client through client socket
	if (bytes < 0)
	{
		std::cerr << "[CLIENT " << client_fd << "] send() failed." << std::endl;
		return;
	}
	this->_clients[client_fd].get_send_buffer().erase(0, bytes); // removes bytes that send() has successfully sent
	if (this->_clients[client_fd].get_send_buffer().empty()) // if all data was sent
	{
		event.events = EPOLLIN; // monitor client only for incoming data
		event.data.fd = client_fd; // store client fd in event
		if (epoll_ctl(this->_epoll_fd, EPOLL_CTL_MOD, client_fd, &event) < 0) // update client events -> remove EPOLLOUT since there is nothing left to send
			throw std::runtime_error("IRC: epoll_ctl() failed.");
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
			this->send_reply(client_fd, "Msg: " + msg + " | Cmd: " + cmd + " | Params: " + params + "\r\n");
			this->handle_cmd(client_fd, cmd, params); // process msg executing its cmd with its params
			this->send_reply(client_fd, "\r\n");
			if (this->_clients.count(client_fd) == 0) // client disconnected -> don't send data
				return;
		}
		pos = client_buffer.find("\r\n"); // find next complete msg in buffer
	}
}

void Server::send_reply(int client_fd, const std::string &reply)
{
	epoll_event	event;

	if (this->_clients.count(client_fd) == 0) // if client disconnected -> don't send data
		return;
	this->_clients[client_fd].get_send_buffer() += reply; // += allows multiple replies to not replace each other
	event.events = EPOLLIN | EPOLLOUT; // monitor client for both incoming data and writting
	event.data.fd = client_fd; // store client fd in event
	if (epoll_ctl(this->_epoll_fd, EPOLL_CTL_MOD, client_fd, &event) < 0) // update client events to add EPOLLOUT
		throw std::runtime_error("IRC: epoll_ctl() failed.");
}

void Server::register_client(int client_fd)
{
    std::string nick; // store nick of client that completed registration

    if (this->_clients[client_fd].get_registered()) // if client is already registered
	{
        return;
	}
	if (this->_clients[client_fd].can_register())
	{
		this->_clients[client_fd].set_registered(true); // mark client as registered
		nick = this->_clients[client_fd].get_nick(); // store nick that has completed regist
		std::cout << "\n==========================================" << std::endl;
		std::cout << "     [CLIENT " << client_fd << "] REGISTERED!" << std::endl;
		std::cout << "==========================================\n" << std::endl;
		this->send_reply(client_fd, "\r\n");
		this->send_reply(client_fd, ":ircserv 001 " + nick + " :Welcome to the IRC server\r\n");
	}
}

void Server::handle_cmd(int client_fd, const std::string &cmd, const std::string &params)
{
    if ((cmd == "PASS" || cmd == "USER") && this->_clients[client_fd].get_registered())
    {
        this->send_reply(client_fd, ":ircserv 462 * :You may not reregister\r\n"); // ERR_ALREADYREGISTRED
        return;
    }
    if (cmd == "PASS")
        this->handle_pass(client_fd, params);
    else if (cmd == "NICK")
        this->handle_nick(client_fd, params);
    else if (cmd == "USER")
        this->handle_user(client_fd, params);
	else if (cmd == "PING")
    	this->handle_ping(client_fd, params);
	else if (cmd == "QUIT")
	{
		this->handle_quit(client_fd, params);
		return;
	}
    else
    {
        this->send_reply(client_fd, ":ircserv 421 * " + cmd + " :Unknown command\r\n"); // ERR_UNKNOWNCOMMAND
        return;
    }
    this->register_client(client_fd);
}

// removes disconnected client from epoll, deletes its client object and closes socket associated with the connection
// EPOLL_CTL_DEL removes socket from epoll interest list
void Server::disconnect_client(int client_fd)
{
    if (epoll_ctl(this->_epoll_fd, EPOLL_CTL_DEL, client_fd, NULL) < 0) // stop monitoring client socket
		throw std::runtime_error("IRC: epoll_ctl() failed.");
	close(client_fd);
    this->_clients.erase(client_fd); // remove client from server container
    std::cout << "==========================================" << std::endl;
    std::cout << "     [CLIENT " << client_fd << "] DISCONNECTED!" << std::endl;
    std::cout << "==========================================\n" << std::endl;
}

std::map<int, Client> &Server::getClients(void)
{
	return this->_clients;
}
