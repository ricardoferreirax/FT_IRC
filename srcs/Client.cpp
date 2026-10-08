/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/07 12:31:53 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/08 14:19:44 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Client.hpp"

Client::Client()
{
    this->_fd = -1;
    this->_recv_buffer = "";
    this->_send_buffer = "";
    this->_nick = "";
    this->_user = "";
    this->_auth = false;
    this->_registered = false;
}

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
	std::cout << "\nNICK set to: " << nick << std::endl;
}

void Client::set_user(const std::string &user)
{
    this->_user = user;
}

void Client::set_auth(bool auth)
{
    this->_auth = auth;
	std::cout << "PASS authentication: " << (auth ? "successful" : "failed") << std::endl;
}

void Client::set_registered(bool registered)
{
    this->_registered = registered;
}

std::string &Client::get_recv_buffer()
{
    return (this->_recv_buffer);
}

std::string &Client::get_send_buffer()
{
    return (this->_send_buffer);
}
