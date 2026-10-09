/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Commands.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/05 20:08:43 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/09 12:57:19 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Server.hpp"
#include "../includes/Utils.hpp"

void Server::handle_pass(int client_fd, const std::string &params)
{
    if (params.empty())
    {
        this->send_reply(client_fd, ":" + this->_name + " 461 * PASS :Not enough parameters\r\n");
        return;
    }
    if (params.find(' ') != std::string::npos || params != this->_pass)
    {
        this->send_reply(client_fd, ":" + this->_name + " 464 * :Password incorrect\r\n");
        return;
    }
    this->_clients[client_fd].set_auth(true);
    this->send_reply(client_fd, "Correct password!\r\n");
}

void Server::handle_nick(int client_fd, const std::string &params)
{
    std::map<int, Client>::iterator it;
    std::string nick;
    size_t space;

    if (params.empty())
    {
        this->send_reply(client_fd, ":" + this->_name + " 431 * :No nickname given\r\n");
        return;
    }
    space = params.find(' ');
    if (space == std::string::npos) // if no space was found
        nick = params; // entire params is the nick
    else
        nick = params.substr(0, space); // extract nick from params ignoring extra params after space
    if (!is_valid_nick(nick))
    {
        this->send_reply(client_fd, ":" + this->_name + " 432 * " + nick + " :Erroneous nickname\r\n");
        return;
    }
    for (it = this->_clients.begin(); it != this->_clients.end(); ++it)
	{
	    if (it->first != client_fd && it->second.get_nick() == nick)
	    {
	        this->send_reply(client_fd, ":" + this->_name + " 433 * " + nick + " :Nickname is already in use\r\n");
	        return;
	    }
	}
    this->_clients[client_fd].set_nick(nick);
	this->send_reply(client_fd, "NICK set to: " + nick + "\r\n");
}

void Server::handle_user(int client_fd, const std::string &params)
{
    std::string username;
    std::string mode;
    std::string unused;
    std::string realname;
    std::string remaining;

    remaining = params;
    if (!get_param(remaining, username) || !get_param(remaining, mode)
    	|| !get_param(remaining, unused) || remaining.size() < 2 || remaining[0] != ':')
    {
        this->send_reply(client_fd, ":" + this->_name + " 461 * USER :Not enough parameters\r\n");
        return;
    }
    realname = remaining.substr(1); // extract realname from remaining removing colon
    this->_clients[client_fd].set_user(username); // associate username with this client fd
    this->send_reply(client_fd, "USER set to: " + username + " | Mode: " + mode + " | Unused: " + unused + " | Realname: " + realname + "\r\n");
}

void Server::handle_ping(int client_fd, const std::string &params)
{
    if (params.empty())
	{
		return;
	}
	this->send_reply(client_fd, "\r\n");
    this->send_reply(client_fd, "PONG " + params + "\r\n");
}

void Server::handle_quit(int client_fd, const std::string &params)
{
    (void)params;
    this->disconnect_client(client_fd);
}
