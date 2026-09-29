/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/28 05:42:35 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/09/29 18:23:58 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


#include "../includes/Server.hpp"

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
    for (size_t i = 0; i < this->_clientFds.size(); i++)
        close(this->_clientFds[i]);  // close connected clients
    if (this->_epoll_fd != -1)
        close(this->_epoll_fd);
    if (this->_listen_fd != -1)
        close(this->_listen_fd);
    std::cout << "Server closed!" << std::endl;
}

void Server::start_socket()  // create and configure the listening socket
{
    int socket_opt;
    sockaddr_in server_addr;

    this->_listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (this->_listen_fd < 0)
        throw std::runtime_error("IRC: socket() failed.");
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(this->_port);
	socket_opt = 1;
    if (setsockopt(this->_listen_fd, SOL_SOCKET, SO_REUSEADDR, &socket_opt, sizeof(socket_opt)) < 0)
        throw std::runtime_error("IRC: setsockopt() failed.");
    if (fcntl(this->_listen_fd, F_SETFL, O_NONBLOCK) < 0)
        throw std::runtime_error("IRC: fcntl() failed.");
    if (bind(this->_listen_fd, reinterpret_cast<sockaddr *>(&server_addr), sizeof(server_addr)) < 0)
        throw std::runtime_error("IRC: bind() failed.");
    if (listen(this->_listen_fd, SOMAXCONN) < 0)
        throw std::runtime_error("IRC: listen() failed.");
    std::cout << "Server listening on port: " << this->_port << std::endl;
}

void Server::start_event_loop()
{
    epoll_event events[10];
    int ready_events;
    int current_fd;
    bool connected;

	connected = false;
    std::cout << "\nWaiting for connections..." << std::endl;
    while (true)
    {
        if (connected)
            std::cout << "\nWaiting for events..." << std::endl;
        ready_events = epoll_wait(this->_epoll_fd, events, 10, -1);
        if (ready_events < 0)
            throw std::runtime_error("IRC: epoll_wait() failed.");
        for (int i = 0; i < ready_events; i++)
        {
            current_fd = events[i].data.fd;
            if (current_fd == this->_listen_fd && (events[i].events & EPOLLIN))
            {
                this->accept_client();
                connected = true;
            }
            else if (current_fd != this->_listen_fd)
            {
                std::cout << "Client event detected! Client Fd: " << current_fd << std::endl;
                epoll_ctl(this->_epoll_fd, EPOLL_CTL_DEL, current_fd, NULL); // unregister the client socket from epoll
            }
        }
    }
}

void Server::setup_epoll()
{
    epoll_event event;

    this->_epoll_fd = epoll_create1(0);
    if (this->_epoll_fd < 0)
        throw std::runtime_error("IRC: epoll_create1() failed.");
    event.events = EPOLLIN;
    event.data.fd = this->_listen_fd;
    if (epoll_ctl(this->_epoll_fd, EPOLL_CTL_ADD, this->_listen_fd, &event) < 0) // register the listening socket with epoll
        throw std::runtime_error("IRC: epoll_ctl() failed.");
}

void Server::accept_client()
{
    int client_fd;
    epoll_event event;

    client_fd = accept(this->_listen_fd, NULL, NULL);
    if (client_fd < 0)
        throw std::runtime_error("IRC: accept() failed.");
    if (fcntl(client_fd, F_SETFL, O_NONBLOCK) < 0)
    {
        close(client_fd);
        throw std::runtime_error("IRC: fcntl() failed.");
    }
    try
    {
        this->_clientFds.push_back(client_fd);  // store the client
    }
    catch (const std::exception &e)
    {
        std::cerr << "IRC: Occurred an error while storing client: " << e.what() << std::endl;
        close(client_fd); // close client socket if we can't store it
        throw std::runtime_error("IRC: Failed to store client!");
    }
    event.events = EPOLLIN;  // register the client with epoll
    event.data.fd = client_fd;
    if (epoll_ctl(this->_epoll_fd, EPOLL_CTL_ADD, client_fd, &event) < 0)  // register the client socket with epoll
    {
        this->_clientFds.pop_back();
        close(client_fd);
        throw std::runtime_error("IRC: epoll_ctl() failed.");
    }
    std::cout << "Client connected! Client Fd: " << client_fd << std::endl;
}
