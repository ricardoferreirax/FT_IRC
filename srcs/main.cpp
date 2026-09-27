/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/27 23:14:15 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/09/27 23:45:02 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <iostream>
#include <string>
#include <cstdlib>

bool checkPortRange(const std::string &str)
{
    int port;

    if (str.empty())
    {
        return (false);
    }
    port = 0;
    for (size_t i = 0; i < str.length(); i++)
    {
        if (str[i] < '0' || str[i] > '9')
            return (false);
        if (port > 6553)
        {
            return (false);
        }
        port = port * 10 + (str[i] - '0');
        if (port > 65535)
            return (false);
    }
    return (port > 0);
}

bool checkServerSetup(int ac, char **av)
{
    if (ac != 3)
    {
        std::cerr << "./ircserv <port> <password>" << std::endl;
        return (false);
    }
    if (!checkPortRange(av[1]))
    {
        std::cerr << "IRC failed O.O: port must be between 1 and 65535." << std::endl;
        return (false);
    }
    if (av[2][0] == '\0')
    {
        std::cerr << "IRC failed O.O: a server password is required." << std::endl;
        return (false);
    }
    return (true);
}

int main(int ac, char **av)
{
    std::string pass;
    int port;

    if (!checkServerSetup(ac, av))
        return (EXIT_FAILURE);
    port = std::atoi(av[1]);
    pass = av[2];

    std::cout << "IRC configuration accepted." << std::endl;
    std::cout << "Listening port configured: " << port << std::endl;

    (void)pass; // pass will be passed to the server later.

    return (EXIT_SUCCESS);
}
