/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/27 23:58:32 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/11/11 22:22:33 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
#define SERVER_HPP

#include "Client.hpp"
#include "Channels.hpp"
#include "../bonus/includes/Bot.hpp"
#include <csignal>
#include <ctime>
#include <map>
#include <string>
#include <sys/epoll.h>
#include <vector>

extern volatile sig_atomic_t	alive;

class							Channel;
typedef struct s_modecmd		t_modecmd;

enum	close_type
{
	CLIENT_DISCONNECT,
	SERVER_SHUTDOWN,
	SERVER_RESTART
};

enum	e_socket_type
{
	CLIENT_SOCKET,
	BOT_SOCKET
};

enum	e_socket_status
{
	SOCKET_ERROR = -1,
	SOCKET_PENDING = 0,
	SOCKET_SUCCESS = 1
};

enum	e_reply
{
	RPL_WELCOME = 1,
	RPL_YOURHOST = 2,
	RPL_CREATED = 3,
	RPL_MYINFO = 4,
	RPL_AWAY = 301,
	RPL_UNAWAY = 305,
	RPL_NOWAWAY = 306,
	RPL_WHOISUSER = 311,
	RPL_WHOISSERVER = 312,
	RPL_ENDOFWHO = 315,
	RPL_ENDOFWHOIS = 318,
	RPL_WHOISCHANNELS = 319,
	RPL_CHANNELMODEIS = 324,
	RPL_NOTOPIC = 331,
	RPL_TOPIC = 332,
	RPL_INVITING = 341,
	RPL_WHOREPLY = 352,
	RPL_NAMREPLY = 353,
	RPL_ENDOFNAMES = 366,
	RPL_YOUREOPER = 381,
	RPL_HELPSTART = 704,
	RPL_HELPTXT = 705,
	RPL_ENDOFHELP = 706,
};

enum	e_irc_error
{
	ERR_NOSUCHNICK = 401,
	ERR_NOSUCHCHANNEL = 403,
	ERR_CANNOTSENDTOCHAN = 404,
	ERR_NOORIGIN = 409,
	ERR_NORECIPIENT = 411,
	ERR_NOTEXTTOSEND = 412,
	ERR_UNKNOWNCOMMAND = 421,
	ERR_NONICKNAMEGIVEN = 431,
	ERR_ERRONEOUSNICKNAME = 432,
	ERR_NICKNAMEINUSE = 433,
	ERR_USERNOTINCHANNEL = 441,
	ERR_NOTONCHANNEL = 442,
	ERR_USERONCHANNEL = 443,
	ERR_NOTREGISTERED = 451,
	ERR_NEEDMOREPARAMS = 461,
	ERR_ALREADYREGISTRED = 462,
	ERR_PASSWDMISMATCH = 464,
	ERR_KEYSET = 467,
	ERR_CHANNELISFULL = 471,
	ERR_UNKNOWNMODE = 472,
	ERR_INVITEONLYCHAN = 473,
	ERR_BANNEDFROMCHAN = 474,
	ERR_BADCHANNELKEY = 475,
	ERR_BADCHANMASK = 476,
	ERR_NOPRIVILEGES = 481,
	ERR_CHANOPRIVSNEEDED = 482,
	ERR_NOOPERHOST = 491,
	ERR_HELPNOTFOUND = 524,
	ERR_PASSREQUIRED = 601,
	ERR_INVALIDUSERMODE = 604,
};

enum	e_notice
{
	REGISTRATION,
	PASSACCEPTED,
	NICKSET,
	USERALREADYOP,
	USERNOTOP,
	NOCHANNELS,
	CHANNELCREATED,
	CHANNELJOINED,
	CHANNELKEYSET,
	CHANNELKEYREMOVED,
	CHANNELINVITEONLY,
	CHANNELINVITEOPEN,
	CHANNELTOPICRESTRICTED,
	CHANNELTOPICOPEN,
	CHANNELOPSET,
	CHANNELOPREMOVED,
	CHANNELLIMITSET,
	CHANNELLIMITREMOVED,
	INVALIDCHANNELLIMIT,
	TOPICCHANGED,
	PONGREPLY,
	ALREADYDISCONNECTED,
	KILLSUICIDE,
	MIKURESPONSE,
	MIKUUNAVAILABLE
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
		bool		_restart;
		Bot			_bot;
		
