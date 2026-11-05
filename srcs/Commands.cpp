/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Commands.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/05 20:08:43 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/24 22:16:18 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Channels.hpp"
#include "../includes/Server.hpp"
#include "../includes/Utils.hpp"

void Server::execute_cmd(Client &client, const std::string &cmd, const std::vector<std::string> &params)
{
	int	error;

	typedef int (Server::*Cmd)(Client &, const std::vector<std::string> &);
	// type cmd points to a member function of server
	std::map<std::string, Cmd> cmds;
	// cmd corresponds to a member function
	cmds["PASS"] = &Server::handle_pass;
	cmds["NICK"] = &Server::handle_nick;
	cmds["USER"] = &Server::handle_user;
	cmds["PRIVMSG"] = &Server::handle_privmsg;
	cmds["JOIN"] = &Server::handle_join;
	cmds["PING"] = &Server::handle_ping;
	cmds["QUIT"] = &Server::handle_quit;
	// cmds["MIKU"] = &Server::handle_miku;
	cmds["FAST"] = &Server::handle_fast;
	cmds["MODE"] = &Server::handle_mode;
	cmds["PART"] = &Server::handle_part;
	cmds["INVITE"] = &Server::handle_invite;
	cmds["KICK"] = &Server::handle_kick;
	cmds["WHOIS"] = &Server::handle_whois;
	cmds["TOPIC"] = &Server::handle_topic;
	if (cmds.find(cmd) == cmds.end()) // check if cmd is in the map
	{
		this->send_error(client, 421, cmd); // unknown command error
		return ;
	}
	if ((cmd != "PASS" && cmd != "NICK" && cmd != "USER" && cmd != "FAST") && !client.get_registered())
	{
		this->send_error(client, 451, cmd); // not registered error
		return ;
	}
	error = (this->*cmds[cmd])(client, params);
	if (error != 0)
	{
		if ((error == ERR_NOSUCHNICK || error == ERR_ERRONEOUSNICKNAME || error == ERR_NICKNAMEINUSE) && !params.empty())
			this->send_error(client, error, params[0]);
		else
			this->send_error(client, error, cmd);
		return ;
	}
	if (cmd == "QUIT")
		return ;
	if (!client.get_registered() && client.can_register())
	{
		client.set_registered(true);
		std::cout << "\n=====================================================" << std::endl;
		std::cout << "          CLIENT " << client.get_client_fd() << " REGISTERED!" << std::endl;
		std::cout << "=====================================================" << std::endl;
		this->send_msg(client, registration_reply(client, this->_name, this->_date));
	}
}


int Server::handle_topic(Client &client, const std::vector<std::string> &params)
{
	if(params.empty())
		return(ERR_NEEDMOREPARAMS);
	std::map<std::string, Channel>::iterator chanit = this->_channels.find(params[0]);
	if(chanit == this->_channels.end())
	{
		this->send_error(client, ERR_NOSUCHCHANNEL, params[0]);
		return -1;
	}
	if(chanit->second.getMode().topic_restricted)
	{
		if(chanit->second.getChannops().find(client.get_client_fd()) == chanit->second.getChannops().end())
		{
			this->send_error(client, ERR_CHANOPRIVSNEEDED, chanit->second.getName());
			return -1;
		}
	}
	if(params.size() < 2)
		chanit->second.setMode(chanit->second.getMode().invite_only, chanit->second.getMode().topic_restricted, "", chanit->second.getMode().users, chanit->second.getMode().key);
	else
		chanit->second.setMode(chanit->second.getMode().invite_only, chanit->second.getMode().topic_restricted, params[1], chanit->second.getMode().users, chanit->second.getMode().key);
	if(chanit->second.getMode().topic.empty())
		this->send_msg(client, "Topic unset");
	else
		this->send_msg(client, "Topic set to: " + chanit->second.getMode().topic);
	std::vector<std::string> notice;
	notice.push_back(chanit->first);
	if(chanit->second.getMode().topic.empty())
		notice.push_back(client.get_nick() + " has unset the channel's topic");
	else
		notice.push_back(client.get_nick() + " has set the channel's topic to: " + chanit->second.getMode().topic);
	this->handle_privmsg(client, notice);
	return 0;
}

