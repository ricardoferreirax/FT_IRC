/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ExtraCommands.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/26 16:29:43 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/28 23:25:57 by rmedeiro         ###   ########.fr       */
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

	descripts["PASS"] = "Authenticate with server.";
	descripts["HELP"] = "Display information about IRC commands.";
	parameters["PASS"] = "<password>";
	parameters["HELP"] = "[<subject>]";
	if (params.empty())
		topic = "";
	else
		topic = params[0];
	if (topic.empty())
	{
		this->send_reply(client, RPL_HELPSTART, "*", "** Help System **");
		this->send_reply(client, RPL_HELPTXT, "*", "Use /HELP USERCMDS to list available commands.");
		this->send_reply(client, RPL_ENDOFHELP, "*", "Use /HELP <command> for specific command information.");
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
	std::map<int, Client>::iterator 		 it;
	std::map<int, Client *> 				 chan_clients;
	std::map<int, Client *>::iterator 		 clienit;
	std::string								 target;
	std::string								 msg;
	std::string								 prefix;

	if (params.size() < 2 || params[0].empty() || params[1].empty())
		return (0);
	target = params[0];
	msg = params[1];
	prefix = ":" + client.get_nick() + "!" + client.get_user() + "@" + client.get_host_ip();
	it = this->_clients.begin();
	while (it != this->_clients.end())
	{
		if (same_nick(it->second.get_nick(), target)) // nick matches a client nick
		{
			this->send_msg(it->second, prefix + " NOTICE " + it->second.get_nick() + " :" + msg); // send msg to that client
			return (0);
		}
		it++;
	}
	chanit = this->_channels.find(target);
	if (chanit != this->_channels.end()) // if target matches a channel name
	{
		chan_clients = chanit->second.getClients(); // get all clients in channel
		if (chan_clients.find(client.get_client_fd()) == chan_clients.end()) // if client is not in channel ->ignore notice
			return (0);
		clienit = chan_clients.begin();
		while (clienit != chan_clients.end())
		{
			if (clienit->second->get_client_fd() != client.get_client_fd()) // if client is not who sends ->send msg to that client
				this->send_msg(*(clienit->second), prefix + " NOTICE " + target + " :" + msg);
			clienit++;
		}
	}
	return (0);
}

// ping is used to check if connection between client and server is still active.
int Server::handle_ping(Client &client, const std::vector<std::string> &params)
{
	std::string	token;
	
	if (params.empty() || params[0].empty())
		return (ERR_NOORIGIN);
	token = params[0];
	this->send_msg(client, ":" + this->_name + " PONG " + this->_name + " :" + token);
	return (0);
}

int Server::handle_quit(Client &client, const std::vector<std::string> &params)
{
	std::map<int, Client>::iterator it;
	std::string 					reason;
	std::string 					msg;
	std::string 					client_name;
	int								client_fd;
	
	client_fd = client.get_client_fd();
	if (params.empty() || params[0].empty())
		reason = "Client quit";
	else
		reason = params[0];
	if (client.get_nick().empty())
		client_name = "unregistered client";
	else
		client_name = client.get_nick();
	msg = ":" + client.get_nick() + "!" + client.get_user() + "@" + client.get_host_ip() + " QUIT :" + reason;
	for (it = this->_clients.begin(); it != this->_clients.end(); ++it)
	{
		if (it->first == client_fd)
			continue ; // skip sending QUIT msg to client that sent it
		this->send_msg(it->second, msg); // send QUIT msg to all other clients
	}
	this->send_msg(client, "Closing connection for " + client_name + ": " + reason);
	client.set_quit(true);
	return (0);
}

<<<<<<< HEAD
=======
int Server::handle_fast(Client &client, const std::vector<std::string> &params)
{
	(void)params;
	
	if (!client.get_auth())
		return (ERR_NOTREGISTERED);
	client.set_nick("Nick");
	client.set_user("User");
	client.set_host_ip("localhost");
	return (0);
}

>>>>>>> c94c482 (Made some improvements and fix leak)
// int Server::handle_fast(Client &client, const std::vector<std::string> &params)
// {
// 	(void)params;
//
// 	if (!client.get_auth())
// 		return (ERR_PASSREQUIRED);
// 	client.set_nick("Nick");
// 	client.set_user("User");
// 	client.set_host_ip("localhost");
// 	return (0);
// }

int Server::handle_fast(Client &client, const std::vector<std::string> &params)
{
	std::string to_send;
	(void)params;

	client.set_auth(true);
	client.set_nick(generateRandomString(3));
	client.set_user(generateRandomString(3));
	client.set_registered(true);
	client.set_host_ip(generateRandomString(9));
	to_send += "You shall be named: " + client.get_nick() + "\n";
	to_send += "Your home kingdom shall be: " + client.get_host_ip() + "\n";
	to_send += "Thy user shall be sung as: " + client.get_user() + "\n";
	to_send += "Go forth, make haste, child of Mercury\r\n";
	this->send_msg(client, to_send);
	return (0);
}
