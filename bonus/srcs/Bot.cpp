/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Bot.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/16 13:13:28 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/16 15:10:50 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Bot.hpp"
#include <iostream>
#include <cstdlib>

Bot::Bot(int epoll_fd, int port)
{
    this->_epoll_fd = epoll_fd;
    this->_port = port;
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

static std::string nb_to_str(size_t nb)
{
    std::string res;

    if (nb == 0)
        return ("0");
    while (nb > 0)
    {
        res.insert(res.begin(), '0' + (nb % 10));
        nb /= 10;
    }
    return (res);
}

std::string Bot::create_request(const std::string &question)
{
    std::string content; // json content for request
    std::string request; // http request to send to bot server

    content = "{\"model\":\"llama3.2\","; // model to use for generating response
    content += "\"prompt\":\"" + question + "\","; // prompt to send to model
    content += "\"stream\":false}"; // we want entire response at once not in chunks
    request = "POST /api/generate HTTP/1.1\r\n";
    request += "Host: 127.0.0.1:11434\r\n";
    request += "Content-Type: application/json\r\n";
    request += "Content-Length: " + nb_to_str(content.size()) + "\r\n";
    request += "Connection: close\r\n";
    request += "\r\n";
    request += content;
    return (request);
}

bool Bot::start_request(int client_fd, const std::string &question)
{
	sockaddr_in addr;
    int fd;

    (void)client_fd;
    (void)question;
    fd = socket(AF_INET, SOCK_STREAM, 0); // create socket for bot request
    if (fd < 0)
    {
        std::cerr << "Miku: socket() failed." << std::endl;
        return (false);
    }
    if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0) // set socket nonblocking
    {
        std::cerr << "Miku: fcntl() failed." << std::endl;
        close(fd);
        return (false);
    }
    addr.sin_family = AF_INET; // use ipv4 address
	addr.sin_port = htons(this->_port);
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	if (connect(fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0) // connect to bot server
	{
	    if (errno != EINPROGRESS) // if connection is not in progress -> connection failed
	    {
	        std::cerr << "Miku: connect() failed." << std::endl;
	        close(fd);
	        return (false);
	    }
	}
    std::cout << "Miku: connection initiated." << std::endl;
    close(fd);
    return (true);
}

// int main(int ac, char **av)
// {
//     if (ac != 2)
//     {
//         std::cerr << "Usage: ./test_bot <port>" << std::endl;
//         return (1);
//     }
//     int port = std::atoi(av[1]);
//     Bot bot(-1, port);
//     if (bot.start_request(5, "who is hatsune miku?"))
//         std::cout << "Connected!" << std::endl;
//     return (0);
// }
