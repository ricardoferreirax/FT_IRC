/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/07 12:31:53 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/10 00:28:16 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Client.hpp"

Client::Client()
{
    this->_fd = -1;
    this->_auth = false;
    this->_registered = false;
}

Client::Client(int fd)
{
    this->_fd = fd;
    this->_auth = false;
    this->_registered = false;
}

std::string &Client::get_recv_buffer()
{
    return (this->_recv_buffer);
}

std::string &Client::get_send_buffer()
{
    return (this->_send_buffer);
}

int Client::get_fd() const
{
    return (this->_fd);
}

const std::string &Client::get_nick() const
{
    return (this->_nick);
}

const std::string &Client::get_user() const
{
	return (this->_user);
}
const std::string &Client::get_host_ip() const
{
	return (this->_host_ip);
}

bool Client::get_auth() const
{
	return (this->_auth);
}

bool Client::get_registered() const
{
    return (this->_registered);
}

void Client::set_nick(const std::string &nick)
{
    this->_nick = nick;
}

void Client::set_host_ip(const std::string &host_ip)
{
	this->_host_ip = host_ip;
}

void Client::set_user(const std::string &user)
{
    this->_user = user;
}

void Client::set_auth(bool auth)
{
    this->_auth = auth;
}

void Client::set_registered(bool registered)
{
    this->_registered = registered;
}

bool Client::can_register() const
{
    return (this->_auth && !this->_nick.empty() && !this->_user.empty());
}
