/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Bot.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/16 13:13:28 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/16 13:56:07 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Bot.hpp"
#include <iostream>

Bot::Bot(int epoll_fd)
{
    this->_epoll_fd = epoll_fd;
}

Bot::~Bot()
{
	
}

bool Bot::has_request(int fd) const
{
    if (this->_requests.find(fd) == this->_requests.end()) // check if fd exists in requests map
        return (false);
    return (true);
}

// int main()
// {
//     Bot bot(-1);

//     std::cout << "Request 5: " << bot.has_request(5) << std::endl;
//     std::cout << "Request 8: " << bot.has_request(8) << std::endl;
//     return (0);
// }