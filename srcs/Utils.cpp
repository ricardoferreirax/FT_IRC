/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Utils.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/06 17:01:42 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/12 16:40:29 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Utils.hpp"
#include <iostream>

bool	validate_args(char **av, int &port)
{
	if (av[1][0] == '\0')
	{
		std::cerr << "IRC: invalid port!" << std::endl;
		return (false);
	}
	port = 0;
	for (size_t i = 0; av[1][i] != '\0'; i++)
	{
		if (av[1][i] < '0' || av[1][i] > '9')
		{
			std::cerr << "IRC: port must contain only numbers!" << std::endl;
			return (false);
		}
		if (port > 6553)
		{
			std::cerr << "IRC: port must be between 1 and 65535!" << std::endl;
			return (false);
		}
		port = port * 10 + (av[1][i] - '0');
		if (port > 65535)
		{
			std::cerr << "IRC: port must be between 1 and 65535!" << std::endl;
			return (false);
		}
	}
	if (port < 1024)
	{
		std::cerr << "IRC: privileged port! Use a port above 1023!" << std::endl;
		return (false);
	}
	if (av[2][0] == '\0')
	{
		std::cerr << "IRC: a server password is required!" << std::endl;
		return (false);
	}
	return (true);
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

bool get_param(std::string &remaining, std::string &param)
{
	size_t space;

	if (remaining.empty()) // no params to extract
		return (false);
	space = remaining.find(' '); // space separates params
	if (space == 0) // if space is at beginning -> no param was provided
		return (false);
	if (space == std::string::npos) // has no space -> remaining is last param
	{
		param = remaining;
		remaining.clear(); // remove param from remaining
	}
	else // has space -> remaining has more params
	{
		param = remaining.substr(0, space); // extract param from remaining
		remaining.erase(0, space + 1); // remove param from remaining
	}
	return (true);
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
