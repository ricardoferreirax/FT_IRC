/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Mode.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/26 17:31:45 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/26 17:34:23 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../bonus/includes/Bot.hpp"
#include "../includes/Channels.hpp"
#include "../includes/Server.hpp"
#include "../includes/Utils.hpp"
#include <arpa/inet.h>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

t_modecmd	create_modecmd(bool needs_param, bool is_add, char flag, std::string param = "")
{
	t_modecmd	cmdmode;

	cmdmode.flag = flag;
	cmdmode.is_add = is_add;
	cmdmode.needs_param = needs_param;
	cmdmode.param = param;
	return (cmdmode);
}

bool	tokenize_mode_flags(const std::string &params, std::vector<t_modecmd> &flags, const std::vector<std::string> cmdargs, size_t arg_idx)
{
	size_t	i;
	bool	is_add;
	bool	needs_param;

	std::string arg = "";
	i = 0;
	if (params.empty() || (params[0] != '+' && params[0] != '-'))
		return (false);
	while (i < params.size())
	{
		{
			arg = "";
			if (params[i] == '+')
			{
				if (i + 1 >= params.size() || params[i + 1] == '+' || params[i + 1] == '-')
					return (false);
				is_add = true;
			}
			else if (params[i] == '-')
			{
				if (i + 1 >= params.size() || params[i + 1] == '+' || params[i + 1] == '-')
					return (false);
				is_add = false;
			}
			else
			{
				if (!is_add && params[i] == 'l')
					needs_param = false;
				else if (params[i] == 'k' || params[i] == 'o' || params[i] == 'l')
				{
					needs_param = true;
					if (arg_idx >= cmdargs.size())
						return (false);
					else
						arg = cmdargs[arg_idx];
					arg_idx++;
				}
				else
					needs_param = false;
				flags.push_back(create_modecmd(needs_param, is_add, params[i], arg));
			}
		}
		i++;
	}
	return (true);
}

Channel *Server::validate_channel_op(Client &client, const std::string &channel_name, bool needs_sudo)
{
	std::map<std::string, Channel>::iterator chanit = this->_channels.find(channel_name);
	
	if (chanit == this->_channels.end())
	{
		this->send_error(client, 401, "No such channel");
		return (NULL);
	}
	if (needs_sudo && chanit->second.getChannops().find(client.get_client_fd()) == chanit->second.getChannops().end())
	{
		this->send_error(client, 401, "No perms, try using sudo");
		return (NULL);
	}
	return (&(chanit->second));
}

bool Server::handle_imode(Client &client, std::string chan_name, t_modecmd flag)
{
	Channel	*channel;

	channel = validate_channel_op(client, chan_name, 1);
	if (!channel)
		return (false);
	channel->setMode(flag.is_add, channel->getMode().topic_restricted, channel->getMode().topic, channel->getMode().users, channel->getMode().key);
	std::cout << "Channel " << chan_name << " now has " << flag.is_add << " i" << std::endl;
	return (true);
}

bool Server::handle_tmode(Client &client, std::string chan_name, t_modecmd flag)
{
	Channel	*channel;

	channel = validate_channel_op(client, chan_name, 0);
	if (!channel)
		return (false);
	channel->setMode(channel->getMode().invite_only, flag.is_add, channel->getMode().topic, channel->getMode().users, channel->getMode().key);
	std::cout << "Channel " << chan_name << " now has " << flag.is_add << " t" << std::endl;
	return (true);
}

bool Server::handle_kmode(Client &client, std::string chan_name, t_modecmd flag)
{
	Channel	*channel;

	channel = validate_channel_op(client, chan_name, 1);
	if (!channel)
		return (false);
	if (flag.is_add)
		channel->setMode(channel->getMode().invite_only, channel->getMode().topic_restricted, channel->getMode().topic, channel->getMode().users, flag.param);
	else
		channel->setMode(channel->getMode().invite_only, channel->getMode().topic_restricted, channel->getMode().topic, channel->getMode().users, "");
	std::cout << "Channel " << chan_name << " now has " << flag.is_add << " k" << std::endl;
	return (true);
}

bool Server::handle_omode(Client &client, std::string chan_name, t_modecmd flag)
{
	Channel	*channel;

	channel = validate_channel_op(client, chan_name, 1);
	if (!channel)
		return (false);
	std::map<int, Client>::iterator clienit = this->_clients.begin();
	while (clienit != this->_clients.end())
	{
		if (clienit->second.get_nick() == flag.param)
			break ;
		clienit++;
	}
	if (clienit == this->_clients.end())
	{
		this->send_error(client, 401, "No such client");
		return (false);
	}
	if (flag.is_add)
		channel->getChannops().insert(clienit->second.get_client_fd());
	else
		channel->getChannops().erase(clienit->second.get_client_fd());
	std::cout << "Channel " << chan_name << " now has " << flag.is_add << " o" << std::endl;
	return (true);
}

bool Server::handle_lmode(Client &client, std::string chan_name, t_modecmd flag)
{
	Channel *channel = validate_channel_op(client, chan_name, 1);
	
	if (!channel)
		return (false);
	t_ulimit users;
	if (flag.is_add)
		users.u_limit = std::atoi(flag.param.c_str());
	else
		users.u_limit = 0;
	channel->setMode(channel->getMode().invite_only, channel->getMode().topic_restricted, channel->getMode().topic, users, channel->getMode().key);
	std::cout << "Channel " << chan_name << " now has " << flag.is_add << " l" << std::endl;
	return (true);
}
