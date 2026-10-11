/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/27 23:58:32 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/11 18:35:07 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
# define SERVER_HPP

# include <csignal>
# include <map>
# include <string>
# include <sys/epoll.h>

# include "Client.hpp"

extern volatile sig_atomic_t running;

class Server
{
	private:
		std::string				_name;
		std::string				_pass;
		int						_port;
		int						_listen_fd;
		int						_epoll_fd;

		std::map<int, Client>	_clients;

		void	accept_client();
		void	receive_client_data(int client_fd);
		void	send_client_data(int client_fd);
		void	update_epoll(int client_fd, int events);
		void	process_message(int client_fd, std::string &client_buffer);
		void	disconnect_client(int client_fd);

		void	handle_cmd(int client_fd, const std::string &cmd, const std::string &params);

		void	handle_pass(Client &client, const std::string &params);
		void	handle_nick(Client &client, const std::string &params);
		void	handle_user(Client &client, const std::string &params);
		void	handle_privmsg(Client &client, const std::string &params);

		void	handle_ping(int client_fd, const std::string &params);
		void	handle_quit(int client_fd, const std::string &params);

	public:
		Server(int port, const std::string &pass);
		~Server();

		void	start_socket();
		void	monitor_epoll_events();
};

void	handle_signal(int signal);
void	setup_signals(void);

#endif