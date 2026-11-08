/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Join.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/08 21:06:31 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/11/08 21:08:19 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/Server.hpp"
#include "../../includes/Client.hpp"
#include "../../includes/Utils.hpp"

int Server::join_cmd(Client &client, const std::vector<std::string> &params)
{
	std::map<std::string, Channel>::iterator chanit;
	std::map<int, Client *>::iterator 		 it;
	std::vector<std::string> 				 c_names;
	std::vector<std::string> 				 keys;
	std::string 							 chan_name;
	std::string 							 key;
	size_t									 i;
	int										 ret;
	
	if (params.empty() || params[0].empty())
		return (ERR_NEEDMOREPARAMS);
	c_names = ft_split(params[0], ',');
	if (params.size() > 1)
		keys = ft_split(params[1], ',');
	i = 0;
	while (i < c_names.size())
	{
		chan_name = c_names[i];
		if (chan_name.empty() || (chan_name[0] != '#' && chan_name[0] != '&'))
		{
			this->send_error(client, ERR_BADCHANMASK, chan_name);
			i++;
			continue ;
		}
		key = "";
		if (i < keys.size())
			key = keys[i];
		chanit = this->_channels.find(chan_name);
		if (chanit == this->_channels.end()) // if channel doesn't exist->create it and add client as chanop
		{
			Channel ch(chan_name, default_mode(), client);
			this->_channels.insert(std::make_pair(chan_name, ch));
			chanit = this->_channels.find(chan_name);
			this->send_cmd(client, client, "NOTICE " + client.get_nick() + " :You created Channel " + chan_name);
		}
		else
		{
			ret = chanit->second.joinChannel(client, key); // if channel exists->try to join client to channel
			if (ret != 0)
			{
				if (ret == 1)
					this->send_error(client, ERR_USERONCHANNEL, chan_name);
				else if (ret == 2)
					this->send_error(client, ERR_CHANNELISFULL, chan_name);
				else if (ret == 3)
					this->send_error(client, ERR_INVITEONLYCHAN, chan_name);
				else if (ret == 4)
					this->send_error(client, ERR_BADCHANNELKEY, chan_name);
				else if (ret == 5)
					this->send_error(client, ERR_BANNEDFROMCHAN, chan_name);
				i++;
				continue ;
			}
		}
		it = chanit->second.getClients().begin();
		while (it != chanit->second.getClients().end())
		{
			this->send_cmd(client, *it->second, "JOIN " + chan_name); // send JOIN to all clients in channel
			it++;
		}
		if (!chanit->second.getMode().topic.empty())
			this->send_reply(client, RPL_TOPIC, chan_name, chanit->second.getMode().topic); // has topic->send topic to client
		else
			this->send_reply(client, RPL_NOTOPIC, chan_name, "No topic is set"); // has no topic->send no topic to client
		this->send_channel_names(client, chanit->second);
		i++;
	}
	return (0);
}