int Server::handle_whois(Client &client, const std::vector<std::string> &params)
{
	size_t	i;
	std::string to_print;

	if (params.empty() || params.size() > 1)
		return (-1);
	std::vector<std::string> users = ft_split(params[0], ',');
	i = 0;
	std::string channels;
	std::string chan_opers;
	while (i < users.size())
	{
		std::map<int, Client>::iterator clienit = this->reverseClientSearch(users[i]);
		if (clienit == this->_clients.end())
		{
			this->send_error(client, ERR_NOSUCHNICK, users[i]);
			i++;
			continue ;
		}
		to_print += users[i] + " is " + clienit->second.get_user() + "@" + clienit->second.get_host_ip() + "\n";
		std::map<std::string, Channel>::iterator chanit = this->_channels.begin();
		while (chanit != this->_channels.end())
		{
			if (chanit->second.getChannops().find(clienit->first) != chanit->second.getChannops().end())
			{
				if(!chan_opers.empty())
					channels += ',';
				chan_opers += chanit->second.getName();
			}
			else if (chanit->second.getClients().find(clienit->first) != chanit->second.getClients().end())
			{
				if(!channels.empty())
					channels += ',';
				channels += chanit->second.getName();
			}
			chanit++;
		}
		if(!chan_opers.empty())
			to_print += "User is channop in the following channels: " + chan_opers + "\n";
		if(!channels.empty())
			to_print += "User is a member of the following channels: " + channels + "\n";
		i++;
	}
	this->send_msg(client, to_print);
	return (0);
}

int Server::handle_invite(Client &client, const std::vector<std::string> &params)
{
	if (params.empty() || params.size() < 2)
	{
		this->send_error(client, ERR_NEEDMOREPARAMS, "");
		return (-1);
	}
	std::map<int, Client>::iterator clienit = this->reverseClientSearch(params[0]);
	if (clienit == this->_clients.end())
	{
		this->send_error(client, ERR_NOSUCHNICK, params[0]);
		return (-1);
	}
	std::map<std::string, Channel>::iterator chanit = this->_channels.find(params[1]);
	if (chanit == this->_channels.end())
	{
		this->send_error(client, ERR_NOSUCHCHANNEL, params[1]);
		return (-1);
	}
	if (chanit->second.getMode().invite_only)
	{
		if (chanit->second.getChannops().find(client.get_client_fd()) == chanit->second.getChannops().end())
		{
			this->send_error(client, ERR_CHANOPRIVSNEEDED, chanit->first);
			return (-1);
		}
	}
	chanit->second.inviteUser(clienit->second);
	this->send_msg(clienit->second, "You have been invited to partcipate in " + chanit->second.getName());
	return (0);
}

int Server::handle_kick(Client &client, const std::vector<std::string> &params)
{
	size_t	i;
	size_t	j;

	if (params.size() < 2)
	{
		this->send_error(client, ERR_NEEDMOREPARAMS, "");
		return (-1);
	}
	std::vector<std::string> channels = ft_split(params[0], ',');
	std::vector<std::string> clients = ft_split(params[1], ',');
	i = 0;
	if (!check_allchans(channels, &client))
		return (-1);
	while (i < channels.size())
	{
		std::map<std::string, Channel>::iterator chanit = this->_channels.find(channels[i]);
		j = 0;
		while (j < clients.size())
		{
			std::map<int, Client>::iterator clienit = this->reverseClientSearch(clients[j]);
			if (clienit == this->_clients.end())
			{
				this->send_error(client, ERR_NOSUCHNICK, clients[j] + ", no action was taken");
				j++;
				continue ;
			}
			if (chanit->second.getChannops().find(client.get_client_fd()) == chanit->second.getChannops().end())
			{
				this->send_error(client, ERR_CHANOPRIVSNEEDED, chanit->first);
				return (-1);
			}
			std::vector<std::string> args;
			args.push_back(channels[i]);
			args.push_back(clients[j]);
			if (params.size() > 2)
				args.push_back(params[2]);
			this->handle_part(clienit->second, args);
			std::string victim_msg = "You have been kicked from " + chanit->first + " by " + client.get_nick();
			if (params.size() > 2)
				victim_msg += " because: " + params[2];
			else
				victim_msg += " for no reason";
			this->send_msg(clienit->second, victim_msg);
			j++;
		}
		i++;
	}
	return (0);
}

bool Server::check_allchans(const std::vector<std::string> &chans, Client *client)
{
	size_t	i;

	i = 0;
	while (i < chans.size())
	{
		if (this->_channels.find(chans[i]) == this->_channels.end())
		{
			if (client != NULL)
				this->send_error(*client, ERR_NOSUCHCHANNEL, chans[i] + ", no action was taken");
			return (false);
		}
		i++;
	}
	return (true);
}

