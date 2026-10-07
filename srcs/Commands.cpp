/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Commands.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/05 20:08:43 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/07 18:06:18 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Server.hpp"

void Server::handle_pass(int client_fd, const std::string &params)
{
    if (params.empty())
	{
	    this->send_reply(client_fd, ":ircserv 461 * PASS :Not enough parameters\r\n"); // ERR_NEEDMOREPARAMS
	    return;
	}
    if (params != this->_pass)
	{
	    this->send_reply(client_fd, ":ircserv 464 * :Password incorrect\r\n"); // ERR_PASSWDMISMATCH
	    return;
	}
	this->_clients[client_fd].set_auth(true);
    std::cout << "\nCORRECT PASS!" << std::endl;
}

void Server::handle_nick(int client_fd, const std::string &params)
{
    std::map<int, Client>::iterator it;

    if (params.empty() || params.find(' ') != std::string::npos) // if nickname is empty or has spaces
    {
        this->send_reply(client_fd, ":ircserv 431 * :No nickname given\r\n"); // ERR_NONICKNAMEGIVEN
        return;
    }
	if (!this->is_valid_nick(params))
	{
	    this->send_reply(client_fd, ":ircserv 432 * " + params + " :Erroneous nickname\r\n"); // ERR_ERRONEUSNICKNAME
	    return;
	}
    for (it = this->_clients.begin(); it != this->_clients.end(); ++it) // iterate through all connected clients
    {
        if (it->second.get_nick() == params) // check if requested nick is being used by another client
        {
            this->send_reply(client_fd, ":ircserv 433 * :Nickname is already in use\r\n"); // ERR_NICKNAMEINUSE
            return;
        }
    }
    this->_clients[client_fd].set_nick(params); // associate nick with this client fd
    std::cout << "\nNICK set to: " << params << std::endl;
}

void Server::handle_user(int client_fd, const std::string &params)
{
	std::string username;
	std::string mode;
	std::string unused;
	std::string realname;
	std::string remaining;
	size_t	space;

    remaining = params;
    space = remaining.find(' '); // space between username and mode
    if (space == std::string::npos || space == 0) // if has no space or if space is at the beginning
    {
        std::cout << "\nINVALID USER PARAMS!" << std::endl; // there's no username
        return;
    }
    username = remaining.substr(0, space); // extract username from params
    remaining.erase(0, space + 1); // remove username and space so remaining has mode, unused, and realname
    space = remaining.find(' '); // space between mode and unused
    if (space == std::string::npos || space == 0)
    {
        std::cout << "\nINVALID USER PARAMS!" << std::endl;
        return;
    }
    mode = remaining.substr(0, space); // extract mode from params
    remaining.erase(0, space + 1);
    space = remaining.find(' '); // space between unused and realname
    if (space == std::string::npos || space == 0)
    {
        std::cout << "\nINVALID USER PARAMS!" << std::endl;
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
    this->_clients[client_fd].set_user(username); // associate username with this client fd
	std::cout << "\nUSER: " << username << " " << mode << " " << unused << " " << realname << std::endl;
}
