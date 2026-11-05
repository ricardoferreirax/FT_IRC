/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   SocketData.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/05 17:17:06 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/11/05 18:38:25 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../bonus/includes/Bot.hpp"
#include "../includes/Server.hpp"
#include "../includes/Utils.hpp"

int Server::handle_socket_data(int fd, int event, e_socket_type type)
{
	std::map<int, Client>::iterator it;
	std::string 				   *buffer;
	t_request					   *req;

	if (type == CLIENT_SOCKET)
	{
		it = this->_clients.find(fd);
		if (it == this->_clients.end())
			return (SOCKET_ERROR);
		if (event == EPOLLIN)
			buffer = &it->second.get_recv_buffer();
		else if (event == EPOLLOUT)
			buffer = &it->second.get_send_buffer();
		else
			return (SOCKET_ERROR);
	}
	else if (type == BOT_SOCKET)
	{
		req = this->_bot.get_request(fd);
		if (!req)
			return (SOCKET_ERROR);
		if (event == EPOLLIN)
			buffer = &req->recv_buffer;
		else if (event == EPOLLOUT)
			buffer = &req->send_buffer;
		else
			return (SOCKET_ERROR);
	}
	else
		return (SOCKET_ERROR);
	if (event == EPOLLIN)
		return (this->handle_socket_read(fd, type, *buffer));
	return (this->handle_socket_write(fd, type, *buffer));
}

// recv() - receive data FROM client through client socket
// recv() - receive response data FROM ollama through bot socket.
int Server::handle_socket_read(int fd, e_socket_type type, std::string &buffer)
{
	char	data[1024];
	ssize_t	bytes;

	bytes = recv(fd, data, sizeof(data), 0);
	if (bytes == 0) // peer closed connection
	{
		if (type == CLIENT_SOCKET)
			this->close_connections(CLIENT_DISCONNECT, fd);
		else
			this->_bot.get_request(fd)->response_complete = true; // request completed
		return (SOCKET_SUCCESS);
	}
	if (bytes < 0)
		return (SOCKET_PENDING); // recv() fails ->keep connection and wait for another event
	buffer.append(data, bytes); // append received data->tcp may deliver msgs in parts
	if (type == CLIENT_SOCKET)
	{
		log_tcp(TCP_RECEIVE, fd, bytes, std::string(data, bytes));
		this->process_msgs(this->_clients[fd], buffer);
	}
	return (SOCKET_SUCCESS);
}

// send() - send pending data TO client through client socket
// send() - send remaining request data TO ollama through bot socket.
int Server::handle_socket_write(int fd, e_socket_type type, std::string &buffer)
{
	epoll_event ev;
	ssize_t bytes;

	if (!buffer.empty()) // there is pending data to send
	{
		bytes = send(fd, buffer.c_str(), buffer.size(), MSG_NOSIGNAL);
		if (bytes <= 0)
			return (SOCKET_PENDING); // if send() fails->keep pending data for next event
		if (type == CLIENT_SOCKET)
			log_tcp(TCP_SEND, fd, bytes, buffer.substr(0, bytes));
		buffer.erase(0, bytes); // rmv only bytes successfully sent
	}
	if (!buffer.empty())
		return (SOCKET_PENDING); // wait for another EPOLLOUT if there is still data to send
	if (type == CLIENT_SOCKET && this->_clients[fd].get_quit())
	{
		this->close_connections(CLIENT_DISCONNECT, fd); // client sent quit ->close connection
		return (SOCKET_SUCCESS);
	}
	ev.events = EPOLLIN;
	ev.data.fd = fd;
	if (epoll_ctl(this->_epoll_fd, EPOLL_CTL_MOD, fd, &ev) < 0) // all data was sent->monitor only incoming data
	{
		if (type == CLIENT_SOCKET)
			this->close_connections(CLIENT_DISCONNECT, fd);
		return (SOCKET_ERROR);
	}
	return (SOCKET_SUCCESS);
}
