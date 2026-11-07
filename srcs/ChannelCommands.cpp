/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ChannelCommands.cpp                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/03 13:21:43 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/11/07 12:04:24 by rmedeiro         ###   ########.fr       */
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
	{
		this->send_error(client, ERR_NEEDMOREPARAMS, "JOIN");
		return (-1);
	}
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
		if (i < keys.size())
			key = keys[i];
		else
			key = "";
		chanit = this->_channels.find(chan_name);
		if (chanit == this->_channels.end())
		{
			Channel ch(chan_name, default_mode(), client);
			this->_channels.insert(std::pair<std::string, Channel>(chan_name, ch));
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
	std::map<std::string, Channel>::iterator chanit;
	std::map<int, Client>::iterator 		 clienit;
	
	if (params.size() < 2 || params[0].empty() || params[1].empty())
	{
		this->send_error(client, ERR_NEEDMOREPARAMS, "INVITE");
		return (-1);
	}
	clienit = this->reverseClientSearch(params[0]);
	if (clienit == this->_clients.end())
	{
		this->send_error(client, ERR_NOSUCHNICK, params[0]);
		return (-1);
	}
	chanit = this->_channels.find(params[1]);
	if (chanit == this->_channels.end())
	{
		this->send_error(client, ERR_NOSUCHCHANNEL, params[1]);
		return (-1);
	}
	if (!chanit->second.getClients().count(client.get_client_fd()))
	{
		this->send_error(client, ERR_NOTONCHANNEL, params[1]);
		return (-1);
	}
	if (chanit->second.getClients().count(clienit->first))
	{
		this->send_error(client, ERR_USERONCHANNEL, params[0] + " " + params[1]);
		return (-1);
	}
	if (chanit->second.getMode().invite_only && !chanit->second.getChannops().count(client.get_client_fd()))
	{
		this->send_error(client, ERR_CHANOPRIVSNEEDED, params[1]);
		return (-1);
	}
	chanit->second.inviteUser(clienit->second);
	this->send_cmd(client, clienit->second, "INVITE " + params[0] + " :" + params[1]);
	return (0);
}

int Server::topic_cmd(Client &client, const std::vector<std::string> &params)
{
	std::map<std::string, Channel>::iterator chanit;
	std::map<int, Client *>::iterator 		 it;
	std::string 							 msg;
	
	if (params.empty() || params[0].empty())
	{
		this->send_error(client, ERR_NEEDMOREPARAMS, "TOPIC");
		return (-1);
	}
	chanit = this->_channels.find(params[0]);
	if (chanit == this->_channels.end())
	{
		this->send_error(client, ERR_NOSUCHCHANNEL, params[0]);
		return (-1);
	}
	if (!chanit->second.getClients().count(client.get_client_fd()))
	{
		this->send_error(client, ERR_NOTONCHANNEL, params[0]);
		return (-1);
	}
	if (params.size() < 2)
	{
		if (chanit->second.getMode().topic.empty())
			this->send_reply(client, RPL_NOTOPIC, params[0], "");
		else
			this->send_reply(client, RPL_TOPIC, params[0], chanit->second.getMode().topic);
		return (0);
	}
	if (chanit->second.getMode().topic_restricted)
	{
		if (!chanit->second.getChannops().count(client.get_client_fd()))
		{
			this->send_error(client, ERR_CHANOPRIVSNEEDED, params[0]);
			return (-1);
		}
	}
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
	{
		this->send_error(client, ERR_NEEDMOREPARAMS, "MODE");
		return (-1);
	}
	chanit = this->_channels.find(params[0]);
	if (chanit == this->_channels.end())
	{
		this->send_error(client, ERR_NOSUCHCHANNEL, params[0]);
		return (-1);
	}
	if (params.size() < 2)
	{
		this->send_reply(client, RPL_CHANNELMODEIS, params[0], "");
		return (0);
	}
	if (!chanit->second.getChannops().count(client.get_client_fd()))
	{
		this->send_error(client, ERR_CHANOPRIVSNEEDED, params[0]);
		return (-1);
	}
	err = tokenize_mode_flags(params[1], flags, params, 2);
	if (err)
	{
		if (err == -1)
			this->send_error(client, ERR_UNKNOWNCOMMAND, params[1]);
		else if (err == -2)
			this->send_error(client, ERR_NEEDMOREPARAMS, "MODE");
		return (-1);
	}
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
			return (-1);
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
	size_t									 i;
	int										 newchannopFD;

	if (params.empty() || params[0].empty())
	{
		this->send_error(client, ERR_NEEDMOREPARAMS, "PART");
		return (-1);
	}
	channels = ft_split(params[0], ',');
	if (channels.empty() || !check_allchans(channels, &client))
		return (-1);
	i = 0;
	while (i < channels.size())
	{
		chanit = this->_channels.find(channels[i]);
		if (!chanit->second.getClients().count(client.get_client_fd()))
		{
			this->send_error(client, ERR_NOTONCHANNEL, channels[i]);
			i++;
			continue ;
		}
		notify_chan.clear();
		// std::cout << client.get_client_fd() << " has left " << chanit->second.getName() << std::endl;
		notify_chan.push_back(channels[i]);
		notify_chan.push_back(client.get_nick() + " has left");
		if (params.size() == 2)
		    notify_chan.push_back("Reason: " + params[1]);
		msg = "PART " + notify_chan[0];
		if (params.size() == 2)
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
			msg = "NOTICE " + channels[i] + " :" + this->_clients.find(newchannopFD)->second.get_nick() + " is the new channel operator";
			it = chanit->second.getClients().begin();
			while (it != chanit->second.getClients().end())
			{
				this->send_cmd(client, *it->second, msg);
				this->send_cmd(client, *it->second, "MODE " + channels[i] + " +o " + this->_clients.find(newchannopFD)->second.get_nick());
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
	size_t									 i;
	size_t									 j;

	if (params.size() < 2)
	{
		this->send_error(client, ERR_NEEDMOREPARAMS, "KICK");
		return (-1);
	}
	channels = ft_split(params[0], ',');
	clients = ft_split(params[1], ',');
	if (channels.empty() || clients.empty())
	{
		this->send_error(client, ERR_NEEDMOREPARAMS, "KICK");
		return (-1);
	}
	if (!check_allchans(channels, &client))
		return (-1);
	i = 0;
	while (i < channels.size())
	{
		chanit = this->_channels.find(channels[i]);
		j = 0;
		while (j < clients.size())
		{
			if (!chanit->second.getChannops().count(client.get_client_fd()))
			{
				this->send_error(client, ERR_CHANOPRIVSNEEDED, channels[i]);
				return (-1);
			}
			clienit = this->reverseClientSearch(clients[j]);
			if (clienit == this->_clients.end())
			{
				this->send_error(client, ERR_NOSUCHNICK, clients[j]);
				j++;
				continue ;
			}
			if (!chanit->second.getClients().count(clienit->first))
			{
				this->send_error(client, ERR_USERNOTINCHANNEL, clients[j] + " " + channels[i]);
				j++;
				continue ;
			}
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
			if (chanit->second.leaveChannel(clienit->second) == -1)
			{
				this->_channels.erase(chanit);
				break ;
			}
			j++;
		}
		i++;
	}
	return (0);
}
