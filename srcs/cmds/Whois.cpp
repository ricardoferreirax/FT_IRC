/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Whois.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/08 21:21:33 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/11/08 21:25:02 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/Server.hpp"
#include "../../includes/Client.hpp"
#include "../../includes/Utils.hpp"

int Server::whois_cmd(Client &client, const std::vector<std::string> &params)
{
	std::map<std::string, Channel>::iterator chanit;
	std::map<int, Client>::iterator 		 clienit;
	std::vector<std::string> 				 users;
	std::string 							 channels;
	std::string 							 chan_opers;
	size_t 									 i;

	if (params.empty() || params.size() > 1 || params[0].empty())
		return (ERR_NEEDMOREPARAMS);
	users = ft_split(params[0], ',');
	i = 0;
	while (i < users.size())
	{
		channels.clear();
		chan_opers.clear();
		clienit = this->reverseClientSearch(users[i]);
		if (clienit == this->_clients.end())
		{
			this->send_error(client, ERR_NOSUCHNICK, users[i]);
			this->send_reply(client, RPL_ENDOFWHOIS, users[i], "");
			i++;
			continue ;
		}
		this->send_reply(client, RPL_WHOISUSER, clienit->second.get_nick(), clienit->second.get_user() + " " + clienit->second.get_host_ip() + " * :" + clienit->second.get_realname()); // send user info to client
		this->send_reply(client, RPL_WHOISSERVER, clienit->second.get_nick(), this->_name + " :IRC Server");
		chanit = this->_channels.begin();
		while (chanit != this->_channels.end())
		{
			if (chanit->second.getChannops().find(clienit->first) != chanit->second.getChannops().end()) // client is chanop
			{
				if (!chan_opers.empty())
					chan_opers += " "; // separate channames with space
				chan_opers += "@" + chanit->second.getName(); // add @ to chanop
			}
			else if (chanit->second.getClients().find(clienit->first) != chanit->second.getClients().end()) // client is in channel
			{
				if (!channels.empty())
					channels += " ";
				channels += chanit->second.getName();
			}
			chanit++;
		}
		if (!chan_opers.empty())
			this->send_reply(client, RPL_WHOISCHANNELS, clienit->second.get_nick(), chan_opers); // user is chanop in channels
		if (!channels.empty())
			this->send_reply(client, RPL_WHOISCHANNELS, clienit->second.get_nick(), channels); // user is member in channels
		this->send_reply(client, RPL_ENDOFWHOIS, clienit->second.get_nick(), "");
		i++;
	}
	return (0);
}
