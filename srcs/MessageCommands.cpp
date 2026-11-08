/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   MessageCommands.cpp                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/03 13:41:14 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/11/08 18:55:54 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Server.hpp"
#include "../includes/Client.hpp"
#include "../includes/Utils.hpp"

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

int Server::help_cmd(Client &client, const std::vector<std::string> &params)
{
	std::map<std::string, std::string>::iterator it;
	std::map<std::string, std::string> 			 descripts;
	std::map<std::string, std::string> 			 parameters;
	std::string 								 cmd;

	descripts["PASS"] = "Authenticate with server using a password. Must be sent before registration.";
	parameters["PASS"] = "<password>";
	descripts["NICK"] = "Set/change nickname on server. NICK is used by other users to send privmsgs or/and invite to channels.";
	parameters["NICK"] = "<nickname>";
	descripts["USER"] = "Provide username and real name. Along with PASS and NICK, USER completes registration.";
	parameters["USER"] = "<username> <mode> <unused> :<realname>";
	descripts["PRIVMSG"] = "Send private message to another user or to all members of a channel. Can send to several users/channels.";
	parameters["PRIVMSG"] = "<nickname|channel>{,<nickname|channel>} :<message>";
	descripts["JOIN"] = "Join one or more channels. If a channel doesn't exist, it'll be created. Some channels require a password (key) or an invitation.";
	parameters["JOIN"] = "<channel>{,<channel>} [<key>{,<key>}]";
	descripts["QUIT"] = "Disconnect from server and notify other user(s) who share same channel(s). Can provide a reason.";
	parameters["QUIT"] = "[:<reason>]";
	descripts["INVITE"] = "Invite another user to join a specific channel. It's useful for invite only channels, where users need invitation before they can join.";
	parameters["INVITE"] = "<nickname> <channel>";
	descripts["MODE"] = "View/modify user and channel modes. Can control invite (+i), pass (+k), user limits (+l), topics (+t) and operators (+o). Use + to enable a mode or - to disable it.";
	parameters["MODE"] = "<channel> [<+|-modes> [<modeparams>]]";
	descripts["PART"] = "Leave one or more channels without disconnecting from server. Other members will be notified, and may include an parting message.";
	parameters["PART"] = "<channel>{,<channel>} [:<part message>]";
	descripts["KICK"] = "Remove users from a channel. Only channops can use this command. An optional reason can be provided for the kick.";
	parameters["KICK"] = "<channel>{,<channel>} <user>{,<user>} [<comment>]";
	descripts["TOPIC"] = "View/modify the topic of a channel. If no topic is set, server will notify the user. Only channops can change the topic if the channel is restricted.";
	parameters["TOPIC"] = "<channel> [<topic>]";
	descripts["WHO"] = "Display info about users matching a nickname/channel/search mask. Use * to search for multiple users or omit mask to list users.";
	parameters["WHO"] = "[<mask> [o]]";
	descripts["AWAY"] = "Mark as away from server by setting an absence message. Use AWAY w/out message to remove away status.";
	parameters["AWAY"] = "[:<message>]";
	descripts["OPER"] = "Authenticate as an IRC operator. Operators have special privileges to manage users and channels.";
	parameters["OPER"] = "<user> <password>";
	descripts["PING"] = "Check if client-server connection is still active. Server responds with PONG containing same token.";
	parameters["PING"] = "<token>";
	descripts["HELP"] = "Display info about IRC commands. Specify command to view its description or use without params to list available commands.";
	parameters["HELP"] = "[<command|USERCMDS>]";
	if (params.empty()) 
		cmd = "";
	else
	{
		cmd = params[0];
		for (size_t i = 0; i < cmd.size(); i++)
			cmd[i] = std::toupper(cmd[i]);
	}
	if (cmd.empty()) // no cmd-> send general help
	{
		this->send_reply(client, RPL_HELPSTART, "*", "** Help System **");
		this->send_reply(client, RPL_HELPTXT, "*", "Use HELP USERCMDS to list available commands.");
		this->send_reply(client, RPL_ENDOFHELP, "*", "Use HELP <command> for specific command information.");
		return (0);
	}
	if (cmd == "USERCMDS")
	{
		this->send_reply(client, RPL_HELPSTART, cmd, "** Available IRC Commands **");
		for (it = parameters.begin(); it != parameters.end(); ++it)
			this->send_reply(client, RPL_HELPTXT, cmd, it->first + " " + it->second); // send each cmd with its parameters
		this->send_reply(client, RPL_ENDOFHELP, cmd, "End of command list.");
		return (0);
	}
	if (descripts.find(cmd) == descripts.end())
		return (ERR_HELPNOTFOUND);
	this->send_reply(client, RPL_HELPSTART, cmd, "** " + cmd + " command **");
	this->send_reply(client, RPL_HELPTXT, cmd, descripts[cmd]);
	this->send_reply(client, RPL_ENDOFHELP, cmd, cmd + " " + parameters[cmd]);
	return (0);
}

int Server::away_cmd(Client &client, const std::vector<std::string> &params)
{
	if (params.empty() || params[0].empty()) // no away msg provided
	{
		client.set_away_msg("");
		this->send_reply(client, RPL_UNAWAY, "", "");
	}
	else // away msg provided
	{
		client.set_away_msg(params[0]);
		this->send_reply(client, RPL_NOWAWAY, "", "");
	}
	return (0);
}
