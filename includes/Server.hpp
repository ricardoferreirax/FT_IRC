/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/27 23:58:32 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/15 13:22:55 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
# define SERVER_HPP

#include <csignal>
#include <map>
#include <vector>
#include <string>
#include <sys/epoll.h>

#include "Client.hpp"

extern volatile sig_atomic_t running;

class Channel;

class Server
{
	protected:
	private:
		std::string				_name;
		std::string				_pass;
		int						_port;
		int						_listen_fd;
		int						_epoll_fd;

		std::map<int, Client>	_clients;
		std::map<std::string, Channel>	_channels;

		void	accept_client();
		void	handle_client_data(int client_fd, int event);
		void	process_msg(Client &client, std::string &client_buffer);
		void	execute_cmd(Client &client, const std::string &cmd, const std::vector<std::string> &params);
		
		void	handle_pass(Client &client, const std::vector<std::string> &params);
		void	handle_nick(Client &client, const std::vector<std::string> &params);
		void	handle_user(Client &client, const std::vector<std::string> &params);
		void	handle_privmsg(Client &client, const std::vector<std::string> &params);
		void	handle_ping(Client &client, const std::vector<std::string> &params);
		void	handle_quit(Client &client, const std::vector<std::string> &params);

		void 	handle_join(Client &client, const std::vector<std::string> &params);
		void	handle_invite(Client &client, const std::vector<std::string> &params);
		void	handle_mode(Client &client, const std::vector<std::string> &params);
		void	handle_fast(Client &client);

	public:
		Server(int port, const std::string &pass);
		Server(const Server &copy);
		~Server();

		std::string getName(void) const;
		void	update_epoll(int client_fd, int events);
		void	start_socket();
		void	monitor_epoll_events();
};

void	handle_signal(int signal);
void	setup_signals(void);

#endif
