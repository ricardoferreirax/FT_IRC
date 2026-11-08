/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Privmsg.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/08 21:20:32 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/11/08 21:22:40 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/Server.hpp"
#include "../../includes/Client.hpp"
#include "../../includes/Utils.hpp"

int Server::privmsg_cmd(Client &client, const std::vector<std::string> &params)
{
	std::map<std::string, Channel>::iterator chanit;
	std::map<int, Client *>::iterator 		 clienit;
	std::map<int, Client *> 				 chan_clients;
	std::map<int, Client>::iterator 		 it;
	std::string 							 target;
	std::string 							 msg;

	if (params.empty() || params[0].empty())
		return (ERR_NORECIPIENT);
	if (params.size() < 2 || params[1].empty())
		return (ERR_NOTEXTTOSEND);
	target = params[0];
	msg = params[1];
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
		return (0);
	}
	for (it = this->_clients.begin(); it != this->_clients.end(); ++it)
	{
		if (same_nick(it->second.get_nick(), target))
		{
			this->send_cmd(client, it->second, "PRIVMSG " + it->second.get_nick() + " :" + msg); // if target matches a client nick->send msg to target client
			if (it->second.is_away())
				this->send_reply(client, RPL_AWAY, it->second.get_nick(), it->second.get_away_msg()); // if target client is away->send away msg to sender
			return (0);
		}
	}
	chanit = this->_channels.find(target);
	if (chanit == this->_channels.end())
		return (ERR_NOSUCHNICK);
	chan_clients = chanit->second.getClients();
	for (clienit = chan_clients.begin(); clienit != chan_clients.end(); ++clienit)
	{
		if (clienit->first != client.get_client_fd())
			this->send_cmd(client, *(clienit->second), "PRIVMSG " + target + " :" + msg); // if target is in channel->send msg to all clients in channel except sender
	}
	return (0);
}
