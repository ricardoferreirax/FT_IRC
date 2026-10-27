/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Commands.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/05 20:08:43 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/27 21:36:13 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Server.hpp"
#include "../includes/Utils.hpp"

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
	cmds["PING"] = &Server::handle_ping;
	cmds["QUIT"] = &Server::handle_quit;
	cmds["FAST"] = &Server::handle_fast;
	if (cmds.find(cmd) == cmds.end())
	{
		this->send_error(client, ERR_UNKNOWNCOMMAND, cmd);
		return ;
	}
	if ((cmd != "PASS" && cmd != "NICK" && cmd != "USER" && cmd != "FAST") && !client.get_registered())
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
		this->send_msg(client, registration_reply(client, this->_name, this->_date));
	}
}

int Server::handle_pass(Client &client, const std::vector<std::string> &params)
{
	if (client.get_registered())
		return (ERR_ALREADYREGISTRED);
	if (client.get_auth())
		return (ERR_PASSALREADYACCEPTED);
	if (params.empty())
		return (ERR_NEEDMOREPARAMS);
	if (params[0] != this->_pass)
		return (ERR_PASSWDMISMATCH);
	client.set_auth(true);
	this->send_notice(client, PASSACCEPTED, "");
	return (0);
}

int Server::handle_nick(Client &client, const std::vector<std::string> &params)
{
	std::map<int, Client>::iterator it;
	std::string						nick;
	std::string						old_nick;
	std::string						prefix;

	if (!client.get_auth())
		return (ERR_PASSREQUIRED);
	if (params.empty() || params[0].empty())
		return (ERR_NONICKNAMEGIVEN);
	nick = params[0];
	if (!is_valid_nick(nick))
		return (ERR_ERRONEOUSNICKNAME);
	old_nick = client.get_nick();
	if (old_nick == nick)
		return (0);
	for (it = this->_clients.begin(); it != this->_clients.end(); ++it)
	{
		if (it->first != client.get_client_fd() && same_nick(it->second.get_nick(), nick)) // nick is already in use by another client
			return (ERR_NICKNAMEINUSE);
	}
	if (client.get_registered())
	{
		prefix = ":" + old_nick + "!" + client.get_user() + "@" + client.get_host_ip();
		this->send_msg(client, prefix + " NICK :" + nick);
	}
	else
		this->send_notice(client, NICKSET, nick);
	client.set_nick(nick);
	return (0);
}

int Server::handle_user(Client &client, const std::vector<std::string> &params)
{
	std::string username;
	std::string mode;
	
	if (client.get_registered())
		return (ERR_ALREADYREGISTRED);
	if (!client.get_auth())
		return (ERR_PASSREQUIRED);
	if (params.size() < 4)
		return (ERR_NEEDMOREPARAMS);
	if (client.get_nick().empty())
		return (ERR_NONICKNAMEGIVEN);
	username = params[0];
	mode = params[1];
	if (username.empty())
		return (ERR_NEEDMOREPARAMS);
	if (!is_valid_user_mode(mode))
		return (ERR_INVALIDUSERMODE);
	client.set_user(username);
	return (0);
}

int Server::handle_privmsg(Client &client, const std::vector<std::string> &params)
{
	std::map<std::string, Channel>::iterator chanit;
	std::map<int, Client *>::iterator 		 clienit;
	std::map<int, Client *> 				 chan_clients;
	std::map<int, Client>::iterator 		 it;
	std::string								 target;
	std::string								 msg;
	std::string								 prefix;

	if (params.empty() || params[0].empty())
		return (ERR_NORECIPIENT);
	if (params.size() < 2 || params[1].empty())
		return (ERR_NOTEXTTOSEND);
	target = params[0];
	msg = params[1];
	prefix = ":" + client.get_nick() + "!" + client.get_user() + "@" + client.get_host_ip();
	if (same_nick(target, "Miku"))
	{
		try
		{
			if (!this->_bot.start_request(client.get_client_fd(), client.get_nick(), "", msg))
				return (ERR_MIKUUNAVAILABLE);
		}
		catch (const std::exception &e)
		{
			std::cerr << "Miku: " << e.what() << std::endl;
			return (ERR_MIKUUNAVAILABLE);
		}
		return (0);
	}
	it = this->_clients.begin();
	while (it != this->_clients.end())
	{
		if (same_nick(it->second.get_nick(), target))
		{
			this->send_msg(it->second, prefix + " PRIVMSG " + it->second.get_nick() + " :" + msg); // nick matches a client nick ->send msg to that client
			return (0);
		}
		it++;
	}
	chanit = this->_channels.find(target);
	if (chanit != this->_channels.end())
	{
		chan_clients = chanit->second.getClients(); // get all clients in channel
		clienit = chan_clients.begin();
		while (clienit != chan_clients.end())
		{
			if (clienit->second->get_client_fd() != client.get_client_fd()) // 
				this->send_msg(*(clienit->second), prefix + " PRIVMSG " + target + " :" + msg); // if client is not who sends ->send msg to that client
			clienit++;
		}
		return (0);
	}
	return (ERR_NOSUCHNICK);
}

