/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/07 12:31:53 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/07 15:49:04 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Client.hpp"

Client::Client()
{
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
