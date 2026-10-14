/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channels.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pedro </var/spool/mail/pedro>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/07 14:11:26 by pedro             #+#    #+#             */
/*   Updated: 2026/10/07 14:18:07 by pedro            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


#ifndef CHANNELS_HPP
#define CHANNELS_HPP

#include <iostream>
#include "Client.hpp"
#include "Server.hpp"
# include <map>
#include <set>

typedef struct s_ulimit
{
	size_t u_limit;
	size_t u_count;
} t_ulimit;

typedef struct s_mode
{
	bool invite_only;
	bool topic_restricted;
	std::string topic;
	t_ulimit users;
	std::string key;
} t_cmode;

class Channel
{
	private:
		std::string _name;
		t_cmode	_mode;
		std::set<int> _chanops;
		std::map<int, Client> _clients;
		std::set <int> _invites;
		Channel(void);
	public:
		Channel(std::string name, t_cmode mode, Client chanop);
		~Channel(void);
		bool joinChannel(Client guest);
		int joinChannel(Client guest, std::string key);
		bool inviteUser(Client guest);
};

#endif // !CHANNELS_HPP

