/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/27 23:58:32 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/21 17:48:17 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
# define SERVER_HPP

# include "../bonus/includes/Bot.hpp"
# include "Client.hpp"
# include <csignal>
# include <ctime>
# include <map>
# include <string>
# include <sys/epoll.h>
# include <vector>

extern volatile sig_atomic_t	alive;

class Channel;

enum	close_type
{
	CLIENT_DISCONNECT,
	SERVER_SHUTDOWN
};

class Server
{
	private:
		std::string	_name;
		std::string	_pass;
		std::string	_date;
		int	_port;
		int	_listen_fd;
		int	_epoll_fd;
		Bot	_bot;
		std::map<int, Client>	_clients;
		std::map<std::string, Channel>	_channels;

		void	accept_client();
		void	handle_socket_event(int fd, int events);
		void	handle_client_data(int client_fd, int event);
		void	process_msg(Client &client, std::string &client_buffer);
		void	close_connections(close_type type, int fd);

		void	disconnect_client(int client_fd);
		void	send_msg(Client &client, const std::string &msg, int events);
		void	send_error(Client &client, int error, const std::string &cmd);

		void	execute_cmd(Client &client, const std::string &cmd, const std::vector<std::string> &params);
		int 	handle_pass(Client &client, const std::vector<std::string> &params);
		int 	handle_nick(Client &client, const std::vector<std::string> &params);
		int 	handle_user(Client &client, const std::vector<std::string> &params);
		int 	handle_privmsg(Client &client, const std::vector<std::string> &params);
		int 	handle_ping(Client &client, const std::vector<std::string> &params);
		int 	handle_quit(Client &client, const std::vector<std::string> &params);
		int 	handle_join(Client &client, const std::vector<std::string> &params);
		int 	handle_fast(Client &client, const std::vector<std::string> &params);

	public:
		Server(const std::string &port, const std::string &pass);
		~Server();

		void	start_socket();
		void	monitor_epoll_events();
		void	send_response(int client_fd, const std::string &msg);

	std::string	getName(void) const;
};

void	handle_signal(int signal);
void	setup_signals(void);

#endif
