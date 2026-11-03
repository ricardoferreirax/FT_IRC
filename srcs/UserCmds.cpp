/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   UserCmds.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/03 13:25:22 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/11/03 13:42:17 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

# include "../includes/Server.hpp"
# include "../includes/Client.hpp"
# include "../includes/Utils.hpp"

int Server::handle_who(Client &client, const std::vector<std::string> &params)
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
				flags = "G";
			if (chanit->second.getChannops().count(clienit->second->get_client_fd()))
				flags += "@"; // if client is channop->add @
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
			if (!match_nick_mask(it->second.get_nick(), mask) && it->second.get_host_ip() != mask 
				&& this->_name != mask && it->second.get_realname() != mask)
				continue ; // skip clients that don't match mask format
			flags = "H";
			if (it->second.is_away())
				flags = "G";
			info = "* " + it->second.get_user() + " " + it->second.get_host_ip() + " " + this->_name + " " + it->second.get_nick() + " " + flags;
			this->send_reply(client, RPL_WHOREPLY, info, "0 " + it->second.get_realname()); // send for matching client
		}
	}
	this->send_reply(client, RPL_ENDOFWHO, mask, "");
	return (0);
}

int Server::handle_help(Client &client, const std::vector<std::string> &params)
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
	parameters["MODE"] = "<nickname|channel> [<+|-modes> [<modeparams>]]";
	descripts["PART"] = "Leave one or more channels without disconnecting from server. Other members will be notified, and may include an parting message.";
	parameters["PART"] = "<channel>{,<channel>} [:<part message>]";
	descripts["NOTICE"] = "Send info message to user/channel. Unlike PRIVMSG, NOTICE shouldn't create automatic error replies, it's useful for automated messages.";
	parameters["NOTICE"] = "<nickname|channel>{,<nickname|channel>} :<message>";
	descripts["WHO"] = "Display info about users matching a nickname/channel/search mask. Use * to search for multiple users or omit mask to list users.";
	parameters["WHO"] = "[<mask> [o]]";
	descripts["AWAY"] = "Mark as away from server by setting an absence message. Use AWAY w/out message to remove away status.";
	parameters["AWAY"] = "[:<message>]";
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

int Server::handle_away(Client &client, const std::vector<std::string> &params)
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

int Server::handle_fast(Client &client, const std::vector<std::string> &params)
{
	(void)params;

	if (!client.get_auth())
		return (ERR_PASSREQUIRED);
	client.set_nick("Nick");
	client.set_user("User");
	client.set_host_ip("localhost");
	return (0);
}

// int Server::handle_fast(Client &client, const std::vector<std::string> &params)
// {
// 	std::string to_send;
// 	(void)params;

// 	client.set_auth(true);
// 	client.set_nick(generateRandomString(3));
// 	client.set_user(generateRandomString(3));
// 	client.set_registered(true);
// 	client.set_host_ip(generateRandomString(9));
// 	to_send += "You shall be named: " + client.get_nick() + "\n";
// 	to_send += "Your home kingdom shall be: " + client.get_host_ip() + "\n";
// 	to_send += "Thy user shall be sung as: " + client.get_user() + "\n";
// 	to_send += "Go forth, make haste, child of Mercury\r\n";
// 	this->send_msg(client, to_send);
// 	return (0);
// }