/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/27 23:58:32 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/22 13:53:56 by pedro            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
# define SERVER_HPP

#include "../bonus/includes/Bot.hpp"
#include "Client.hpp"
#include <csignal>
#include <map>
#include <vector>
#include <string>
#include <sys/epoll.h>
#include <ctime>

extern volatile sig_atomic_t running;

class Channel;
typedef struct s_modecmd t_modecmd;

class Server
{
	protected:
	private:
		std::string	_name;
		std::string	_pass;
		std::string	_date;
		int			_port;
		int			_listen_fd;
		int			_epoll_fd;
		Bot 		_bot;

		std::map<int, Client>	_clients;
		std::map<std::string, Channel>	_channels;

		void	accept_client();
		void	handle_client_data(int client_fd, int event);
		void	process_msg(Client &client, std::string &client_buffer);
		void 	disconnect_client(int client_fd);
		void	send_msg(Client &client, const std::string &msg, int events);
		void 	send_error(Client &client, int error, const std::string &cmd);
		
		void	execute_cmd(Client &client, const std::string &cmd, const std::vector<std::string> &params);
		int 	handle_pass(Client &client, const std::vector<std::string> &params);
		int 	handle_nick(Client &client, const std::vector<std::string> &params);
		int 	handle_user(Client &client, const std::vector<std::string> &params);
		int 	handle_privmsg(Client &client, const std::vector<std::string> &params);
		int 	handle_ping(Client &client, const std::vector<std::string> &params);
		int 	handle_quit(Client &client, const std::vector<std::string> &params);
		int 	handle_join(Client &client, const std::vector<std::string> &params);
		int 	handle_miku(Client &client, const std::vector<std::string> &params);
		int 	handle_fast(Client &client, const std::vector<std::string> &params);
		int handle_part(Client &client, const std::vector<std::string> &params);
		bool handle_imode(Client &client, std::string chan_name, t_modecmd flag);
		bool handle_tmode(Client &client, std::string chan_name, t_modecmd flag);
		bool handle_kmode(Client &client, std::string chan_name, t_modecmd flag);
		bool handle_omode(Client &client, std::string chan_name, t_modecmd flag);
		bool handle_lmode(Client &client, std::string chan_name, t_modecmd flag);
		Channel *validate_channel_op(Client &client, const std::string &channel_name, bool needs_sudo);

	int		handle_mode(Client &client, const std::vector<std::string> &params);
	
	public:
		Server(const std::string &port, const std::string &pass);
		~Server();
		
		void	start_socket();
		void	monitor_epoll_events();
		
		void 	send_response(int client_fd, const std::string &msg);
		std::string	getName(void) const;
};

void	handle_signal(int signal);
void	setup_signals(void);

bool	tokenize_mode_flags(const std::string &params,
		std::vector<t_modecmd> &flags, const std::vector<std::string> cmdargs, size_t arg_idx);
#endif
