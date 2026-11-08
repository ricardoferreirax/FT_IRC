/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Who.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/08 21:21:36 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/11/08 21:24:46 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/Server.hpp"
#include "../../includes/Client.hpp"
#include "../../includes/Utils.hpp"

int Server::who_cmd(Client &client, const std::vector<std::string> &params)
{
	std::map<std::string, Channel>::iterator chanit;
	std::map<int, Client *>::iterator 		 clienit;
	std::map<int, Client>::iterator 		 it;
	std::map<int, Client *> 				 clients;
	std::string 							 mask;
	std::string 							 info;
	std::string 							 flags;

	mask = "*";
	if (!params.empty() && !params[0].empty() && params[0] != "0") // mask is provided and isn't 0
		mask = params[0];
	chanit = this->_channels.find(mask);
	if (chanit != this->_channels.end()) // mask matches a channel
	{
		clients = chanit->second.getClients();
		for (clienit = clients.begin(); clienit != clients.end(); ++clienit)
		{
			if (!clienit->second->get_registered())
				continue ;
			flags = "H";
			if (clienit->second->is_away())
				flags = "G";
			if (chanit->second.getChannops().count(clienit->second->get_client_fd()))
				flags += "@"; // if client is channop->add @
			info = mask + " " + clienit->second->get_user() + " " + clienit->second->get_host_ip() + " " + this->_name + " " + clienit->second->get_nick() + " " + flags;
			this->send_reply(client, RPL_WHOREPLY, info, "0 " + clienit->second->get_realname()); // send for each client in channel
		}
	}
	else // mask doesn't match a channel
	{
		for (it = this->_clients.begin(); it != this->_clients.end(); ++it)
		{
			if (!it->second.get_registered())
				continue ;
			if (!same_nick(mask, it->second.get_nick()) && !match_wildcard(mask, it->second.get_nick(), 0, 0) && !match_wildcard(mask, it->second.get_host_ip(), 0, 0)
				&& !match_wildcard(mask, this->_name, 0, 0) && !match_wildcard(mask, it->second.get_realname(), 0, 0))
				continue ; // skip clients that don't match mask
			flags = "H";
			if (it->second.is_away())
				flags = "G";
			info = "* " + it->second.get_user() + " " + it->second.get_host_ip() + " " + this->_name + " " + it->second.get_nick() + " " + flags;
			this->send_reply(client, RPL_WHOREPLY, info, "0 " + it->second.get_realname()); // send for matching client
		}
	}
	this->send_reply(client, RPL_ENDOFWHO, mask, "");
	return (0);
}
