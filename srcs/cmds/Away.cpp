/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Away.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/08 21:20:59 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/11/08 21:23:06 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/Server.hpp"
#include "../../includes/Client.hpp"
#include "../../includes/Utils.hpp"

int Server::away_cmd(Client &client, const std::vector<std::string> &params)
{
	if (params.empty() || params[0].empty()) // no away msg provided
	{
		client.set_away_msg("");
		this->send_reply(client, RPL_UNAWAY, "", "");
	}
	else // away msg provided
	{
		client.set_away_msg(params[0]);
		this->send_reply(client, RPL_NOWAWAY, "", "");
	}
	return (0);
}
