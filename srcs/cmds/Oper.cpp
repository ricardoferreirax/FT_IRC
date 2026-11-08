/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Oper.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/08 21:27:05 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/11/08 21:27:58 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/Server.hpp"
#include "../../includes/Client.hpp"
#include "../../includes/Utils.hpp"

int Server::oper_cmd(Client &client, const std::vector<std::string> &params)
{
	std::map<std::string, std::string>::iterator it;

	if (params.size() < 2 || params[0].empty() || params[1].empty())
		return (ERR_NEEDMOREPARAMS);
	it = this->_opers.find(params[0]);
	if (it == this->_opers.end())
		return (ERR_NOOPERHOST);
	if (it->second != params[1])
		return (ERR_PASSWDMISMATCH);
	client.set_server_op(true);
	this->send_reply(client, RPL_YOUREOPER, "", "");
	return (0);
}
