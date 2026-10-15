/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Commands.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/05 20:08:43 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/15 17:44:16 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Server.hpp"
#include "../includes/Utils.hpp"

void Server::handle_pass(Client &client, const std::vector<std::string> &params)
{
	if (client.get_auth())
	{
		this->send_msg(client, ":" + this->_name + " NOTICE * :PASS already accepted\r\n", EPOLLIN | EPOLLOUT);
		return;
	}
	if (params.empty())
	{
		this->send_msg(client, ":" + this->_name + " 461 PASS :Not enough parameters\r\n", EPOLLIN | EPOLLOUT);
		return;
	}
	if (params.size() > 1)
	{
		this->send_msg(client, ":" + this->_name + " 461 PASS :Too many parameters\r\n", EPOLLIN | EPOLLOUT);
		return;
	}
	if (params[0] != this->_pass)
	{
		this->send_msg(client, ":" + this->_name + " 464 * :Password incorrect\r\n", EPOLLIN | EPOLLOUT);
		return;
	}
	client.set_auth(true);
	this->send_msg(client, ":" + this->_name + " NOTICE * :PASS accepted\r\n", EPOLLIN | EPOLLOUT);
}

void Server::handle_nick(Client &client, const std::vector<std::string> &params)
{
	std::map<int, Client>::iterator it;
	std::string nick;
	int client_fd;

	client_fd = client.get_client_fd();
	if (!client.get_auth())
	{
		this->send_msg(client, ":" + this->_name + " NOTICE * :PASS must be accepted before NICK\r\n", EPOLLIN | EPOLLOUT);
		return;
	}
	if (params.empty())
	{
		this->send_msg(client, ":" + this->_name + " 431 * :No nickname given\r\n", EPOLLIN | EPOLLOUT);
		return;
	}
	nick = params[0];
	if (!is_valid_nick(nick))
	{
		this->send_msg(client, ":" + this->_name + " 432 * " + nick + " :Erroneous nickname\r\n", EPOLLIN | EPOLLOUT);
		return;
	}
	if (same_nick(client.get_nick(), nick))
	{
		this->send_msg(client, ":" + this->_name + " NOTICE " + client.get_nick() + " :You are already using this nickname\r\n", EPOLLIN | EPOLLOUT);
		return;
	}
	for (it = this->_clients.begin(); it != this->_clients.end(); ++it)
	{
		if (it->first != client_fd && same_nick(it->second.get_nick(), nick))
		{
			this->send_msg(client, ":" + this->_name + " 433 * " + nick + " :Nickname is already in use\r\n", EPOLLIN | EPOLLOUT);
			return;
		}
	}
	client.set_nick(nick);
	this->send_msg(client, ":" + this->_name + " NOTICE * :Set nickname to " + nick + "\r\n", EPOLLIN | EPOLLOUT);
}

void Server::handle_user(Client &client, const std::vector<std::string> &params)
{
	std::string username;
	std::string mode;
	std::string unused;
	std::string realname;

	if (client.get_nick().empty())
	{
		this->send_msg(client, ":" + this->_name + " NOTICE * :NICK must be set before USER\r\n", EPOLLIN | EPOLLOUT);
		return;
	}
	if (params.size() < 4)
	{
		this->send_msg(client, ":" + this->_name + " 461 USER :Not enough parameters\r\n", EPOLLIN | EPOLLOUT);
		return;
	}
	username = params[0];
	mode = params[1];
	unused = params[2];
	realname = params[3];
	if (!is_valid_user_mode(mode))
	{
		this->send_msg(client, ":" + this->_name + " NOTICE " + client.get_nick() + " :Invalid USER mode: only numbers are allowed\r\n", EPOLLIN | EPOLLOUT);
		return;
	}
	client.set_user(username);
	this->send_msg(client, ":" + this->_name + " NOTICE * :Set username to " + username + "\r\n", EPOLLIN | EPOLLOUT);
}

void Server::handle_privmsg(Client &client, const std::vector<std::string> &params)
{
	std::map<int, Client>::iterator it;
	std::string target;
	std::string msg;
	std::string prefix;

	if (params.empty())
	{
		this->send_msg(client, ":" + this->_name + " 411 " + client.get_nick() + " :No recipient given (PRIVMSG)\r\n", EPOLLIN | EPOLLOUT);
		return;
	}
	if (params.size() < 2 || params[1].empty())
	{
		this->send_msg(client, ":" + this->_name + " 412 " + client.get_nick() + " :No text to send\r\n", EPOLLIN | EPOLLOUT);
		return;
	}
	target = params[0];
	msg = params[1];
	for (it = this->_clients.begin(); it != this->_clients.end(); ++it)
	{
		if (same_nick(it->second.get_nick(), target))
		{
			prefix = ":" + client.get_nick() + "!" + client.get_user() + "@" + client.get_host_ip();
			this->send_msg(it->second, prefix + " PRIVMSG " + it->second.get_nick() + " :" + msg + "\r\n", EPOLLIN | EPOLLOUT);
			return;
		}
	}
	this->send_msg(client, ":" + this->_name + " 401 " + client.get_nick() + " " + target + " :No such nick/channel\r\n", EPOLLIN | EPOLLOUT);
}

