/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/28 05:42:35 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/06 15:27:39 by rmedeiro         ###   ########.fr       */
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
    int socket_opt;
    sockaddr_in server_addr;  // describes ipv4 address where server will listen

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
    std::cout << "[SERVER] Listening on 0.0.0.0:" << this->_port << std::endl;
}

// creates epoll to monitor listening socket and connected client sockets.
// listening socket is monitored for new connections, while client sockets are monitored for incoming data. 
// epoll_wait() blocks until one or more registered sockets become ready.
void Server::monitor_epoll_events()
{
    epoll_event event;
    epoll_event events[10];
    int ready_events; // nb of ready events returned by epoll_wait()
    int current_fd;

    this->_epoll_fd = epoll_create1(0); // create epoll to monitor socket events
    if (this->_epoll_fd < 0)
        throw std::runtime_error("IRC: epoll_create1() failed.");
    event.events = EPOLLIN; // monitor for EPOLLIN events for a listening socket, EPOLLIN means a new connection is ready to be accepted
    event.data.fd = this->_listen_fd; // store listening socket fd inside event so it can be identified later
    if (epoll_ctl(this->_epoll_fd, EPOLL_CTL_ADD, this->_listen_fd, &event) < 0) // add the listening socket to the epoll interest list
        throw std::runtime_error("IRC: epoll_ctl() failed.");
    std::cout << "\n[SERVER] Waiting for connections..." << std::endl;
    while (running) // keep monitoring socket events till server is stopped
    {
        ready_events = epoll_wait(this->_epoll_fd, events, 10, -1); // waits indefinitely (-1) till at least one registered socket has an event
        if (ready_events < 0)
        {
            if (!running) // stop server when ctrl-c
                break;
            throw std::runtime_error("IRC: epoll_wait() failed.");
        }
        for (int i = 0; i < ready_events; i++) // process every event returned by epoll_wait()
        {
            current_fd = events[i].data.fd; // get fd that generated the current event
            if (current_fd == this->_listen_fd && (events[i].events & EPOLLIN)) // if returned event is from listening socket and is an EPOLLIN
                this->accept_client(); // EPOLLIN on the listening socket means a new client is waiting to connect
            else if (current_fd != this->_listen_fd && (events[i].events & EPOLLIN)) // if returned event is from a client socket and is an EPOLLIN
                this->process_client_data(current_fd); // EPOLLIN on a client socket means client has sent data
        }
    }
}

// client socket is made non-blocking, added to epoll and its initial data is stored so server can manage client independently
// each client has its own buffer to store incoming data, authentication status, nickname and username
void Server::accept_client()
{
    int client_fd;
    epoll_event event;

    client_fd = accept(this->_listen_fd, NULL, NULL); // accept pending tcp connection and create new socket to communicate with client socket
    if (client_fd < 0)
        throw std::runtime_error("IRC: accept() failed.");
    if (fcntl(client_fd, F_SETFL, O_NONBLOCK) < 0) // make client socket non-blocking so recv() doesn't block server while waiting for data from specific client
    {
        close(client_fd);
        throw std::runtime_error("IRC: fcntl() failed.");
    }
    event.events = EPOLLIN; // monitor the client socket for incoming data
    event.data.fd = client_fd; // store client fd in event so server can identify which client generated it
    if (epoll_ctl(this->_epoll_fd, EPOLL_CTL_ADD, client_fd, &event) < 0) // add client socket to epoll interest list
    {
        close(client_fd);
        throw std::runtime_error("IRC: epoll_ctl() failed.");
    }
    this->_client_fds.push_back(client_fd); // store client fd in list of connected clients
    this->_client_buffers[client_fd] = ""; // create empty receive buffer used to store incoming data from client
    this->_authenticated[client_fd] = false;
	this->_registered[client_fd] = false;
	std::cout << "\n==========================================" << std::endl;	
    std::cout << "			[CLIENT " << client_fd << "] CONNECTED!" << std::endl;
	std::cout << "==========================================\n" << std::endl;	
}

// receives available data from client socket and appends it to client's buffer.
// buffer keeps received data till a complete irc msg ending with "\r\n" is found.
// complete messages are processed while incomplete data remains in buffer for next recv().
void Server::process_client_data(int client_fd)
{
	std::string &client_buffer = this->_client_buffers[client_fd]; // buffer associated with this client fd
	std::string msg;
    char buffer[1024];
    ssize_t bytes_recv;
    size_t pos;
    size_t space;

    bytes_recv = recv(client_fd, buffer, sizeof(buffer), 0); // reads tcp data from connected client socket and stores it in buffer
	if (bytes_recv < 0)
        return;
    if (bytes_recv == 0) // if peer has closed its side of tcp connection
    {
        this->disconnect_client(client_fd);
        return;
    }
    client_buffer.append(buffer, bytes_recv); // append exactly received bytes to buffer associated with this client
    pos = client_buffer.find("\r\n"); // search for first "\r\n" in buffer
    while (pos != std::string::npos) // while there is at least one complete msg ready to be processed
    {
        msg = client_buffer.substr(0, pos); // copy till "\r\n" into msg
        std::cout << "------------------------------------------------------------" << std::endl;
        std::cout << "[CLIENT " << client_fd << "] Message: " << msg << std::endl;
        space = msg.find(' '); // search for first space in msg, which separates cmd from its params
        if (space == std::string::npos) // if no space was found
            this->handle_cmd(client_fd, msg, ""); // msg has only a cmd, params is empty string
        else
            this->handle_cmd(client_fd, msg.substr(0, space), msg.substr(space + 1)); // msg has a cmd followed by at least one param
        client_buffer.erase(0, pos + 2); // remove processed message and "\r\n" from client's buffer
        pos = client_buffer.find("\r\n"); // search again cause buffer may has another complete msg after one that was removed
    }
}

void Server::handle_cmd(int client_fd, const std::string &cmd, const std::string &params)
{
    std::cout << "\n[CMD]: " << cmd << std::endl;
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

void Server::check_registration(int client_fd)
{
    if (this->_registered[client_fd])
        return;
    if (this->_authenticated[client_fd] && !this->_nicknames[client_fd].empty() && !this->_usernames[client_fd].empty())
    {
        this->_registered[client_fd] = true;
		std::cout << "\n==========================================" << std::endl;
        std::cout << "[CLIENT " << client_fd << "] REGISTERED!" << std::endl;
		std::cout << "==========================================" << std::endl;

    }
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
    for (it = begin; it != end; ++it)  // iterate through the vector of active client fds
    {
        if (*it == client_fd) // if matches the disconnected client
        {
            this->_client_fds.erase(it); // remove the client fd from the vector
            break;
        }
    }
    this->_client_buffers.erase(client_fd); // remove any complete/incomplete data stored for this client buffer
	this->_authenticated.erase(client_fd); // remove authentication status of this client
	this->_registered.erase(client_fd);
	this->_nicknames.erase(client_fd); // remove nickname associated with this client
	this->_usernames.erase(client_fd); // remove username associated with this client
    close(client_fd);
    std::cout << "[CLIENT " << client_fd << "] DISCONNECTED!" << std::endl;
}
