/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Part.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/08 21:15:24 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/11/08 21:16:52 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/Server.hpp"
#include "../../includes/Client.hpp"
#include "../../includes/Utils.hpp"

int Server::part_cmd(Client &client, const std::vector<std::string> &params)
{
	std::map<std::string, Channel>::iterator chanit;
	std::vector<std::string> channels, 		 notify_chan;
	std::map<int, Client *>::iterator 		 it;
	std::string 							 msg;
	size_t									 i;
	int										 newchannopFD;

	if (params.empty() || params[0].empty())
		return (ERR_NEEDMOREPARAMS);
	channels = ft_split(params[0], ',');
	if (channels.empty())
		return (ERR_NEEDMOREPARAMS);
	i = 0;
	while (i < channels.size())
	{
		chanit = this->_channels.find(channels[i]);
		if (chanit == this->_channels.end())
			this->send_error(client, ERR_NOSUCHCHANNEL, channels[i]);
		else if (!chanit->second.getClients().count(client.get_client_fd()))
			this->send_error(client, ERR_NOTONCHANNEL, channels[i]);
		else
		{
			notify_chan.clear();
			notify_chan.push_back(channels[i]);
			notify_chan.push_back(client.get_nick() + " has left");
			if (params.size() > 1)
				notify_chan.push_back("Reason: " + params[1]);
			msg = "PART " + notify_chan[0];
			if (params.size() > 1)
				msg += " :" + params[1];
			it = chanit->second.getClients().begin();
			while (it != chanit->second.getClients().end())
			{
				this->send_cmd(client, *it->second, msg); // send PART to all clients in channel
				it++;
			}
			newchannopFD = chanit->second.leaveChannel(client); // rnv client from channel and get new chanop if need
			if (newchannopFD > 0)
				this->notify_new_op(chanit->second, newchannopFD); // notify clients about new chanop
			else if (newchannopFD == -1)
				this->_channels.erase(chanit); // channel is empty->erase it
		}
		i++;
	}
	return (0);
}