int	Server::handle_join(Client &client, const std::vector<std::string> &params)
{
	std::map<std::string, Channel>::iterator it;
	std::vector<std::string>				 c_names;
	std::vector<std::string>				 keys;
	std::string 							 chan_name;
	std::string 							 key;
	size_t 									 i;
	int 									 ret;

	if (params.empty() || params[0].empty() || params.size() > 2)
		return (ERR_NEEDMOREPARAMS);
	c_names = ft_split(params[0], ',');
	if (params.size() > 1)
		keys = ft_split(params[1], ',');
	i = 0;
	while (i < c_names.size())
	{
		chan_name = c_names[i];
		if (chan_name.empty() || (chan_name[0] != '#' && chan_name[0] != '&'))
		{
			this->send_error(client, ERR_BADCHANMASK, chan_name);
			i++;
			continue ;
		}
		if (i < keys.size())
			key = keys[i];
		else
			key = "";
		it = this->_channels.find(chan_name);
		if (it == this->_channels.end())
		{
			Channel ch(chan_name, default_mode(), client);
			
			this->_channels.insert(std::pair<std::string, Channel>(chan_name, ch));
			this->send_notice(client, CHANNELCREATED, chan_name);
		}
		else
		{
			ret = it->second.joinChannel(client, key);
			if (ret == 1)
				this->send_error(client, ERR_USERONCHANNEL, chan_name);
			else if (ret == 2)
				this->send_error(client, ERR_CHANNELISFULL, chan_name);
			else if (ret == 3)
				this->send_error(client, ERR_INVITEONLYCHAN, chan_name);
			else if (ret == 4)
				this->send_error(client, ERR_BADCHANNELKEY, chan_name);
			else
				this->send_notice(client, CHANNELJOINED, chan_name);
		}
		i++;
	}
	return (0);
}

// TODO: User invite is not consumed when used
int Server::handle_invite(Client &client, const std::vector<std::string> &params)
{
	std::map<std::string, Channel>::iterator chanit;
	std::map<int, Client>::iterator 		 clienit;
	
	if (params.empty() || params.size() < 2)
		return (ERR_NEEDMOREPARAMS);
	chanit = this->_channels.find(params[1]);
	if (chanit == this->_channels.end())
		return (ERR_NOSUCHCHANNEL);
	clienit = this->_clients.begin();
	while (clienit != this->_clients.end())
	{
		if (clienit->second.get_nick() == params[0]) // target nick matches a client nick
			break ;
		clienit++;
	}
	if (clienit == this->_clients.end())
		return (ERR_NOSUCHNICK);
	if (chanit->second.getMode().invite_only)
	{
		if (chanit->second.getChannops().find(client.get_client_fd()) == chanit->second.getChannops().end()) // client isn't channel operator
			return (ERR_CHANOPRIVSNEEDED);
	}
	chanit->second.inviteUser(clienit->second);
	this->send_reply(client, RPL_INVITING, params[0] + " " + params[1], ""); // confirmation msg to inviter
	// this->send_msg(clienit->second, "You have been invited to partcipate in " + chanit->second.getName());
	this->send_msg(clienit->second, ":" + client.get_nick() + "!" + client.get_user() + "@" + client.get_host_ip() + " INVITE " + params[0] + " :" + params[1]);
	return (0);
}

// TODO: Users can use MODE in a channel they are not part of nor have channop
int Server::handle_mode(Client &client, const std::vector<std::string> &params)
{
	std::vector<t_modecmd> flags;
	size_t 				   i;
	bool				   res;

	if (params.empty() || params[0].empty())
		return (ERR_NEEDMOREPARAMS);
	if (this->_channels.find(params[0]) == this->_channels.end())
		return (ERR_NOSUCHCHANNEL);
	if (params.size() < 2)
	{
		// send RPL_CHANNELMODEIS 
		return (0);
	}
	if (!tokenize_mode_flags(params[1], flags, params, 2))
		return (ERR_UNKNOWNMODE);
	i = 0;
	while (i < flags.size())
	{
		res = false;
		switch (flags[i].flag)
		{
			case 'i':
				res = handle_imode(client, params[0], flags[i]);
				break ;
			case 't':
				res = handle_tmode(client, params[0], flags[i]);
				break ;
			case 'k':
				res = handle_kmode(client, params[0], flags[i]);
				break ;
			case 'o':
				res = handle_omode(client, params[0], flags[i]);
				break ;
			case 'l':
				res = handle_lmode(client, params[0], flags[i]);
				break ;
			default:
				return (ERR_UNKNOWNMODE);
		}
		if (!res)
			return (0);
		i++;
	}
	return (0);
}

int Server::handle_part(Client &client, const std::vector<std::string> &params)
{
	std::map<std::string, Channel>::iterator chanit;
	std::vector<std::string>				 channels;
	std::vector<std::string>				 notify_chan;
	size_t 									 i;

	if (params.empty() || params[0].empty())
		return (ERR_NEEDMOREPARAMS);
	i = 0;
	while (i < channels.size())
	{
		chanit = this->_channels.find(channels[i]);
		if (chanit == this->_channels.end())
		{
			this->send_error(client, ERR_NOSUCHCHANNEL, channels[i]);
			return (0);
		}
		i++;
	}
	i = 0;
	while (i < channels.size())
	{
		chanit = this->_channels.find(channels[i]);
		chanit->second.leaveChannel(client);
		chanit->second.getInvites().erase(client.get_client_fd());
		std::cout << client.get_client_fd() << " has left " << chanit->second.getName() << std::endl;
		if (chanit->second.getClients().size() <= 0)
			this->_channels.erase(chanit);
		else
		{
			notify_chan.clear();
			notify_chan.push_back(channels[i]);
			notify_chan.push_back(client.get_nick() + " has left");
			if (params.size() == 2)
				notify_chan.push_back("Reason: " + params[1]);
			this->handle_privmsg(client, notify_chan);
		}
		i++;
	}
	return (0);
}
