/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/27 23:58:32 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/17 15:03:44 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
# define SERVER_HPP

#include <csignal>
#include <map>
#include <vector>
#include <string>
#include <sys/epoll.h>
#include <ctime>

#include "Client.hpp"
#include "../bonus/includes/Bot.hpp"

extern volatile sig_atomic_t running;

class Channel;

class Server
{
	private:
		std::string		_name;
		std::string		_pass;
		std::string		_date;
		int				_port;
		int				_listen_fd;
		int				_epoll_fd;
		Bot 			_bot;

		std::map<int, Client>	_clients;
		std::map<std::string, Channel>	_channels;

		void	accept_client();
		void	handle_client_data(int client_fd, int event);
		void	send_msg(Client &client, const std::string &msg, int events);
		void	process_msg(Client &client, std::string &client_buffer);
		void	execute_cmd(Client &client, const std::string &cmd, const std::vector<std::string> &params);
		
		void	handle_pass(Client &client, const std::vector<std::string> &params);
		void	handle_nick(Client &client, const std::vector<std::string> &params);
		void	handle_user(Client &client, const std::vector<std::string> &params);
		void	handle_privmsg(Client &client, const std::vector<std::string> &params);
		void	handle_ping(Client &client, const std::vector<std::string> &params);
		void 	handle_join(Client &client, const std::vector<std::string> &params);
		void	handle_fast(Client &client);
		void 	handle_miku(Client &client, const std::vector<std::string> &params);
		void	handle_quit(Client &client, const std::vector<std::string> &params);


	public:
		Server(const std::string &port, const std::string &pass);
		~Server();

		void	start_socket();
		void	monitor_epoll_events();

		std::string getName(void) const;
};

void	handle_signal(int signal);
void	setup_signals(void);

#endif
