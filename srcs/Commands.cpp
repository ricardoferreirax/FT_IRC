/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Commands.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/05 20:08:43 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/11 18:26:34 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Server.hpp"
#include "../includes/Utils.hpp"

void Server::handle_pass(Client &client, const std::string &params)
{
	std::string	&send_buffer = client.get_send_buffer();
	int	client_fd;

	client_fd = client.get_client_fd();
	if (client.get_auth())
	{
		send_buffer += ":" + this->_name + " NOTICE * :Password already validated\r\n";
		this->update_epoll(client_fd, EPOLLIN | EPOLLOUT);
		return;
	}
	if (params.empty())
	{
		send_buffer += ":" + this->_name + " 461 PASS :Not enough parameters\r\n";
		this->update_epoll(client_fd, EPOLLIN | EPOLLOUT);
		return;
	}
	if (params.find(' ') != std::string::npos)
	{
		send_buffer += ":" + this->_name + " NOTICE * :PASS accepts only one parameter\r\n";
		this->update_epoll(client_fd, EPOLLIN | EPOLLOUT);
		return;
	}
	if (params != this->_pass)
	{
		send_buffer += ":" + this->_name + " 464 * :Password incorrect\r\n";
		this->update_epoll(client_fd, EPOLLIN | EPOLLOUT);
		return;
	}
	client.set_auth(true);
	send_buffer += ":" + this->_name + " NOTICE * :Password accepted\r\n";
	this->update_epoll(client_fd, EPOLLIN | EPOLLOUT);
}

void Server::handle_nick(Client &client, const std::string &params)
{
	std::map<int, Client>::iterator	it;
	std::string	&send_buffer = client.get_send_buffer();
	std::string	remaining;
	std::string	nick;
	int	client_fd;

	client_fd = client.get_client_fd();
	if (!client.get_auth())
	{
		send_buffer += ":" + this->_name + " NOTICE * :PASS must be accepted before NICK\r\n";
		this->update_epoll(client_fd, EPOLLIN | EPOLLOUT);
		return;
	}
	remaining = params;
	if (!get_param(remaining, nick))
	{
		send_buffer += ":" + this->_name + " 431 * :No nickname given\r\n";
		this->update_epoll(client_fd, EPOLLIN | EPOLLOUT);
		return;
	}
	if (!is_valid_nick(nick))
	{
		send_buffer += ":" + this->_name + " 432 * " + nick + " :Erroneous nickname\r\n";
		this->update_epoll(client_fd, EPOLLIN | EPOLLOUT);
		return;
	}
	for (it = this->_clients.begin(); it != this->_clients.end(); ++it)
	{
		if (it->first != client_fd && same_nick(it->second.get_nick(), nick))
		{
			send_buffer += ":" + this->_name + " 433 * " + nick + " :Nickname is already in use\r\n";
			this->update_epoll(client_fd, EPOLLIN | EPOLLOUT);
			return;
		}
	}
	client.set_nick(nick);
	send_buffer += ":" + this->_name + " NOTICE * :Nickname set to " + nick + "\r\n";
	this->update_epoll(client_fd, EPOLLIN | EPOLLOUT);
}

void Server::handle_user(Client &client, const std::string &params)
{
	std::string	&send_buffer = client.get_send_buffer();
	std::string	remaining;
	std::string	username;
	std::string	mode;
	std::string	unused;
	std::string	realname;
	int	client_fd;

	client_fd = client.get_client_fd();
	if (client.get_nick().empty())
	{
		send_buffer += ":" + this->_name + " NOTICE * :NICK must be set before USER\r\n";
		this->update_epoll(client_fd, EPOLLIN | EPOLLOUT);
		return;
	}
	remaining = params;
	if (!get_param(remaining, username) || !get_param(remaining, mode) || !get_param(remaining, unused)
		|| remaining.size() < 2 || remaining[0] != ':')
	{
		send_buffer += ":" + this->_name + " 461 USER :Not enough parameters\r\n";
		this->update_epoll(client_fd, EPOLLIN | EPOLLOUT);
		return;
	}
	realname = remaining.substr(1);
	client.set_user(username);
	send_buffer += ":" + this->_name + " NOTICE * :Set username to " + username + "\r\n";
	this->update_epoll(client_fd, EPOLLIN | EPOLLOUT);
}

void Server::handle_privmsg(Client &client, const std::string &params)
{
	std::map<int, Client>::iterator	it;
	std::string	&send_buffer = client.get_send_buffer();
	std::string	remaining;
	std::string	target;
	std::string	prefix;
	int	client_fd;

	client_fd = client.get_client_fd();
	remaining = params;
	prefix = ":" + client.get_nick() + "!" + client.get_user() + "@" + client.get_host_ip();
	if (!get_param(remaining, target))
	{
		send_buffer += ":" + this->_name + " 411 * :No recipient given (PRIVMSG)\r\n";
		this->update_epoll(client_fd, EPOLLIN | EPOLLOUT);
		return;
	}
	if (remaining.empty() || remaining[0] != ':')
	{
		send_buffer += ":" + this->_name + " 412 * :No text to send\r\n";
		this->update_epoll(client_fd, EPOLLIN | EPOLLOUT);
		return;
	}
	for (it = this->_clients.begin(); it != this->_clients.end(); ++it)
	{
		if (same_nick(it->second.get_nick(), target))
		{
			it->second.get_send_buffer() += prefix + " PRIVMSG " + target + " " + remaining + "\r\n";
			this->update_epoll(it->first, EPOLLIN | EPOLLOUT);
			return;
		}
	}
	send_buffer += ":" + this->_name + " 401 * " + target + " :No such nick/channel\r\n";
	this->update_epoll(client_fd, EPOLLIN | EPOLLOUT);
}

void Server::handle_ping(int client_fd, const std::string &params)
{
	std::string	&send_buffer = this->_clients[client_fd].get_send_buffer();

	if (params.empty())
		return;
	send_buffer += ":" + this->_name + " NOTICE * :PONG " + params + "\r\n";
	this->update_epoll(client_fd, EPOLLIN | EPOLLOUT);
}

void Server::handle_quit(int client_fd, const std::string &params)
{
	(void)params;
	this->disconnect_client(client_fd);
}
