/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/07 12:31:53 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/22 13:15:23 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Client.hpp"

Client::Client()
{
	this->_client_fd = -1;
	this->_auth = false;
	this->_registered = false;
	this->_quit = false;
}

Client::Client(int client_fd)
{
	this->_client_fd = client_fd;
	this->_auth = false;
	this->_registered = false;
	this->_quit = false;
}

std::string &Client::get_recv_buffer()
{
	return (this->_recv_buffer);
}

std::string &Client::get_send_buffer()
{
	return (this->_send_buffer);
}

int Client::get_client_fd() const
{
	return (this->_client_fd);
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

bool Client::get_quit() const
{
	return (this->_quit);
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

void Client::set_quit(bool quit)
{
	this->_quit = quit;
}

bool Client::can_register() const
{
	return (this->_auth && !this->_nick.empty() && !this->_user.empty());
}
