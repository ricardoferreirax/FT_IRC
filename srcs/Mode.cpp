/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Mode.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pedro <pfreire-@student.42lisboa.com>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/22 13:29:59 by pedro             #+#    #+#             */
/*   Updated: 2026/10/22 13:53:56 by pedro            ###   ########.fr       */
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

std::map<int, Client>::iterator Server::reverseClientSearch(std::string nick)
{
	std::map<int, Client>::iterator clienit;

	clienit = this->_clients.begin();
	while(clienit != this->_clients.end())
	{
		if (same_nick(nick, clienit->second.get_nick()))
			break;
		clienit++;
	}
	return (clienit);
}

// +i/-i, +t/-t, -k and -l dont need param
// +k need key, +o/-o need nick, +l need limit and +b/-b need banmask
int	tokenize_mode_flags(const std::string &params, std::vector<t_modecmd> &flags, const std::vector<std::string> cmdargs, size_t arg_idx)
{
	std::string arg;
	size_t 		i;
	bool 		is_add;
	bool 		needs_param;

	i = 0;
	is_add = true;
	if (params.empty() || (params[0] != '+' && params[0] != '-'))
		return (-1);
	while (i < params.size())
	{
		arg = "";
		if (params[i] == '+')
		{
			if (i + 1 >= params.size() || params[i + 1] == '+' || params[i + 1] == '-')
				return (-1);
			is_add = true;
		}
		else if (params[i] == '-')
		{
			if (i + 1 >= params.size() || params[i + 1] == '+' || params[i + 1] == '-')
				return (-1);
			is_add = false;
		}
		else
		{
			if (params[i] != 'i' && params[i] != 't' && params[i] != 'k' && params[i] != 'o' 
				&& params[i] != 'l' && params[i] != 'b')
				return (-1); // invalid mode flag
			needs_param = false;
			if (params[i] == 'o' || (params[i] == 'k' && is_add) || (params[i] == 'l' && is_add))
				needs_param = true; // need param when adding
			if (params[i] == 'b' && arg_idx < cmdargs.size())
				needs_param = true; // need param when adding or removing ban
			if (needs_param)
			{
				if (arg_idx >= cmdargs.size())
					return (-2); // no more args to use as param->return error
				else
					arg = cmdargs[arg_idx]; // otherwise use next arg as param
				arg_idx++;
			}
			flags.push_back(create_modecmd(needs_param, is_add, params[i], arg));
		}
		i++;
	}
	return (0);
}

Channel *Server::validate_channel_op(Client &client, const std::string &channel_name)
{
	std::map<std::string, Channel>::iterator chanit;

	chanit = this->_channels.find(channel_name);
	if (chanit == this->_channels.end())
	{
		this->send_error(client, ERR_NOSUCHCHANNEL, channel_name);
		return (NULL);
	}
	if (chanit->second.getChannops().find(client.get_client_fd()) == chanit->second.getChannops().end())
	{
		this->send_error(client, ERR_CHANOPRIVSNEEDED, channel_name);
		return (NULL);
	}
	return (&chanit->second);
}

bool Server::handle_imode(Client &client, std::string chan_name, t_modecmd flag)
{
	Channel *channel;

	channel = this->validate_channel_op(client, chan_name);
	if (!channel)
		return (false);
	channel->setMode(flag.is_add, channel->getMode().topic_restricted, channel->getMode().topic, channel->getMode().users, channel->getMode().key);
	if (flag.is_add)
		this->send_notice(client, CHANNELINVITEONLY, chan_name);
	else
		this->send_notice(client, CHANNELINVITEOPEN, chan_name);
	// std::cout << "Channel " << chan_name << " now has " << flag.is_add << " i" << std::endl;
	return (true);
}

bool Server::handle_tmode(Client &client, std::string chan_name, t_modecmd flag)
{
	Channel *channel;

	channel = this->validate_channel_op(client, chan_name);
	if (!channel)
		return (false);
	channel->setMode(channel->getMode().invite_only, flag.is_add, channel->getMode().topic, channel->getMode().users, channel->getMode().key);
	if (flag.is_add)
		this->send_notice(client, CHANNELTOPICRESTRICTED, chan_name);
	else
		this->send_notice(client, CHANNELTOPICOPEN, chan_name);
	// std::cout << "Channel " << chan_name << " now has " << flag.is_add << " t" << std::endl;
	return (true);
}

