/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Commands.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/05 20:08:43 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/05 21:38:18 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Server.hpp"

void Server::handle_pass(int client_fd, const std::string &params)
{
    if (params.empty())
    {
		std::cout << "\nPASSWORD IS MISSING!";
		std::cout << " | [AUTH]: " << this->_authenticated[client_fd] << std::endl;
        return;
    }
    if (params != this->_pass)
    {
		std::cout << "\nPASSWORD IS INCORRECT!";
        std::cout << " | [AUTH]: " << this->_authenticated[client_fd] << std::endl;
        return;
    }
	this->_authenticated[client_fd] = true;
    std::cout << "\nCORRECT PASSWORD!";
	std::cout << " | [AUTH]: " << this->_authenticated[client_fd] << std::endl;
}

void Server::handle_nick(int client_fd, const std::string &params)
{
    std::map<int, std::string>::iterator it;
    std::map<int, std::string>::iterator begin;
    std::map<int, std::string>::iterator end;

    begin = this->_nicknames.begin(); // points to first client nickname
    end = this->_nicknames.end(); // points to position after last nickname
    if (params.empty())
    {
        std::cout << "\nNICKNAME IS MISSING!" << std::endl;
        return;
    }
    for (it = begin; it != end; ++it) // iterate through all stored client nicknames
    {
        if (it->second == params) // check if requested nickname is already being used
        {
            std::cout << "\nNICKNAME ALREADY IN USE!" << std::endl;
            return;
        }
    }
    this->_nicknames[client_fd] = params; // associate nickname with this client fd
    std::cout << "\nNICKNAME SET TO " << params << std::endl;
}

void Server::handle_user(int client_fd, const std::string &params)
{
    (void)client_fd;
    (void)params;

    std::cout << "\n>> USER COMMAND RECEIVED!" << std::endl;
}
