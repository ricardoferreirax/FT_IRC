/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/28 05:42:35 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/05 21:46:50 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Server.hpp"

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
	std::cout << "Closing server..." << std::endl;
	for (size_t i = 0; i < this->_client_fds.size(); i++)
	{
		close(this->_client_fds[i]);
		std::cout << "[CLIENT " << this->_client_fds[i] << "] Closed!" << std::endl;
	}
	if (this->_epoll_fd != -1)
	{
		close(this->_epoll_fd);
		std::cout << "[EPOLL FD " << this->_epoll_fd << "] Closed!" << std::endl;
	}
	if (this->_listen_fd != -1)
	{
		close(this->_listen_fd);
		std::cout << "[SOCKET FD " << this->_listen_fd << "] Closed!" << std::endl;
	}
	std::cout << "[SERVER] Closed!" << std::endl;
}

// creates and prepares tcp listening socket used by irc server
void Server::start_socket()
{
	int	socket_opt;

	sockaddr_in server_addr; // describes ipv4 address where server will listen
	this->_listen_fd = socket(AF_INET, SOCK_STREAM, 0); // create ipv4 tcp listening socket
	if (this->_listen_fd < 0)
		throw std::runtime_error("IRC: socket() failed.");
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // configure socket to accept connections on any local network interface
	server_addr.sin_port = htons(this->_port);      // configure socket to listen on specified port (convert to network byte order)
	socket_opt = 1;
	if (setsockopt(this->_listen_fd, SOL_SOCKET, SO_REUSEADDR, &socket_opt,
			sizeof(socket_opt)) < 0) // enable SO_REUSEADDR so listening address can be reused after restarting the server
		throw std::runtime_error("IRC: setsockopt() failed.");
	if (fcntl(this->_listen_fd, F_SETFL, O_NONBLOCK) < 0) // configure socket as non blocking while performing socket operations
		throw std::runtime_error("IRC: fcntl() failed.");
	if (bind(this->_listen_fd, reinterpret_cast<sockaddr *>(&server_addr),
			sizeof(server_addr)) < 0) // associate socket with configured address and port so it can receive incoming connections
		throw std::runtime_error("IRC: bind() failed.");
	if (listen(this->_listen_fd, SOMAXCONN) < 0) // changes socket into a listening socket so it can receive incoming tcp connection
		throw std::runtime_error("IRC: listen() failed.");
	std::cout << "[SERVER] Listening On Port " << this->_port << std::endl;
}

// creates epoll which allows server monitor multiple sockets w/out blocking while waiting for events.
// listening socket is added to epoll list so epoll can notify server whenever a new tcp connection is waiting to be accepted.
void Server::prepare_epoll()
{
	epoll_event	event;

	this->_epoll_fd = epoll_create1(0);
	if (this->_epoll_fd < 0)
		throw std::runtime_error("IRC: epoll_create1() failed.");
	event.events = EPOLLIN;                                                     // fd server socket wanna know when data can be read and is ready for reading -> a new connection is waiting
	event.data.fd = this->_listen_fd;                                           // stores the fd inside the event, allows to know which fd generated the event
	if (epoll_ctl(this->_epoll_fd, EPOLL_CTL_ADD, this->_listen_fd, &event) < 0) // add socket to epoll list
		throw std::runtime_error("IRC: epoll_ctl() failed.");
}

void Server::handle_events()
{
	epoll_event	events[10];
	int			current_fd;

	int ready_events; // how many events were returned by epoll_wait()
	std::cout << "\n[SERVER] Waiting for connections..." << std::endl;
	while (running) // runs until ctrl-c changes running to 0
	{
		ready_events = epoll_wait(this->_epoll_fd, events, 10, -1); // wait till at least one registered socket becomes ready
		if (ready_events < 0)
		{
			if (!running) // ctrl-cinterrupted epoll_wait(), so stop event loop
				break ;
			throw std::runtime_error("IRC: epoll_wait() failed.");
		}
		for (int i = 0; i < ready_events; i++) // process all events returned by epoll_wait()
		{
			current_fd = events[i].data.fd;                                    // get fd socket that generated the event
			if (current_fd == this->_listen_fd && (events[i].events & EPOLLIN)) // if returned event is from listening socket and it's ready for reading...
			{
				this->accept_client(); // a new client is waiting to be accepted
			}
			else if (current_fd != this->_listen_fd
				&& (events[i].events & EPOLLIN)) // if returned event is from another socket and it's ready for reading...
			{
				this->receive_data(current_fd); // an already connected client has sent data
			}
		}
	}
}

