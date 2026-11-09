/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/07 12:28:03 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/11/03 10:33:37 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLIENT_HPP
# define CLIENT_HPP

# include <ctime>
# include <iostream>
# include <string>

class Client
{
	private:
		std::string _recv_buffer;
		std::string _send_buffer;
		std::string _nick;
		std::string _user;
		std::string _host_ip;
		std::string _realname;
		std::string _away_msg;
		bool		_auth;
		bool		_registered;
		bool		_quit;
		bool		_server_op;
		int			_client_fd;
				
	public:
		Client();
		Client(int client_fd);

		std::string 	  &get_recv_buffer();
		std::string 	  &get_send_buffer();
		const std::string &get_nick() const;
		const std::string &get_user() const;
		const std::string &get_host_ip() const;
		const std::string &get_realname() const;
		const std::string &get_away_msg() const;
		
		int 	get_client_fd() const;
		bool	get_auth() const;
		bool	get_registered() const;
		bool	get_quit() const;

		void	set_nick(const std::string &nick);
		void	set_user(const std::string &user);
		void	set_host_ip(const std::string &host_ip);
		void	set_realname(const std::string &realname);
		void	set_away_msg(const std::string &msg);
		void 	set_server_op(bool value);

		void	clear_buffers();
		void	set_auth(bool auth);
		void	set_registered(bool registered);
		void	set_quit(bool quit);
		bool	can_register() const;
		bool	is_away() const;
		bool	is_server_op() const;
};

#endif
