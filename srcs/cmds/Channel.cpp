/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/09 14:50:06 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/11/09 14:52:22 by rmedeiro         ###   ########.fr       */
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

int Server::invite_cmd(Client &client, const std::vector<std::string> &params)
{
	std::map<int, Client>::iterator clienit;
	std::map<std::string, Channel>::iterator chanit;
	
	if (params.size() < 2 || params[0].empty() || params[1].empty())
		return (ERR_NEEDMOREPARAMS);
	clienit = this->reverseClientSearch(params[0]); // find client by nick
	if (clienit == this->_clients.end())
		return (ERR_NOSUCHNICK);
	chanit = this->_channels.find(params[1]); // find channel by name
	if (chanit == this->_channels.end())
		return (ERR_NOSUCHCHANNEL);
	if (!chanit->second.getClients().count(client.get_client_fd()))
		return (ERR_NOTONCHANNEL);
	if (chanit->second.getClients().count(clienit->first))
		return (ERR_USERONCHANNEL); // user is already in channel
	if (chanit->second.getMode().invite_only && !chanit->second.getChannops().count(client.get_client_fd()))
		return (ERR_CHANOPRIVSNEEDED); // user is not a chanop and channel is invite only
	chanit->second.inviteUser(clienit->second); // invite user to channel
	this->send_cmd(client, clienit->second, "INVITE " + params[0] + " :" + params[1]);
	this->send_reply(client, RPL_INVITING, params[0] + " " + params[1], "");
	return (0);
}

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

int Server::mode_cmd(Client &client, const std::vector<std::string> &params)
{
	std::map<std::string, Channel>::iterator chanit;
	std::vector<t_modecmd> 					 flags;
	size_t									 i;
	int										 ret;
	int										 newopFD;
	bool									 res;

	if (params.empty() || params[0].empty())
		return (ERR_NEEDMOREPARAMS);
	chanit = this->_channels.find(params[0]);
	if (chanit == this->_channels.end())
		return (ERR_NOSUCHCHANNEL);
	if (params.size() < 2)
	{
		this->send_channel_modes(client, chanit->second); // send channel modes to client
		return (0);
	}
	if (!chanit->second.getChannops().count(client.get_client_fd()))
		return (ERR_CHANOPRIVSNEEDED); // user is not a chanop
	ret = tokenize_mode_flags(params[1], flags, params, 2);
	if (ret == -1)
		return (ERR_UNKNOWNMODE);
	if (ret == -2)
		return (ERR_NEEDMOREPARAMS);
	if (ret != 0)
		return (0);
	i = 0;
	while (i < flags.size())
	{
		res = false;
		newopFD = -1; // no new chanop is set
		if (flags[i].flag == 'o' && !flags[i].is_add && chanit->second.getChannops().size() == 1)
			newopFD = *chanit->second.getChannops().begin(); // chanop is rmvd and there are no more chanops->set to first client in channel
		switch (flags[i].flag)
		{
		case 'i':
			res = handle_imode(client, params[0], flags[i]);
			break ;
		case 't':
			res = handle_tmode(client, params[0], flags[i]);
			break ;
		case 'k':
			res = handle_kmode(client, params[0], flags[i]);
			break ;
		case 'o':
			res = handle_omode(client, params[0], flags[i]);
			break ;
		case 'l':
			res = handle_lmode(client, params[0], flags[i]);
			break ;
		case 'b':
			res = handle_bmode(client, params[0], flags[i]);
			break ;
		}
		if (!res)
			return (0);
		this->notify_mode_change(client, chanit->second, flags[i]); // notify clients about mode change
		if (newopFD > 0 && !chanit->second.getChannops().count(newopFD))
			this->notify_new_op(chanit->second, *chanit->second.getChannops().begin()); // notify clients about new chanop
		i++;
	}
	return (0);
}

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
