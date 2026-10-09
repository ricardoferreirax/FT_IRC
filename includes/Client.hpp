/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/07 12:28:03 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/09 15:12:40 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLIENT_HPP
# define CLIENT_HPP

#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>

typedef struct s_ip
{
	sockaddr_in data;
	socklen_t	len;
} t_ip;

class Client
{
    private:
		int _fd;
        bool _auth;
        bool _registered;
		std::string _recv_buffer;
		std::string _send_buffer;
		std::string _nick;
		std::string _user;
		std::string _hostname;
		t_ip		_net;

    public:
        Client();
        Client(int fd, t_ip net);

        std::string &get_recv_buffer();
        std::string &get_send_buffer();
	
		int get_fd() const;
        bool get_registered() const;
        const std::string &get_nick() const;
		const std::string &get_hostname() const;

        void set_nick(const std::string &nick);
        void set_user(const std::string &user);
		void set_hostname(const std::string &hostname);
		
        void set_auth(bool auth);
        void set_registered(bool registered);

        bool can_register() const;
};

#endif