int Server::handle_part(Client &client, const std::vector<std::string> &params)
{
	size_t	i;
	int		newchannopFD;

	if (params.empty() || params.size() > 3)
	{
		if(params.empty())
			this->send_error(client, ERR_NEEDMOREPARAMS, "");
		else
			this->send_error(client, ERR_UNKNOWNCOMMAND, params[2]);
		return (-1);
	}
	std::vector<std::string> channels = ft_split(params[0], ',');
	if (!check_allchans(ft_split(params[0], ','), &client))
		return (-1);
	i = 0;
	std::vector<std::string> notify_chan;
	while (i < channels.size())
	{
		std::map<std::string, Channel>::iterator chanit = this->_channels.find(channels[i]);
		std::cout << client.get_client_fd() << " has left " << chanit->second.getName() << std::endl;
		notify_chan.push_back(channels[i]);
		notify_chan.push_back(client.get_nick() + " has left");
		if (params.size() == 2)
			notify_chan.push_back("Reason: " + params[1]);
		this->handle_privmsg(client, notify_chan);
		newchannopFD = chanit->second.leaveChannel(client);
		if (newchannopFD && newchannopFD != -1)
			this->send_msg(this->_clients.find(newchannopFD)->second, "You are now a channop in channel " + chanit->second.getName());
		if (newchannopFD == -1)
			this->_channels.erase(chanit);
		i++;
	}
	return (0);
}

int Server::handle_pass(Client &client, const std::vector<std::string> &params)
{
	if (client.get_registered())
		return (ERR_ALREADYREGISTRED);
	if (client.get_auth())
		return (ERR_PASSALREADYACCEPTED);
	if (params.size() != 1)
		return (ERR_NEEDMOREPARAMS);
	if (params[0] != this->_pass)
		return (ERR_PASSWDMISMATCH);
	client.set_auth(true);
	this->send_msg(client, ":" + this->_name + " NOTICE * :PASS accepted");
	return (0);
}

int Server::handle_nick(Client &client, const std::vector<std::string> &params)
{
	int	client_fd;

	std::map<int, Client>::iterator it;
	std::string nick;
	client_fd = client.get_client_fd();
	if (!client.get_auth())
		return (ERR_PASSREQUIRED);
	if (params.empty())
		return (ERR_NONICKNAMEGIVEN);
	nick = params[0];
	if (!is_valid_nick(nick))
		return (ERR_ERRONEOUSNICKNAME);
	if (client.get_nick() == nick)
		return (0);
	it = this->reverseClientSearch(nick);
	if (it->first != client_fd && same_nick(it->second.get_nick(), nick))
		return (ERR_NICKNAMEINUSE);
	client.set_nick(nick);
	this->send_msg(client, ":" + this->_name + " NOTICE * :Set nickname to " + nick);
	return (0);
}

int Server::handle_user(Client &client, const std::vector<std::string> &params)
{
	std::string username;
	std::string mode;
	if (client.get_registered())
		return (ERR_ALREADYREGISTRED);
	if (client.get_nick().empty())
		return (ERR_NONICKNAMEGIVEN); // nick must be set before user error
	if (params.size() < 4)
		return (ERR_NEEDMOREPARAMS);
	username = params[0];
	mode = params[1];
	if (!is_valid_user_mode(mode))
		return (ERR_INVALIDUSERMODE);
	client.set_user(username);
	this->send_msg(client, ":" + this->_name + " NOTICE * :Set username to " + username);
	return (0);
}

int Server::handle_privmsg(Client &client, const std::vector<std::string> &params)
{
	std::map<int, Client>::iterator clienit;
	std::map<int, Client>::iterator it;
	std::string target;
	std::string msg;
	std::string prefix;
	if (params.empty())
		return (411);                           // no recipient given error
	if (params.size() < 2 || params[1].empty()) // check if msg text is empty
		return (412);
	target = params[0];
	msg = params[1];
	clienit = this->_clients.begin();
	if (same_nick(target, "Miku"))
	{
		try
		{
			if (!this->_bot.start_request(client.get_client_fd(), client.get_nick(), "", msg))
				// bot is unavailable-> send error to client
				return (ERR_MIKUUNAVAILABLE);
		}
		catch (const std::exception &e)
		{
			std::cerr << "Miku: " << e.what() << std::endl;
			return (ERR_MIKUUNAVAILABLE);
		}
		return (0);
	}
	it = this->reverseClientSearch(target);
	if (it != this->_clients.end())
	{
		prefix = ":" + client.get_nick() + "!" + client.get_user() + "@" + client.get_host_ip();
		this->send_msg(it->second, prefix + " PRIVMSG " + it->second.get_nick() + " :" + msg);
		return (0);
	}
	std::map<std::string, Channel>::iterator chanit = this->_channels.find(target);
	if (chanit != this->_channels.end())
	{
		std::map<int, Client *> chan_clients = chanit->second.getClients();
		if (chan_clients.find(client.get_client_fd()) == chan_clients.end())
		{
			this->send_error(client, ERR_NOTONCHANNEL, chanit->first);
			return (-1);
		}
		std::map<int, Client *>::iterator clienit = chan_clients.begin();
		prefix = ":" + client.get_nick() + "!" + client.get_user() + "@" + client.get_host_ip();
		while (clienit != chan_clients.end())
		{
			if (clienit->second->get_client_fd() != client.get_client_fd())
				this->send_msg(*(clienit->second), prefix + " PRIVMSG " + clienit->second->get_nick() + " :" + msg + "\r\n" + "\r\n");
			clienit++;
		}
		return (0);
	}
	this->send_error(client, ERR_NOSUCHCHANNEL, target); // no such nick/channel error
	return (0);
}

