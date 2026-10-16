/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Bot.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/16 13:13:28 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/16 16:13:17 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Bot.hpp"
#include <iostream>
#include <cstdlib>
#include <cerrno>

Bot::Bot(int epoll_fd, int port)
{
    this->_epoll_fd = epoll_fd;
    this->_port = port;
}

Bot::~Bot()
{
    std::map<int, bot_request>::iterator it;
	
    for (it = this->_requests.begin(); it != this->_requests.end(); ++it)
    {
        epoll_ctl(this->_epoll_fd, EPOLL_CTL_DEL, it->first, NULL);
        close(it->first);
    }
}

bool Bot::has_request(int fd) const
{
    if (this->_requests.find(fd) == this->_requests.end())
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
    std::string content;
    std::string request;

    content = "{\"model\":\"llama3.2\",";
    content += "\"prompt\":\"" + question + "\",";
    content += "\"stream\":false}";
    request = "POST /api/generate HTTP/1.1\r\n";
    request += "Host: 127.0.0.1:" + nb_to_str(this->_port) + "\r\n";
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
    bot_request req;
    int fd;
    int res;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
    {
        std::cerr << "Miku: socket() failed." << std::endl;
        return (false);
    }
    if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0)
    {
        std::cerr << "Miku: fcntl() failed." << std::endl;
        close(fd);
        return (false);
    }
    addr.sin_family = AF_INET;
    addr.sin_port = htons(this->_port);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    res = connect(fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr));
    if (res < 0 && errno != EINPROGRESS)
    {
        std::cerr << "Miku: connect() failed." << std::endl;
        close(fd);
        return (false);
    }
    req.fd = fd;
    req.client_fd = client_fd;
    req.connection_pending = false;
    if (res < 0)
        req.connection_pending = true;
    req.send_buffer = this->create_request(question);
    req.recv_buffer = "";
    req.bytes_sent = 0;
    this->_requests[fd] = req;
    if (!this->handle_events(fd))
    {
        this->_requests.erase(fd);
        epoll_ctl(this->_epoll_fd, EPOLL_CTL_DEL, fd, NULL);
        close(fd);
        return (false);
    }
    return (true);
}

bool Bot::handle_events(int fd)
{
    epoll_event event;
    int ready;

    event.events = EPOLLOUT; // monitor for write events -> ready to send data
    event.data.fd = fd; // store event with bot fd
    if (epoll_ctl(this->_epoll_fd, EPOLL_CTL_ADD, fd, &event) < 0)
    {
        std::cerr << "Miku: epoll_ctl() failed." << std::endl;
        return (false);
    }
    ready = epoll_wait(this->_epoll_fd, &event, 1, 3000); // wait with a timeout of 3 secs
    if (ready < 0)
    {
        std::cerr << "Miku: epoll_wait() failed." << std::endl;
        return (false);
    }
    if (ready == 0)
    {
        std::cerr << "Miku: timeout." << std::endl;
        return (false);
    }
    if (!this->has_request(event.data.fd)) // check if event is for a valid request
        return (false);
    if (event.events & (EPOLLERR | EPOLLHUP)) // if error or hangup event occurred
    {
        std::cerr << "Miku: connection failed." << std::endl;
        return (false);
    }
    if (event.events & EPOLLOUT)
    {
        std::cout << "Miku: socket ready for writing." << std::endl;
        return (true);
    }
    return (false);
}
