/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/28 05:42:35 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/09/29 15:21:15 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Server.hpp"

Server::Server(int port, const std::string &pass)
{
    this->_pass = pass;
    this->_port = port;
    this->_listenFd = -1;
	this->_epollFd = -1;
}

Server::~Server()
{
    if (this->_epollFd != -1)
    {
        close(this->_epollFd);
        std::cout << "\nepoll closed!" << std::endl;
    }
    if (this->_listenFd != -1)
    {
        close(this->_listenFd);
        std::cout << "Listening socket closed!" << std::endl;
    }
}

void Server::startSocket()
{
    int socketOpt;
    sockaddr_in serverAddr;

    std::cout << std::endl;
    if ((this->_listenFd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        throw std::runtime_error("IRC: socket() failed.");
    std::cout << "Listening TCP socket created!" << std::endl;

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(this->_port);
    std::cout << "IPv4 address configured!" << std::endl;

    socketOpt = 1;
    if (setsockopt(this->_listenFd, SOL_SOCKET, SO_REUSEADDR, &socketOpt, sizeof(socketOpt)) < 0)
        throw std::runtime_error("IRC: setsockopt() failed.");
    std::cout << "Socket address reuse enabled!" << std::endl;

    if (fcntl(this->_listenFd, F_SETFL, O_NONBLOCK) < 0)
        throw std::runtime_error("IRC: fcntl() failed.");
    std::cout << "Non-blocking mode enabled!" << std::endl;

    if (bind(this->_listenFd, reinterpret_cast<sockaddr *>(&serverAddr), sizeof(serverAddr)) < 0)
        throw std::runtime_error("IRC: bind() failed.");
    if (listen(this->_listenFd, SOMAXCONN) < 0)
        throw std::runtime_error("IRC: listen() failed.");
    std::cout << "Server bound and listening on port " << this->_port << " for incoming TCP connections!" << std::endl;
}

void Server::startEventLoop()
{
    epoll_event serverEvent;
	epoll_event events[10];
    int eventCount;
    int clientFd;

    if ((this->_epollFd = epoll_create1(0)) < 0)
        throw std::runtime_error("IRC: epoll_create1() failed.");
    std::cout << "epoll created!" << std::endl;

    serverEvent.events = EPOLLIN;
    serverEvent.data.fd = this->_listenFd;
	std::cout << "Listening socket event configured!" << std::endl;

    if (epoll_ctl(this->_epollFd, EPOLL_CTL_ADD, this->_listenFd, &serverEvent) < 0)
        throw std::runtime_error("IRC: epoll_ctl() failed.");
    std::cout << "Listening socket registered with epoll!" << std::endl;
	while (true)
	{
	    eventCount = epoll_wait(this->_epollFd, events, 10, -1);
	    if (eventCount < 0)
	        throw std::runtime_error("IRC: epoll_wait() failed.");
	    for (int i = 0; i < eventCount; i++)
	    {
	        if (events[i].data.fd == this->_listenFd && (events[i].events & EPOLLIN))
	        {
	            clientFd = accept(this->_listenFd, NULL, NULL);
	            if (clientFd < 0)
	                throw std::runtime_error("IRC: accept() failed.");
	            std::cout << "New TCP client connected!" << std::endl;
	            close(clientFd);
	        }
	    }
	}
}
