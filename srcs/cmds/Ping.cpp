/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Ping.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/08 21:26:45 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/11/08 21:27:42 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/Server.hpp"
#include "../../includes/Client.hpp"
#include "../../includes/Utils.hpp"

// ping is used to check if connection between client and server is still active.
int Server::ping_cmd(Client &client, const std::vector<std::string> &params)
{
	if (params.empty() || params[0].empty())
		return (ERR_NOORIGIN);
	this->send_notice(client, PONGREPLY, params[0]);
	return (0);
}
