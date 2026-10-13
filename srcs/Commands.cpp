/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Commands.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/05 20:08:43 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/13 14:46:27 by rmedeiro         ###   ########.fr       */
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
		send_buffer += "\n";
		this->update_epoll(client_fd, EPOLLIN | EPOLLOUT);
		return;
	}
	if (params.empty()) // no pass was provided
	{
		send_buffer += ":" + this->_name + " 461 PASS :Not enough parameters\r\n";
		send_buffer += "\n";
		this->update_epoll(client_fd, EPOLLIN | EPOLLOUT);
		return;
	}
	if (params.find(' ') != std::string::npos) // if pass contains spaces is invalid
	{
		send_buffer += ":" + this->_name + " NOTICE * :PASS accepts only one parameter\r\n";
		send_buffer += "\n";
		this->update_epoll(client_fd, EPOLLIN | EPOLLOUT);
		return;
	}
	if (params != this->_pass) // pass does not match server password
	{
		send_buffer += ":" + this->_name + " 464 * :Password incorrect\r\n";
		send_buffer += "\n";
		this->update_epoll(client_fd, EPOLLIN | EPOLLOUT);
		return;
	}
	client.set_auth(true); // set client as authenticated
	send_buffer += ":" + this->_name + " NOTICE * :Password accepted\r\n";
	send_buffer += "\n";
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
		send_buffer += "\n";
		this->update_epoll(client_fd, EPOLLIN | EPOLLOUT);
		return;
	}
	remaining = params; // copy params to remaining to extract nick from it
	if (!get_param(remaining, nick)) // no nick was provided
	{
		send_buffer += ":" + this->_name + " 431 * :No nickname given\r\n";
		send_buffer += "\n";
		this->update_epoll(client_fd, EPOLLIN | EPOLLOUT);
		return;
	}
	if (!is_valid_nick(nick))
	{
		send_buffer += ":" + this->_name + " 432 * " + nick + " :Erroneous nickname\r\n";
		send_buffer += "\n";
		this->update_epoll(client_fd, EPOLLIN | EPOLLOUT);
		return;
	}
	if (same_nick(client.get_nick(), nick)) // nickname already belongs to this client
	{
	    send_buffer += ":" + this->_name + " NOTICE " + client.get_nick() + " :You are already using this nickname\r\n";
		send_buffer += "\n";
	    this->update_epoll(client_fd, EPOLLIN | EPOLLOUT);
	    return;
	}
	for (it = this->_clients.begin(); it != this->_clients.end(); ++it) // iterate all clients to check if nick is already in use
	{
		if (it->first != client_fd && same_nick(it->second.get_nick(), nick)) // nick is in use by another client
		{
			send_buffer += ":" + this->_name + " 433 * " + nick + " :Nickname is already in use\r\n";
			send_buffer += "\n";
			this->update_epoll(client_fd, EPOLLIN | EPOLLOUT);
			return;
		}
	}
	client.set_nick(nick); // set nick to client
	send_buffer += ":" + this->_name + " NOTICE * :Nickname set to " + nick + "\r\n";
	send_buffer += "\n";
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
		send_buffer += "\n";
		this->update_epoll(client_fd, EPOLLIN | EPOLLOUT);
		return;
	}
	remaining = params;
	if (!get_param(remaining, username) || !get_param(remaining, mode) || !get_param(remaining, unused)
		|| remaining.size() < 2 || remaining[0] != ':')
	{
		send_buffer += ":" + this->_name + " 461 USER :Not enough parameters\r\n";
		send_buffer += "\n";
		this->update_epoll(client_fd, EPOLLIN | EPOLLOUT);
		return;
	}
	if (!is_valid_user_mode(mode)) // USER mode must contain only numbers
    {
        send_buffer += ":" + this->_name + " NOTICE " + client.get_nick() + " :Invalid USER mode: only numbers are allowed\r\n";
		send_buffer += "\n";
        this->update_epoll(client_fd, EPOLLIN | EPOLLOUT);
        return;
    }
	realname = remaining.substr(1); // remove ':' from realname
	client.set_user(username); // set username to client
	send_buffer += ":" + this->_name + " NOTICE * :Set username to " + username + "\r\n";
	send_buffer += "\n";
	this->update_epoll(client_fd, EPOLLIN | EPOLLOUT);
}

