/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ChannelCommands.cpp                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/03 13:21:43 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/11/07 13:20:05 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Server.hpp"
#include "../includes/Client.hpp"
#include "../includes/Utils.hpp"

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
		if (chanit == this->_channels.end())
		{
			Channel ch(chan_name, default_mode(), client);
			this->_channels.insert(std::make_pair(chan_name, ch));
			chanit = this->_channels.find(chan_name);
			this->send_cmd(client, client, "NOTICE " + client.get_nick() + " :You created Channel " + chan_name);
		}
		else
		{
			ret = chanit->second.joinChannel(client, key);
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
			this->send_cmd(client, *it->second, "JOIN :" + chan_name);
			it++;
		}
		if (!chanit->second.getMode().topic.empty())
			this->send_reply(client, RPL_TOPIC, chan_name, chanit->second.getMode().topic);
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
	clienit = this->reverseClientSearch(params[0]);
	if (clienit == this->_clients.end())
		return (ERR_NOSUCHNICK);
	chanit = this->_channels.find(params[1]);
	if (chanit == this->_channels.end())
		return (ERR_NOSUCHCHANNEL);
	if (!chanit->second.getClients().count(client.get_client_fd()))
		return (ERR_NOTONCHANNEL);
	if (chanit->second.getClients().count(clienit->first))
		return (ERR_USERONCHANNEL);
	if (chanit->second.getMode().invite_only && !chanit->second.getChannops().count(client.get_client_fd()))
		return (ERR_CHANOPRIVSNEEDED);
	chanit->second.inviteUser(clienit->second);
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
			this->send_reply(client, RPL_NOTOPIC, params[0], "");
		else
			this->send_reply(client, RPL_TOPIC, params[0], chanit->second.getMode().topic);
		return (0);
	}
	if (chanit->second.getMode().topic_restricted && !chanit->second.getChannops().count(client.get_client_fd()))
		return (ERR_CHANOPRIVSNEEDED);
	chanit->second.setMode(chanit->second.getMode().invite_only, chanit->second.getMode().topic_restricted, params[1], chanit->second.getMode().users, chanit->second.getMode().key);
	msg = "TOPIC " + params[0] + " :" + chanit->second.getMode().topic;
	it = chanit->second.getClients().begin();
	while (it != chanit->second.getClients().end())
	{
		this->send_cmd(client, *it->second, msg);
		it++;
	}
	return (0);
}

int Server::mode_cmd(Client &client, const std::vector<std::string> &params)
{
	std::map<std::string, Channel>::iterator chanit;
	std::vector<t_modecmd> 					 flags;
	size_t									 i;
	int										 err;
	bool									 res;

	if (params.empty() || params[0].empty())
		return (ERR_NEEDMOREPARAMS);
	chanit = this->_channels.find(params[0]);
	if (chanit == this->_channels.end())
		return (ERR_NOSUCHCHANNEL);
	if (params.size() < 2)
	{
		this->send_reply(client, RPL_CHANNELMODEIS, params[0], "");
		return (0);
	}
	if (!chanit->second.getChannops().count(client.get_client_fd()))
		return (ERR_CHANOPRIVSNEEDED);
	err = tokenize_mode_flags(params[1], flags, params, 2);
	if (err == -1)
		return (ERR_UNKNOWNMODE);
	if (err == -2)
		return (ERR_NEEDMOREPARAMS);
	if (err != 0)
		return (-1);
	i = 0;
	while (i < flags.size())
	{
		res = false;
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
		i++;
	}
	return (0);
}

int Server::part_cmd(Client &client, const std::vector<std::string> &params)
{
	std::map<std::string, Channel>::iterator chanit;
	std::map<int, Client *>::iterator 		 it;
	std::vector<std::string> 				 channels;
	std::vector<std::string> 				 notify_chan;
	std::string 							 msg;
	std::string 							 newchannop;
	int										 newchannopFD;
	size_t									 i;
	
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
		{
			this->send_error(client, ERR_NOSUCHCHANNEL, channels[i]);
			i++;
			continue ;
		}
		if (!chanit->second.getClients().count(client.get_client_fd()))
		{
			this->send_error(client, ERR_NOTONCHANNEL, channels[i]);
			i++;
			continue ;
		}
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
			this->send_cmd(client, *it->second, msg);
			it++;
		}
		newchannopFD = chanit->second.leaveChannel(client);
		if (newchannopFD > 0)
		{
			newchannop = this->_clients.find(newchannopFD)->second.get_nick();
			msg = "NOTICE " + channels[i] + " :" + newchannop + " is the new channel operator";
			it = chanit->second.getClients().begin();
			while (it != chanit->second.getClients().end())
			{
				this->send_cmd(client, *it->second, msg);
				this->send_cmd(client, *it->second, "MODE " + channels[i] + " +o " + newchannop);
				it++;
			}
		}
		if (newchannopFD == -1)
			this->_channels.erase(chanit);
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
	if (!check_allchans(channels, &client))
		return (0);
	i = 0;
	while (i < channels.size())
	{
		chanit = this->_channels.find(channels[i]);
		if (!chanit->second.getChannops().count(client.get_client_fd()))
		{
			this->send_error(client, ERR_CHANOPRIVSNEEDED, channels[i]);
			i++;
			continue ;
		}
		j = 0;
		while (j < clients.size())
		{
			clienit = this->reverseClientSearch(clients[j]);
			if (clienit == this->_clients.end())
				this->send_error(client, ERR_NOSUCHNICK, clients[j++]);
			else if (!chanit->second.getClients().count(clienit->first))
				this->send_error(client, ERR_USERNOTINCHANNEL, clients[j++] + " " + channels[i]);
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
					this->send_cmd(client, *it->second, "KICK " + args[0] + " " + args[1] + " :" + args[2]);
					it++;
				}
				ret = chanit->second.leaveChannel(clienit->second);
				if (ret == -1)
				{
					this->_channels.erase(chanit);
					break ;
				}
				if (!chanit->second.getChannops().count(client.get_client_fd()))
					break ;
				j++;
			}
		}
		i++;
	}
	return (0);
}
