/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/27 23:58:32 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/12 14:48:45 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
# define SERVER_HPP

# include "Client.hpp"
# include <csignal>
# include <map>
# include <string>
# include <sys/epoll.h>

# include "Client.hpp"

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
		// std::map<std::string, Channel>	_channels;

		void	accept_client();
		void	handle_client_data(int client_fd, int event);
		void	process_message(int client_fd, std::string &client_buffer);

		void	handle_cmd(int client_fd, const std::string &cmd, const std::string &params);
		void	handle_pass(Client &client, const std::string &params);
		void	handle_nick(Client &client, const std::string &params);
		void	handle_user(Client &client, const std::string &params);
		void	handle_privmsg(Client &client, const std::string &params);
		void	handle_ping(Client &client, const std::string &params);
		void	handle_quit(int client_fd, const std::string &params);

	public:
		Server(int port, const std::string &pass);
		Server(const Server &copy);
		~Server();

		std::string getName(void) const;
		void	start_socket();
		void	monitor_epoll_events();
		void	update_epoll(int client_fd, int events);
};

void	handle_signal(int signal);
void	setup_signals(void);

#endif
