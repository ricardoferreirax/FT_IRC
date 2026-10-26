/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channels.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/07 14:11:26 by pedro             #+#    #+#             */
/*   Updated: 2026/10/22 13:28:21 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CHANNELS_HPP
# define CHANNELS_HPP

# include "Client.hpp"
# include "Server.hpp"
# include <iostream>
# include <map>
# include <set>

typedef struct s_ulimit
{
	size_t	u_limit;
	size_t	u_count;
}	t_ulimit;

typedef struct s_mode
{
	std::string topic;
	std::string key;
	t_ulimit	users;
	bool		invite_only;
	bool		topic_restricted;
}	t_cmode;

typedef struct s_modecmd
{
	bool needs_param;
	std::string param;
	bool is_add;
	char flag;
} t_modecmd;


class Channel
{
	private:
		std::string _name;
		t_cmode	_mode;
		std::set<int> _chanops;
		std::map<int, Client*> _clients;
		std::set <int> _invites;
		Channel(void);

	public:
		Channel(std::string name, t_cmode mode, Client &chanop);
		~Channel(void);
		int joinChannel(Client &guest, std::string key);
		int leaveChannel(Client guest);
		bool inviteUser(Client guest);
		std::string	getName(void) const;
		std::set<int> getChannops(void) const;
		std::map<int, Client*> getClients(void) const;
		std::set<int> getInvites(void) const;
		t_cmode getMode(void) const;
		void setMode(bool invite, bool topic_restricted, std::string topic, t_ulimit users, std::string key);
};

#endif // !CHANNELS_HPP