// ping is used to check if connection between client and server is still active.
int Server::handle_ping(Client &client, const std::vector<std::string> &params)
{
	std::string token;
	if (params.empty() || params[0].empty())
		return (ERR_NOORIGIN);
	token = params[0];
	this->send_msg(client, ":" + this->_name + " PONG " + this->_name + " :" + token);
	return (0);
}

int Server::handle_quit(Client &client, const std::vector<std::string> &params)
{
	int	client_fd;

	std::map<int, Client>::iterator it;
	std::string reason;
	std::string msg;
	std::string client_name;
	client_fd = client.get_client_fd();
	if (params.empty() || params[0].empty())
		reason = "Client quit";
	else
		reason = params[0];
	if (client.get_nick().empty())
		client_name = "unregistered client";
	else
		client_name = client.get_nick();
	msg = ":" + client.get_nick() + "!" + client.get_user() + "@" + client.get_host_ip() + " QUIT :" + reason;
	for (it = this->_clients.begin(); it != this->_clients.end(); ++it)
	{
		if (it->first == client_fd)
			continue ;
		// skip sending QUIT msg to client that sent it
		this->send_msg(it->second, msg); // send QUIT msg to all other clients
	}
	this->send_msg(client, "Closing connection for " + client_name + ": " + reason);
	client.set_quit(true);
	return (0);
}

int Server::handle_join(Client &client, const std::vector<std::string> &params)
{
	size_t	i;
	int		ret;

	std::map<std::string, Channel>::iterator chanit;
	std::vector<std::string> c_names;
	std::vector<std::string> keys;
	std::string chan_name;
	std::string key;
	if (params.empty() || params.size() > 2)
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
		chanit = this->_channels.find(chan_name);
		if (chanit == this->_channels.end())
		{
			Channel ch(chan_name, default_mode(), client);
			this->_channels.insert(std::pair<std::string, Channel>(chan_name, ch));
			this->send_msg(client, "You created Channel " + chan_name);
		}
		else
		{
			ret = chanit->second.joinChannel(client, key);
			if (ret == 1)
				this->send_error(client, ERR_USERONCHANNEL, chan_name);
			else if (ret == 2)
				this->send_error(client, ERR_CHANNELISFULL, chan_name);
			else if (ret == 3)
				this->send_error(client, ERR_INVITEONLYCHAN, chan_name);
			else if (ret == 4)
				this->send_error(client, ERR_BADCHANNELKEY, chan_name);
			else if(ret == 5)
				this->send_error(client, ERR_BANNEDFROMCHAN, chan_name);
			else
			{
				this->send_msg(client, "You joined " + chan_name);
				if(!chanit->second.getMode().topic.empty())
					this->send_msg(client, "Topic is: " + chanit->second.getMode().topic);
			}
		}
		i++;
	}
	return (0);
}

int Server::handle_fast(Client &client, const std::vector<std::string> &params)
{
	(void)params;
	client.set_auth(true);
	std::string to_send;
	client.set_nick(generateRandomString(3));
	client.set_user(generateRandomString(3));
	client.set_registered(true);
	client.set_host_ip(generateRandomString(9));
	to_send += "You shall be named: " + client.get_nick() + "\n";
	to_send += "Your home kingdom shall be: " + client.get_host_ip() + "\n";
	to_send += "Thy user shall be sung as: " + client.get_user() + "\n";
	to_send += "Go forth, make haste, child of Mercury\r\n";
	this->send_msg(client, to_send);
	return (0);
}

int Server::handle_mode(Client &client, const std::vector<std::string> &params)
{
	size_t	i;
	bool	res;

	std::string to_send = client.get_send_buffer();
	std::vector<t_modecmd> flags;
	if (params.empty() || params.size() < 2)
	{
		this->send_error(client, ERR_NEEDMOREPARAMS, "");
		return (-1);
	}
	if (int err = tokenize_mode_flags(params[1], flags, params, 2))
	{
		if(err == -1)
			this->send_error(client, ERR_UNKNOWNCOMMAND, params[1]);
		else if(err == -2)
			this->send_error(client, ERR_NEEDMOREPARAMS, params[1]);
		return (-1);
	}
	i = 0;
	res = false;
	while (i < flags.size())
	{
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
		case 'b':
			res = handle_bmode(client, params[0], flags[i]);
			break ;
		}
		if (!res)
			return (-1);
		i++;
	}
	return (0);
}
