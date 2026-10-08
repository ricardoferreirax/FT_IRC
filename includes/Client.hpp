/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/07 12:28:03 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/08 16:30:21 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLIENT_HPP
# define CLIENT_HPP

#include <iostream>
#include <string>

// 
class Client
{
    private:
        std::string _recv_buffer;
        std::string _send_buffer;
        std::string _nick;
        std::string _user;
        int _fd;
        bool _auth;
        bool _registered;

    public:
        Client();
        Client(int fd);
        ~Client();

        int get_fd() const;

        std::string &get_recv_buffer();
        std::string &get_send_buffer();

        const std::string &get_nick() const;
        const std::string &get_user() const;
        bool get_auth() const;
        bool get_registered() const;

        void set_nick(const std::string &nick);
        void set_user(const std::string &user);
        void set_auth(bool auth);
        void set_registered(bool registered);

		bool can_register() const;
};

#endif
