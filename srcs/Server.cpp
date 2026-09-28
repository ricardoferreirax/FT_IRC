/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/28 05:42:35 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/09/28 05:58:39 by rmedeiro         ###   ########.fr       */
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
        close(this->_listenFd);
}

void Server::initListener()
{
    this->_listenFd = socket(AF_INET, SOCK_STREAM, 0);
    if (this->_listenFd == -1)
        throw std::runtime_error("IRC: socket creation failed.");
    std::cout << "Socket created! FD: " << this->_listenFd << std::endl;
}
