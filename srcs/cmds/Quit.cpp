/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Quit.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/08 21:21:15 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/11/09 21:22:29 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/Server.hpp"
#include "../../includes/Client.hpp"
#include "../../includes/Utils.hpp"

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
