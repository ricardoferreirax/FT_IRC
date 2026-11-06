/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channels.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/07 14:11:26 by pedro             #+#    #+#             */
/*   Updated: 2026/11/06 10:37:41 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CHANNELS_HPP
#define CHANNELS_HPP

#include "Utils.hpp"
#include "Client.hpp"
#include "Server.hpp"
#include <iostream>
#include <map>
#include <set>

class	Channel
{
	private:
		std::string   			_name;
		t_cmode 	  			_mode;
		std::map<int, Client *> _clients;
		std::set<int> 			_chanops;
		std::set<int> 			_invites;
		std::vector<t_banmask>  _bans;
		Channel(void);

	public:
		Channel(std::string name, t_cmode mode, Client &chanop);
		~Channel(void);
		
		std::map<int, Client *> &getClients(void);
		std::set<int> 			&getChannops(void);
		std::set<int> 			&getInvites(void);
		std::string   			getName(void) const;
		std::vector<t_banmask>  getBanmask(void);
		t_cmode 				getMode(void) const;
				
		int	 joinChannel(Client &guest, std::string key);
		int  leaveChannel(Client &guest);
		int  PAC_check(Client &guest);
		bool addBanMask(std::string banmask);
		bool inviteUser(Client &guest);
		void removeBanMask(std::string banmask);
		void setMode(bool invite, bool topic_restricted, std::string topic, t_ulimit users, std::string key);
};

#endif // !CHANNELS_HPP
