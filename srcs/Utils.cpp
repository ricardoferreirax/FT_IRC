/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Utils.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/06 17:01:42 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/11/07 12:24:52 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Utils.hpp"
#include "../includes/Server.hpp"

t_cmode	default_mode(void)
{
	t_cmode	def;

	def.invite_only = false;
	def.topic_restricted = false;
	def.topic = "";
	def.users.u_count = 1;
	def.users.u_limit = 0;
	def.key = "";
	return (def);
}

int	validate_port(const std::string &port)
{
	int	value;

	if (port.empty() || port.size() > 5)
		throw std::runtime_error("IRC: invalid port!");
	for (size_t i = 0; i < port.size(); i++)
	{
		if (port[i] < '0' || port[i] > '9')
			throw std::runtime_error("IRC: port must contain only numbers!");
	}
	value = std::atoi(port.c_str());
	if (value < 1024 || value > 65535)
		throw std::runtime_error("IRC: port must be between 1024 and 65535!");
	return (value);
}

static bool	is_nick_special(char c)
{
	if (c == '[' || c == ']' || c == '\\' || c == '`' || c == '_' || c == '^'
		|| c == '{' || c == '|' || c == '}' || c == '-')
		return (true);
	return (false);
}

bool	is_valid_nick(const std::string &nick)
{
	if (nick.empty() || nick.size() > 9)
		return (false);
	if (!std::isalpha(nick[0]) && !is_nick_special(nick[0]))
		return (false);
	for (size_t i = 1; i < nick.size(); i++)
	{
		if (!std::isalnum(nick[i]) && !is_nick_special(nick[i]))
			return (false);
	}
	return (true);
}

bool	same_nick(const std::string &nick1, const std::string &nick2)
{
	if (nick1.size() != nick2.size()) // nicks with different sizes are not equal
		return (false);
	for (size_t i = 0; i < nick1.size(); i++)
	{
		if (std::tolower(nick1[i]) != std::tolower(nick2[i])) // compare nicks ignoring case
			return (false);
	}
	return (true);
}

// space separates: cmd from params and params from each other
void	parse_msg(const std::string &msg, std::string &cmd, std::vector<std::string> &params)
{
	size_t	space;

	std::string	remaining;
	std::string trailing;
	
	params.clear(); // clear previous params
	space = msg.find(' ');
	cmd = msg.substr(0, space);
	for (size_t i = 0; i < cmd.size(); i++)
        cmd[i] = std::toupper(cmd[i]);
	if (space == std::string::npos) // no space -> no params
		return ;
	remaining = msg.substr(space + 1); // everything after cmd is params
	while (!remaining.empty())
	{
		while (!remaining.empty() && remaining[0] == ' ')
			remaining.erase(0, 1); // rmv spaces before next param
		if (remaining.empty())
			break ;
		if (remaining[0] == ':' || params.size() == 14)
		{
			trailing = remaining; // after ':' is single param 
			if (trailing[0] == ':') 
				trailing.erase(0, 1);
			params.push_back(trailing); // add this single param
			break ;
		}
		space = remaining.find(' ');
		if (space == std::string::npos) // no space -> last param
		{
			params.push_back(remaining); // add last param
			break ;
		}
		params.push_back(remaining.substr(0, space)); // with space -> add param
		remaining.erase(0, space + 1); // rmv it so loop can parse next one
	}
}

std::string registration_reply(const Client &client, const std::string &_name, const std::string &date)
{
	std::string nick;
	std::string user;
	std::string host;
	std::string reply;

	nick = client.get_nick();
	user = client.get_user();
	host = client.get_host_ip();
	reply += ":" + _name + " 001 " + nick + " :Welcome to the Internet Relay Network " + nick + "!" + user + "@" + host + "\r\n";
	reply += ":" + _name + " 002 " + nick + " :Your host is " + _name + ", running version 1.1\r\n";
	reply += ":" + _name + " 003 " + nick + " :This server was created " + date + "\r\n";
	reply += ":" + _name + " 004 " + nick + " " + _name + " 1.1 io kost k\r\n";
	reply += "\r\n";
	return (reply);
}

bool	is_valid_user_mode(const std::string &mode)
{
	if (mode.empty())
		return (false);
	for (size_t i = 0; i < mode.size(); i++)
	{
		if (mode[i] < '0' || mode[i] > '9')
			return (false);
	}
	return (true);
}

std::vector<std::string> ft_split(const std::string &s, char delim)
{
	std::vector<std::string> result;
	std::stringstream ss(s);
	std::string item;
	
	while (getline(ss, item, delim))
	{
		if (item.empty())
			continue ;
		result.push_back(item);
	}
	return (result);
}

std::string generateRandomString(int length)
{
	const std::string chars = "abcdefghijklmnopqrstuvwxyz0123456789";
	std::string random_string = "";
	int	index;
	
	for (int i = 0; i < length; ++i)
	{
		index = rand() % chars.size();
		random_string += chars[index];
	}
	return (random_string);
}

bool match_ban(Client &client, t_banmask ban)
{
	if(!ban.valid)
		return false;
	return (match_wildcard(ban.host, client.get_host_ip()) && match_wildcard(ban.user, client.get_user()) && match_wildcard(ban.nick, client.get_nick()));
}

