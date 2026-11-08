/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Privmsg.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/08 21:20:32 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/11/08 21:47:36 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/Server.hpp"
#include "../../includes/Client.hpp"
#include "../../includes/Utils.hpp"

int Server::privmsg_cmd(Client &client, const std::vector<std::string> &params)
{
	std::map<std::string, Channel>::iterator chanit;
	std::map<int, Client *>::iterator 		 clienit;
	std::map<int, Client>::iterator 		 it;
	std::vector<std::string> 				 targets;
	std::string 							 target;
	std::string 							 msg;
	size_t 									 i;

	if (params.empty() || params[0].empty())
		return (ERR_NORECIPIENT);
	if (params.size() < 2 || params[1].empty())
		return (ERR_NOTEXTTOSEND);
	targets = ft_split(params[0], ',');
	msg = params[1];
	i = 0;
	while (i < targets.size())
	{
		target = targets[i];
		if (same_nick(target, "Miku"))
		{
			try
			{
				if (!this->_bot.start_request(client.get_client_fd(), client.get_nick(), "", msg))
					this->send_notice(client, MIKUUNAVAILABLE, "");
			}
			catch (const std::exception &e)
			{
				std::cerr << "Miku: " << e.what() << std::endl;
				this->send_notice(client, MIKUUNAVAILABLE, "");
			}
			i++;
			continue ; // skip to next target->will not search for miku in clients/channels
		}
		it = this->_clients.begin();
		while (it != this->_clients.end())
		{
			if (same_nick(it->second.get_nick(), target)) // target matches a client nick w/ case insensitive
				break ;
			it++;
		}
		if (it != this->_clients.end())
		{
			this->send_cmd(client, it->second, "PRIVMSG " + it->second.get_nick() + " :" + msg); // send msg to target client
			if (it->second.is_away())
				this->send_reply(client, RPL_AWAY, it->second.get_nick(), it->second.get_away_msg()); // target client is away->send away msg to sender
			i++;
			continue ; // skip to next target->will not search for target in channels
		}
		chanit = this->_channels.find(target);
		if (chanit == this->_channels.end())
		{
			if (!target.empty() && (target[0] == '#' || target[0] == '&'))
				this->send_error(client, ERR_NOSUCHCHANNEL, target); // target is a channel but doesn't exist
			else
				this->send_error(client, ERR_NOSUCHNICK, target); // target is a nick but doesn't exist
			i++;
			continue ;
		}
		clienit = chanit->second.getClients().begin();
		while (clienit != chanit->second.getClients().end())
		{
			if (clienit->first != client.get_client_fd())
				this->send_cmd(client, *clienit->second, "PRIVMSG " + chanit->second.getName() + " :" + msg); // send msg to all clients in channel except who sends
			clienit++;
		}
		i++;
	}
	return (0);
}
