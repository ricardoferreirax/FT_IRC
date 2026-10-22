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

class	Channel
{
	private:
		std::string	_name;
		t_cmode 	_mode;
		
		std::map<int, Client> _clients;
		std::set<int> 		  _chanops;
		std::set<int> 		  _invites;
		
		Channel(void);

	public:
		Channel(std::string name, t_cmode mode, Client chanop);
		~Channel(void);
		
		bool	joinChannel(Client guest);
		int		joinChannel(Client guest, std::string key);
		bool	inviteUser(Client guest);
};

#endif
