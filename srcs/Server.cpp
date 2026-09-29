/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/28 05:42:35 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/09/29 14:25:16 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Server.hpp"

Server::Server(int port, const std::string &pass)
{
    this->_pass = pass;
    this->_port = port;
    this->_listenFd = -1;
}

Server::~Server()
{
    if (this->_listenFd != -1)
    {
        close(this->_listenFd);
        std::cout << "\nSocket closed!" << std::endl;
    }
}

void Server::startSocket()
{
    int socketOpt;
    sockaddr_in serverAddr;

    std::cout << std::endl;
    if ((this->_listenFd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        throw std::runtime_error("IRC: socket() failed.");
    std::cout << "[DEBUG] Listening TCP socket created! FD: " << this->_listenFd << std::endl;

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(this->_port);
    std::cout << "[DEBUG] IPv4 address configured!" << std::endl;

    socketOpt = 1;
    if (setsockopt(this->_listenFd, SOL_SOCKET, SO_REUSEADDR, &socketOpt, sizeof(socketOpt)) < 0)
        throw std::runtime_error("IRC: setsockopt() failed.");
    std::cout << "[DEBUG] Socket address reuse enabled!" << std::endl;

    if (fcntl(this->_listenFd, F_SETFL, O_NONBLOCK) < 0)
        throw std::runtime_error("IRC: F_SETFL failed.");
    std::cout << "[DEBUG] Non-blocking mode enabled!" << std::endl;

    if (bind(this->_listenFd, reinterpret_cast<sockaddr *>(&serverAddr), sizeof(serverAddr)) < 0)
        throw std::runtime_error("IRC: bind() failed.");
    if (listen(this->_listenFd, SOMAXCONN) < 0)
        throw std::runtime_error("IRC: listen() failed.");
    std::cout << "[DEBUG] Server bound and listening on port " << this->_port << " for incoming TCP connections!" << std::endl;
}

