/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Utils.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/06 17:01:42 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/10 00:14:44 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Utils.hpp"

static bool is_nick_special(char c)
{
	if (c == '[' || c == ']' || c == '\\' || c == '`' || c == '_' || c == '^' 
		|| c == '{' || c == '|' || c == '}')
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
		if (!std::isalnum(nick[i]) && !is_nick_special(nick[i]) && nick[i] != '-') // nick can have alnum, special chars and '-'
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
		if (std::tolower(nick1[i]) != std::tolower(nick2[i]))
			return (false);
	}
	return (true);
}

bool get_param(std::string &remaining, std::string &param)
{
	size_t space;

	if (remaining.empty())
		return (false);
	space = remaining.find(' ');
	if (space == 0)
		return (false);
	if (space == std::string::npos)
	{
		param = remaining;
		remaining.clear();
	}
	else
	{
		param = remaining.substr(0, space);
		remaining.erase(0, space + 1);
	}
	return (true);
}
