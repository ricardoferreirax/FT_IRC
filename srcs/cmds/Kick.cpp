/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Kick.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/08 21:14:58 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/11/08 21:17:17 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/Server.hpp"
#include "../../includes/Client.hpp"
#include "../../includes/Utils.hpp"

int Server::kick_cmd(Client &client, const std::vector<std::string> &params)
{
	std::map<std::string, Channel>::iterator chanit;
	std::map<int, Client>::iterator 		 clienit;
	std::map<int, Client *>::iterator 		 it;
	std::vector<std::string> 				 channels;
	std::vector<std::string> 				 clients;
	std::vector<std::string> 				 args;
	size_t 									 i;
	size_t 									 j;
	int 									 ret;
	
	if (params.size() < 2 || params[0].empty() || params[1].empty())
		return (ERR_NEEDMOREPARAMS);
	channels = ft_split(params[0], ',');
	clients = ft_split(params[1], ',');
	if (channels.empty() || clients.empty())
		return (ERR_NEEDMOREPARAMS);
	if (!check_allchans(channels, &client)) // check if client is in all channels
		return (0);
	i = 0;
	while (i < channels.size())
	{
		chanit = this->_channels.find(channels[i]);
		if (!chanit->second.getChannops().count(client.get_client_fd()))
			this->send_error(client, ERR_CHANOPRIVSNEEDED, channels[i]); // user is not a chanop
		else
		{
			j = 0;
			while (j < clients.size())
			{
				clienit = this->reverseClientSearch(clients[j]); // find client by nick
				if (clienit == this->_clients.end())
					this->send_error(client, ERR_NOSUCHNICK, clients[j]);
				else if (!chanit->second.getClients().count(clienit->first))
					this->send_error(client, ERR_USERNOTINCHANNEL, clients[j] + " " + channels[i]);
				else
				{
					args.clear();
					args.push_back(channels[i]);
					args.push_back(clients[j]);
					if (params.size() > 2 && !params[2].empty())
						args.push_back(params[2]);
					else
						args.push_back("No reason provided");
					it = chanit->second.getClients().begin();
					while (it != chanit->second.getClients().end())
					{
						this->send_cmd(client, *it->second, "KICK " + args[0] + " " + args[1] + " :" + args[2]); // send KICK to all clients
						it++;
					}
					ret = chanit->second.leaveChannel(clienit->second); // rmv client from channel and get new chanop if need
					if (ret == -1)
					{
						this->_channels.erase(chanit); // channel is empty->erase it
						break ;
					}
					if (ret > 0)
						this->notify_new_op(chanit->second, ret); // notify clients about new chanop
					if (!chanit->second.getChannops().count(client.get_client_fd()))
						break ; // client is no longer a chanop->stop kicking clients
				}
				j++;
			}
		}
		i++;
	}
	return (0);
}
