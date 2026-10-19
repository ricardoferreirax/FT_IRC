/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Bot.hpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/16 13:03:54 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/19 21:18:05 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef BOT_HPP
#define BOT_HPP

#include <string>
#include <map>
#include <sys/epoll.h>
#include <netinet/in.h>

class	Server;

struct t_request
{
    int	client_fd;
    size_t bytes_sent;
    bool response_complete;
    std::string send_buffer;
    std::string recv_buffer;
};

class Bot
{
    private:
        int _epoll_fd;
        std::map<int, t_request> _requests;

        std::string create_request(const std::string &question);
        std::string extract_response(const std::string &json);
		bool handle_request_data(int bot_fd, int event);

    public:
        Bot();
        ~Bot();

        void set_epoll_fd(int epoll_fd);
        bool has_request(int bot_fd) const;
        void close_requests(int bot_fd);
        bool start_request(int client_fd, const std::string &question);
        bool handle_event(Server &server, int bot_fd, int events);
};

#endif
