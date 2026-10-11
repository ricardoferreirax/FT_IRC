/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Commands.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/05 20:08:43 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/11 22:29:39 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Server.hpp"
#include "../includes/Utils.hpp"

void Server::handle_pass(Client &client, const std::string &params)
{
	std::string	&send_buffer = client.get_send_buffer();
	int	client_fd;

	client_fd = client.get_client_fd();
	if (client.get_auth()) // if pass was validated it cannot be set again
	{
		send_buffer += ":" + this->_name + " NOTICE * :Password already validated\r\n";
		this->update_epoll(client_fd, EPOLLIN | EPOLLOUT);
		return;
	}
	if (params.empty()) // no pass was provided
	{
		send_buffer += ":" + this->_name + " 461 PASS :Not enough parameters\r\n";
		this->update_epoll(client_fd, EPOLLIN | EPOLLOUT);
		return;
	}
	if (params.find(' ') != std::string::npos) // if pass contains spaces is invalid
	{
		send_buffer += ":" + this->_name + " NOTICE * :PASS accepts only one parameter\r\n";
		this->update_epoll(client_fd, EPOLLIN | EPOLLOUT);
		return;
	}
	if (params != this->_pass) // pass does not match server password
	{
		send_buffer += ":" + this->_name + " 464 * :Password incorrect\r\n";
		this->update_epoll(client_fd, EPOLLIN | EPOLLOUT);
		return;
	}
	client.set_auth(true); // set client as authenticated
	send_buffer += ":" + this->_name + " NOTICE * :Password accepted\r\n";
	this->update_epoll(client_fd, EPOLLIN | EPOLLOUT); // update epoll to monitor client_fd for both read and write events
}

void Server::handle_nick(Client &client, const std::string &params)
{
	std::map<int, Client>::iterator	it;
	std::string	&send_buffer = client.get_send_buffer();
	std::string	remaining;
	std::string	nick;
	int	client_fd;

	client_fd = client.get_client_fd();
	if (!client.get_auth()) // if pass was not validated it cannot set nick
	{
		send_buffer += ":" + this->_name + " NOTICE * :PASS must be accepted before NICK\r\n";
		this->update_epoll(client_fd, EPOLLIN | EPOLLOUT);
		return;
	}
	remaining = params; // copy params to remaining to extract nick from it
	if (!get_param(remaining, nick)) // no nick was provided
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
	for (it = this->_clients.begin(); it != this->_clients.end(); ++it) // iterate all clients to check if nick is already in use
	{
		if (it->first != client_fd && same_nick(it->second.get_nick(), nick)) // nick is in use by another client
		{
			send_buffer += ":" + this->_name + " 433 * " + nick + " :Nickname is already in use\r\n";
			this->update_epoll(client_fd, EPOLLIN | EPOLLOUT);
			return;
		}
	}
	client.set_nick(nick); // set nick to client
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
	if (client.get_nick().empty()) // if nick was not set it cannot set user
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
	realname = remaining.substr(1); // remove ':' from realname
	client.set_user(username); // set username to client
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
	if (!get_param(remaining, target)) // no target was provided
	{
		send_buffer += ":" + this->_name + " 411 * :No recipient given (PRIVMSG)\r\n";
		this->update_epoll(client_fd, EPOLLIN | EPOLLOUT);
		return;
	}
	if (remaining.empty() || remaining[0] != ':') // no msg was provided or msg does not start with ':'
	{
		send_buffer += ":" + this->_name + " 412 * :No text to send\r\n";
		this->update_epoll(client_fd, EPOLLIN | EPOLLOUT);
		return;
	}
	for (it = this->_clients.begin(); it != this->_clients.end(); ++it) // iterate all clients to find target client to send msg
	{
		if (same_nick(it->second.get_nick(), target)) // if target nick matches client nick -> send msg to target client
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

	if (epoll_ctl(this->_epoll_fd, EPOLL_CTL_DEL, client_fd, NULL) < 0)
		throw std::runtime_error("IRC: epoll_ctl() failed.");
	close(client_fd);
	this->_clients.erase(client_fd);
	std::cout << "\n=====================================================" << std::endl;
	std::cout << "          CLIENT " << client_fd << " DISCONNECTED!" << std::endl;
	std::cout << "=====================================================" << std::endl;
}
