/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Bot.hpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/16 13:03:54 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/11/16 22:40:06 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef BOT_HPP
# define BOT_HPP

# include <map>
# include <netinet/in.h>
# include <string>
# include <sys/epoll.h>
# include <ctime>

class	Server;

enum	clear_type
{
	REQUEST_COMPLETED,
	REQUEST_FAILED,
	CLIENT_DISCONNECTED,
	BOT_SHUTDOWN
};

typedef struct	s_request
{
	std::string	send_buffer;
	std::string	recv_buffer;
	std::string	question;
	std::string	nick;
	std::string channel;
	bool		response_complete;
	int			client_fd;
}	t_request;

class	Bot
{
	private:
		int			_epoll_fd;	

		std::map<std::string, std::string> _chan_memory;
		std::map<int, std::string>  	   _user_memory;
		std::map<int, t_request> 		   _requests;

		std::string create_request(int client_fd, const std::string &nick, const std::string &channel, const std::string &question);
		bool		handle_request_data(int bot_fd, int event);

	public:
		Bot();
		~Bot();

		bool	start_request(Server &server, int client_fd, const std::string &nick, const std::string &channel, const std::string &question);
		void	process_bot_response(t_request &req, Server &server);
		bool	handle_bot_event(Server &server, int bot_fd, int events);
		void	close_requests(int bot_fd);
		void 	cleanup(clear_type type, int fd, Server *server);
		void	clear_channel(const std::string &channel);
		bool	has_request(int bot_fd) const;
		
		void		set_epoll_fd(int epoll_fd);
		t_request	*get_request(int fd);
	};

#endif
