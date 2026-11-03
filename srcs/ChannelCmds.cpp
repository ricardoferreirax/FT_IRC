/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ChannelCmds.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/03 13:21:43 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/11/03 16:44:25 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

# include "../includes/Server.hpp"
# include "../includes/Client.hpp"
# include "../includes/Utils.hpp"

void Server::execute_channels_cmds(Client &client, const std::string &cmd, const std::vector<std::string> &params)
{
	typedef int (Server::*ChanCmd)(Client &, const std::vector<std::string> &);
	std::map<std::string, ChanCmd>::iterator it;
	std::map<std::string, ChanCmd> 			 channels;
	std::string 							 param;
	int 							 		 ret;

	channels["JOIN"] = &Server::join_cmd;
	channels["INVITE"] = &Server::invite_cmd;
	channels["MODE"] = &Server::mode_cmd;
	channels["PART"] = &Server::part_cmd;
	it = channels.find(cmd);
	if (it == channels.end())
	{
		this->send_error(client, ERR_UNKNOWNCOMMAND, cmd);
		return ;
	}
	if (!client.get_registered())
	{
		this->send_error(client, ERR_NOTREGISTERED, cmd);
		return ;
	}
	ret = (this->*(it->second))(client, params);
	if (ret != 0)
	{
		param = cmd;
		if (cmd == "INVITE" && params.size() >= 2)
		{
			if (ret == ERR_USERONCHANNEL)
				param = params[0] + " " + params[1]; // send nick and channel error
			else if (ret == ERR_NOSUCHCHANNEL || ret == ERR_NOTONCHANNEL || ret == ERR_CHANOPRIVSNEEDED)
				param = params[1]; // send channel error
			else if (ret == ERR_NOSUCHNICK)
				param = params[0]; // send nick error
		}
		else if (cmd == "PART" && !params.empty() && (ret == ERR_NOSUCHCHANNEL || ret == ERR_NOTONCHANNEL))
			param = params[0]; // send channel error
		else if (cmd == "MODE" && !params.empty() && ret == ERR_NOSUCHCHANNEL)
			param = params[0]; // send channel error
		this->send_error(client, ret, param);
	}
}

int	Server::join_cmd(Client &client, const std::vector<std::string> &params)
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
int Server::invite_cmd(Client &client, const std::vector<std::string> &params)
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
	this->send_cmd(client, clienit->second, "INVITE " + params[0] + " :" + params[1]);
	return (0);
}

// TODO: Users can use MODE in a channel they are not part of nor have channop
int Server::mode_cmd(Client &client, const std::vector<std::string> &params)
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

int Server::part_cmd(Client &client, const std::vector<std::string> &params)
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
			this->privmsg_cmd(client, notify_chan);
		}
		i++;
	}
	return (0);
}
