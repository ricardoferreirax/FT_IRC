/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   SocketData.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/05 17:17:06 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/11/09 23:57:19 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../bonus/includes/Bot.hpp"
#include "../includes/Server.hpp"
#include "../includes/Utils.hpp"

int Server::handle_socket_data(int fd, int event, e_socket_type type)
{
	std::string *recv_buffer;
	std::string *send_buffer;
	t_request 	*req;

	if (type == CLIENT_SOCKET)
	{
		if (this->_clients.find(fd) == this->_clients.end())
			return (SOCKET_ERROR);
		recv_buffer = &this->_clients[fd].get_recv_buffer();
		send_buffer = &this->_clients[fd].get_send_buffer();
	}
	else if (type == BOT_SOCKET)
	{
		req = this->_bot.get_request(fd);
		if (!req)
			return (SOCKET_ERROR);
		recv_buffer = &req->recv_buffer;
		send_buffer = &req->send_buffer;
	}
	else
		return (SOCKET_ERROR);
	if (event == EPOLLIN) // EPOLLIN->socket is ready to receive data.
		return (this->handle_socket_read(fd, type, *recv_buffer));
	if (event == EPOLLOUT) // EPOLLOUT->socket is ready to send data.
		return (this->handle_socket_write(fd, type, *send_buffer));
	return (SOCKET_ERROR);
}

// recv() - receive data FROM client through client socket
// recv() - receive response data FROM ollama through bot socket.
// irc msgs end with \r\n, which separates one complete msg from the next one
// incomplete msgs remain in client_buffer till more data is received with recv()
int Server::handle_socket_read(int fd, e_socket_type type, std::string &buffer)
{
	std::vector<std::string> 		params;
	std::string 					msg;
	std::string 					cmd;
	ssize_t							bytes;
	size_t							pos;
	char							data[1024];

	bytes = recv(fd, data, sizeof(data), 0);
	if (bytes < 0)
		return (SOCKET_PENDING); // recv() fails ->keep connection and wait for another event
	if (bytes == 0) // peer closed connection
	{
		if (type == CLIENT_SOCKET)
			return (SOCKET_ERROR); // client disconnected ->close connection
		else if (type == BOT_SOCKET)
			this->_bot.get_request(fd)->response_complete = true;
		return (SOCKET_SUCCESS);
	}
	buffer.append(data, bytes); // append received data->tcp may deliver msgs in parts
	if (type == BOT_SOCKET)
		return (SOCKET_SUCCESS); // bot responses are processed separately so don't parse bot response here
	log_tcp(TCP_RECEIVE, fd, bytes, std::string(data, bytes));
	Client &client = this->_clients.find(fd)->second;
	if (client.get_quit())
		return (SOCKET_SUCCESS); // client quit ->don't process any more msgs
	pos = buffer.find("\r\n");
	while (pos != std::string::npos)
	{
		msg = buffer.substr(0, pos); // extract one complete msg from buffer
		buffer.erase(0, pos + 2); // rmv processed msg + \r\n
		if (!msg.empty())
		{
			cmd.clear();
			params.clear();
			parse_msg(msg, cmd, params);
			if (!cmd.empty())
				this->execute_cmd(client, cmd, params);
			if (this->_restart || client.get_quit())
    			return (SOCKET_SUCCESS);
		}
		pos = buffer.find("\r\n");
	}
	return (SOCKET_SUCCESS);
}

// send() - send pending data TO client through client socket
// send() - send remaining request data TO ollama through bot socket.
int Server::handle_socket_write(int fd, e_socket_type type, std::string &buffer)
{
	std::map<int, Client>::iterator it;
	epoll_event 					ev;
	ssize_t 						bytes;

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
	if (type == CLIENT_SOCKET)
	{
		it = this->_clients.find(fd);
		if (it == this->_clients.end())
			return (SOCKET_ERROR);
		if (it->second.get_quit())
		{
			this->close_connections(CLIENT_DISCONNECT, fd);
			return (SOCKET_SUCCESS);
		}
	}
	ev.events = EPOLLIN;
	ev.data.fd = fd;
	if (epoll_ctl(this->_epoll_fd, EPOLL_CTL_MOD, fd, &ev) < 0) // all data was sent->monitor only incoming data
		return (SOCKET_ERROR);
	return (SOCKET_SUCCESS);
}
