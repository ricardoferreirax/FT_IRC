/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Commands.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/05 20:08:43 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/26 18:06:26 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Server.hpp"
#include "../includes/Utils.hpp"

void	Server::execute_cmd(Client &client, const std::string &cmd, const std::vector<std::string> &params)
{
	typedef int (Server::*Cmd)(Client &, const std::vector<std::string> &); // type cmd points to a member function of server
	std::map<std::string, Cmd> cmds; // cmd corresponds to a member function
	int						   error;
	
	cmds["PASS"] = &Server::handle_pass;
	cmds["NICK"] = &Server::handle_nick;
	cmds["USER"] = &Server::handle_user;
	cmds["PRIVMSG"] = &Server::handle_privmsg;
	cmds["JOIN"] = &Server::handle_join;
	cmds["PING"] = &Server::handle_ping;
	cmds["QUIT"] = &Server::handle_quit;
	cmds["FAST"] = &Server::handle_fast;
	cmds["INVITE"] = &Server::handle_invite;
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
	    if ((error == ERR_NOSUCHNICK || error == ERR_ERRONEOUSNICKNAME || error == ERR_NICKNAMEINUSE) && !params.empty()) // error is related to nick and params isn't empty
	        this->send_error(client, error, params[0]);
	    else if ((error == ERR_NOSUCHCHANNEL || error == ERR_NOTONCHANNEL || error == ERR_CHANOPRIVSNEEDED) && cmd == "INVITE" && params.size() >= 2) // error is related to channel and cmd is INVITE
	        this->send_error(client, error, params[1]); // send error with channel name as param
	    else if (error == ERR_USERONCHANNEL && cmd == "INVITE" && params.size() >= 2) // error is related to user and cmd is INVITE and params has at least 2 elements
	        this->send_error(client, error, params[0] + " " + params[1]); // send error with user and channel as param
	    else // for all other errors send error with cmd as param
	        this->send_error(client, error, cmd);
	    return ;
	}
	if (cmd == "QUIT")
		return ;
	if (!client.get_registered() && client.can_register())
	{
		client.set_registered(true);
		std::cout << "\n\033[1;36m" << "✓ CLIENT " <<  client.get_client_fd() << " REGISTERED" << "\033[0m\n\n";
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
	this->send_msg(client, ":" + this->_name + " NOTICE * :PASS accepted");
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
		this->send_msg(client, ":" + this->_name + " NOTICE * :Set nickname to " + nick);
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
	std::map<int, Client>::iterator it;
	std::string						target;
	std::string						msg;
	std::string						prefix;

	if (params.empty() || params[0].empty())
		return (ERR_NORECIPIENT);
	if (params.size() < 2 || params[1].empty())
		return (ERR_NOTEXTTOSEND);
	target = params[0];
	msg = params[1];
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
	for (it = this->_clients.begin(); it != this->_clients.end(); ++it)
	{
		if (same_nick(it->second.get_nick(), target)) // if target nick matches a client nick
		{
			prefix = ":" + client.get_nick() + "!" + client.get_user() + "@" + client.get_host_ip();
			this->send_msg(it->second, prefix + " PRIVMSG " + it->second.get_nick() + " :" + msg);
			return (0);
		}
	}
	return (ERR_NOSUCHNICK);
}

int	Server::handle_join(Client &client, const std::vector<std::string> &params)
{
	std::map<std::string, Channel>::iterator it;
	std::vector<std::string> 				 c_names;
	std::vector<std::string> 				 keys;
	std::string 							 chan_name;
	std::string 							 key;
	size_t 									 i;
	int 									 ret;

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
		it = this->_channels.find(chan_name);
		if (it == this->_channels.end())
		{
			Channel ch(chan_name, default_mode(), client);
			this->_channels.insert(std::pair<std::string, Channel>(chan_name, ch));
			this->send_msg(client, "You created Channel " + chan_name);
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
				this->send_msg(client, "You joined " + chan_name);
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
	if (chanit == this->_channels.end()) // channel doesn't exist
	{
		this->send_error(client, ERR_NOSUCHCHANNEL, params[1]);
		return (0);
	}
	clienit = this->_clients.begin();
	while (clienit != this->_clients.end())
	{
		if (clienit->second.get_nick() == params[0]) // target nick matches a client nick
			break ;
		clienit++;
	}
	if (clienit == this->_clients.end()) // target nick doesn't match any client nick
	{
		this->send_error(client, ERR_NOSUCHNICK, params[0]);
		return (0);
	}
	if (chanit->second.getMode().invite_only)
	{
		if (chanit->second.getChannops().find(client.get_client_fd()) == chanit->second.getChannops().end()) // client isn't channel operator
		{
			// this->send_error(client, 401, "You lack sudo");
			this->send_error(client, ERR_CHANOPRIVSNEEDED, params[1]);
			return (0);
		}
	}
	chanit->second.inviteUser(clienit->second);
	this->send_msg(client, ":" + this->_name + " 341 " + client.get_nick() + " " + params[0] + " " + params[1]); // confirmation msg to inviter
	// this->send_msg(clienit->second, "You have been invited to partcipate in " + chanit->second.getName());
	this->send_msg(clienit->second, ":" + client.get_nick() + "!" + client.get_user() + "@" + client.get_host_ip() + " INVITE " + params[0] + " :" + params[1]);
	return (0);
}
