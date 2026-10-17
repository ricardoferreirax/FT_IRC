/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Bot.hpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/16 13:03:54 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/17 14:55:57 by rmedeiro         ###   ########.fr       */
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

typedef struct request
{
    int fd;
    int client_fd;
    bool connection_pending;
	bool response_complete;
    std::string send_buffer;
    std::string recv_buffer;
    size_t bytes_sent;
} request;

class Bot
{
    private:
        int _epoll_fd;
        std::map<int, request> _requests;

        std::string create_request(const std::string &question);
        bool send_request(int bot_fd);
        bool receive_response(int bot_fd);

    public:
        Bot();
        Bot(int epoll_fd);
        ~Bot();

        void set_epoll_fd(int epoll_fd);
        void close_requests();

        bool has_request(int bot_fd) const;
        bool start_request(int client_fd, const std::string &question);
        bool handle_event(int bot_fd, uint32_t events);
};

#endif
