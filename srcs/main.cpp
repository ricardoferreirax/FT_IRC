/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/27 23:14:15 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/05 18:46:14 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Server.hpp"

volatile sig_atomic_t	running = 1;

void	handle_signal(int signal)
{
	(void)signal;
	running = 0;
}

void	setup_signals(void)
{
	struct sigaction	action;

	action.sa_handler = handle_signal; // set signal handler function for sigint
	sigemptyset(&action.sa_mask); // initialize mask to empty so no signals are blocked during execution of the handler
	action.sa_flags = 0;
	if (sigaction(SIGINT, &action, NULL) < 0) // set the action for sigint (ctrl-c) to the specified handler
		throw std::runtime_error("IRC: sigaction() failed.");
}

bool	validate_args(char **av, int &port)
{
	if (av[1][0] == '\0')
	{
		std::cerr << "IRC: invalid port!" << std::endl;
		return (false);
	}
	port = 0;
	for (size_t i = 0; av[1][i] != '\0'; i++)
	{
		if (av[1][i] < '0' || av[1][i] > '9')
		{
			std::cerr << "IRC: port must contain only numbers!" << std::endl;
			return (false);
		}
		if (port > 6553)
		{
			std::cerr << "IRC: port must be between 1 and 65535!" << std::endl;
			return (false);
		}
		port = port * 10 + (av[1][i] - '0');
		if (port > 65535)
		{
			std::cerr << "IRC: port must be between 1 and 65535!" << std::endl;
			return (false);
		}
	}
	if (port < 1024)
	{
		std::cerr << "IRC: privileged port! Use a port above 1023!" << std::endl;
		return (false);
	}
	if (av[2][0] == '\0')
	{
		std::cerr << "IRC: a server password is required!" << std::endl;
		return (false);
	}
	return (true);
}

int	main(int ac, char **av)
{
	int	port;

	if (ac != 3)
	{
		std::cerr << "./ircserv <port> <password>" << std::endl;
		return (EXIT_FAILURE);
	}
	if (!validate_args(av, port))
		return (EXIT_FAILURE);
	try
	{
		setup_signals();
		Server server(port, av[2]);
		server.start_socket();
		server.prepare_epoll();
		server.handle_events();
	}
	catch (const std::exception &e)
	{
		std::cerr << e.what() << std::endl;
		return (EXIT_FAILURE);
	}
	return (EXIT_SUCCESS);
}
