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

#include "../includes/Server.hpp"
#include "../includes/Channels.hpp"

Channel::Channel(std::string name, int mode, Client chanop, Server serv) :
	_name(name), _mode(mode), _clients(serv.getClients()) 
{
	std::cout << "New Channel " << this->_name << " has been created.";
	this->_chanops.insert(chanop.get_fd());
}

