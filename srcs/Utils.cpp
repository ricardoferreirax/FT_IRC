/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Utils.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/06 17:01:42 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/13 22:40:42 by rmedeiro         ###   ########.fr       */
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

void parse_params(std::string params, std::vector<std::string> &parsed)
{
	size_t space;

	while (!params.empty())
	{
		while (!params.empty() && params[0] == ' ')
			params.erase(0, 1); // remove spaces before next param
		if (params.empty())
			break;
		if (params[0] == ':') // after ':' is one single param
		{
			parsed.push_back(params.substr(1)); // add this single param to parsed
			break;
		}
		space = params.find(' '); // space separates params
		if (space == std::string::npos) // no space ->  is last param
		{
			parsed.push_back(params); // add last param
			break;
		}
		parsed.push_back(params.substr(0, space)); // add param to parsed
		params.erase(0, space); // remove it so loop can parse next one
	}
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
