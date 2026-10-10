/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Commands.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/05 20:08:43 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/10 14:17:56 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Server.hpp"
#include "../includes/Utils.hpp"

void Server::handle_pass(int client_fd, const std::string &params)
{
	if (params.empty())
	{
		this->send_reply(client_fd, ":" + this->_name + " 461 PASS :Not enough parameters\r\n");
		return;
	}
	if (params.find(' ') != std::string::npos)
	{
		this->send_reply(client_fd, ":" + this->_name + " NOTICE * :PASS accepts only one parameter\r\n");
		return;
	}
	if (params != this->_pass)
	{
		this->send_reply(client_fd, ":" + this->_name + " 464 * :Password incorrect\r\n");
		return;
	}
	this->_clients[client_fd].set_auth(true);
	this->send_reply(client_fd, ":" + this->_name + " NOTICE * :Password accepted\r\n");
}

void Server::handle_nick(int client_fd, const std::string &params)
{
	std::map<int, Client>::iterator	it;
	std::string						remaining;
	std::string						nick;

	if (!this->_clients[client_fd].get_auth())
	{
		this->send_reply(client_fd, ":" + this->_name + " NOTICE * :PASS must be accepted before NICK\r\n");
		return;
	}
	remaining = params;
	if (!get_param(remaining, nick))
	{
		this->send_reply(client_fd, ":" + this->_name + " 431 * :No nickname given\r\n");
		return;
	}
	if (!is_valid_nick(nick))
	{
		this->send_reply(client_fd, ":" + this->_name + " 432 * " + nick + " :Erroneous nickname\r\n");
		return;
	}
	for (it = this->_clients.begin(); it != this->_clients.end(); ++it)
	{
		if (it->first != client_fd && same_nick(it->second.get_nick(), nick)) // check if nick is already in use by another client
		{
			this->send_reply(client_fd, ":" + this->_name + " 433 * " + nick + " :Nickname is already in use\r\n");
			return;
		}
	}
	this->_clients[client_fd].set_nick(nick);
	this->send_reply(client_fd, ":" + this->_name + " NOTICE * :Nickname set to " + nick + "\r\n");
}

void Server::handle_user(int client_fd, const std::string &params)
{
	std::string	remaining;
	std::string	username;
	std::string	mode;
	std::string	unused;
	std::string	realname;

	if (this->_clients[client_fd].get_nick().empty())
	{
		this->send_reply(client_fd, ":" + this->_name + " NOTICE * :NICK must be set before USER\r\n");
		return;
	}
	remaining = params;
	if (!get_param(remaining, username) || !get_param(remaining, mode) || !get_param(remaining, unused)
		|| remaining.size() < 2 || remaining[0] != ':')
	{
		this->send_reply(client_fd, ":" + this->_name + " 461 USER :Not enough parameters\r\n");
		return;
	}
	realname = remaining.substr(1);
	this->_clients[client_fd].set_user(username);
	this->send_reply(client_fd, ":" + this->_name + " NOTICE * :Set username to " + username + "\r\n");
}

void Server::handle_ping(int client_fd, const std::string &params)
{
    if (params.empty())
	{
		return;
	}
	this->send_reply(client_fd, "\r\n");
    this->send_reply(client_fd, ":" + this->_name + " NOTICE * :PONG " + params + "\r\n");
}

void Server::handle_quit(int client_fd, const std::string &params)
{
    (void)params;
    this->disconnect_client(client_fd);
}
