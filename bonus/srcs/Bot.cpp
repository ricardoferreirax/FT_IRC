/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Bot.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/16 13:13:28 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/17 14:58:50 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Bot.hpp"
#include <iostream>
#include <cstdlib>
#include <cerrno>

Bot::Bot()
{
    this->_epoll_fd = -1;
}

Bot::Bot(int epoll_fd)
{
    this->_epoll_fd = epoll_fd;
}

Bot::~Bot()
{
    this->close_requests();
}

void Bot::set_epoll_fd(int epoll_fd)
{
    this->_epoll_fd = epoll_fd;
}

bool Bot::has_request(int bot_fd) const
{
    if (this->_requests.find(bot_fd) == this->_requests.end())
        return (false);
    return (true);
}

void Bot::close_requests()
{
    std::map<int, request>::iterator it;

    for (it = this->_requests.begin(); it != this->_requests.end(); ++it)
    {
        if (this->_epoll_fd >= 0)
            epoll_ctl(this->_epoll_fd, EPOLL_CTL_DEL, it->first, NULL);
        close(it->first);
    }
    this->_requests.clear();
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

    content = "{\"model\":\"qwen:latest\",";
    content += "\"prompt\":\"" + question + "\",";
    content += "\"stream\":false,";
    content += "\"options\":{\"num_predict\":512}}";
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
    request req;
    epoll_event event;
    int bot_fd;
    int res;

    if (this->_epoll_fd < 0)
        return (false);
    bot_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (bot_fd < 0)
    {
        std::cerr << "Miku: socket() failed." << std::endl;
        return (false);
    }
    if (fcntl(bot_fd, F_SETFL, O_NONBLOCK) < 0)
    {
        std::cerr << "Miku: fcntl() failed." << std::endl;
        close(bot_fd);
        return (false);
    }
    addr.sin_family = AF_INET;
    addr.sin_port = htons(11434);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    res = connect(bot_fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr));
    if (res < 0 && errno != EINPROGRESS)
    {
        std::cerr << "Miku: connect() failed." << std::endl;
        close(bot_fd);
        return (false);
    }
    req.fd = bot_fd;
	req.client_fd = client_fd;
	req.connection_pending = true;
	if (res == 0)
		req.connection_pending = false;
	req.response_complete = false;
	req.send_buffer = this->create_request(question);
	req.recv_buffer = "";
	req.bytes_sent = 0;
	this->_requests[bot_fd] = req;
    event.events = EPOLLOUT;
    event.data.fd = bot_fd;
    if (epoll_ctl(this->_epoll_fd, EPOLL_CTL_ADD, bot_fd, &event) < 0)
    {
        std::cerr << "Miku: epoll_ctl() failed." << std::endl;
        close(bot_fd);
        return (false);
    }
    this->_requests[bot_fd] = req;
    return (true);
}

// send pending request data to Ollama
bool Bot::send_request(int bot_fd)
{
    request &req = this->_requests[bot_fd];
    const char *msg;
    size_t remaining;
    ssize_t sent;

    msg = req.send_buffer.c_str() + req.bytes_sent;
    remaining = req.send_buffer.size() - req.bytes_sent;
    if (remaining == 0)
        return (true);
    sent = send(bot_fd, msg, remaining, MSG_NOSIGNAL);
    if (sent < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
            return (true);
        std::cerr << "Miku: send() failed." << std::endl;
        return (false);
    }
    if (sent == 0)
        return (false);
    req.bytes_sent += sent;
    return (true);
}

// receive response data from Ollama
bool Bot::receive_response(int bot_fd)
{
    request &req = this->_requests[bot_fd];
    char buffer[4096];
    ssize_t received;

    received = recv(bot_fd, buffer, sizeof(buffer), 0);
    if (received < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
            return (true);
        std::cerr << "Miku: recv() failed." << std::endl;
        return (false);
    }
    if (received == 0)
    {
        req.response_complete = true;
        return (true);
    }
    req.recv_buffer.append(buffer, received);
    return (true);
}

// Process a ready event from the IRC server epoll loop
bool Bot::handle_event(int bot_fd, uint32_t events)
{
    epoll_event event;
    request &req = this->_requests[bot_fd];
    bool success;

	success = true;
    if (events & EPOLLERR)
        success = false;
    if (success && (events & EPOLLOUT))
    {
        if (!this->send_request(bot_fd))
            success = false;
        if (success && req.bytes_sent == req.send_buffer.size())
        {
            event.events = EPOLLIN;
            event.data.fd = bot_fd;
            if (epoll_ctl(this->_epoll_fd, EPOLL_CTL_MOD,
                bot_fd, &event) < 0)
                success = false;
            else
                std::cout << "Miku: HTTP request sent!" << std::endl;
        }
    }
    if (success && (events & (EPOLLIN | EPOLLHUP)))
    {
        if (!this->receive_response(bot_fd))
            success = false;
    }
    if (success && req.response_complete)
    {
        std::cout << "Miku: response received!" << std::endl;
        std::cout << req.recv_buffer << std::endl;
    }
    if (!success || req.response_complete)
    {
        epoll_ctl(this->_epoll_fd, EPOLL_CTL_DEL, bot_fd, NULL);
        close(bot_fd);
        this->_requests.erase(bot_fd);
    }
    return (success);
}
