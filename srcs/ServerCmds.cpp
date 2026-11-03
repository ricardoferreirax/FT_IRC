/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerCmds.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/03 13:39:00 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/11/03 16:15:37 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

# include "../includes/Server.hpp"
# include "../includes/Client.hpp"
# include "../includes/Utils.hpp"

void Server::execute_cmd(Client &client, const std::string &cmd, const std::vector<std::string> &params)
{
	typedef int (Server::*Cmd)(Client &, const std::vector<std::string> &);
	std::map<std::string, Cmd> cmds;
	std::string param;
	int ret;

	if (cmd == "PASS" || cmd == "NICK" || cmd == "USER" || cmd == "FAST")
	{
		this->process_registration(client, cmd, params);
		return ;
	}
	if (cmd == "JOIN" || cmd == "INVITE" || cmd == "MODE" || cmd == "PART")
	{
		this->process_channels(client, cmd, params);
		return ;
	}
	if (cmd == "WHO" || cmd == "HELP" || cmd == "AWAY")
	{
		this->process_info(client, cmd, params);
		return ;
	}
	cmds["PRIVMSG"] = &Server::handle_privmsg;
	cmds["NOTICE"] = &Server::handle_notice;
	cmds["OPER"] = &Server::handle_oper;
	cmds["QUIT"] = &Server::handle_quit;
	cmds["PING"] = &Server::handle_ping;
	if (cmds.find(cmd) == cmds.end())
	{
		this->send_error(client, ERR_UNKNOWNCOMMAND, cmd);
		return ;
	}
	if (cmd != "QUIT" && cmd != "PING" && !client.get_registered())
	{
		this->send_error(client, ERR_NOTREGISTERED, cmd);
		return ;
	}
	ret = (this->*cmds[cmd])(client, params);
	if (cmd == "QUIT")
		return ;
	if (ret != 0)
	{
		param = cmd;
		if (!params.empty() && ret == ERR_NOSUCHNICK)
			param = params[0];
		this->send_error(client, ret, param);
	}
}

int Server::handle_oper(Client &client, const std::vector<std::string> &params)
{
	std::map<std::string, std::string>::iterator it;

	if (params.size() < 2 || params[0].empty() || params[1].empty())
		return (ERR_NEEDMOREPARAMS);
	it = this->_opers.find(params[0]);
	if (it == this->_opers.end())
		return (ERR_NOOPERHOST);
	if (it->second != params[1])
		return (ERR_PASSWDMISMATCH);
	client.set_server_op(true);
	this->send_reply(client, RPL_YOUREOPER, "", "");
	return (0);
}

// ping is used to check if connection between client and server is still active.
int Server::handle_ping(Client &client, const std::vector<std::string> &params)
{
	if (params.empty() || params[0].empty())
		return (ERR_NOORIGIN);
	this->send_notice(client, PONGREPLY, params[0]);
	return (0);
}

int Server::handle_quit(Client &client, const std::vector<std::string> &params)
{
	std::map<std::string, Channel>::iterator chanit;
	std::map<int, Client>::iterator 		 it;
	std::string 							 reason;
	epoll_event 							 event;

	if (params.empty() || params[0].empty()) // no reason provided
		reason = "Client quit";
	else
		reason = params[0];
	for (it = this->_clients.begin(); it != this->_clients.end(); ++it)
	{
		if (it->first == client.get_client_fd())
			continue ; // skip client who quit
		for (chanit = this->_channels.begin(); chanit != this->_channels.end(); ++chanit) // iterate through all channels
		{
			if (chanit->second.getClients().count(client.get_client_fd()) && chanit->second.getClients().count(it->first))
			{
				this->send_cmd(client, it->second, "QUIT :" + reason); // if both clients are in same channel->send quit to other client
				break ;
			}
		}
	}
	client.get_send_buffer() += "Closing connection: " + client.get_nick() + reason + "\r\n";
	event.events = EPOLLOUT; // set EPOLLOUT to send quit msg before closing connection
	event.data.fd = client.get_client_fd();
	if (epoll_ctl(this->_epoll_fd, EPOLL_CTL_MOD, client.get_client_fd(), &event) < 0)
		throw std::runtime_error("IRC: epoll_ctl() failed.");
	client.set_quit(true);
	return (0);
}