// accepts a new tcp client and prepares its socket so it can be monitored by the server.
void Server::accept_client()
{
	int			client_fd;
	epoll_event	event;

	client_fd = accept(this->_listen_fd, NULL, NULL); // accepts pending connection from listening sockcet, client_fd is socket to communicate with new connected client
	if (client_fd < 0)
		throw std::runtime_error("IRC: accept() failed.");
	if (fcntl(client_fd, F_SETFL, O_NONBLOCK) < 0) // make client socket non-blocking so recv() doesn't block server while waiting data from one specific client
	{
		close(client_fd);
		throw std::runtime_error("IRC: fcntl() failed.");
	}
	event.events = EPOLLIN; // configure event that epoll should monitor for this client
	event.data.fd = client_fd;
	if (epoll_ctl(this->_epoll_fd, EPOLL_CTL_ADD, client_fd, &event) < 0)
	{
		close(client_fd);
		throw std::runtime_error("IRC: epoll_ctl() failed.");
	}
	this->_client_fds.push_back(client_fd); // store client fd so server knows which clients are connected
	this->_client_buffers[client_fd] = ""; // create empty receive buffer for this client, each client needs its own buffer
	this->_authenticated[client_fd] = false;
	std::cout << "\n>>>>>>>>>> [CLIENT " << client_fd << "] CONNECTED! <<<<<<<<<<\n" << std::endl;
}

// recv() reads and tells how many bytes are available to read from client socket, if there are bytes available, they are read
// and appended to client's buffer. buffer may contain incomplete data from previous recv() calls.
void Server::receive_data(int client_fd)
{
	char	buffer[1024];
	ssize_t	bytes_recv;

	bytes_recv = recv(client_fd, buffer, sizeof(buffer), 0); // reads tcp data from connected client socket and stores it in buffer
	if (bytes_recv > 0)                                     // if client sent data and it was successfully read into buffer
	{
		this->_client_buffers[client_fd].append(buffer, bytes_recv); // append exactly received bytes to buffer associated with this client
		this->process_messages(client_fd);                          // extracts complete irc msgs from client's buffer and processes them
	}
	else if (bytes_recv == 0) // if peer has closed its side of tcp connection
	{
		this->disconnect_client(client_fd);
	}
}

// extracts complete irc msgs from client's receive buffer, incomplete data stays in buffer till more data is received by recv().
void Server::process_messages(int client_fd)
{
	size_t	pos;
	size_t	space;

	std::string &buffer = this->_client_buffers[client_fd]; // buffer associated with this client fd.
	std::string msg;
	std::string cmd;
	std::string params;
	pos = buffer.find("\r\n");       // search for first "\r\n" in buffer
	while (pos != std::string::npos) // while there is at least one complete msg ready to be processed
	{
		msg = buffer.substr(0, pos); // copy till "\r\n" into msg
		std::cout << "------------------------------------------------------------" << std::endl;
		std::cout << "[CLIENT " << client_fd << "] MSG: " << msg << std::endl;
		space = msg.find(' ');          // search for first space in msg, which separates cmd from its params
		if (space == std::string::npos)
			// if no space was found msg has only a cmd
		{
			cmd = msg;
			params = "";
		}
		else // otherwise msg has a cmd followed by at least one param
		{
			cmd = msg.substr(0, space);    // extract everything before first space
			params = msg.substr(space + 1); // extract everything after first space
		}
		std::cout << std::endl;
		this->handle_cmd(client_fd, cmd, params);
		buffer.erase(0, pos + 2); // remove processed message and "\r\n" from client's buffer
		pos = buffer.find("\r\n"); // search again cause buffer may has another complete msg after one that was removed
	}
}

void Server::handle_cmd(int client_fd, const std::string &cmd,
	const std::string &params)
{
	std::cout << "[CMD]: " << cmd << std::endl;
	std::cout << "[PARAMS]: " << params << std::endl;
	if (cmd == "PASS")
		this->handle_pass(client_fd, params);
	else if (cmd == "NICK")
		this->handle_nick(client_fd, params);
	else if (cmd == "USER")
		this->handle_user(client_fd, params);
	else
		std::cout << "\nUNKNOWN COMMAND" << std::endl;
	std::cout << std::endl;
}

// removes a disconnected client from every part of server. EPOLL_CTL_DEL removes socket from epoll interest list
void Server::disconnect_client(int client_fd)
{
	std::vector<int>::iterator it;
	std::vector<int>::iterator begin;
	std::vector<int>::iterator end;
	if (epoll_ctl(this->_epoll_fd, EPOLL_CTL_DEL, client_fd, NULL) < 0) // stop monitoring client socket so server doesn't receive events for this client.
		throw std::runtime_error("IRC: epoll_ctl() failed.");
	begin = this->_client_fds.begin();
	end = this->_client_fds.end();
	for (it = begin; it != end; ++it) // iterate through the vector of active client fds
	{
		if (*it == client_fd) // if matches the disconnected client
		{
			this->_client_fds.erase(it); // remove the client fd from the vector
			break ;
		}
	}
	this->_client_buffers.erase(client_fd); // remove any complete/incomplete data stored for this client buffer
	this->_authenticated.erase(client_fd); // remove authentication status of this client
	this->_nicknames.erase(client_fd);     // remove nickname associated with this client
	this->_usernames.erase(client_fd);     // remove username associated with this client
	close(client_fd);
	std::cout << "[CLIENT " << client_fd << "] DISCONNECTED!" << std::endl;
}
