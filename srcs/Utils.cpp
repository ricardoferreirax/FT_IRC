/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Utils.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/06 17:01:42 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/09 13:14:20 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Utils.hpp"

bool is_valid_nick(const std::string &nick)
{
	if (nick.empty() || nick.size() > 9)
		return (false);
	if (!std::isalpha(nick[0]))
		return (false);
	for (size_t i = 1; i < nick.size(); i++)
	{
		if (!std::isalnum(nick[i]) && nick[i] != '_' && nick[i] != '-')
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
