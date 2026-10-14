/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channels.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/07 12:50:09 by pedro             #+#    #+#             */
/*   Updated: 2026/10/14 17:55:10 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Channels.hpp"
#include "../includes/Server.hpp"
#include "../includes/Utils.hpp"

// Channel::Channel(void)
// {
// 	std::cout << "Were live baby";
// }

Channel::Channel(std::string name, t_cmode mode, Client chanop) : _name(name), _mode(mode)
{
	std::cout << "New Channel " << this->_name << " has been created.";
	this->_chanops.insert(chanop.get_client_fd());
	this->_clients.insert(std::make_pair(chanop.get_client_fd(), chanop));
}

Channel::~Channel()
{
}

int Channel::joinChannel(Client guest, std::string key)
{
	if(this->_clients.find(guest.get_client_fd()) != this->_clients.end())
		return 1;
	if(this->_mode.users.u_count + 1 > this->_mode.users.u_limit && this->_mode.users.u_limit != 0)
		return 2;
	if(this->_mode.invite_only && !(this->_invites.find(guest.get_client_fd()) != this->_invites.end()))
		return 3;
	if(!this->_mode.key.empty() && this->_mode.key != key)
		return 4;
	this->_clients.insert(std::make_pair(guest.get_client_fd(), guest));
	return 0;
}
