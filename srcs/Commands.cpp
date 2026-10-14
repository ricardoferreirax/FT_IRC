/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Commands.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/05 20:08:43 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/14 15:41:19 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Server.hpp"
#include "../includes/Utils.hpp"

void Server::handle_pass(Client &client, const std::vector<std::string> &params)
{
	std::string &to_send = client.get_send_buffer();
	int client_fd;

	client_fd = client.get_client_fd();
	if (client.get_auth()) // if client validated pass no need to validate again
	{
		to_send += ":" + this->_name + " NOTICE * :Password already validated\r\n";
		this->update_epoll(client_fd, EPOLLIN | EPOLLOUT);
		return;
	}
	if (params.empty())
	{
		to_send += ":" + this->_name + " 461 PASS :Not enough parameters\r\n";
		this->update_epoll(client_fd, EPOLLIN | EPOLLOUT);
		return;
	}
	if (params.size() > 1) // pass accepts only one param
	{
		to_send += ":" + this->_name + " NOTICE * :PASS accepts only one parameter\r\n";
		this->update_epoll(client_fd, EPOLLIN | EPOLLOUT);
		return;
	}
	if (params[0] != this->_pass) // pass must match server pass
	{
		to_send += ":" + this->_name + " 464 * :Password incorrect\r\n";
		this->update_epoll(client_fd, EPOLLIN | EPOLLOUT);
		return;
	}
	client.set_auth(true); // mark client with a validated pass
	to_send += ":" + this->_name + " NOTICE * :Password accepted\r\n";
	to_send += "\n";
	this->update_epoll(client_fd, EPOLLIN | EPOLLOUT);
}

void Server::handle_nick(Client &client, const std::vector<std::string> &params)
{
	std::map<int, Client>::iterator it;
	std::string &to_send = client.get_send_buffer();
	std::string nick;
	int client_fd;

	client_fd = client.get_client_fd();
	if (!client.get_auth()) // must validate pass before setting nick
	{
		to_send += ":" + this->_name + " NOTICE * :PASS must be accepted before NICK\r\n";
		this->update_epoll(client_fd, EPOLLIN | EPOLLOUT);
		return;
	}
	if (params.empty())
	{
		to_send += ":" + this->_name + " 431 * :No nickname given\r\n";
		this->update_epoll(client_fd, EPOLLIN | EPOLLOUT);
		return;
	}
	nick = params[0]; // get requested nick
	if (!is_valid_nick(nick))
	{
		to_send += ":" + this->_name + " 432 * " + nick + " :Erroneous nickname\r\n";
		this->update_epoll(client_fd, EPOLLIN | EPOLLOUT);
		return;
	}
	if (same_nick(client.get_nick(), nick)) // client is using this nick -> no need to set it again
	{
		to_send += ":" + this->_name + " NOTICE " + client.get_nick() + " :You are already using this nickname\r\n";
		this->update_epoll(client_fd, EPOLLIN | EPOLLOUT);
		return;
	}
	for (it = this->_clients.begin(); it != this->_clients.end(); ++it)
	{
		if (it->first != client_fd && same_nick(it->second.get_nick(), nick)) // check if any other client is using this nick
		{
			to_send += ":" + this->_name + " 433 * " + nick + " :Nickname is already in use\r\n";
			this->update_epoll(client_fd, EPOLLIN | EPOLLOUT);
			return;
		}
	}
	client.set_nick(nick);
	to_send += ":" + this->_name + " NOTICE * :Nickname set to " + nick + "\r\n";
	to_send += "\n";
	this->update_epoll(client_fd, EPOLLIN | EPOLLOUT);
}

void Server::handle_user(Client &client, const std::vector<std::string> &params)
{
	std::string &to_send = client.get_send_buffer();
	std::string username;
	std::string mode;
	std::string unused;
	std::string realname;
	int client_fd;

	client_fd = client.get_client_fd();
	if (client.get_nick().empty()) // nick must be set before user
	{
		to_send += ":" + this->_name + " NOTICE * :NICK must be set before USER\r\n";
		this->update_epoll(client_fd, EPOLLIN | EPOLLOUT);
		return;
	}
	if (params.size() < 4) // user requires 4 params
	{
		to_send += ":" + this->_name + " 461 USER :Not enough parameters\r\n";
		this->update_epoll(client_fd, EPOLLIN | EPOLLOUT);
		return;
	}
	username = params[0];
	mode = params[1];
	unused = params[2];
	realname = params[3];
	if (!is_valid_user_mode(mode)) // user mode must be a number
	{
		to_send += ":" + this->_name + " NOTICE " + client.get_nick() + " :Invalid USER mode: only numbers are allowed\r\n";
		this->update_epoll(client_fd, EPOLLIN | EPOLLOUT);
		return;
	}
	client.set_user(username);
	to_send += ":" + this->_name + " NOTICE * :Set username to " + username + "\r\n";
	to_send += "\n";
	this->update_epoll(client_fd, EPOLLIN | EPOLLOUT);
}

