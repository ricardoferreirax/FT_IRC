/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Utils.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/06 17:01:42 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/08 16:15:35 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Utils.hpp"

bool is_valid_nick(const std::string &nick)
{
    if (nick.empty())
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

    space = remaining.find(' ');
    if (space == std::string::npos || space == 0)
        return (false);
    param = remaining.substr(0, space);
    remaining.erase(0, space + 1);
    return (true);
}
