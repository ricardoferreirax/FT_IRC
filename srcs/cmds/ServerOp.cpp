/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerOp.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/09 14:58:44 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/11/09 21:28:13 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/Server.hpp"
#include "../../includes/Client.hpp"
#include "../../includes/Utils.hpp"

int Server::oper_cmd(Client &client, const std::vector<std::string> &params)
{
	std::map<std::string, std::string>::iterator it;

	if (params.size() < 2 || params[0].empty() || params[1].empty())
		return (ERR_NEEDMOREPARAMS);
	if (!client.get_registered())
		return (ERR_NOTREGISTERED);
	if (client.is_server_op())
		return (RPL_YOUREOPER);
	it = this->_opers.find(params[0]);
	if (it == this->_opers.end() || it->second != params[1])
		return (ERR_PASSWDMISMATCH);
	client.set_server_op(true);
	this->send_reply(client, RPL_YOUREOPER, "", "");
	return (0);
}

int Server::kill_cmd(Client &client, const std::vector<std::string> &params)
{
	std::map<int, Client>::iterator it;
	std::vector<std::string> 		quit;
	std::string 					nick;
	std::string 					reason;

	if (!client.is_server_op())
		return (ERR_NOPRIVILEGES);
	if (params.size() < 2 || params[0].empty() || params[1].empty())
		return (ERR_NEEDMOREPARAMS);
	nick = params[0];
	reason = params[1];
	for (it = this->_clients.begin(); it != this->_clients.end(); ++it)
	{
		if (same_nick(it->second.get_nick(), nick))
			break ;
	}
	if (it == this->_clients.end())
		return (ERR_NOSUCHNICK);
	if (it->second.get_quit())
	{
		this->send_notice(client, ALREADYDISCONNECTED, "");
		return (0);
	}
	if (it->first == client.get_client_fd())
		this->send_notice(client, KILLSUICIDE, "");
	this->send_cmd(client, it->second, "KILL " + it->second.get_nick() + " :" + reason);
	quit.push_back("killed by " + client.get_nick());
	this->quit_cmd(it->second, quit);
	// std::cout << "[KILL] " << client.get_nick() << " killed " << it->second.get_nick() << " (" << reason << ")" << std::endl;
	return (0);
}
