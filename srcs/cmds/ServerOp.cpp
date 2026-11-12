/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerOp.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/09 14:58:44 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/11/12 16:56:01 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/Server.hpp"
#include "../../includes/Client.hpp"
#include "../../includes/Utils.hpp"

int Server::oper_cmd(Client &client, const std::vector<std::string> &params)
{
	std::map<std::string, std::string>::iterator it;
	std::map<int, Client>::iterator 			 clienit;

	if (!client.get_registered())
		return (ERR_NOTREGISTERED);
	if (params.size() < 2 || params[0].empty() || params[1].empty())
		return (ERR_NEEDMOREPARAMS);
	if (client.is_server_op())
	{
		this->send_reply(client, RPL_YOUREOPER, "", "");
		return (0);
	}
	it = this->_opers.find(params[0]);
	if (it == this->_opers.end() || it->second != params[1])
		return (ERR_PASSWDMISMATCH);
	client.set_server_op(true);
	this->send_reply(client, RPL_YOUREOPER, "", "");
	for (clienit = this->_clients.begin(); clienit != this->_clients.end(); ++clienit)
	{
		if (clienit->first != client.get_client_fd() && clienit->second.get_registered() && clienit->second.is_server_op())
		{
			this->send_cmd(client, clienit->second, "NOTICE " + clienit->second.get_nick() + " :" + client.get_nick() + " is now an IRC operator");
		}
	}
	return (0);
}

int Server::kill_cmd(Client &client, const std::vector<std::string> &params)
{
	std::map<int, Client>::iterator it;
	std::vector<std::string> 		quit;
	std::string 					nick;
	std::string 					reason;

	if (!client.is_server_op())
		return (ERR_NOPRIVILEGES);
	if (params.size() < 2 || params[0].empty() || params[1].empty())
		return (ERR_NEEDMOREPARAMS);
	nick = params[0];
	reason = params[1];
	for (it = this->_clients.begin(); it != this->_clients.end(); ++it)
	{
		if (same_nick(it->second.get_nick(), nick))
			break ;
	}
	if (it == this->_clients.end())
		return (ERR_NOSUCHNICK);
	if (it->second.get_quit())
	{
		this->send_notice(client, "Client disconnected");
		return (0);
	}
	if (it->first == client.get_client_fd())
		this->send_notice(client, "You killed yourself");
	this->send_cmd(client, it->second, "KILL " + it->second.get_nick() + " :" + reason);
	quit.push_back("killed by " + client.get_nick());
	this->quit_cmd(it->second, quit);
	// std::cout << "[KILL] " << client.get_nick() << " killed " << it->second.get_nick() << " (" << reason << ")" << std::endl;
	return (0);
}

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
	client.get_send_buffer() += "Closing " + client.get_nick() + " connection :" + reason + "\r\n";
	event.events = EPOLLOUT; // set EPOLLOUT to send quit msg before closing connection
	event.data.fd = client.get_client_fd();
	if (epoll_ctl(this->_epoll_fd, EPOLL_CTL_MOD, client.get_client_fd(), &event) < 0)
		throw std::runtime_error("IRC: epoll_ctl() failed.");
	client.set_quit(true);
	return (0);
}

int Server::restart_cmd(Client &client, const std::vector<std::string> &params)
{
	(void)params;

	if (!client.is_server_op())
		return (ERR_NOPRIVILEGES);
	this->_restart = true;
	return (0);
}

int Server::ping_cmd(Client &client, const std::vector<std::string> &params)
{
	epoll_event event;

	if (params.empty() || params[0].empty())
		return (ERR_NOORIGIN);
	client.get_send_buffer() += ":" + this->_name + " PONG " + this->_name + " :" + params[0] + "\r\n";
	event.events = EPOLLIN | EPOLLOUT;
	event.data.fd = client.get_client_fd();
	if (epoll_ctl(this->_epoll_fd, EPOLL_CTL_MOD, event.data.fd, &event) < 0)
		throw std::runtime_error("IRC: epoll_ctl() failed.");
	return (0);
}