t_banmask	parseBanMask(std::string banmask)
{
	// Pedro's Anti-Cheat ban 😆
	t_banmask	PAC_ban;

	std::string::size_type exclamationPoint;
	std::string::size_type arrobaPos;
	std::string 		   remainer;
	std::string 		   nick;
	std::string 		   user;
	std::string 		   host;
	
	nick = "*";
	user = "*";
	host = "*";
	exclamationPoint = banmask.find("!");
	if (exclamationPoint != banmask.npos)
	{
		nick = banmask.substr(0, exclamationPoint);
		remainer = banmask.substr(exclamationPoint + 1);
		if (nick.empty())
		{
			PAC_ban.valid = false;
			return (PAC_ban);
		}
		arrobaPos = remainer.find("@");
		if (arrobaPos == remainer.npos)
		{
			PAC_ban.valid = false;
			return (PAC_ban);
		}
		user = remainer.substr(0, arrobaPos);
		host = remainer.substr(arrobaPos + 1);
		if (user.empty() || host.empty())
		{
			PAC_ban.valid = false;
			return (PAC_ban);
		}
	}
	else
	{
		arrobaPos = banmask.find("@");
		if (arrobaPos != banmask.npos)
		{
			user = banmask.substr(0, arrobaPos);
			host = banmask.substr(arrobaPos + 1);
			if (user.empty() || host.empty())
			{
				PAC_ban.valid = false;
				return (PAC_ban);
			}
		}
		else
		{
			if(banmask.empty())
			{
				PAC_ban.valid = false;
				return (PAC_ban);
			}
			host = banmask;
		}
	}
	PAC_ban.nick = nick;
	PAC_ban.user = user;
	PAC_ban.host = host;
	PAC_ban.valid = true;
	return (PAC_ban);
}

std::string get_error_param(const std::string &cmd, int error, const std::vector<std::string> &params)
{
	if (params.empty())
		return (cmd);
	if (cmd == "INVITE")
	{
		if (error == ERR_USERONCHANNEL && params.size() > 1)
			return (params[0] + " " + params[1]); // send error w/ target and channel as param
		if ((error == ERR_NOSUCHCHANNEL || error == ERR_NOTONCHANNEL || error == ERR_CHANOPRIVSNEEDED) && params.size() > 1)
			return (params[1]); // send error w/ channel as param
	}
	if (cmd == "KICK")
	{
		if (error == ERR_NOSUCHNICK && params.size() > 1)
			return (params[1]); // send error w/ nick as param
		if (error == ERR_USERNOTINCHANNEL && params.size() > 1)
			return (params[1] + " " + params[0]); // send error w/ target and channel as param
	}
	if (error == ERR_NOSUCHNICK || error == ERR_NICKNAMEINUSE || error == ERR_ERRONEOUSNICKNAME)
		return (params[0]); // send error w/ nick as param
	if (cmd == "JOIN" || cmd == "MODE" || cmd == "PART" || cmd == "TOPIC" || cmd == "KICK")
	{
		if (error == ERR_NOSUCHCHANNEL || error == ERR_NOTONCHANNEL || error == ERR_CHANOPRIVSNEEDED || error == ERR_INVITEONLYCHAN
			|| error == ERR_BADCHANNELKEY || error == ERR_CHANNELISFULL || error == ERR_BADCHANMASK || error == ERR_BANNEDFROMCHAN)
			return (params[0]); // send error w/ channel as param
	}
	if (cmd == "HELP" && error == ERR_HELPNOTFOUND)
		return (params[0]); // send error w/ help topic as param
	return (cmd); // send cmd as param
}

void	log_tcp(log_type type, int fd, ssize_t bytes, const std::string &data)
{
	bool	nl;
	bool	started;
	
	if (type == TCP_RECEIVE)
		std::cout << "\n\033[1;32m[Client " << fd << " -> Server]  Recv\033[0m   <<   ";
	else if (type == TCP_SEND)
		std::cout << "\033[1;34m[Server -> Client " << fd << "]  Send\033[0m   >>   ";
	else
		return ;
	nl = false;
	started = false;
	for (size_t i = 0; i < data.size(); i++)
	{
		if (data[i] == '\r')
			continue ;
		if (data[i] == '\n')
		{
			if (started)
				nl = true;
			continue ;
		}
		if (nl)
		{
			std::cout << "\n                                  ";
			nl = false;
		}
		std::cout << data[i];
		started = true;
	}
	std::cout << " \033[90m(" << bytes << "B)\033[0m\n";
}

bool match_wildcard(std::string pattern, std::string match, size_t pi, size_t mi)
{
	if(pattern[pi] == '\0')
		return match[mi] == '\0';
	if(pattern[pi] == '*')
	{
		if(match_wildcard(pattern , match, pi + 1, mi))
			return (true);
		if(match[mi] != '\0' && match_wildcard(pattern, match, pi, mi + 1))
			return (true);
		return (false);
	}
	if(match[mi] == '\0')
		return (false);
	if(match[mi] == pattern[pi])
		return(match_wildcard(pattern,  match, pi + 1, mi + 1));
	return false;
}
