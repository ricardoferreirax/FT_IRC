/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/28 05:42:35 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/07 15:53:33 by rmedeiro         ###   ########.fr       */
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
    std::map<int, Client>::iterator it;

    std::cout << "Closing server..." << std::endl;
    for (it = this->_clients.begin(); it != this->_clients.end(); ++it)
    {
        close(it->first);
        std::cout << "[CLIENT " << it->first << "] Closed!" << std::endl;
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
    std::cout << "[SERVER] Listening on 0.0.0.0:" << this->_port << std::endl;
}

// creates epoll to monitor listening socket and connected client sockets.
// listening socket is monitored for new connections, while client sockets are monitored for incoming and outgoing data.
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
    event.events = EPOLLIN; // monitor listening socket for new incoming connections
    event.data.fd = this->_listen_fd; // store listening socket fd inside event so it can be identified later
    if (epoll_ctl(this->_epoll_fd, EPOLL_CTL_ADD, this->_listen_fd, &event) < 0) // add listening socket to epoll interest list
        throw std::runtime_error("IRC: epoll_ctl() failed.");
    std::cout << "\n[SERVER] Waiting for connections..." << std::endl;
    while (running) // keep monitoring socket events till server is stopped
    {
        ready_events = epoll_wait(this->_epoll_fd, events, 10, -1); // wait till at least one registered socket has an event
        if (ready_events < 0)
        {
            if (!running) // stop server when ctrl-c
                break;
            throw std::runtime_error("IRC: epoll_wait() failed.");
        }
        for (int i = 0; i < ready_events; i++) // process every event returned by epoll_wait()
        {
            current_fd = events[i].data.fd; // get fd that generated current event
            if (current_fd == this->_listen_fd && (events[i].events & EPOLLIN)) // if returned event is from listening socket and is EPOLLIN
                this->accept_client(); // EPOLLIN on listening socket means a new client is waiting to connect
            else // if returned event is from a client socket
            {
                if (events[i].events & EPOLLIN) // EPOLLIN on client socket means client has sent data
                    this->receive_client_data(current_fd);
                if (events[i].events & EPOLLOUT) // EPOLLOUT means client socket is ready to receive data from server
                    this->send_client_data(current_fd);
            }
        }
    }
}

// client socket is made non-blocking, added to epoll and a Client object is created
// to store all data and state associated with this connection
void Server::accept_client()
{
	int			client_fd;
	epoll_event	event;

    client_fd = accept(this->_listen_fd, NULL, NULL); // accept pending tcp connection and create new socket to communicate with client
    if (client_fd < 0)
        throw std::runtime_error("IRC: accept() failed.");
    if (fcntl(client_fd, F_SETFL, O_NONBLOCK) < 0) // make client socket non-blocking so socket operations don't block server
    {
        close(client_fd);
        throw std::runtime_error("IRC: fcntl() failed.");
    }
    event.events = EPOLLIN; // initially monitor client socket only for incoming data
    event.data.fd = client_fd; // store fd to server identify which client generated event
    if (epoll_ctl(this->_epoll_fd, EPOLL_CTL_ADD, client_fd, &event) < 0) // add client socket to epoll interest list
    {
        close(client_fd);
        throw std::runtime_error("IRC: epoll_ctl() failed.");
    }
	this->_clients[client_fd] = Client(); // create and store client object associated with its socket fd
    this->_authenticated[client_fd] = false;
    this->_registered[client_fd] = false;
    std::cout << "\n==========================================" << std::endl;
    std::cout << "     [CLIENT " << client_fd << "] CONNECTED!" << std::endl;
    std::cout << "==========================================\n" << std::endl;
}

// client socket data is ready to be read/received, received bytes are appended to buffer associated with this client,
// one recv() may contain a partial irc msg or multiple, only msgs ending with "\r\n" are processed,
// incomplete data stays in client's buffer and will be completed by a future recv()
void Server::receive_client_data(int client_fd)
{
    std::string &client_buffer = this->_clients[client_fd].get_recv_buffer(); // buffer stores received data for this specific client
    std::string msg;
    char buffer[1024];
    ssize_t bytes_recv; // nb of bytes received from client socket
    size_t pos;
    size_t space;

    bytes_recv = recv(client_fd, buffer, sizeof(buffer), 0); // receive/reads available tcp data from client socket and store it in buffer
    if (bytes_recv < 0)
    {
        std::cerr << "[CLIENT " << client_fd << "] recv() failed." << std::endl;
        return;
    }
    if (bytes_recv == 0) // if client has closed its tcp connection -> recv() returns 0
	{
		this->disconnect_client(client_fd);
		return;
	}
    client_buffer.append(buffer, bytes_recv); // append received bytes to this client's buffer
    pos = client_buffer.find("\r\n"); // search for complete irc msgs
    while (pos != std::string::npos) // process every complete msg stored in buffer
    {
        msg = client_buffer.substr(0, pos); // extract one complete message without "\r\n"
        std::cout << "------------------------------------------------------------" << std::endl;
        std::cout << "[CLIENT " << client_fd << "] Message: " << msg << std::endl;
        space = msg.find(' '); // search for first space that separates cmd from its params
        if (space == std::string::npos) // no space means msg contains only a cmd
            this->handle_cmd(client_fd, msg, "");
        else
            this->handle_cmd(client_fd, msg.substr(0, space), msg.substr(space + 1));
        client_buffer.erase(0, pos + 2); // remove processed message + "\r\n" from buffer
        pos = client_buffer.find("\r\n"); // search again cause another complete message may be stored in buffer
    }
}

