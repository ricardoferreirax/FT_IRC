/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Invite.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/08 21:13:50 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/11/08 21:15:41 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/Server.hpp"
#include "../../includes/Client.hpp"
#include "../../includes/Utils.hpp"

int Server::invite_cmd(Client &client, const std::vector<std::string> &params)
{
	std::map<int, Client>::iterator clienit;
	std::map<std::string, Channel>::iterator chanit;
	
	if (params.size() < 2 || params[0].empty() || params[1].empty())
		return (ERR_NEEDMOREPARAMS);
	clienit = this->reverseClientSearch(params[0]); // find client by nick
	if (clienit == this->_clients.end())
		return (ERR_NOSUCHNICK);
	chanit = this->_channels.find(params[1]); // find channel by name
	if (chanit == this->_channels.end())
		return (ERR_NOSUCHCHANNEL);
	if (!chanit->second.getClients().count(client.get_client_fd()))
		return (ERR_NOTONCHANNEL);
	if (chanit->second.getClients().count(clienit->first))
		return (ERR_USERONCHANNEL); // user is already in channel
	if (chanit->second.getMode().invite_only && !chanit->second.getChannops().count(client.get_client_fd()))
		return (ERR_CHANOPRIVSNEEDED); // user is not a chanop and channel is invite only
	chanit->second.inviteUser(clienit->second); // invite user to channel
	this->send_cmd(client, clienit->second, "INVITE " + params[0] + " :" + params[1]);
	this->send_reply(client, RPL_INVITING, params[0] + " " + params[1], "");
	return (0);
}
