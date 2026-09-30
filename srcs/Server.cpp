/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/28 05:42:35 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/09/30 16:32:43 by rmedeiro         ###   ########.fr       */
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

void Server::start_socket()
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
    std::cout << "Server listening on port " << this->_port << std::endl;
}

void Server::add_to_epoll(int fd)
{
    epoll_event event;

    event.events = EPOLLIN;
    event.data.fd = fd;
    if (epoll_ctl(this->_epoll_fd, EPOLL_CTL_ADD, fd, &event) < 0)
        throw std::runtime_error("IRC: epoll_ctl() failed.");
}

void Server::start_epoll()
{
    epoll_event events[10];
    int ready_events;
    int current_fd;

    this->_epoll_fd = epoll_create1(0);
    if (this->_epoll_fd < 0)
		throw std::runtime_error("IRC: epoll_create1() failed.");
    this->add_to_epoll(this->_listen_fd);
    std::cout << "\nWaiting for connections..." << std::endl;
    while (true)
    {
        ready_events = epoll_wait(this->_epoll_fd, events, 10, -1);
        if (ready_events < 0)
            throw std::runtime_error("IRC: epoll_wait() failed.");
        for (int i = 0; i < ready_events; i++)
        {
            current_fd = events[i].data.fd;
            if (current_fd == this->_listen_fd && (events[i].events & EPOLLIN))
            {
                this->accept_client();
            }
            else if (current_fd != this->_listen_fd && (events[i].events & EPOLLIN))
            {
                this->receive_data(current_fd);
            }
        }
    }
}

void Server::accept_client()
{
    int client_fd;

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
        this->_client_fds.push_back(client_fd);
    }
    catch (const std::exception &)
    {
        close(client_fd);
        throw;
    }
    try
    {
        this->add_to_epoll(client_fd);
    }
    catch (const std::exception &)
    {
        this->_client_fds.pop_back();
        close(client_fd);
        throw;
    }
    std::cout << "[Fd = " << client_fd << "] Client connected!" << std::endl;
}

void Server::receive_data(int client_fd)
{
    char buffer[1024];
    ssize_t bytes_recv;

    bytes_recv = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    if (bytes_recv > 0)
    {
        buffer[bytes_recv] = '\0';
        std::cout << "[Fd = " << client_fd << "] Received: " << buffer;
    }
    else if (bytes_recv == 0)
        this->disconnect_client(client_fd);
}

void Server::disconnect_client(int client_fd)
{
    std::vector<int>::iterator it;
    std::vector<int>::iterator begin;
    std::vector<int>::iterator end;

    if (epoll_ctl(this->_epoll_fd, EPOLL_CTL_DEL, client_fd, NULL) < 0)
        throw std::runtime_error("IRC: epoll_ctl() failed.");
    begin = this->_client_fds.begin();
    end = this->_client_fds.end();
    for (it = begin; it != end; ++it)
    {
        if (*it == client_fd)
        {
            this->_client_fds.erase(it);
            break;
        }
    }
    close(client_fd);
    std::cout << "[Fd = " << client_fd << "] Client disconnected!" << std::endl;
}