// client socket is ready for writing/send, only successfully sent bytes are removed, pending server replies are stored in 
// output buffer till they can be sent to this client, when output buffer becomes empty, EPOLLOUT is removed because there is nothing left to send.
void Server::send_client_data(int client_fd)
{
    std::string &output = this->_clients[client_fd].get_send_buffer(); // buffer has data waiting to be sent to this specific client
    ssize_t bytes_sent; // nb of bytes sent to client socket
    epoll_event event;

    if (output.empty()) // nothing is waiting to be sent to this client
        return;
    bytes_sent = send(client_fd, output.c_str(), output.size(), 0); // send part of pending tcp data from buffer to client socket
    if (bytes_sent < 0)
    {
        std::cerr << "[CLIENT " << client_fd << "] send() failed." << std::endl;
        return;
    }
    output.erase(0, bytes_sent); // remove only successfully sent bytes
    if (output.empty()) // if all pending data was sent, socket no longer needs to be monitored for writing
    {
        event.events = EPOLLIN; // keep monitoring client socket for new incoming data
        event.data.fd = client_fd; // store client fd in event so server can identify which client generated it
        if (epoll_ctl(this->_epoll_fd, EPOLL_CTL_MOD, client_fd, &event) < 0) // update epoll and stop monitoring EPOLLOUT for this client
            throw std::runtime_error("IRC: epoll_ctl() failed.");
    }
}

// welcome reply is stored in buffer and EPOLLOUT is enabled so epoll can notify when socket is ready for writing
// EPOLL_CTL_MOD modify events monitored for this client socket, adding EPOLLOUT to existing EPOLLIN
void Server::register_client(int client_fd)
{
    std::string nick; // store nick of client that has completed registration
    epoll_event event;

    if (this->_registered[client_fd])
        return;
    if (this->_authenticated[client_fd] && !this->_nicknames[client_fd].empty() && !this->_usernames[client_fd].empty())
    {
        this->_registered[client_fd] = true;
        nick = this->_nicknames[client_fd];
        std::cout << "\n==========================================" << std::endl;
        std::cout << "     [CLIENT " << client_fd << "] REGISTERED!" << std::endl;
        std::cout << "==========================================" << std::endl;
        this->_clients[client_fd].get_send_buffer() += ":ircserv 001 " + nick + " :Welcome to the IRC server\r\n"; // += allows multiple replies to not replace each other 
        event.events = EPOLLIN | EPOLLOUT; // monitor client for both incoming data and readiness for writing
        event.data.fd = client_fd;
        if (epoll_ctl(this->_epoll_fd, EPOLL_CTL_MOD, client_fd, &event) < 0) // update client events to add EPOLLOUT to EPOLLIN
            throw std::runtime_error("IRC: epoll_ctl() failed.");
        std::cout << std::endl;
    }
}

void Server::handle_cmd(int client_fd, const std::string &cmd,
	const std::string &params)
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
    {
        std::cout << "\nUNKNOWN COMMAND" << std::endl;
        return;
    }
    this->register_client(client_fd);
    std::cout << std::endl;
}

// removes disconnected client from epoll, deletes its client object and closes socket associated with the connection
// EPOLL_CTL_DEL removes socket from epoll interest list
void Server::disconnect_client(int client_fd)
{
    if (epoll_ctl(this->_epoll_fd, EPOLL_CTL_DEL, client_fd, NULL) < 0) // stop monitoring client socket
        throw std::runtime_error("IRC: epoll_ctl() failed.");
    this->_clients.erase(client_fd); // remove client from server container
    this->_authenticated.erase(client_fd);
    this->_registered.erase(client_fd);
    this->_nicknames.erase(client_fd);
    this->_usernames.erase(client_fd);
    close(client_fd);
    std::cout << "==========================================" << std::endl;
    std::cout << "     [CLIENT " << client_fd << "] DISCONNECTED!" << std::endl;
    std::cout << "==========================================\n" << std::endl;
}
