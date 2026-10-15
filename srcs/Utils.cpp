/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Utils.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/06 17:01:42 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/15 16:36:42 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Utils.hpp"

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

static bool is_nick_special(char c)
{
	if (c == '[' || c == ']' || c == '\\' || c == '`' || c == '_' || c == '^' 
		|| c == '{' || c == '|' || c == '}' || c == '-')
		return (true);
	return (false);
}

bool is_valid_nick(const std::string &nick)
{
	if (nick.empty() || nick.size() > 9)
		return (false);
	if (!std::isalpha(nick[0]) && !is_nick_special(nick[0]))
		return (false);
	for (size_t i = 1; i < nick.size(); i++)
	{
		if (!std::isalnum(nick[i]) && !is_nick_special(nick[i])) // nick can have alnum and special chars
			return (false);
	}
	return (true);
}

bool same_nick(const std::string &nick1, const std::string &nick2)
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
void parse_msg(const std::string &msg, std::string &cmd, std::vector<std::string> &params)
{
	std::string remaining;
	std::string trailing;
	size_t space;

	params.clear(); // clear previous params
	space = msg.find(' ');
	cmd = msg.substr(0, space);
	if (space == std::string::npos) // no space -> no params
		return;
	remaining = msg.substr(space + 1); // everything after cmd is params
	while (!remaining.empty())
	{
		while (!remaining.empty() && remaining[0] == ' ')
			remaining.erase(0, 1); // rmv spaces before next param
		if (remaining.empty())
			break;
		if (remaining[0] == ':' || params.size() == 14)
		{
			trailing = remaining; // after ':' is single param -> store it in trailing
			if (trailing[0] == ':') 
				trailing.erase(0, 1);
			params.push_back(trailing); // add this single param
			break;
		}
		space = remaining.find(' ');
		if (space == std::string::npos) // no space -> last param
		{
			params.push_back(remaining); // add last param
			break;
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
	reply += ":" + _name + " 001 " + nick + " :Welcome to the IRC Network " + nick + "!" + user + "@" + host + "\r\n";
	reply += ":" + _name + " 002 " + nick + " :Your host is " + _name + ", running version 1.0\r\n";
	reply += ":" + _name + " 003 " + nick + " :This server was created " + date + "\r\n";
	reply += ":" + _name + " 004 " + nick + " " + _name + " version 1.0 o itkol\r\n" + "\n";
	return (reply);
}

bool is_valid_user_mode(const std::string &mode)
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
	const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
	std::string random_string = "";
	int	index;

	for (int i = 0; i < length; ++i)
	{
		index = rand() % chars.size();
		random_string += chars[index];
	}
	return (random_string);
}
