/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Bot.hpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/16 13:03:54 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/16 13:30:01 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef BOT_HPP
# define BOT_HPP

# include <string>
# include <map>
# include <sys/epoll.h>

struct bot_request
{
    int         fd;
    int         client_fd;
    bool        connection_pending;
    std::string send_buffer;
    std::string recv_buffer;
    size_t      bytes_sent;
};

class Bot
{
    private:
        int 	_epoll_fd;

		std::map<int, bot_request> _requests; // store bot requests (fd, bot_request) pairs

    public:
        Bot(int epoll_fd);
        ~Bot();

		bool has_request(int fd) const;

};

#endif
