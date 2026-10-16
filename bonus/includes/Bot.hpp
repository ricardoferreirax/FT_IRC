/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Bot.hpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/16 13:03:54 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/16 15:04:37 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef BOT_HPP
# define BOT_HPP

#include <string>
#include <map>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <cstdio>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <cerrno>
#include <cstring>

typedef struct bot_request
{
    int fd;
    int client_fd;
    bool connection_pending;
    std::string send_buffer;
    std::string recv_buffer;
    size_t bytes_sent;
} bot_request;

class Bot
{
    private:
        int _epoll_fd;
		int _port;
		std::map<int, bot_request> _requests; // store bot requests (fd, bot_request) pairs

		std::string create_request(const std::string &question);
		
	public:
        Bot(int epoll_fd, int port);
        ~Bot();
		
		bool has_request(int fd) const;
		bool start_request(int client_fd, const std::string &question);
};

#endif