// ping is used to check if connection between client and server is still active.
// receiver must respond with pong containing corresponding param
void Server::handle_ping(Client &client, const std::vector<std::string> &params)
{
	std::string token;

	if (params.empty() || params[0].empty())
	{
		this->send_msg(client, ":" + this->_name + " 409 * :No origin specified\r\n", EPOLLIN | EPOLLOUT);
		return;
	}
	token = params[0];
	this->send_msg(client, ":" + this->_name + " PONG " + this->_name + " :" + token + "\r\n", EPOLLIN | EPOLLOUT);
}

void Server::handle_quit(Client &client, const std::vector<std::string> &params)
{
	std::map<int, Client>::iterator it;
	std::string reason;
	std::string msg;
	std::string client_name;
	int client_fd;

	client_fd = client.get_client_fd();
	if (params.empty() || params[0].empty())
		reason = "Client quit";
	else
		reason = params[0];
	if (client.get_nick().empty())
		client_name = "unregistered client";
	else
		client_name = client.get_nick();
	msg = ":" + client.get_nick() + "!" + client.get_user() + "@" + client.get_host_ip() + " QUIT :" + reason + "\r\n";
	for (it = this->_clients.begin(); it != this->_clients.end(); ++it)
	{
		if (it->first == client_fd)
			continue;
		this->send_msg(it->second, msg, EPOLLIN | EPOLLOUT);
	}
	this->send_msg(client, "ERROR :Closing connection for " + client_name + ": " + reason + "\r\n", EPOLLIN | EPOLLOUT);
	client.set_quit(true);
}

void Server::handle_join(Client &client, const std::vector<std::string> &params)
{
	std::map<std::string, Channel>::iterator it;
	std::vector<std::string> c_names;
	std::vector<std::string> keys;
	std::string chan_name;
	std::string key;
	size_t i;
	int ret;

	if (params.empty())
	{
		this->send_msg(client, ":" + this->_name + " XXX * :Need more params\r\n", EPOLLIN | EPOLLOUT);
		return;
	}
	if (params.size() > 2)
	{
		this->send_msg(client, ":" + this->_name + " XXX * :Too many params\r\n", EPOLLIN | EPOLLOUT);
		return;
	}
	c_names = ft_split(params[0], ',');
	if (params.size() > 1)
		keys = ft_split(params[1], ',');
	i = 0;
	while (i < c_names.size())
	{
		chan_name = c_names[i];
		if (chan_name.empty() || (chan_name[0] != '#' && chan_name[0] != '&'))
		{
			this->send_msg(client, "You fucked up homie, channel can only start with # or &\r\n", EPOLLIN | EPOLLOUT);
			i++;
			continue;
		}
		if (i < keys.size())
			key = keys[i];
		else
			key = "";
		it = this->_channels.find(chan_name);
		if (it == this->_channels.end())
		{
			Channel ch(chan_name, default_mode(), client);
			this->_channels.insert(std::pair<std::string, Channel>(chan_name, ch));
			this->send_msg(client, "You created Channel " + chan_name + "\r\n", EPOLLIN | EPOLLOUT);
		}
		else
		{
			ret = it->second.joinChannel(client, key);
			if (ret)
			{
				switch (ret)
				{
					case 1:
						this->send_msg(client, "NOTICE: User is already in Channel\r\n", EPOLLIN | EPOLLOUT);
						break;
					case 2:
						this->send_msg(client, ":" + this->getName() + " XXX * :" + chan_name + " This place is full, get yo ass outta here\r\n", EPOLLIN | EPOLLOUT);
						break;
					case 3:
						this->send_msg(client, ":" + this->getName() + " 473 * :" + chan_name + " You weren't invited to the boys club. Access denied\r\n", EPOLLIN | EPOLLOUT);
						break;
					case 4:
						this->send_msg(client, ":" + this->getName() + " XXX * :" + chan_name + " What's the pass? This is boys only club\r\n", EPOLLIN | EPOLLOUT);
						break;
				}
			}
			else
			{
				this->send_msg(client, "You joined " + chan_name + "\r\n", EPOLLIN | EPOLLOUT);
			}
		}
		i++;
	}
}

void Server::handle_fast(Client &client)
{
	std::string nick;
	std::string user;
	std::string host;

	nick = generateRandomString(9);
	user = generateRandomString(9);
	host = generateRandomString(9);
	client.set_auth(true);
	client.set_nick(nick);
	client.set_user(user);
	client.set_host_ip(host);
	if (client.can_register())
		client.set_registered(true);
	this->send_msg(client, "You shall be named: " + nick + "\n", EPOLLIN | EPOLLOUT);
	this->send_msg(client, "Your home kingdom shall be: " + host + "\n", EPOLLIN | EPOLLOUT);
	this->send_msg(client, "Thy user shall be sung as: " + user +  "\n", EPOLLIN | EPOLLOUT);
	this->send_msg(client, "Go forth, make haste, child of Mercury\r\n", EPOLLIN | EPOLLOUT);
	this->send_msg(client, registration_reply(client, this->_name, this->_date), EPOLLIN | EPOLLOUT);
}
