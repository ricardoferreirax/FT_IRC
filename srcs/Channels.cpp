/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channels.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/07 12:50:09 by pedro             #+#    #+#             */
/*   Updated: 2026/10/22 15:50:49 by pedro            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Channels.hpp"
#include "../includes/Server.hpp"
#include "../includes/Utils.hpp"

// Channel::Channel(void)
// {
// 	std::cout << "Were live baby";
// }

Channel::Channel(std::string name, t_cmode mode, Client &chanop) : _name(name),
	_mode(mode)
{
	std::cout << "New Channel " << this->_name << " has been created.";
	this->_chanops.insert(chanop.get_client_fd());
	this->_clients.insert(std::make_pair(chanop.get_client_fd(), &chanop));
}

Channel::~Channel()
{
	std::cout << "Channel " << this->_name << " was destroyed" << std::endl;
}

int Channel::joinChannel(Client &guest, std::string key = "")
{
	if(this->_clients.find(guest.get_client_fd()) != this->_clients.end())
		return 1;
	if(this->_mode.users.u_limit != 0 && this->_mode.users.u_count + 1 > this->_mode.users.u_limit )
		return 2;
	if(this->_mode.invite_only && !(this->_invites.find(guest.get_client_fd()) != this->_invites.end()))
		return 3;
	if(!this->_mode.key.empty() && this->_mode.key != key)
		return 4;
	this->_clients.insert(std::make_pair(guest.get_client_fd(), &guest));
	this->_invites.erase(guest.get_client_fd());
	this->_mode.users.u_count++;
	std::cout << "Client has joined the channel" << std::endl;
	return 0;
}

int Channel::leaveChannel(Client guest)
{
	this->_clients.erase(guest.get_client_fd());
	this->_chanops.erase(guest.get_client_fd());
	this->_mode.users.u_count--;
	return 0;
}

bool Channel::inviteUser(Client guest)
{
	this->_invites.insert(guest.get_client_fd());
	return true;
}

std::string	Channel::getName(void) const
{
	return (this->_name);
}

std::set<int> Channel::getChannops(void) const
{
	return (this->_chanops);
}


std::map<int, Client*> Channel::getClients(void) const
{
	return (this->_clients);
}

std::set<int> Channel::getInvites(void) const
{
	return (this->_invites);
}


t_cmode Channel::getMode(void) const
{
	return (this->_mode);
}


void Channel::setMode(bool invite, bool topic_restricted, std::string topic, t_ulimit users, std::string key)
{
	this->_mode.invite_only = invite;
	this->_mode.topic_restricted = topic_restricted;
	this->_mode.topic = topic;
	this->_mode.users = users;
	this->_mode.key = key;
}
