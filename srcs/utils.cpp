/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/06 17:01:42 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/07 17:53:51 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Server.hpp"

bool Server::is_valid_nick(const std::string &nick)
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
