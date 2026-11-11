/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Whos.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/08 21:21:36 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/11/11 23:51:54 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/Server.hpp"
#include "../../includes/Client.hpp"
#include "../../includes/Utils.hpp"

int Server::who_cmd(Client &client, const std::vector<std::string> &params)
{
	std::map<std::string, Channel>::iterator chanit;
	std::map<int, Client *>::iterator 		 clienit;
	std::map<int, Client>::iterator 		 it;
	std::map<int, Client *> 				 clients;
	std::string 							 mask;
	std::string 							 info;
	std::string 							 flags;

	mask = "*";
	if (!params.empty() && !params[0].empty() && params[0] != "0") // mask is provided and isn't 0
		mask = params[0];
	chanit = this->_channels.find(mask);
	if (chanit != this->_channels.end()) // mask matches a channel
	{
		clients = chanit->second.getClients();
		for (clienit = clients.begin(); clienit != clients.end(); ++clienit)
		{
			if (!clienit->second->get_registered())
				continue ;
			flags = "H";
			if (clienit->second->is_away())
    			flags = "G"; // client is away->set flags to G
			if (clienit->second->is_server_op())
				flags += "*"; // client is server op->add *
			if (chanit->second.getChannops().count(clienit->second->get_client_fd()))
				flags += "@"; // client is channop->add @
			info = mask + " " + clienit->second->get_user() + " " + clienit->second->get_host_ip() + " " + this->_name + " " + clienit->second->get_nick() + " " + flags;
			this->send_reply(client, RPL_WHOREPLY, info, "0 " + clienit->second->get_realname()); // send for each client in channel
		}
	}
	else // mask doesn't match a channel
	{
		for (it = this->_clients.begin(); it != this->_clients.end(); ++it)
		{
			if (!it->second.get_registered())
				continue ;
			if (!same_nick(mask, it->second.get_nick()) && !match_wildcard(mask, it->second.get_nick(), 0, 0) && !match_wildcard(mask, it->second.get_host_ip(), 0, 0)
				&& !match_wildcard(mask, this->_name, 0, 0) && !match_wildcard(mask, it->second.get_realname(), 0, 0))
				continue ; // skip clients that don't match mask
			flags = "H";
			if (it->second.is_away())
				flags = "G";
			if (it->second.is_server_op())
				flags += "*";
			info = "* " + it->second.get_user() + " " + it->second.get_host_ip() + " " + this->_name + " " + it->second.get_nick() + " " + flags;
			this->send_reply(client, RPL_WHOREPLY, info, "0 " + it->second.get_realname()); // send for matching client
		}
	}
	this->send_reply(client, RPL_ENDOFWHO, mask, "");
	return (0);
}

int Server::whois_cmd(Client &client, const std::vector<std::string> &params)
{
	std::map<std::string, Channel>::iterator chanit;
	std::map<int, Client>::iterator 		 clienit;
	std::vector<std::string> 				 users;
	std::string 							 channels;
	std::string 							 chan_opers;
	size_t 									 i;

	if (params.empty() || params.size() > 1 || params[0].empty())
		return (ERR_NEEDMOREPARAMS);
	users = ft_split(params[0], ',');
	i = 0;
	while (i < users.size())
	{
		channels.clear();
		chan_opers.clear();
		clienit = this->reverseClientSearch(users[i]);
		if (clienit == this->_clients.end())
		{
			this->send_error(client, ERR_NOSUCHNICK, users[i]);
			this->send_reply(client, RPL_ENDOFWHOIS, users[i], "");
			i++;
			continue ;
		}
		this->send_reply(client, RPL_WHOISUSER, clienit->second.get_nick(), clienit->second.get_user() + " " + clienit->second.get_host_ip() + " * :" + clienit->second.get_realname()); // send user info to client
		this->send_reply(client, RPL_WHOISSERVER, clienit->second.get_nick(), this->_name + " :IRC Server");
		chanit = this->_channels.begin();
		while (chanit != this->_channels.end())
		{
			if (chanit->second.getChannops().find(clienit->first) != chanit->second.getChannops().end()) // client is chanop
			{
				if (!chan_opers.empty())
					chan_opers += " "; // separate channames with space
				chan_opers += "@" + chanit->second.getName(); // add @ to chanop
			}
			else if (chanit->second.getClients().find(clienit->first) != chanit->second.getClients().end()) // client is in channel
			{
				if (!channels.empty())
					channels += " ";
				channels += chanit->second.getName();
			}
			chanit++;
		}
		if (!chan_opers.empty())
			this->send_reply(client, RPL_WHOISCHANNELS, clienit->second.get_nick(), chan_opers); // user is chanop in channels
		if (!channels.empty())
			this->send_reply(client, RPL_WHOISCHANNELS, clienit->second.get_nick(), channels); // user is member in channels
		this->send_reply(client, RPL_ENDOFWHOIS, clienit->second.get_nick(), "");
		i++;
	}
	return (0);
}

int Server::whowas_cmd(Client &client, const std::vector<std::string> &params)
{
	std::map<std::string, t_whowas>::iterator it;
	std::vector<std::string> 				  nicks;
	std::string 							  msg;
	size_t									  i;

	if (params.empty() || params[0].empty())
		return (ERR_NONICKNAMEGIVEN);
	nicks = ft_split(params[0], ',');
	if (nicks.empty())
		return (ERR_NONICKNAMEGIVEN);
	i = 0;
	while (i < nicks.size())
	{
		it = this->_whowas.find(nicks[i]);
		if (it == this->_whowas.end())
			this->send_error(client, ERR_WASNOSUCHNICK, nicks[i]);
		else
		{
			msg = it->second.nick + " " + it->second.user + " " + it->second.host + " *";
			this->send_reply(client, RPL_WHOWASUSER, msg, ":" + it->second.realname); // send whowas info to client
			msg = it->second.nick + " " + this->_name;
			this->send_reply(client, RPL_WHOISSERVER, msg, ":IRC Server");
		}
		this->send_reply(client, RPL_ENDOFWHOWAS, nicks[i], "");
		i++;
	}
	return (0);
}
