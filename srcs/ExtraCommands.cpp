/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ExtraCommands.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/26 16:29:43 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/27 10:21:53 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Server.hpp"

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

// int Server::handle_help(Client &client, const std::vector<std::string> &params)
// {
// 	std::string subject = "*";
// 	std::string help;
	
// 	if (!params.empty())
// 		subject = params[0];
// 	if (subject == "*")
// 		help = "Commands: PASS NICK USER PING JOIN PRIVMSG PART INVITE MODE QUIT FAST HELP";
// 	else if (subject == "PASS")
// 		help = "PASS <password>";
// 	else if (subject == "NICK")
// 		help = "NICK <nickname>";
// 	else if (subject == "USER")
// 		help = "USER <username> <mode> <unused> :<realname>";
// 	else if (subject == "PING")
// 		help = "PING <token>";
// 	else if (subject == "JOIN")
// 		help = "JOIN <channel> [key]";
// 	else if (subject == "PRIVMSG")
// 		help = "PRIVMSG <target> :<message>";
// 	else if (subject == "PART")
// 		help = "PART <channel> [:reason]";
// 	else if (subject == "INVITE")
// 		help = "INVITE <nickname> <channel>";
// 	else if (subject == "MODE")
// 		help = "MODE <channel> [modes] [parameters]";
// 	else if (subject == "QUIT")
// 		help = "QUIT [:reason]";
// 	else if (subject == "FAST")
// 		help = "FAST";
// 	else if (subject == "HELP")
// 		help = "HELP [command]";
// 	else
// 		return (ERR_HELPNOTFOUND);
// 	this->send_msg(client, ":" + this->_name + " 704 " + client.get_nick() + " "
// 		+ subject + " :IRC Help");
// 	this->send_msg(client, ":" + this->_name + " 705 " + client.get_nick() + " "
// 		+ subject + " :" + help);
// 	this->send_msg(client, ":" + this->_name + " 706 " + client.get_nick() + " "
// 		+ subject + " :End of HELP");
// 	return (0);
// }