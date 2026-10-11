/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/07 12:28:03 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/11 20:01:26 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLIENT_HPP
# define CLIENT_HPP

#include <iostream>
#include <string>

class Client
{
    private:
		int _client_fd;
        bool _auth;
        bool _registered;
		std::string _recv_buffer;
		std::string _send_buffer;
		std::string _nick;
		std::string _user;
		std::string _host_ip;
		
    public:
        Client();
        Client(int client_fd);

        bool can_register() const;

        std::string &get_recv_buffer();
        std::string &get_send_buffer();
	
		int get_client_fd() const;
		bool get_auth() const;
        bool get_registered() const;
        const std::string &get_nick() const;
		const std::string &get_user() const;
		const std::string &get_host_ip() const;

        void set_nick(const std::string &nick);
        void set_user(const std::string &user);
		void set_host_ip(const std::string &host_ip);
        void set_auth(bool auth);
        void set_registered(bool registered);
};

#endif
