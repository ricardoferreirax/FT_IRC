/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Commands.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/05 20:08:43 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/05 20:37:12 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Server.hpp"

void Server::handle_pass(int client_fd, const std::string &params)
{
    if (params.empty())
    {
        std::cout << "\n>> PASSWORD IS MISSING!" << std::endl;
		std::cout << "Authenticated: " << this->_authenticated[client_fd] << std::endl;
        return;
    }
    if (params != this->_pass)
    {
        std::cout << "\n>> INCORRECT PASSWORD!" << std::endl;
		std::cout << "Authenticated: " << this->_authenticated[client_fd] << std::endl;
        return;
    }
	this->_authenticated[client_fd] = true;
	std::cout << "Authenticated: " << this->_authenticated[client_fd] << std::endl;
    std::cout << "\n>> CORRECT PASSWORD!" << std::endl;
}

void Server::handle_nick(int client_fd, const std::string &params)
{
    (void)client_fd;
    (void)params;

    std::cout << "\n>> NICK COMMAND RECEIVED!" << std::endl;
}

void Server::handle_user(int client_fd, const std::string &params)
{
    (void)client_fd;
    (void)params;

    std::cout << "\n>> USER COMMAND RECEIVED!" << std::endl;
}
