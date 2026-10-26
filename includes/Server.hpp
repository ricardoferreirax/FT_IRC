/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/27 23:58:32 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/26 20:40:33 by rmedeiro         ###   ########.fr       */
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

class							Channel;
typedef struct s_modecmd		t_modecmd;

enum	close_type
{
	CLIENT_DISCONNECT,
	SERVER_SHUTDOWN
};

enum	e_irc_error
{
	ERR_NOSUCHNICK = 401,
	ERR_NOSUCHCHANNEL = 403,
	ERR_NOORIGIN = 409,
	ERR_NORECIPIENT = 411,
	ERR_NOTEXTTOSEND = 412,
	ERR_UNKNOWNCOMMAND = 421,
	ERR_NONICKNAMEGIVEN = 431,
	ERR_ERRONEOUSNICKNAME = 432,
	ERR_NICKNAMEINUSE = 433,
	ERR_NOTONCHANNEL = 442,
    ERR_USERONCHANNEL = 443,
	ERR_NOTREGISTERED = 451,
	ERR_NEEDMOREPARAMS = 461,
	ERR_ALREADYREGISTRED = 462,
	ERR_PASSWDMISMATCH = 464,
	ERR_CHANNELISFULL = 471,
    ERR_INVITEONLYCHAN = 473,
    ERR_BADCHANNELKEY = 475,
	ERR_BADCHANMASK = 476,
	ERR_CHANOPRIVSNEEDED = 482,
	ERR_PASSALREADYACCEPTED = 601,
	ERR_PASSREQUIRED = 602,
	ERR_NICKREQUIRED = 603,
	ERR_INVALIDUSERMODE = 604,
	ERR_MIKUUNAVAILABLE = 605
};

class	Server
{
	private:
		std::string	_name;
		std::string	_pass;
		std::string	_date;
		int			_port;
		int			_listen_fd;
		int			_epoll_fd;
		Bot			_bot;
		
		std::map<std::string, Channel>	_channels;
		std::map<int, Client>			_clients;

		void	accept_client();
		void	handle_client_data(int client_fd, int event);
		void	process_msg(Client &client, std::string &client_buffer);
		void	close_connections(close_type type, int fd);

		void	disconnect_client(int client_fd);
		void 	send_msg(Client &client, const std::string &msg);
		void	send_error(Client &client, int error, const std::string &cmd);

		void	execute_cmd(Client &client, const std::string &cmd, const std::vector<std::string> &params);
		int 	handle_pass(Client &client, const std::vector<std::string> &params);
		int 	handle_nick(Client &client, const std::vector<std::string> &params);
		int 	handle_user(Client &client, const std::vector<std::string> &params);
		int 	handle_privmsg(Client &client, const std::vector<std::string> &params);
		int		handle_invite(Client &client, const std::vector<std::string> &params);
		int 	handle_part(Client &client, const std::vector<std::string> &params);

		int 	handle_ping(Client &client, const std::vector<std::string> &params);
		int 	handle_quit(Client &client, const std::vector<std::string> &params);
		int 	handle_join(Client &client, const std::vector<std::string> &params);
		int 	handle_fast(Client &client, const std::vector<std::string> &params);

		int		handle_mode(Client &client, const std::vector<std::string> &params);
		bool 	handle_imode(Client &client, std::string chan_name, t_modecmd flag);
		bool 	handle_tmode(Client &client, std::string chan_name, t_modecmd flag);
		bool 	handle_kmode(Client &client, std::string chan_name, t_modecmd flag);
		bool 	handle_omode(Client &client, std::string chan_name, t_modecmd flag);
		bool 	handle_lmode(Client &client, std::string chan_name, t_modecmd flag);
		
		Channel *validate_channel_op(Client &client, const std::string &channel_name, bool needs_sudo);

	public:

	public:
		Server(const std::string &port, const std::string &pass);
		~Server();
		
		std::string	getName(void) const;
		
		void	start_socket();
		void	monitor_epoll_events();
		void	send_response(int client_fd, const std::string &msg);

};

void	handle_signal(int signal);
void	setup_signals(void);

bool	tokenize_mode_flags(const std::string &params, std::vector<t_modecmd> &flags, const std::vector<std::string> cmdargs, size_t arg_idx);

#endif
