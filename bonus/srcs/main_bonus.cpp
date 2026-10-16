/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main_bonus.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/16 16:07:29 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/16 17:22:16 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Bot.hpp"
#include <iostream>
#include <cstdlib>

int main(int ac, char **av)
{
    int epoll_fd;
    int port;

    if (ac != 2)
    {
        std::cerr << "Usage: ./miku <ollama_port>" << std::endl;
        return (EXIT_FAILURE);
    }
    port = std::atoi(av[1]);
    epoll_fd = epoll_create1(0);
    if (epoll_fd < 0)
    {
        std::cerr << "Miku: epoll_create1() failed." << std::endl;
        return (EXIT_FAILURE);
    }

    Bot bot(epoll_fd, port);

    if (!bot.start_request(-1, "Who is Hatsune Miku?"))
    {
        std::cerr << "Miku: request failed." << std::endl;
        return (EXIT_FAILURE);
    }
    std::cout << "Miku: request started!" << std::endl;
    return (EXIT_SUCCESS);
}
