/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/28 05:42:35 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/09/28 06:33:36 by rmedeiro         ###   ########.fr       */
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
	int			socketOpt;
	sockaddr_in	listenAddr;

	this->_listenFd = socket(AF_INET, SOCK_STREAM, 0);
	if (this->_listenFd < 0)
		throw std::runtime_error("IRC: socket creation failed.");
	std::cout << "Socket created! FD: " << this->_listenFd << std::endl;
	socketOpt = 1;
	if (setsockopt(this->_listenFd, SOL_SOCKET, SO_REUSEADDR, &socketOpt,
			sizeof(socketOpt)) < 0)
		throw std::runtime_error("IRC: setsockopt failed.");
	std::cout << "Socket options configured!" << std::endl;
	// configure listening address
	listenAddr.sin_family = AF_INET;
	listenAddr.sin_port = htons(this->_port);
	listenAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(this->_listenFd, reinterpret_cast<sockaddr *>(&listenAddr),
			sizeof(listenAddr)) < 0)
	{
		close(this->_listenFd);
		throw std::runtime_error("IRC: bind failed.");
	}
	std::cout << "Socket bound to port: " << this->_port << std::endl;
}
