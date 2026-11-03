/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerCmds.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/03 13:39:00 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/11/03 13:43:35 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

# include "../includes/Server.hpp"
# include "../includes/Client.hpp"
# include "../includes/Utils.hpp"

void Server::execute_cmd(Client &client, const std::string &cmd, const std::vector<std::string> &params)
{
	typedef int (Server::*Cmd)(Client &, const std::vector<std::string> &);
	std::map<std::string, Cmd> cmds;
	std::string				   param;
	int						   ret;

	cmds["PASS"] = &Server::handle_pass;
	cmds["NICK"] = &Server::handle_nick;
	cmds["USER"] = &Server::handle_user;
	cmds["PRIVMSG"] = &Server::handle_privmsg;
	cmds["JOIN"] = &Server::handle_join;
	cmds["INVITE"] = &Server::handle_invite;
	cmds["MODE"] = &Server::handle_mode;
	cmds["PART"] = &Server::handle_part;
	cmds["NOTICE"] = &Server::handle_notice;
	cmds["OPER"] = &Server::handle_oper;
	cmds["WHO"] = &Server::handle_who;
	cmds["QUIT"] = &Server::handle_quit;
	cmds["PING"] = &Server::handle_ping;
	cmds["AWAY"] = &Server::handle_away;
	cmds["FAST"] = &Server::handle_fast;
	cmds["HELP"] = &Server::handle_help;
	if (cmds.find(cmd) == cmds.end())
	{
		this->send_error(client, ERR_UNKNOWNCOMMAND, cmd);
		return ;
	}
	if ((cmd != "PASS" && cmd != "NICK" && cmd != "USER" && cmd != "FAST" && cmd != "HELP" 
		&& cmd != "QUIT" && cmd != "PING") && !client.get_registered())
	{
		this->send_error(client, 451, cmd); // not registered error
		return ;
	}
	ret = (this->*cmds[cmd])(client, params);
	if (ret != 0)
	{
		param = cmd;
		if (ret == ERR_USERONCHANNEL && cmd == "INVITE" && params.size() >= 2)
			param = params[0] + " " + params[1]; // error w/ channel and nick
		else if (cmd == "INVITE" && params.size() >= 2 && (ret == ERR_NOSUCHCHANNEL || ret == ERR_NOTONCHANNEL || ret == ERR_CHANOPRIVSNEEDED))
			param = params[1]; // error w/ INVITE + nick
		else if (cmd == "PART" && !params.empty() && (ret == ERR_NOSUCHCHANNEL || ret == ERR_NOTONCHANNEL))
			param = params[0]; // error w/ PART + channel
		else if (!params.empty() && (ret == ERR_NOSUCHNICK || ret == ERR_ERRONEOUSNICKNAME || ret == ERR_NICKNAMEINUSE))
			param = params[0]; // error w/ nick
		this->send_error(client, ret, param); // error w/ cmd
		return ;
	}
	if (cmd == "QUIT")
		return ;
	if (!client.get_registered() && client.can_register())
	{
		client.set_registered(true);
		std::cout << "\n\033[1;36m" << "✓ CLIENT " << client.get_client_fd() << " REGISTERED" << "\033[0m\n\n";
		this->send_notice(client, REGISTRATION, "");
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
