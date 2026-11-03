/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RegistrationCmds.cpp                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/03 13:19:59 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/11/03 13:42:04 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

# include "../includes/Server.hpp"
# include "../includes/Client.hpp"
# include "../includes/Utils.hpp"

int Server::handle_pass(Client &client, const std::vector<std::string> &params)
{
	if (client.get_registered() || client.get_auth())
		return (ERR_ALREADYREGISTRED);
	if (params.empty())
		return (ERR_NEEDMOREPARAMS);
	if (params[0] != this->_pass)
		return (ERR_PASSWDMISMATCH);
	client.set_auth(true);
	this->send_notice(client, PASSACCEPTED, "");
	return (0);
}

int Server::handle_nick(Client &client, const std::vector<std::string> &params)
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
	if (client.get_nick() == nick)
		return (0);
	for (it = this->_clients.begin(); it != this->_clients.end(); ++it)
	{
		if (it->first != client.get_client_fd() && same_nick(it->second.get_nick(), nick))
			return (ERR_NICKNAMEINUSE);
	}
	if (client.get_registered())
	{
		this->send_cmd(client, client, "NICK :" + nick); // if client is registered->send NICK to client
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
		this->send_notice(client, NICKSET, nick); // if client isn't registered->send notice to client
	client.set_nick(nick);
	return (0);
}

int Server::handle_user(Client &client, const std::vector<std::string> &params)
{
	std::string username;
	std::string mode;
	std::string realname;

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
