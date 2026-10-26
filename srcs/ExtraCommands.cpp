/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ExtraCommands.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/26 16:29:43 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/26 16:30:38 by rmedeiro         ###   ########.fr       */
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
