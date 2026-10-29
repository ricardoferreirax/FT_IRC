/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ExtraCommands.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/26 16:29:43 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/29 10:43:13 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

# include "../includes/Server.hpp"
# include "../includes/Utils.hpp"

int Server::handle_help(Client &client, const std::vector<std::string> &params)
{
	std::map<std::string, std::string>::iterator it;
	std::map<std::string, std::string> 			 descripts;
	std::map<std::string, std::string> 			 parameters;
	std::string 								 topic;

	descripts["PASS"] = "Authenticate with server using a password before registering.";
	parameters["PASS"] = "<password>";
	descripts["NICK"] = "Set/change your nickname which will be used to identify you to other users on server.";
	parameters["NICK"] = "<nickname>";
	descripts["USER"] = "Set your username and real name to complete the registration.";
	parameters["USER"] = "<username> <mode> <unused> :<realname>";
	descripts["PRIVMSG"] = "Send a message to another user or  to all members of a channel.";
	parameters["PRIVMSG"] = "<nickname|channel> :<message>";
	descripts["JOIN"] = "Join an existing channel or create a new one if it doesn't exist. A key may be required to join certain channels.";
	parameters["JOIN"] = "<channel>[,<channel>...] [<key>[,<key>...]]";
	descripts["QUIT"] = "Disconnects from server and optionally sendes a reason for disconnection.";
	parameters["QUIT"] = "[:<reason>]";
	descripts["INVITE"] = "Invite another user to a channel.";
	parameters["INVITE"] = "<nickname> <channel>";
	descripts["MODE"] = "View or change channel/user modes, including invite access, keys, user limits and operators.";
	parameters["MODE"] = "<channel> [<+|-mode> [<parameter>]]";
	descripts["PART"] = "Leave one or more channels without disconnecting from server. An optional reason may be provided.";
	parameters["PART"] = "<channel>[,<channel>...] [:<reason>]";
	descripts["NOTICE"] = "Send a informational message to a user or channel without expecting an automatic error reply.";
	parameters["NOTICE"] = "<nickname|channel> :<message>";
	descripts["PING"] = "Check if the connection to the server is still active.";
	parameters["PING"] = "<token>";
	descripts["HELP"] = "Display available IRC commands, their descriptions and parameters required to use them.";
	parameters["HELP"] = "[<command|USERCMDS>]";
	if (params.empty())
	topic = "";
	else
	{
		topic = params[0];
		for (size_t i = 0; i < topic.size(); i++)
			topic[i] = std::toupper(static_cast<unsigned char>(topic[i]));
	}
	if (topic.empty())
	{
		this->send_reply(client, RPL_HELPSTART, "*", "** Help System **");
		this->send_reply(client, RPL_HELPTXT, "*", "Use HELP USERCMDS to list available commands.");
		this->send_reply(client, RPL_ENDOFHELP, "*", "Use HELP <command> for specific command information.");
		return (0);
	}
	if (topic == "USERCMDS")
	{
		this->send_reply(client, RPL_HELPSTART, topic, "** Available IRC Commands **");
		for (it = parameters.begin(); it != parameters.end(); ++it)
			this->send_reply(client, RPL_HELPTXT, topic, it->first + " " + it->second);
		this->send_reply(client, RPL_ENDOFHELP, topic, "End of command list.");
		return (0);
	}
	if (descripts.find(topic) == descripts.end())
		return (ERR_HELPNOTFOUND);
	this->send_reply(client, RPL_HELPSTART, topic, "** " + topic + " command **");
	this->send_reply(client, RPL_HELPTXT, topic, descripts[topic]);
	this->send_reply(client, RPL_ENDOFHELP, topic, topic + " " + parameters[topic]);
	return (0);
}

// used to send a notice msg to a specific client or all clients in a channel.
int Server::handle_notice(Client &client, const std::vector<std::string> &params)
{
	std::map<std::string, Channel>::iterator chanit;
	std::map<int, Client *> 				 chan_clients;
	std::map<int, Client *>::iterator 		 clienit;
	std::map<int, Client>::iterator 		 it;
	std::string 							 target;
	std::string 							 msg;

	if (params.size() < 2 || params[0].empty() || params[1].empty())
		return (0);
	target = params[0];
	msg = params[1];
	for (it = this->_clients.begin(); it != this->_clients.end(); ++it)
	{
		if (same_nick(it->second.get_nick(), target))
		{
			this->send_cmd(client, it->second, "NOTICE " + it->second.get_nick() + " :" + msg);
			return (0);
		}
	}
	chanit = this->_channels.find(target);
	if (chanit == this->_channels.end())
		return (0);
	chan_clients = chanit->second.getClients();
	if (chan_clients.find(client.get_client_fd()) == chan_clients.end())
		return (0);
	for (clienit = chan_clients.begin(); clienit != chan_clients.end(); ++clienit)
	{
		if (clienit->first != client.get_client_fd())
			this->send_cmd(client, *(clienit->second), "NOTICE " + target + " :" + msg);
	}
	return (0);
}

// ping is used to check if connection between client and server is still active.
int Server::handle_ping(Client &client, const std::vector<std::string> &params)
{
	if (params.empty() || params[0].empty())
		return (ERR_NOORIGIN);
	this->send_notice(client, PONGREPLY, params[0]);
	return (0);
}

int Server::handle_quit(Client &client, const std::vector<std::string> &params)
{
	std::map<std::string, Channel>::iterator chanit;
	std::map<int, Client>::iterator 		 it;
	std::string 							 reason;

	if (params.empty() || params[0].empty())
		reason = "Client quit";
	else
		reason = params[0];
	for (it = this->_clients.begin(); it != this->_clients.end(); ++it)
	{
		if (it->first == client.get_client_fd())
			continue ;
		for (chanit = this->_channels.begin(); chanit != this->_channels.end(); ++chanit)
		{
			if (chanit->second.getClients().count(client.get_client_fd()) && chanit->second.getClients().count(it->first))
			{
				this->send_cmd(client, it->second, "QUIT :" + reason);
				break ;
			}
		}
	}
	client.set_quit(true);
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
