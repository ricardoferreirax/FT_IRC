/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Message.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/03 13:41:14 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/11/03 16:40:55 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

# include "../includes/Server.hpp"
# include "../includes/Client.hpp"
# include "../includes/Utils.hpp"

void Server::execute_msgs(Client &client, const std::string &cmd, const std::vector<std::string> &params)
{
	typedef int (Server::*MsgCmd)(Client &, const std::vector<std::string> &);
	std::map<std::string, MsgCmd>::iterator it;
	std::map<std::string, MsgCmd> 			msgs;
	std::string 							param;
	int 									ret;

	msgs["PRIVMSG"] = &Server::privmsg_cmd;
	msgs["NOTICE"] = &Server::notice_cmd;
	it = msgs.find(cmd);
	if (it == msgs.end())
	{
		this->send_error(client, ERR_UNKNOWNCOMMAND, cmd);
		return ;
	}
	if (!client.get_registered())
	{
		if (cmd != "NOTICE")
			this->send_error(client, ERR_NOTREGISTERED, cmd);
		return ;
	}
	ret = (this->*(it->second))(client, params);
	if (ret != 0)
	{
		if (cmd == "NOTICE")
			return ;
		param = cmd;
		if (!params.empty() && ret == ERR_NOSUCHNICK)
			param = params[0];
		this->send_error(client, ret, param);
	}
}

int Server::privmsg_cmd(Client &client, const std::vector<std::string> &params)
{
	std::map<std::string, Channel>::iterator chanit;
	std::map<int, Client *>::iterator 		 clienit;
	std::map<int, Client *> 				 chan_clients;
	std::map<int, Client>::iterator 		 it;
	std::string 							 target;
	std::string 							 msg;

	if (params.empty() || params[0].empty())
		return (ERR_NORECIPIENT);
	if (params.size() < 2 || params[1].empty())
		return (ERR_NOTEXTTOSEND);
	target = params[0];
	msg = params[1];
	if (same_nick(target, "Miku"))
	{
		try
		{
			if (!this->_bot.start_request(client.get_client_fd(), client.get_nick(), "", msg))
				this->send_notice(client, MIKUUNAVAILABLE, "");
		}
		catch (const std::exception &e)
		{
			std::cerr << "Miku: " << e.what() << std::endl;
			this->send_notice(client, MIKUUNAVAILABLE, "");
		}
		return (0);
	}
	for (it = this->_clients.begin(); it != this->_clients.end(); ++it)
	{
		if (same_nick(it->second.get_nick(), target))
		{
			this->send_cmd(client, it->second, "PRIVMSG " + it->second.get_nick() + " :" + msg); // if target matches a client nick->send msg to target client
			if (it->second.is_away())
				this->send_reply(client, RPL_AWAY, it->second.get_nick(), it->second.get_away_msg()); // if target client is away->send away msg to sender
			return (0);
		}
	}
	chanit = this->_channels.find(target);
	if (chanit == this->_channels.end())
		return (ERR_NOSUCHNICK);
	chan_clients = chanit->second.getClients();
	for (clienit = chan_clients.begin(); clienit != chan_clients.end(); ++clienit)
	{
		if (clienit->first != client.get_client_fd())
			this->send_cmd(client, *(clienit->second), "PRIVMSG " + target + " :" + msg); // if target is in channel->send msg to all clients in channel except sender
	}
	return (0);
}

// used to send a notice msg to a specific client or all clients in a channel.
int Server::notice_cmd(Client &client, const std::vector<std::string> &params)
{
	std::map<std::string, Channel>::iterator chanit;
	std::map<int, Client *> 				 chan_clients;
	std::map<int, Client *>::iterator 		 clienit;
	std::map<int, Client>::iterator 		 it;
	std::string 							 target;
	std::string 							 msg;

	if (params.size() < 2 || params[0].empty() || params[1].empty()) // check target and msg are provided
		return (0);
	target = params[0];
	msg = params[1];
	for (it = this->_clients.begin(); it != this->_clients.end(); ++it)
	{
		if (same_nick(it->second.get_nick(), target)) // target matches a client nick
		{
			this->send_cmd(client, it->second, "NOTICE " + it->second.get_nick() + " :" + msg); // send notice to target client
			return (0);
		}
	}
	chanit = this->_channels.find(target);
	if (chanit == this->_channels.end()) // target isn't in channel
		return (0);
	chan_clients = chanit->second.getClients();
	if (chan_clients.find(client.get_client_fd()) == chan_clients.end()) // client isn't part of channel
		return (0);
	for (clienit = chan_clients.begin(); clienit != chan_clients.end(); ++clienit) // iterate through all clients in channel
	{
		if (clienit->first != client.get_client_fd())
			this->send_cmd(client, *(clienit->second), "NOTICE " + target + " :" + msg);
	}
	return (0);
}
