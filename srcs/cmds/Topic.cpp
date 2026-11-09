/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Topic.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/08 21:16:07 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/11/08 21:16:31 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/Server.hpp"
#include "../../includes/Client.hpp"
#include "../../includes/Utils.hpp"

int Server::topic_cmd(Client &client, const std::vector<std::string> &params)
{
	std::map<std::string, Channel>::iterator chanit;
	std::map<int, Client *>::iterator 		 it;
	std::string 							 msg;
	
	if (params.empty() || params[0].empty())
		return (ERR_NEEDMOREPARAMS);
	chanit = this->_channels.find(params[0]);
	if (chanit == this->_channels.end())
		return (ERR_NOSUCHCHANNEL);
	if (!chanit->second.getClients().count(client.get_client_fd()))
		return (ERR_NOTONCHANNEL);
	if (params.size() < 2)
	{
		if (chanit->second.getMode().topic.empty())
			this->send_reply(client, RPL_NOTOPIC, params[0], ""); // no topic->send no topic to client
		else
			this->send_reply(client, RPL_TOPIC, params[0], chanit->second.getMode().topic); // has topic->send topic to client
		return (0);
	}
	if (chanit->second.getMode().topic_restricted && !chanit->second.getChannops().count(client.get_client_fd()))
		return (ERR_CHANOPRIVSNEEDED); // user is not a chanop and channel is topic restricted
	chanit->second.setMode(chanit->second.getMode().invite_only, chanit->second.getMode().topic_restricted, params[1], chanit->second.getMode().users, chanit->second.getMode().key); // set new topic
	msg = "TOPIC " + params[0] + " :" + chanit->second.getMode().topic;
	it = chanit->second.getClients().begin();
	while (it != chanit->second.getClients().end())
	{
		this->send_cmd(client, *it->second, msg); // send new topic to all clients in channel
		it++;
	}
	return (0);
}
