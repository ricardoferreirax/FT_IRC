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

#include <iostream>
#include "Client.hpp"
#include "Server.hpp"
# include <map>
#include <set>

typedef enum e_mode
{
	NONE,
	INVITE_ONLY,
	TOPIC_RESTRICTED,
	CHANNEL_KEY,
	OPERATOR_PRIV,
	USER_LIMIT
};

class Channel
{
	private:
		std::string _name;
		int _mode;
		std::set<int> _chanops;
		std::map<int, Client> _clients;
		std::set<t_ip> _invite_ips;
		std::set<t_ip> _naughtyList;
		Channel(void);
	public:
		Channel(std::string name, int mode, Client chanop);
		~Channel(void);
		bool joinChannel(Client guest);
};
