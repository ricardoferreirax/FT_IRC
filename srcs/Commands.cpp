/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Commands.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/05 20:08:43 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/12 19:15:13 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Server.hpp"
#include "../includes/Utils.hpp"
#include "../includes/Channels.hpp"

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
	std::string	&send_buffer = client.get_send_buffer();
	std::string	remaining;
	std::string	target;
	std::string	prefix;
	int client_fd;

	client_fd = client.get_client_fd();
	remaining = params;
	send_buffer += "\n";
	prefix = ":" + client.get_nick() + "!" + client.get_user() + "@" + client.get_host_ip();
	send_buffer += "\n";
	if (!get_param(remaining, target)) // no target was provided
	{
		send_buffer += ":" + this->_name + " 411 * :No recipient given (PRIVMSG)\r\n";
		this->update_epoll(client_fd, EPOLLIN | EPOLLOUT);
		return;
	}
	if (remaining.empty()) // no msg was provided
	{
		send_buffer += ":" + this->_name + " 412 * :No text to send\r\n";
		send_buffer += "\n";
		this->update_epoll(client_fd, EPOLLIN | EPOLLOUT);
		return;
	}
	if (remaining[0] == ':')
		remaining.erase(0, 1); // remove ':' that marks trailing param
	if (remaining.empty()) // ':' was provided but msg is empty
	{
		send_buffer += ":" + this->_name + " 412 * :No text to send\r\n";
		send_buffer += "\n";
		this->update_epoll(client_fd, EPOLLIN | EPOLLOUT);
		return;
	}
	for (it = this->_clients.begin(); it != this->_clients.end(); ++it) // iterate all clients to find target client to send msg
	{
		if (same_nick(it->second.get_nick(), target)) // if target nick matches client nick -> send msg to target client
		{
			send_buffer += "\n";
			it->second.get_send_buffer() += prefix + " PRIVMSG " + target + " :" + remaining + "\r\n";
			send_buffer += "\n";
			this->update_epoll(it->first, EPOLLIN | EPOLLOUT);
			return;
		}
	}
	send_buffer += ":" + this->_name + " 401 * " + target + " :No such nick/channel\r\n";
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
		this->update_epoll(client_fd, EPOLLIN | EPOLLOUT);
		return;
	}
	if (token[0] == ':')
		token.erase(0, 1); // remove ':' from param
	if (token.empty()) // ':' was provided but param is empty
	{
		send_buffer += ":" + this->_name + " 409 * :No origin specified\r\n";
		this->update_epoll(client_fd, EPOLLIN | EPOLLOUT);
		return;
	}
	send_buffer += "\n";
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


void Server::handle_join(Client &client, const std::string &params)
{
	size_t	i;

	std::string &to_send = client.get_send_buffer();
	if (params.empty())
	{
		to_send += ":" + this->_name + " XXX *: Need more params\r\n";
		update_epoll(client.get_client_fd(), EPOLLIN | EPOLLOUT);
		return ;
	}
	std::vector<std::string> arr2 = ft_split(params, ' ');
	if (arr2.size() > 2)
	{
		to_send += ":" + this->_name + " XXX *: Too many params\r\n";
		update_epoll(client.get_client_fd(), EPOLLIN | EPOLLOUT);
		return ;
	}
	std::vector<std::string> c_names = ft_split(arr2[0], ',');
	std::vector<std::string> keys;
	if (arr2.size() > 1)
		keys = ft_split(arr2[1], ',');
	i = -1;
	while (++i < c_names.size())
	{
		std::string chan_name = c_names[i];
		if(chan_name[0] != '#' && chan_name[0] != '&')
		{
			to_send += "You fucked up homie, channel can only start with # or &\r\n";
			this->update_epoll(client.get_client_fd(), EPOLLIN | EPOLLOUT);
			continue;
		}
		std::string key;
		if (i < keys.size())
			key = keys[i];
		else
			key = "";
		std::map<std::string,
			Channel>::iterator it = this->_channels.find(chan_name);
		if (it == this->_channels.end())
		{
			Channel ch(chan_name, default_mode(), client);
			this->_channels.insert(std::pair<std::string, Channel>(chan_name,
					ch));
			to_send += "You created Channel " + chan_name + "\r\n";
			this->update_epoll(client.get_client_fd(), EPOLLIN | EPOLLOUT);
		}
		else
		{
			int ret = it->second.joinChannel(client, key);
			if (ret)
			{
				switch (ret)
				{
					case 1:
						to_send += "NOTICE: User is already in Channel\r\n";
						break;
					case 2:
						to_send += ":" + this->getName() + " XXX *: #" + chan_name + " This place is full, get yo ass outta here\r\n";
						break;
					case 3:
						to_send += ":" + this->getName() + " 473 *: #" + chan_name + " You weren't invited to the boys club. Access denied\r\n";
						break;
					case 4:
						to_send = ":" + this->getName() + " XXX *: #" + chan_name + " What's the pass? This is boys only club\r\n";
						break;
					this->update_epoll(client.get_client_fd(), EPOLLIN | EPOLLOUT);
				}
			}
			else
			{
				to_send += "You joined #" + chan_name + " \r\n";
				this->update_epoll(client.get_client_fd(), EPOLLIN | EPOLLOUT);
			}
		}
	}
}

void Server::handle_fast(Client &client)
{
	client.set_auth(true);
	client.set_nick(generateRandomString(9));
	client.set_user(generateRandomString(9));
	client.set_registered(true);
	client.set_host_ip(generateRandomString(9));
	std::string &to_send = client.get_send_buffer();
	to_send += "You shall be named: " + client.get_nick() + "\n";
	to_send += "Your home kingdom shall be: " + client.get_host_ip() + "\n";
	to_send += "Thy user shall be sung as: " + client.get_user() +  "\n";
	to_send += "Go forth, make haste, child of Mercury\r\n";
	this->update_epoll(client.get_client_fd(), EPOLLIN | EPOLLOUT);
}
