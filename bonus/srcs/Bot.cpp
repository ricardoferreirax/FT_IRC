/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Bot.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/16 13:13:28 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/16 17:54:33 by rmedeiro         ###   ########.fr       */
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

bool Bot::has_request(int bot_fd) const
{
    if (this->_requests.find(bot_fd) == this->_requests.end())
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
    bot_request request;
    int bot_fd;
    int res;

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
    addr.sin_port = htons(this->_port);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    res = connect(bot_fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr));
    if (res < 0 && errno != EINPROGRESS)
    {
        std::cerr << "Miku: connect() failed." << std::endl;
        close(bot_fd);
        return (false);
    }
    request.fd = bot_fd;
    request.client_fd = client_fd;
    request.connection_pending = false;
    if (res < 0)
		request.connection_pending = true;
	request.response_complete = false;
    request.send_buffer = this->create_request(question);
    request.recv_buffer = "";
    request.bytes_sent = 0;
    this->_requests[bot_fd] = request;
    if (!this->handle_events(bot_fd))
    {
        this->_requests.erase(bot_fd);
        epoll_ctl(this->_epoll_fd, EPOLL_CTL_DEL, bot_fd, NULL);
        close(bot_fd);
        return (false);
    }
    return (true);
}

bool Bot::handle_events(int bot_fd)
{
    epoll_event event;
    int ready;
    bot_request &req = this->_requests[bot_fd];

    event.events = EPOLLOUT; // monitor socket for writing -> ready to send request
    event.data.fd = bot_fd; // store fd in event data
    if (epoll_ctl(this->_epoll_fd, EPOLL_CTL_ADD, bot_fd, &event) < 0)
        return (false);
    while (req.bytes_sent < req.send_buffer.size()) // waiting till entire request is sent
    {
        ready = epoll_wait(this->_epoll_fd, &event, 1, 3000);
        if (ready < 0) // if wait failed
        {
            if (errno == EINTR) // if was interrupted by a signal -> retry
                continue;
            return (false);
        }
        if (ready == 0) // timed out -> no events occurred within 3 secs
            return (false);
        if (event.events & (EPOLLERR | EPOLLHUP)) // if error or hangup event occurred
            return (false);
        if (event.events & EPOLLOUT) // if socket is ready for writing
        {
            if (!this->send_request(bot_fd)) // send request to ollama
                return (false);
        }
    }
	if (!this->handle_response(bot_fd)) // request fully sent -> start receiving ollama response
    	return (false);
	return (true);
}

bool Bot::send_request(int bot_fd)
{
    bot_request &req = this->_requests[bot_fd];
    const char *msg;
    size_t remaining;
    ssize_t sent;

    msg = req.send_buffer.c_str() + req.bytes_sent; // part of request that hasn't sent yet -> points to next byte to send
    remaining = req.send_buffer.size() - req.bytes_sent; // how many bytes are still waiting to be sent
    if (remaining == 0) // if all bytes have been sent
        return (true);
    sent = send(bot_fd, msg, remaining, MSG_NOSIGNAL); // send data TO the bot through the bot socket -> try send remaining bytes to ollama
    if (sent < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) // socket is busy or send was interrupted
            return (true); // request is still valid and can be retried later
        std::cerr << "Miku: send() failed." << std::endl;
        return (false); // an actual error occurred
    }
    if (sent == 0) // no data was sent -> connection closed
        return (false);
    req.bytes_sent += sent; // update the nb of bytes sent so far
    return (true);
}

bool Bot::receive_response(int bot_fd)
{
    bot_request &req = this->_requests[bot_fd];
    char buffer[4096]; // buffer to store received data -> 4096 bytes common size for network buffers
    ssize_t received;

    received = recv(bot_fd, buffer, sizeof(buffer), 0); // receive data FROM the bot through the bot socket -> read data sent by ollama
    if (received < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) // no data available yet or was interrupted
            return (true); // request is still valid and can be retried later
        std::cerr << "Miku: recv() failed." << std::endl;
        return (false);
    }
    if (received == 0) // connection closed -> entire response has been received
    {
        std::cout << "Miku: response received!" << std::endl;
        std::cout << req.recv_buffer << std::endl;
        return (true);
    }
    req.recv_buffer.append(buffer, received); // append bytes to buffer
    return (true);
}

bool Bot::handle_response(int bot_fd)
{
    epoll_event event;
    int ready;
    bot_request &req = this->_requests[bot_fd];

    event.events = EPOLLIN;
    event.data.fd = bot_fd;
    if (epoll_ctl(this->_epoll_fd, EPOLL_CTL_MOD, bot_fd, &event) < 0)
        return (false);
    while (!req.response_complete) // wait till ollama finishes sending response
    {
        ready = epoll_wait(this->_epoll_fd, &event, 1, 30000);
        if (ready < 0 && errno == EINTR) // wait was interrupted by a signal
            continue; // retry epoll_wait()
        if (ready <= 0) // wait failed or timed out
            return (false);
        if (event.events & EPOLLERR) // socket error occurred
            return (false);
        if (event.events & (EPOLLIN | EPOLLHUP)) // socket is ready for reading or connection closed
        {
            if (!this->receive_response(bot_fd)) // receive response from ollamma
                return (false);
        }
    }
    std::cout << req.recv_buffer << std::endl;
    return (true);
}