		std::map<std::string, std::string> _opers;
		std::map<std::string, Channel>	   _channels;
		std::map<int, Client>			   _clients;

		void	close_connections(close_type type, int fd);
		void	accept_client();
		int		handle_epoll_event(int fd, int events);
		int 	handle_socket_read(int fd, e_socket_type type, std::string &buffer);
		int 	handle_socket_write(int fd, e_socket_type type, std::string &buffer);
		
		void	send_cmd(Client &sender, Client &receiver, const std::string &message);
		void	send_reply(Client &client, int reply, const std::string &param, const std::string &text);
		void	send_error(Client &client, int error, const std::string &cmd);
		void 	send_names(Client &client, Channel &channel, const std::string &chan_name);
		void    send_channel_modes(Client &client, Channel &channel);
		void 	notify_new_op(Channel &channel, int fd);

		void	execute_cmd(Client &client, const std::string &cmd, const std::vector<std::string> &params);
		int		pass_cmd(Client &client, const std::vector<std::string> &params);
		int		nick_cmd(Client &client, const std::vector<std::string> &params);
		int		user_cmd(Client &client, const std::vector<std::string> &params);
		int		fast_cmd(Client &client, const std::vector<std::string> &params);
		int 	oper_cmd(Client &client, const std::vector<std::string> &params);
		int 	kill_cmd(Client &client, const std::vector<std::string> &params);
		int 	ping_cmd(Client &client, const std::vector<std::string> &params);
		int 	quit_cmd(Client &client, const std::vector<std::string> &params);
		int 	privmsg_cmd(Client &client, const std::vector<std::string> &params);
		int 	who_cmd(Client &client, const std::vector<std::string> &params);
		int 	whois_cmd(Client &client, const std::vector<std::string> &params);
		int 	help_cmd(Client &client, const std::vector<std::string> &params);
		int 	names_cmd(Client &client, const std::vector<std::string> &params);
		int 	away_cmd(Client &client, const std::vector<std::string> &params);
		int		join_cmd(Client &client, const std::vector<std::string> &params);
		int 	invite_cmd(Client &client, const std::vector<std::string> &params);
		int 	mode_cmd(Client &client, const std::vector<std::string> &params);
		int 	part_cmd(Client &client, const std::vector<std::string> &params);
		int 	kick_cmd(Client &client, const std::vector<std::string> &params);
		int 	topic_cmd(Client &client, const std::vector<std::string> &params);

		bool 	handle_imode(Client &client, std::string chan_name, t_modecmd flag);
		bool 	handle_tmode(Client &client, std::string chan_name, t_modecmd flag);
		bool 	handle_kmode(Client &client, std::string chan_name, t_modecmd flag);
		bool 	handle_omode(Client &client, std::string chan_name, t_modecmd flag);
		bool 	handle_lmode(Client &client, std::string chan_name, t_modecmd flag);
		bool 	handle_bmode(Client &client, std::string chan_name, t_modecmd flag);
		
		void 	leaveChans(Client client);
		bool 	check_allchans(const std::vector<std::string> &chans, Client *client = NULL);
		std::map<int, Client>::iterator reverseClientSearch(std::string nick);
		Channel *validate_channel_op(Client &client, const std::string &channel_name);

	public:
		Server(const std::string &port, const std::string &pass);
		~Server();
		
		std::map<int, Client> &getClients(void);
		std::string	getName(void) const;
		
		void	start_socket();
		void	monitor_epoll_events();
		int		handle_socket_data(int fd, int event, e_socket_type type);
		void	send_notice(Client &client, int notice, const std::string &param);
		int 	restart_cmd(Client &client, const std::vector<std::string> &params);
		void 	restart_server();
};

void	handle_signal(int signal);
void	setup_signals(void);

int 	tokenize_mode_flags(const std::string &params, std::vector<t_modecmd> &flags, const std::vector<std::string> cmdargs, size_t arg_idx);

#endif
