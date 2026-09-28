/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/28 05:42:35 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/09/28 13:56:27 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Server.hpp"

Server::Server(int port, const std::string &pass)
{
    this->_pass = pass;
    this->_port = port;
    this->_listenFd = -1;
	std::cout << "Server created! Port: " << this->_port << std::endl;
}

Server::~Server()
{
    if (this->_listenFd != -1)
	{
		close(this->_listenFd);
		std::cout << "Socket closed! FD: " << this->_listenFd << std::endl;
	}
}

void Server::initListener()
{
    int socketOpt;
	int initialFdStatus;
	int updatedFdStatus;
    sockaddr_in bindAdress;     // initialize ipv4 listening address

    this->_listenFd = socket(AF_INET, SOCK_STREAM, 0);
    if (this->_listenFd < 0)
        throw std::runtime_error("IRC: socket creation failed.");
    std::cout << "Socket created! FD: " << this->_listenFd << std::endl;

    socketOpt = 1;
    if (setsockopt(this->_listenFd, SOL_SOCKET,
        SO_REUSEADDR, &socketOpt, sizeof(socketOpt)) < 0)
        throw std::runtime_error("IRC: setsockopt failed.");
    std::cout << "Socket options configured!" << std::endl;
		
    // configure listening address
    bindAdress.sin_family = AF_INET;    // set ipv4 as address family
    bindAdress.sin_port = htons(this->_port);     // set server port and convert it from host to network byte order
    bindAdress.sin_addr.s_addr = htonl(INADDR_ANY);  // allow accept connections on all available local ipv4 interfaces

    if (bind(this->_listenFd, reinterpret_cast<sockaddr *>(&bindAdress), sizeof(bindAdress)) < 0)
	{
		close(this->_listenFd);
        throw std::runtime_error("IRC: bind failed.");
	}
    std::cout << "Socket bound to port: " << this->_port << std::endl;
	
	initialFdStatus = fcntl(this->_listenFd, F_GETFL, 0);  // get current file status flags
	if (initialFdStatus < 0 || fcntl(this->_listenFd, F_SETFL, initialFdStatus | O_NONBLOCK) < 0)
	    throw std::runtime_error("IRC: fcntl failed.");
	updatedFdStatus = fcntl(this->_listenFd, F_GETFL, 0); // read file status flags again because F_SETFL updates the socket, not the original variable	
	if (updatedFdStatus < 0)
	    throw std::runtime_error("IRC: fcntl verification failed.");		
	if (updatedFdStatus & O_NONBLOCK)
	    std::cout << "Non-blocking enabled!" << std::endl;
	else
	    std::cout << "Socket is blocking!" << std::endl;
}
