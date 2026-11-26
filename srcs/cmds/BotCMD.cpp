/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   BotCMD.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pedro <pfreire-@student.42lisboa.com>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/25 17:28:35 by pedro             #+#    #+#             */
/*   Updated: 2026/11/25 17:54:09 by pedro            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../bonus/includes/Bot.hpp"
#include "../../includes/Server.hpp"

int Server::botadd_cmd(Client &client, const std::vector<std::string> &params)
{
	if (!client.is_server_op())
		return (ERR_NOPRIVILEGES);
	if (params.size() < 2)
		return (ERR_NEEDMOREPARAMS);
	if (!is_valid_nick(params[0]) || this->isBotNick(params[0]))
		return (ERR_NICKNAMEINUSE);
	try
	{
		Bot newBot(params[0], params[1]); // name, filepath, color
		newBot.set_epoll_fd(this->_epoll_fd);
		this->_bots.insert(std::make_pair(str_lower(params[0]), newBot));
	}
	catch (const std::exception &e)
	{
		this->send_notice(client, "Failed to add bot: " + std::string(e.what()));
		return (0);
	}
	this->send_notice(client, "Bot " + params[0] + " added.");
	return (0);
}

int Server::botdel_cmd(Client &client, const std::vector<std::string> &params)
{
	if (!client.is_server_op())
		return (ERR_NOPRIVILEGES);
	if (params.empty() || params[0].empty())
		return (ERR_NEEDMOREPARAMS);
	std::map<std::string, Bot>::iterator botit = this->_bots.find(str_lower(params[0]));
	if (botit == this->_bots.end())
		return (ERR_NOSUCHNICK);
	botit->second.cleanup(BOT_REMOVED, -1, this);
	this->_bots.erase(botit);
	this->send_notice(client, "Bot " + params[0] + " removed.");
	return (0);
}

int Server::botlist_cmd(Client &client, const std::vector<std::string> &params)
{
	std::string reply;
	(void)params;
	std::map<std::string, Bot>::iterator botit = this->_bots.begin();
	while (botit != this->_bots.end())
	{
		reply += botit->second.getName();
		reply += '\n';
		botit++;
	}
	reply += ":End of /BOTLIST List \n";
	this->send_notice(client, reply);
	return (0);
}
