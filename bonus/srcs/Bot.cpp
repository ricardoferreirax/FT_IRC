/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Bot.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/16 13:13:28 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/18 22:19:17 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Bot.hpp"
#include "../../includes/Server.hpp"
#include "../../includes/Utils.hpp"
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
    this->close_requests(-1);
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

void Bot::close_requests(int bot_fd)
{
    if (bot_fd == -1)
    {
        while (!this->_requests.empty())
            this->close_requests(this->_requests.begin()->first);
        return;
    }
    if (this->_epoll_fd >= 0)
        epoll_ctl(this->_epoll_fd, EPOLL_CTL_DEL, bot_fd, NULL);
    close(bot_fd);
    this->_requests.erase(bot_fd);
}

std::string Bot::create_request(const std::string &question)
{
    std::string content;
    std::string request;

    content = "{\"model\":\"qwen:latest\",";
    content += "\"system\":\"You are Miku, a reliable AI assistant. "
    "Prioritize factual accuracy over creativity. "
    "Give clear and correct answers. "
    "Only provide detailed explanations when requested. "
    "For mathematics, calculate carefully. "
    "For science and programming, use established knowledge. "
    "If uncertain, say so.\",";
    content += "\"prompt\":\"" + question + "\",";
    content += "\"stream\":false,";
    content += "\"keep_alive\":\"10m\",";
    content += "\"options\":{";
    content += "\"num_predict\":1024,";
    content += "\"num_ctx\":4096,";
    content += "\"temperature\":0.2";
    content += "}}";

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
    epoll_event event;
    t_request req;
    int bot_fd;
	int res;

    if (this->_epoll_fd < 0)
        return (false);
    bot_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (bot_fd < 0)
        return (false);
    if (fcntl(bot_fd, F_SETFL, O_NONBLOCK) < 0)
    {
        close(bot_fd);
        return (false);
    }
    addr.sin_family = AF_INET;
    addr.sin_port = htons(11434);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    res = connect(bot_fd, (sockaddr *)&addr, sizeof(addr));
    if (res < 0 && errno != EINPROGRESS)
    {
        close(bot_fd);
        return (false);
    }
    req.fd = bot_fd;
    req.client_fd = client_fd;
    req.connection_pending = (res < 0);
    req.response_complete = false;
    req.send_buffer = this->create_request(question);
    req.recv_buffer = "";
    req.bytes_sent = 0;
    event.events = EPOLLOUT;
    event.data.fd = bot_fd;
    if (epoll_ctl(this->_epoll_fd, EPOLL_CTL_ADD, bot_fd, &event) < 0)
    {
        close(bot_fd);
        return (false);
    }
    this->_requests[bot_fd] = req;
    return (true);
}

// EPOLLIN -> receive response FROM Ollama | EPOLLOUT -> send request TO Ollama
bool Bot::handle_request_data(int bot_fd, int event)
{
    t_request &req = this->_requests[bot_fd];
    char buffer[4096];
    const char *msg;
    size_t remaining;
    ssize_t bytes;

    if (event == EPOLLIN) // receive response
    {
        bytes = recv(bot_fd, buffer, sizeof(buffer), 0); // receive data FROM ollama through bot socket
        if (bytes > 0)
            req.recv_buffer.append(buffer, bytes); // append bytes to buffer
        else if (bytes == 0) // connection closed by ollama
            req.response_complete = true; // mark response as complete
    }
    else if (event == EPOLLOUT)
    {
        msg = req.send_buffer.c_str() + req.bytes_sent; // msg points to next byte to send
        remaining = req.send_buffer.size() - req.bytes_sent; // remaining bytes to send
        if (remaining == 0) // all bytes sent
            return (true); // true -> update to EPOLLIN for receiving response
        bytes = send(bot_fd, msg, remaining, MSG_NOSIGNAL); // send data TO ollama through bot socket
        if (bytes > 0)
            req.bytes_sent += bytes; // update bytes sent
        else if (bytes == 0) // connection closed by ollama
            return (false);
    }
    else // unknown event
        return (true);
    if (bytes < 0) // send or recv failed
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) // if send/recv would block or was interrupted
            return (true); // try again
        return (false);
    }
    return (true); // true -> update to EPOLLIN for receiving response
}

// process ready event from server epoll loop
bool Bot::handle_event(int bot_fd, int events)
{
    t_request &req = this->_requests[bot_fd];
    t_response response;
    epoll_event event;

    if (events & EPOLLERR)
        return (this->close_requests(bot_fd), false);
    if (events & EPOLLOUT)
    {
        if (!this->handle_request_data(bot_fd, EPOLLOUT))
            return (this->close_requests(bot_fd), false);
        if (req.bytes_sent == req.send_buffer.size())
        {
            event.events = EPOLLIN;
            event.data.fd = bot_fd;
            if (epoll_ctl(this->_epoll_fd, EPOLL_CTL_MOD, bot_fd, &event) < 0)
                return (this->close_requests(bot_fd), false);
        }
    }
    if (events & (EPOLLIN | EPOLLHUP))
    {
        if (!this->handle_request_data(bot_fd, EPOLLIN))
            return (this->close_requests(bot_fd), false);
    }
    if (req.response_complete)
    {
        response.client_fd = req.client_fd;
        response.msg = this->extract_response(req.recv_buffer);
        this->_responses.push_back(response);
        this->close_requests(bot_fd);
    }

    return (true);
}

bool Bot::get_response(t_response &response)
{
    if (this->_responses.empty()) // no responses available
        return (false);
    response = this->_responses[0]; // get first response
    this->_responses.erase(this->_responses.begin()); // remove it from vector
    return (true); // response retrieved
}

void Bot::send_responses(Server &server)
{
    t_response response;

    while (this->get_response(response))
        server.send_response(response.client_fd, response.msg);
}

std::string Bot::extract_response(const std::string &json)
{
    std::string result;
    size_t start;

    start = json.find("\"response\":\"");
    if (start == std::string::npos)
        return ("");
    start += 12;
    while (start < json.size()) // loop until end of json string
    {
        if (json[start] == '"')
            break; // stop at closing quote of response
        if (json[start] == '\\' && start + 1 < json.size()) // if escape found and not at end of string
        {
            start++; // move to next char
            if (json[start] == 'n')
                result += '\n'; // replace \n with newline
            else if (json[start] == 'r')
                result += '\r'; // replace \r with carriage return
            else if (json[start] == 't')
                result += '\t'; // replace \t with tab
            else
                result += json[start]; // add escaped char
        }
        else
            result += json[start]; // no escape -> add char to result
        start++;
    }
    return (result);
}
