/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/27 23:14:15 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/21 18:04:08 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Server.hpp"
#include "../includes/Utils.hpp"
#include <cstdlib>
#include <signal.h>

volatile sig_atomic_t	alive = 1;

void	handle_signal(int signal)
{
	(void)signal;
	alive = 0;
}

int	main(int ac, char **av)
{
	struct sigaction	action;

	if (ac != 3)
	{
		std::cerr << "./ircserv <port> <password>" << std::endl;
		return (EXIT_FAILURE);
	}
	try
	{
		action.sa_handler = handle_signal; // set handler function for sigint
		sigemptyset(&action.sa_mask); // empty mask so no signals are blocked during execution
		action.sa_flags = 0;
		if (sigaction(SIGINT, &action, NULL) < 0) // set action for sigint to specified handler
			throw std::runtime_error("IRC: sigaction() failed.");
		Server server(av[1], av[2]);
		server.start_socket();
		server.monitor_epoll_events();
	}
	catch (const std::exception &e)
	{
		std::cerr << e.what() << std::endl;
		return (EXIT_FAILURE);
	}
	return (EXIT_SUCCESS);
}
