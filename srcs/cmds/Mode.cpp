/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Mode.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/08 21:12:27 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/11/08 21:13:07 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/Server.hpp"
#include "../../includes/Client.hpp"
#include "../../includes/Utils.hpp"

int Server::mode_cmd(Client &client, const std::vector<std::string> &params)
{
	std::map<std::string, Channel>::iterator chanit;
	std::vector<t_modecmd> 					 flags;
	size_t									 i;
	int										 ret;
	int										 newopFD;
	bool									 res;

	if (params.empty() || params[0].empty())
		return (ERR_NEEDMOREPARAMS);
	chanit = this->_channels.find(params[0]);
	if (chanit == this->_channels.end())
		return (ERR_NOSUCHCHANNEL);
	if (params.size() < 2)
	{
		this->send_channel_modes(client, chanit->second); // send channel modes to client
		return (0);
	}
	if (!chanit->second.getChannops().count(client.get_client_fd()))
		return (ERR_CHANOPRIVSNEEDED); // user is not a chanop
	ret = tokenize_mode_flags(params[1], flags, params, 2);
	if (ret == -1)
		return (ERR_UNKNOWNMODE);
	if (ret == -2)
		return (ERR_NEEDMOREPARAMS);
	if (ret != 0)
		return (0);
	i = 0;
	while (i < flags.size())
	{
		res = false;
		newopFD = -1; // no new chanop is set
		if (flags[i].flag == 'o' && !flags[i].is_add && chanit->second.getChannops().size() == 1)
			newopFD = *chanit->second.getChannops().begin(); // chanop is rmvd and there are no more chanops->set to first client in channel
		switch (flags[i].flag)
		{
		case 'i':
			res = handle_imode(client, params[0], flags[i]);
			break ;
		case 't':
			res = handle_tmode(client, params[0], flags[i]);
			break ;
		case 'k':
			res = handle_kmode(client, params[0], flags[i]);
			break ;
		case 'o':
			res = handle_omode(client, params[0], flags[i]);
			break ;
		case 'l':
			res = handle_lmode(client, params[0], flags[i]);
			break ;
		case 'b':
			res = handle_bmode(client, params[0], flags[i]);
			break ;
		}
		if (!res)
			return (0);
		this->notify_mode_change(client, chanit->second, flags[i]); // notify clients about mode change
		if (newopFD > 0 && !chanit->second.getChannops().count(newopFD))
			this->notify_new_op(chanit->second, *chanit->second.getChannops().begin()); // notify clients about new chanop
		i++;
	}
	return (0);
}
