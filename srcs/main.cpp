/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/27 23:14:15 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/09/28 05:55:49 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Server.hpp"
#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>

bool	checkPortRange(const std::string &str)
{
	int	port;

	if (str.empty())
		return (false);
	port = 0;
	for (size_t i = 0; i < str.length(); i++)
	{
		if (str[i] < '0' || str[i] > '9')
			return (false);
		if (port > 6553)
			return (false);
		port = port * 10 + (str[i] - '0');
		if (port > 65535)
			return (false);
	}
	return (port > 0);
}

bool	checkServerSetup(int ac, char **av)
{
	if (ac != 3)
	{
		std::cerr << "./ircserv <port> <password>" << std::endl;
		return (false);
	}
	if (!checkPortRange(av[1]))
	{
		std::cerr << "IRC: port must be between 1 and 65535." << std::endl;
		return (false);
	}
	if (av[2][0] == '\0')
	{
		std::cerr << "IRC: a server password is required." << std::endl;
		return (false);
	}
	return (true);
}

int	main(int ac, char **av)
{
	int	port;

	if (!checkServerSetup(ac, av))
		return (EXIT_FAILURE);
	port = std::atoi(av[1]);
	std::string pass = av[2];
	try
	{
		Server server(port, pass);
		std::cout << "IRC config accepted." << std::endl;
		std::cout << "Listening port: " << port << std::endl;
		server.initListener();
	}
	catch (const std::exception &e)
	{
		std::cerr << e.what() << std::endl;
		return (EXIT_FAILURE);
	}
	return (EXIT_SUCCESS);
}
