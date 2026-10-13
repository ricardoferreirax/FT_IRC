/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channels.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pedro </var/spool/mail/pedro>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/07 12:50:09 by pedro             #+#    #+#             */
/*   Updated: 2026/10/07 14:18:26 by pedro            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Channels.hpp"
#include "../includes/Server.hpp"

Channel::Channel(std::string name, t_cmode mode, Client chanop) : _name(name),
	_mode(mode)
{
	std::cout << "New Channel " << this->_name << " has been created.";
	this->_chanops.insert(chanop.get_fd());
	this->_clients.insert(std::make_pair(chanop.get_fd(), chanop));
}

bool Channel::joinChannel(Client guest)
{
	if(!guest.is_registered())
	{
		send(guest.get_fd(), "Client is not registered\r\n", 27, 0);
	}
	if(this->_clients.find(guest.get_fd()) != this->_clients.end())
	{
		std::cout << "User already in Channel #" << this->_name << std::endl;
		return false;
	}
	if(this->_mode.invite_only && !(this->_invites.find(guest.get_fd()) != this->_invites.end()))
	{
		std::cout << "You weren't invited to the boys club. Access denied\n";
		return false;
	}
	return true;
}
