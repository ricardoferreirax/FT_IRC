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
#include "Utils.hpp"

class Channel
{
	private:
		std::string _name;
		t_cmode	_mode;
		// Pedro's Anti-Cheat (PAC)
		std::set<int> _chanops;
		std::map<int, Client*> _clients;
		std::set <int> _invites;
		Channel(void);
		std::vector<t_banmask> _bans;

	public:
		Channel(std::string name, t_cmode mode, Client &chanop);
		~Channel(void);
		int joinChannel(Client &guest, std::string key);
		int leaveChannel(Client guest);
		bool inviteUser(Client guest);
		std::string	getName(void) const;
		std::set<int> &getChannops(void);
		std::map<int, Client*> &getClients(void);
		std::set<int> &getInvites(void);
		t_cmode getMode(void) const;
		void setMode(bool invite, bool topic_restricted, std::string topic, t_ulimit users, std::string key);
		std::vector<t_banmask> getBanmask(void);
		bool addBanMask(std::string banmask);
		void removeBanMask(std::string banmask);

		int PAC_check(Client &guest);
};

#endif // !CHANNELS_HPP
