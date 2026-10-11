/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/27 23:14:15 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/11 19:10:33 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Server.hpp"
#include "../includes/Utils.hpp"
#include <cstdlib>

volatile sig_atomic_t	running = 1;

void	handle_signal(int signal)
{
	(void)signal;
	running = 0;
}

int main(int ac, char **av)
{
	int					port;
	struct sigaction	action;

	if (ac != 3)
	{
		std::cerr << "./ircserv <port> <password>" << std::endl;
		return (EXIT_FAILURE);
	}
	if (!validate_args(av, port))
		return (EXIT_FAILURE);
	try
	{
		action.sa_handler = handle_signal; // set signal handler function for sigint
		sigemptyset(&action.sa_mask); // initialize mask to empty so no signals are blocked during execution of handler
		action.sa_flags = 0;
		if (sigaction(SIGINT, &action, NULL) < 0) // set action for sigint (ctrl-c) to specified handler
			throw std::runtime_error("IRC: sigaction() failed.");
		Server server(port, av[2]);
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
