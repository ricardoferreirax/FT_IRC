/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channels.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/07 12:50:09 by pedro             #+#    #+#             */
/*   Updated: 2026/11/10 21:02:04 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Channels.hpp"
#include "../includes/Server.hpp"
#include "../includes/Utils.hpp"

// Channel::Channel(void)
// {
// 	std::cout << "Were live baby";
// }

Channel::Channel(std::string name, t_cmode mode, Client &chanop) : _name(name), _mode(mode)
{
	this->_chanops.insert(chanop.get_client_fd());
	this->_clients.insert(std::make_pair(chanop.get_client_fd(), &chanop));
}

Channel::~Channel()
{
	
}

std::string Channel::getName(void) const
{
	return (this->_name);
}

std::map<int, Client *> &Channel::getClients(void)
{
	return (this->_clients);
}

std::set<int> &Channel::getChannops(void)
{
	return (this->_chanops);
}

std::set<int> &Channel::getInvites(void)
{
	return (this->_invites);
}

t_cmode Channel::getMode(void) const
{
	return (this->_mode);
}

std::vector<t_banmask> Channel::getBanmask(void)
{
	return (this->_bans);
}

bool Server::check_allchans(const std::vector<std::string> &chans, Client *client)
{
	size_t	i;

	i = 0;
	while (i < chans.size())
	{
		if (this->_channels.find(chans[i]) == this->_channels.end())
		{
			if (client != NULL)
				this->send_error(*client, ERR_NOSUCHCHANNEL, chans[i] + ", no action was taken");
			return (false);
		}
		i++;
	}
	return (true);
}

bool Channel::addBanMask(std::string banmask)
{
	t_banmask	ban;

	ban = parseBanMask(banmask);
	if (ban.valid)
		this->_bans.push_back(ban);
	return (ban.valid);
}

void Channel::removeBanMask(std::string banmask)
{
	std::vector<t_banmask>::iterator bannit;
	t_banmask						 target;

	target = parseBanMask(banmask);
	bannit = this->_bans.begin();
	while (bannit != this->_bans.end())
	{
		if (bannit->nick == target.nick && bannit->user == target.user && bannit->host == target.host)
			bannit = this->_bans.erase(bannit);
		else
			bannit++;
	}
}

int Channel::PAC_check(Client &guest)
{
	std::vector<t_banmask>::iterator bannit;
	
	bannit = this->_bans.begin();
	while (bannit != this->_bans.end())
	{
		if (match_ban(guest, *bannit))
			return (-1);
		bannit++;
	}
	return (0);
}

int Channel::joinChannel(Client &guest, std::string key)
{
	if (this->_clients.find(guest.get_client_fd()) != this->_clients.end())
		return (1);
	if (this->_mode.users.u_limit != 0 && this->_mode.users.u_count + 1 > this->_mode.users.u_limit)
		return (2);
	if (this->_mode.invite_only && !(this->_invites.find(guest.get_client_fd()) != this->_invites.end()))
		return (3);
	if (!this->_mode.key.empty() && this->_mode.key != key)
		return (4);
	if (this->PAC_check(guest))
		return (5);
	this->_clients.insert(std::make_pair(guest.get_client_fd(), &guest));
	this->_invites.erase(guest.get_client_fd());
	this->_mode.users.u_count++;
	// std::cout << "Client has joined the channel" << std::endl;
	return (0);
}

/**
 * @brief Makes a client leave the channel
 * @return Returns the new channop FD if the use that left was the last channop, -1 if there's no more user in the channel, otherwise it retunrs 0
 */
int Channel::leaveChannel(Client &guest)
{
	std::map<int, Client *>::iterator clienit;
	int 							  ret_val;

	ret_val = 0;
	if (this->_clients.find(guest.get_client_fd()) == this->_clients.end())
		return (0);
	this->_clients.erase(guest.get_client_fd());
	this->_chanops.erase(guest.get_client_fd());
	this->getInvites().erase(guest.get_client_fd());
	this->_mode.users.u_count--;
	if (this->_clients.size() <= 0)
	{
		this->_mode.users.u_count = 0;
		return (-1);
	}
	if (this->_chanops.size() <= 0)
	{
		clienit = this->_clients.begin();
		this->_chanops.insert(clienit->first);
		ret_val = clienit->first;
	}
	if (this->_mode.users.u_count <= 0)
		return (-1);
	return (ret_val);
}

bool Channel::inviteUser(Client &guest)
{
	this->_invites.insert(guest.get_client_fd());
	return (true);
}

void Channel::setMode(bool invite, bool topic_restricted, std::string topic, t_ulimit users, std::string key)
{
	this->_mode.invite_only = invite;
	this->_mode.topic_restricted = topic_restricted;
	this->_mode.topic = topic;
	this->_mode.users = users;
	this->_mode.key = key;
}