void Server::handle_privmsg(Client &client, const std::string &params)
{
	std::map<int, Client>::iterator	it;
	std::string						&send_buffer = client.get_send_buffer();
	std::string						remaining;
	std::string						target;
	std::string						prefix;
	int								client_fd;

	client_fd = client.get_client_fd();
	remaining = params;
	if (!get_param(remaining, target))
	{
		send_buffer += ":" + this->_name + " 411 " + client.get_nick() + " :No recipient given (PRIVMSG)\r\n";
		this->update_epoll(client_fd, EPOLLIN | EPOLLOUT);
		return;
	}
	if (remaining.empty())
	{
		send_buffer += ":" + this->_name + " 412 " + client.get_nick() + " :No text to send\r\n";
		this->update_epoll(client_fd, EPOLLIN | EPOLLOUT);
		return;
	}
	if (remaining[0] == ':')
		remaining.erase(0, 1);
	else if (remaining.find(' ') != std::string::npos)
		remaining = remaining.substr(0, remaining.find(' '));
	if (remaining.empty())
	{
		send_buffer += ":" + this->_name + " 412 " + client.get_nick() + " :No text to send\r\n";
		this->update_epoll(client_fd, EPOLLIN | EPOLLOUT);
		return;
	}
	for (it = this->_clients.begin(); it != this->_clients.end(); ++it)
	{
		if (same_nick(it->second.get_nick(), target))
		{
			prefix = ":" + client.get_nick() + "!" + client.get_user() + "@" + client.get_host_ip();
			it->second.get_send_buffer() += prefix + " PRIVMSG " + target + " :" + remaining + "\r\n";
			this->update_epoll(it->first, EPOLLIN | EPOLLOUT);
			return;
		}
	}
	send_buffer += ":" + this->_name + " 401 " + client.get_nick() + " " + target + " :No such nick/channel\r\n";
	this->update_epoll(client_fd, EPOLLIN | EPOLLOUT);
}

// ping is used to check if connection between client and server is still active.
// receiver must respond with pong containing corresponding param
void Server::handle_ping(Client &client, const std::string &params)
{
	std::string	&send_buffer = client.get_send_buffer();
	std::string	token;
	int	client_fd;

	client_fd = client.get_client_fd();
	token = params;
	if (token.empty()) // ping has no param
	{
		send_buffer += ":" + this->_name + " 409 * :No origin specified\r\n";
		send_buffer += "\n";
		this->update_epoll(client_fd, EPOLLIN | EPOLLOUT);
		return;
	}
	if (token[0] == ':')
		token.erase(0, 1); // remove ':' from param
	if (token.empty()) // ':' was provided but param is empty
	{
		send_buffer += ":" + this->_name + " 409 * :No origin specified\r\n";
		this->update_epoll(client_fd, EPOLLIN | EPOLLOUT);
		send_buffer += "\n";
		return;
	}
	send_buffer += ":" + this->_name + " PONG " + this->_name + " :" + token + "\r\n";
	send_buffer += "\n";
	this->update_epoll(client_fd, EPOLLIN | EPOLLOUT);
}

void Server::handle_quit(int client_fd, const std::string &params)
{
	Client		&client = this->_clients[client_fd];
	std::string	&send_buffer = client.get_send_buffer();
	std::string	reason;
	std::string	client_name;

	reason = params;
	if (!reason.empty() && reason[0] == ':')
		reason.erase(0, 1); // remove ':' from trailing param
	if (reason.empty())
		reason = "Client Quit"; // fallback for unregistered client
	if (client.get_nick().empty())
		client_name = "unregistered client"; // if client has no nick set -> use unregistered client as name
	else
		client_name = client.get_nick(); // if client has nick set -> use it as name
	send_buffer += "ERROR :Closing connection for " + client_name + ": " + reason + "\r\n";
	client.set_quit(true); // mark client for disconnection
	this->update_epoll(client_fd, EPOLLIN | EPOLLOUT);
}
