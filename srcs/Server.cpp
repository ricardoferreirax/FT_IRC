/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/28 05:42:35 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/01 11:00:15 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Server.hpp"

// creates server object and stores config received.
// port: where server will listen for incoming tcp connections.
// pass: password clients will need during irc registration.
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
        std::cout << "[Fd = " << this->_client_fds[i] << "] Client socket fd closed!" << std::endl;
    }
    if (this->_epoll_fd != -1)
    {
        close(this->_epoll_fd);
        std::cout << "[Fd = " << this->_epoll_fd << "] epoll fd closed!" << std::endl;
    }
    if (this->_listen_fd != -1)
    {
        close(this->_listen_fd);
        std::cout << "[Fd = " << this->_listen_fd << "] Listening socket fd closed!" << std::endl;
    }
    std::cout << "Server closed!" << std::endl;
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
    std::cout << "Server listening on port " << this->_port << std::endl;
}

// creates epoll and starts the event loop of the server
// epoll allows a single server process to monitor multiple sockets without blocking while waiting for one specific client.
// each returned event contains fd that generated it, if it is listen_fd, there is a new connection to accept.
// otherwise, event belongs to an already connected client.
void Server::start_epoll()
{
    epoll_event events[10];
    int ready_events;       // how many events were returned by epoll_wait()
    int current_fd;

    this->_epoll_fd = epoll_create1(0);  // create epoll instance that will be used to monitor multiple fds for events
    if (this->_epoll_fd < 0)
        throw std::runtime_error("IRC: epoll_create1() failed.");
    this->add_to_epoll(this->_listen_fd); // register listening socket with epoll so server can be notified when a new connection is waiting to be accepted
    std::cout << "\nWaiting for connections..." << std::endl;
    while (true)
    {
        ready_events = epoll_wait(this->_epoll_fd, events, 10, -1);  // wait until one or more registered fds become ready for reading. epoll_wait() blocks until at least one fd is ready.
        if (ready_events < 0)
            throw std::runtime_error("IRC: epoll_wait() failed.");
        for (int i = 0; i < ready_events; i++) // iterate through all the events returned by epoll_wait() and process them one by one
        {
            current_fd = events[i].data.fd;
            if (current_fd == this->_listen_fd && (events[i].events & EPOLLIN))  // An EPOLLIN event on listening socket means that a new client is waiting to be accepted
            {
                this->accept_client();
            }
            else if (current_fd != this->_listen_fd && (events[i].events & EPOLLIN)) // An EPOLLIN event on a client socket means that client has sent data that can be read with recv()
            {
                this->receive_data(current_fd);
            }
        }
    }
}

// registers a fd inside the server's epoll instance. epoll needs to know which fds server wants to monitor.
void Server::add_to_epoll(int fd)
{
    epoll_event event;

    event.events = EPOLLIN;  // tells epoll that we are interested in events where the fd becomes ready for reading
    event.data.fd = fd;      // stores the fd inside the event, this allows to know which fd generated the event
    if (epoll_ctl(this->_epoll_fd, EPOLL_CTL_ADD, fd, &event) < 0) // tells epoll_ctl() that this fd should be added to epoll interest list
        throw std::runtime_error("IRC: epoll_ctl() failed.");
}

// accepts one new tcp client connection and prepares its socket so it can be managed by the server.
// buffer is necessary because tcp is a byte stream and one recv() call doesn't necessarily correspond to one complete irc message
void Server::accept_client()
{
    int client_fd;

    client_fd = accept(this->_listen_fd, NULL, NULL);  // accepts the pending connection from listening sockcet. client_fd represents the new connected client
    if (client_fd < 0)
        throw std::runtime_error("IRC: accept() failed.");
    if (fcntl(client_fd, F_SETFL, O_NONBLOCK) < 0) // every connected client socket must also be non-blocking so that recv() does not block the server while waiting for data from one specific client
    {
        close(client_fd);
        throw std::runtime_error("IRC: fcntl() failed.");
    }
    try
    {
        this->_client_fds.push_back(client_fd);  // store client fd so server knows which clients are currently connected
    }
    catch (const std::exception &)
    {
        close(client_fd); // if vector allocation fails, close the socket
        throw;
    }
    try
    {
        this->add_to_epoll(client_fd); // register the client socket with epoll so server can be notified when this client sends data
    }
    catch (const std::exception &)
    {
        this->_client_fds.pop_back(); // remove the client fd from the vector before closing the socket
        close(client_fd);
        throw;
    }
    this->_client_buffers[client_fd] = "";  // create empty receive buffer for this client. Each client needs its own buffer because data from different clients must never be mixed
    std::cout << "[Fd = " << client_fd << "] Client connected!" << std::endl;
}

// reads available tcp data from one connected client.
// recv() tells how many bytes are currently available, it doesn't guarantee that those bytes contain exactly one complete irc message.
// Otherwise, multiple irc messages may arrive in one recv() call.
// Because of this, received bytes are appended to the persistent buffer associated with this client.
void Server::receive_data(int client_fd)
{
    char buffer[1024];
    ssize_t bytes_recv;

    bytes_recv = recv(client_fd, buffer, sizeof(buffer), 0);  // reads data from client socket and stores it in the buffer. It returns the number of bytes read.
    if (bytes_recv > 0)
    {
        this->_client_buffers[client_fd].append(buffer, bytes_recv); // append the exactly received bytes to the buffer associated with this client. This buffer may contain incomplete data from previous recv() calls.
        this->process_buffer(client_fd); // extracts complete irc messages from the client's receive buffer and processes them
    }
    else if (bytes_recv == 0) // means that peer has closed its side of the tcp connection
    {
        this->disconnect_client(client_fd);
    }
}

// extracts complete irc messages from a client's receive buffer. irc messages are terminated by \r\n.
// any incomplete data left after the loop stays inside the client buffer and will be combined with bytes received by a future recv().
void Server::process_buffer(int client_fd)
{
    std::string &buffer = this->_client_buffers[client_fd];
    std::string message;
    size_t pos;

    pos = buffer.find("\r\n"); // searches for the end of the first complete irc message in the buffer
    while (pos != std::string::npos)
    {
        message = buffer.substr(0, pos); // extracts the message without the terminating \r\n
        std::cout << "[Fd = " << client_fd << "] Message: " << message << std::endl;
        buffer.erase(0, pos + 2); // remove the processed message and its "\r\n" terminator from the buffer
        pos = buffer.find("\r\n"); // searches for the end of the next complete irc message in the buffer
    }
}

// removes a disconnected client from every part of the server that was tracking it
// EPOLL_CTL_DEL removes the socket from the epoll interest list
void Server::disconnect_client(int client_fd)
{
    std::vector<int>::iterator it;
    std::vector<int>::iterator begin;
    std::vector<int>::iterator end;

    if (epoll_ctl(this->_epoll_fd, EPOLL_CTL_DEL, client_fd, NULL) < 0) // stop monitoring client socket with epoll (removes from epoll list) so server will no longer receive events for this client.
        throw std::runtime_error("IRC: epoll_ctl() failed.");
    begin = this->_client_fds.begin();
    end = this->_client_fds.end();
    for (it = begin; it != end; ++it)  // iterate through the vector of active client fds
    {
        if (*it == client_fd) // to find the one that matches the disconnected client
        {
            this->_client_fds.erase(it); // remove the client fd from the vector
            break;
        }
    }
    this->_client_buffers.erase(client_fd); // remove any complete or incomplete data stored for this client buffer
    close(client_fd); // close socket so that OS can reuse the fd for future connections
    std::cout << "[Fd = " << client_fd << "] Client disconnected!" << std::endl;
}
