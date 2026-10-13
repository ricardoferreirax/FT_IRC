/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channels.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pedro </var/spool/mail/pedro>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/07 12:50:09 by pedro             #+#    #+#             */
/*   Updated: 2026/10/13 13:22:31 by pedro            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Channels.hpp"
#include "../includes/Server.hpp"

// Channel::Channel(void)
// {
// 	std::cout << "Were live baby";
// }

Channel::Channel(std::string name, t_cmode mode, Client chanop, Server &serv) : _name(name),
	_mode(mode), _serv(serv)
{
	std::cout << "New Channel " << this->_name << " has been created.";
	this->_chanops.insert(chanop.get_client_fd());
	this->_clients.insert(std::make_pair(chanop.get_client_fd(), chanop));
}

bool Channel::joinChannel(Client guest, std::string key)
{
	std::string &to_send = guest.get_send_buffer();

	if(!guest.is_registered())
	{
		to_send += "NOTICE: Client is not registered\r\n";
		this->_serv.update_epoll(guest.get_client_fd(), EPOLLIN | EPOLLOUT);
		return false;
	}
	if(this->_clients.find(guest.get_client_fd()) != this->_clients.end())
	{
		to_send += "NOTICE: User is already in Channel\r\n";
		this->_serv.update_epoll(guest.get_client_fd(), EPOLLIN | EPOLLOUT);
		return false;
	}
	if(this->_mode.users.u_count + 1 > this->_mode.users.u_limit)
	{
		to_send += ":" + this->_serv.getName() + " XXX *: #" + this->_name + " This place is full, get yo ass outta here\r\n";
		this->_serv.update_epoll(guest.get_client_fd(), EPOLLIN | EPOLLOUT);
		return false;
	}
	if(this->_mode.invite_only && !(this->_invites.find(guest.get_client_fd()) != this->_invites.end()))
	{
		to_send += ":" + this->_serv.getName() + " 473 *: #" + this->_name + " You weren't invited to the boys club. Access denied\r\n";
		this->_serv.update_epoll(guest.get_client_fd(), EPOLLIN | EPOLLOUT);
		return false;
	}
	if(!this->_mode.key.empty() && this->_mode.key != key)
	{
		to_send = ":" + this->_serv.getName() + " XXX *: #" + this->_name + " What's the pass? This is boys only club\r\n";
		this->_serv.update_epoll(guest.get_client_fd(), EPOLLIN | EPOLLOUT);
		return false;
	}
	this->_clients.insert(std::make_pair(guest.get_client_fd(), guest));
	to_send = ":" + this->_serv.getName() + " XX *: you joined #" + this->_name;
	this->_serv.update_epoll(guest.get_client_fd(), EPOLLIN | EPOLLOUT);
	return true;
}
