/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Commands.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/05 20:08:43 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/06 16:39:08 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Server.hpp"

void Server::handle_pass(int client_fd, const std::string &params)
{
    if (params.empty() || params.find(' ') != std::string::npos)
    {
		std::cout << "\nPASSWORD IS MISSING!" << std::endl;
        return;
    }
    if (params != this->_pass)
    {
		std::cout << "\nPASSWORD IS INCORRECT!" << std::endl;
        return;
    }
	this->_authenticated[client_fd] = true;
    std::cout << "\nCORRECT PASSWORD!" << std::endl;
	this->register_client(client_fd);
}

void Server::handle_nick(int client_fd, const std::string &params)
{
    std::map<int, std::string>::iterator it;
    std::map<int, std::string>::iterator begin;
    std::map<int, std::string>::iterator end;

    begin = this->_nicknames.begin(); // points to first client nickname
    end = this->_nicknames.end(); // points to position after last nickname
    if (params.empty() || params.find(' ') != std::string::npos)
    {
        std::cout << "\nINVALID NICKNAME!" << std::endl;
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
	this->register_client(client_fd);
}

void Server::handle_user(int client_fd, const std::string &params)
{
    std::string remaining;
    std::string username;
    std::string mode;
    std::string unused;
    std::string realname;
    size_t space;

    remaining = params;
    space = remaining.find(' '); // space between username and mode
    if (space == std::string::npos || space == 0) // if has no space or if space is at the beginning
    {
        std::cout << "\nINVALID USER PARAMETERS!" << std::endl; // there's no username
        return;
    }
    username = remaining.substr(0, space); // extract username from params
    remaining.erase(0, space + 1); // remove username and space so remaining has mode, unused, and realname
    space = remaining.find(' '); // space between mode and unused
    if (space == std::string::npos || space == 0)
    {
        std::cout << "\nINVALID USER PARAMETERS!" << std::endl;
        return;
    }
    mode = remaining.substr(0, space); // extract mode from params
    remaining.erase(0, space + 1);
    space = remaining.find(' '); // space between unused and realname
    if (space == std::string::npos || space == 0)
    {
        std::cout << "\nINVALID USER PARAMETERS!" << std::endl;
        return;
    }
    unused = remaining.substr(0, space); // extract unused from params
    remaining.erase(0, space + 1);
    if (remaining.size() < 2 || remaining[0] != ':' || remaining[1] == ' ') // realname has at least 2 characters and starts with a colon
    {
        std::cout << "\nINVALID REALNAME!" << std::endl;
        return;
    }
    realname = remaining.substr(1); // extract realname from params, removing colon
    this->_usernames[client_fd] = username;
    std::cout << "\nUSERNAME: " << username << std::endl;
    std::cout << "MODE: " << mode << std::endl;
    std::cout << "UNUSED: " << unused << std::endl;
    std::cout << "REALNAME: " << realname << std::endl;
	this->register_client(client_fd);
}
