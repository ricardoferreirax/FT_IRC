/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/07 12:31:53 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/07 12:56:22 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Client.hpp"

// creates a new client and initializes its connection and registration data
Client::Client(int fd)
{
    this->_fd = fd;
    this->_recv_buffer = "";
    this->_send_buffer = "";
    this->_nick = "";
    this->_user = "";
    this->_auth = false;
    this->_registered = false;
}

Client::~Client()
{
	
}

std::string &Client::get_recv_buffer()
{
	return (this->_recv_buffer);
}

std::string &Client::get_send_buffer()
{
	return (this->_send_buffer);
}