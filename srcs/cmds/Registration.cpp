/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Registration.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/08 21:09:17 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/11/08 21:09:48 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/Server.hpp"
#include "../../includes/Client.hpp"
#include "../../includes/Utils.hpp"

int Server::pass_cmd(Client &client, const std::vector<std::string> &params)
{
	if (client.get_registered() || client.get_auth())
		return (ERR_ALREADYREGISTRED);
	if (params.empty() || params[0].empty())
		return (ERR_NEEDMOREPARAMS);
	if (params[0] != this->_pass)
		return (ERR_PASSWDMISMATCH);
	client.set_auth(true);
	this->send_notice(client, PASSACCEPTED, "");
	return (0);
}

int Server::nick_cmd(Client &client, const std::vector<std::string> &params)
{
	std::map<std::string, Channel>::iterator chanit;
	std::map<int, Client>::iterator 		 it;
	std::string 							 nick;
	
	if (!client.get_auth())
		return (ERR_PASSREQUIRED);
	if (params.empty() || params[0].empty())
		return (ERR_NONICKNAMEGIVEN);
	nick = params[0];
	if (!is_valid_nick(nick))
		return (ERR_ERRONEOUSNICKNAME);
	if (same_nick(client.get_nick(), nick))
		return (0);
	for (it = this->_clients.begin(); it != this->_clients.end(); ++it)
	{
		if (it->first != client.get_client_fd() && same_nick(it->second.get_nick(), nick))
			return (ERR_NICKNAMEINUSE);
	}
	if (client.get_registered())
	{
		this->send_cmd(client, client, "NICK :" + nick); // if client registered->send NICK to client
		for (it = this->_clients.begin(); it != this->_clients.end(); ++it)
		{
			if (it->first == client.get_client_fd())
				continue ; // if client is the same as sender->skip
			for (chanit = this->_channels.begin(); chanit != this->_channels.end(); ++chanit)
			{
				if (chanit->second.getClients().count(client.get_client_fd()) && chanit->second.getClients().count(it->first))
				{
					this->send_cmd(client, it->second, "NICK :" + nick); // if clients are in same channel->send NICK to other client
					break ;
				}
			}
		}
	}
	else
		this->send_notice(client, NICKSET, nick); // if client not registered->send notice to client
	client.set_nick(nick);
	return (0);
}

int Server::user_cmd(Client &client, const std::vector<std::string> &params)
{
	std::string 	username;
	std::string 	mode;
	std::string 	realname;
	
	if (client.get_registered())
		return (ERR_ALREADYREGISTRED);
	if (!client.get_auth())
		return (ERR_NOTREGISTERED);
	if (params.size() < 4)
		return (ERR_NEEDMOREPARAMS);
	if (client.get_nick().empty())
		return (ERR_NONICKNAMEGIVEN);
	username = params[0];
	mode = params[1];
	realname = params[3];
	if (username.empty())
		return (ERR_NEEDMOREPARAMS);
	if (!is_valid_user_mode(mode))
		return (ERR_INVALIDUSERMODE);
	client.set_user(username);
	client.set_realname(realname);
	return (0);
}

int Server::fast_cmd(Client &client, const std::vector<std::string> &params)
{
	(void)params;

	if (!client.get_auth())
		return (ERR_PASSREQUIRED);
	client.set_nick("Nick");
	client.set_user("User");
	client.set_host_ip("localhost");
	return (0);
}

// int Server::fast_cmd(Client &client, const std::vector<std::string> &params)
// {
// 	std::string to_send;
// 	(void)params;

// 	client.set_auth(true);
// 	client.set_nick(generateRandomString(3));
// 	client.set_user(generateRandomString(3));
// 	client.set_registered(true);
// 	client.set_host_ip(generateRandomString(9));
// 	to_send += "You shall be named: " + client.get_nick() + "\n";
// 	to_send += "Your home kingdom shall be: " + client.get_host_ip() + "\n";
// 	to_send += "Thy user shall be sung as: " + client.get_user() + "\n";
// 	to_send += "Go forth, make haste, child of Mercury\r\n";
// 	this->send_msg(client, to_send);
// 	return (0);
// }
