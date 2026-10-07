/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/07 12:31:53 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/07 12:32:55 by rmedeiro         ###   ########.fr       */
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
