/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerCommands.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/03 13:39:00 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/11/05 22:48:37 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Server.hpp"
#include "../includes/Client.hpp"
#include "../includes/Utils.hpp"

int Server::quit_cmd(Client &client, const std::vector<std::string> &params)
{
	std::map<std::string, Channel>::iterator chanit;
	std::map<int, Client>::iterator 		 it;
	std::string 							 reason;
	epoll_event 							 event;

	if (params.empty() || params[0].empty()) // no reason provided
		reason = "Client quit";
	else
		reason = params[0];
	for (it = this->_clients.begin(); it != this->_clients.end(); ++it)
	{
		if (it->first == client.get_client_fd())
			continue ; // skip client who quit
		for (chanit = this->_channels.begin(); chanit != this->_channels.end(); ++chanit) // iterate through all channels
		{
			if (chanit->second.getClients().count(client.get_client_fd()) && chanit->second.getClients().count(it->first))
			{
				this->send_cmd(client, it->second, "QUIT :" + reason); // if both clients are in same channel->send quit to other client
				break ;
			}
		}
	}
	client.get_send_buffer() += "Closing connection: " + client.get_nick() + " (" + reason + ")\r\n";
	event.events = EPOLLOUT; // set EPOLLOUT to send quit msg before closing connection
	event.data.fd = client.get_client_fd();
	if (epoll_ctl(this->_epoll_fd, EPOLL_CTL_MOD, client.get_client_fd(), &event) < 0)
		throw std::runtime_error("IRC: epoll_ctl() failed.");
	client.set_quit(true);
	return (0);
}

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

// ping is used to check if connection between client and server is still active.
int Server::ping_cmd(Client &client, const std::vector<std::string> &params)
{
	if (params.empty() || params[0].empty())
		return (ERR_NOORIGIN);
	this->send_notice(client, PONGREPLY, params[0]);
	return (0);
}

int Server::oper_cmd(Client &client, const std::vector<std::string> &params)
{
	std::map<std::string, std::string>::iterator it;

	if (params.size() < 2 || params[0].empty() || params[1].empty())
		return (ERR_NEEDMOREPARAMS);
	it = this->_opers.find(params[0]);
	if (it == this->_opers.end())
		return (ERR_NOOPERHOST);
	if (it->second != params[1])
		return (ERR_PASSWDMISMATCH);
	client.set_server_op(true);
	this->send_reply(client, RPL_YOUREOPER, "", "");
	return (0);
}