void Server::handle_privmsg(Client &client, const std::vector<std::string> &params)
{
	std::map<int, Client>::iterator it;
	std::string &to_send = client.get_send_buffer();
	std::string target;
	std::string message;
	std::string prefix;
	int client_fd;

	client_fd = client.get_client_fd();
	if (params.empty()) // no params -> no target and no message
	{
		to_send += ":" + this->_name + " 411 " + client.get_nick() + " :No recipient given (PRIVMSG)\r\n";
		this->update_epoll(client_fd, EPOLLIN | EPOLLOUT);
		return;
	}
	if (params.size() < 2 || params[1].empty()) // only one param or second param is empty -> no message to send
	{
		to_send += ":" + this->_name + " 412 " + client.get_nick() + " :No text to send\r\n";
		this->update_epoll(client_fd, EPOLLIN | EPOLLOUT);
		return;
	}
	target = params[0];
	message = params[1];
	for (it = this->_clients.begin(); it != this->_clients.end(); ++it)
	{
		if (same_nick(it->second.get_nick(), target)) // check if any client is using the target nick
		{
			prefix = ":" + client.get_nick() + "!" + client.get_user() + "@" + client.get_host_ip();
			it->second.get_send_buffer() += prefix + " PRIVMSG " + it->second.get_nick() + " :" + message + "\r\n" + "\n";
			this->update_epoll(it->first, EPOLLIN | EPOLLOUT);
			return;
		}
	}
	to_send += ":" + this->_name + " 401 " + client.get_nick() + " " + target + " :No such nick/channel\r\n";
	this->update_epoll(client_fd, EPOLLIN | EPOLLOUT);
}

// ping is used to check if connection between client and server is still active.
// receiver must respond with pong containing corresponding param
void Server::handle_ping(Client &client, const std::vector<std::string> &params)
{
	std::string &to_send = client.get_send_buffer();
	std::string token;
	int client_fd;

	client_fd = client.get_client_fd();
	if (params.empty() || params[0].empty()) // no params -> no token to send back in pong
	{
		to_send += ":" + this->_name + " 409 * :No origin specified\r\n";
		this->update_epoll(client_fd, EPOLLIN | EPOLLOUT);
		return;
	}
	token = params[0];
	to_send += ":" + this->_name + " PONG " + this->_name + " :" + token + "\r\n";
	to_send += "\n";
	this->update_epoll(client_fd, EPOLLIN | EPOLLOUT);
}

void Server::handle_quit(Client &client, const std::vector<std::string> &params)
{
	std::map<int, Client>::iterator it;
	std::string &send_buffer = client.get_send_buffer();
	std::string reason;
	std::string message;
	std::string client_name;
	int client_fd;

	client_fd = client.get_client_fd();
	if (params.empty() || params[0].empty()) 
		reason = client.get_nick(); // if no reason is provided -> use nick as reason
	else
		reason = params[0]; // otherwise use provided reason
	if (client.get_nick().empty())
		client_name = "unregistered client"; // if client has no nick set
	else
		client_name = client.get_nick();
	message = ":" + client.get_nick() + "!" + client.get_user() + "@" + client.get_host_ip() + " QUIT :" + reason + "\r\n"; // msg for other clients
	for (it = this->_clients.begin(); it != this->_clients.end(); ++it) // notify all other clients about client quitting
	{
		if (it->first == client_fd) // dont notify leaving client about its own quit
			continue;
		it->second.get_send_buffer() += message; // add quit msg to other client's send buffer
		this->update_epoll(it->first, EPOLLIN | EPOLLOUT); // enable EPOLLOUT so msg can be sent
	}
	send_buffer += "Closing connection for " + client_name + ": " + reason + "\r\n";
	client.set_quit(true); // disconnect client
	this->update_epoll(client_fd, EPOLLIN | EPOLLOUT); // enable EPOLLOUT to send final msg
}