bool Server::handle_kmode(Client &client, std::string chan_name, t_modecmd flag)
{
	Channel *channel;

	channel = this->validate_channel_op(client, chan_name);
	if (!channel)
		return (false);
	if (flag.is_add && flag.param.empty())
	{
		this->send_error(client, ERR_NEEDMOREPARAMS, "MODE");
		return (false);
	}
	if (flag.is_add && !channel->getMode().key.empty())
	{
		this->send_error(client, ERR_KEYSET, chan_name);
		return (false);
	}
	if (flag.is_add)
	{
		channel->setMode(channel->getMode().invite_only, channel->getMode().topic_restricted, channel->getMode().topic, channel->getMode().users, flag.param); // set new key
		this->send_notice(client, CHANNELKEYSET, chan_name + " (Pass: " + flag.param + ")");
	}
	else
	{
		channel->setMode(channel->getMode().invite_only, channel->getMode().topic_restricted, channel->getMode().topic, channel->getMode().users, ""); // clear key
		this->send_notice(client, CHANNELKEYREMOVED, chan_name);
	}
	// std::cout << "Channel " << chan_name << " now has " << flag.is_add << " k" << std::endl;
	return (true);
}

bool Server::handle_omode(Client &client, std::string chan_name, t_modecmd flag)
{
	std::map<int, Client *>::iterator it;
	std::map<int, Client>::iterator   clienit;
	Channel	*channel;

	channel = this->validate_channel_op(client, chan_name);
	if (!channel)
		return (false);
	clienit = this->reverseClientSearch(flag.param);
	if (clienit == this->_clients.end())
	{
		this->send_error(client, ERR_NOSUCHNICK, flag.param);
		return (false);
	}
	if (!channel->getClients().count(clienit->first))
	{
		this->send_error(client, ERR_USERNOTINCHANNEL, flag.param + " " + chan_name);
		return (false);
	}
	if (flag.is_add && channel->getChannops().count(clienit->first))
	{
		this->send_notice(client, USERALREADYOP, flag.param + " in " + chan_name);
		return (false);
	}
	if (!flag.is_add && !channel->getChannops().count(clienit->first))
	{
		this->send_notice(client, USERNOTOP, flag.param + " in " + chan_name);
		return (false);
	}
	if (flag.is_add)
	{
		channel->getChannops().insert(clienit->first); // add user as chanop
		this->send_notice(client, CHANNELOPSET, flag.param + " in " + chan_name);
	}
	else
	{
		channel->getChannops().erase(clienit->first); // rmv user as chanop
		this->send_notice(client, CHANNELOPREMOVED, flag.param + " in " + chan_name);
	}
	if (channel->getChannops().empty() && !channel->getClients().empty()) // no op left->promote another client
	{
		it = channel->getClients().begin();
		if (it->first == clienit->first && channel->getClients().size() > 1) // client removed is first in list
			it++; // to avoid promoting removed op->move to next client
		channel->getChannops().insert(it->first); // set client as new chanop
	}
	// std::cout << "Channel " << chan_name << " now has " << flag.is_add << " o for " << flag.param << std::endl;
	return (true);
}

bool Server::handle_lmode(Client &client, std::string chan_name, t_modecmd flag)
{
	Channel *channel;
	t_ulimit users;

	channel = this->validate_channel_op(client, chan_name);
	if (!channel)
		return (false);
	users = channel->getMode().users;
	if (!flag.param.empty() && std::atoi(flag.param.c_str()) <= 0)
	{
		this->send_notice(client, INVALIDCHANNELLIMIT, "");
		return (false);
	}
	if (flag.is_add)
		users.u_limit = std::atoi(flag.param.c_str());
	else
		users.u_limit = 0;
	channel->setMode(channel->getMode().invite_only, channel->getMode().topic_restricted, channel->getMode().topic, users, channel->getMode().key);
	std::cout << "Channel " << chan_name << " now has " << flag.is_add << " l" << std::endl;
	return true;
}

std::string ban_list(std::vector<t_banmask> banlist)
{
	std::string flist;
	std::vector<t_banmask>::iterator banlistit = banlist.begin();
	while(banlistit != banlist.end())
	{
		flist += banlistit->nick + "!" + banlistit->user + "@" + banlistit->host;
		flist += "\n";
		banlistit++;
	}
	return flist;
}

bool Server::handle_bmode(Client &client, std::string chan_name, t_modecmd flag)
{
	Channel *channel;

	if (flag.param.empty())
		return (false);
	channel = this->validate_channel_op(client, chan_name);
	if (!channel)
		return (false);
	if (flag.is_add)
	{
		if (!channel->addBanMask(flag.param))
			return (false); // if banmask is invalid->return false
	}
	else
		channel->removeBanMask(flag.param);
	// std::cout << "Channel " << chan_name << " now has " << flag.is_add << " b" << std::endl;
	return (true);
}
