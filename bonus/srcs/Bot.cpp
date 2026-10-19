/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Bot.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/16 13:13:28 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/19 23:30:27 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Bot.hpp"
#include "../../includes/Server.hpp"
#include "../../includes/Utils.hpp"
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <cerrno>
#include <iostream>

Bot::Bot()
{
    this->_epoll_fd = -1;
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
    return (this->_requests.find(bot_fd) != this->_requests.end());
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

void Bot::clear_memory(int client_fd)
{
    std::map<int, t_request>::iterator it;
	int bot_fd;

    this->_memory.erase(client_fd); // clear memory for client
    it = this->_requests.begin();
    while (it != this->_requests.end()) // iterate all requests
    {
        if (it->second.client_fd == client_fd) // if request belongs to client
        {
            bot_fd = it->first; // get fd for that request
            ++it; // move to next request before deleting
            this->close_requests(bot_fd); // close request and remove from map
        }
        else
            ++it; // move to next request
    }
}

std::string Bot::create_request(int client_fd, const std::string &question)
{
    std::string content;
    std::string request;
    std::string prompt;

    prompt = this->_memory[client_fd];
    prompt += "User: " + question + " Assistant:";

    content = "{\"model\":\"qwen:latest\",";
    content += "\"system\":\"You are Miku, a japanese singer. "
    "Your name is Miku. "
    "The user is a different person from you. "
    "Remember important information about the user. "
    "Always answer the user's latest question. "
    "Use previous messages only to understand the context. "
    "Do not repeat previous answers unless the user asks you to. "
    "For simple questions, give short answers. "
    "If you do not know something, say so.\",";
    content += "\"prompt\":\"" + prompt + "\",";
    content += "\"stream\":false,";
    content += "\"options\":{\"num_predict\":512,";
    content += "\"num_ctx\":4096,\"temperature\":0.3}}";

    request = "POST /api/generate HTTP/1.1\r\n";
    request += "Host: 127.0.0.1:11434\r\n";
    request += "Content-Type: application/json\r\n";
    request += "Content-Length: " + nb_to_str(content.size()) + "\r\n";
    request += "Connection: close\r\n\r\n";
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
    bot_fd = socket(AF_INET, SOCK_STREAM, 0); // create a new socket to connect to ollama
    if (bot_fd < 0)
        return (false);
    if (fcntl(bot_fd, F_SETFL, O_NONBLOCK) < 0) // set socket non blocking to not block event loop
    {
        close(bot_fd);
        return (false);
    }
    addr.sin_family = AF_INET;
    addr.sin_port = htons(11434); // ollama default port
    addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // connect to ollama on localhost
    res = connect(bot_fd, (sockaddr *)&addr, sizeof(addr)); // 
    if (res < 0 && errno != EINPROGRESS)
    {
        close(bot_fd);
        return (false);
    }
    req.client_fd = client_fd;
    req.bytes_sent = 0;
    req.response_complete = false;
	req.question = question;
    req.send_buffer = this->create_request(client_fd, question); // create http request to send to ollama
    req.recv_buffer = ""; // init response buffer to receive ollama response
    event.events = EPOLLOUT; // begin by monitoring socket for writing -> send request to ollama
    event.data.fd = bot_fd;
    if (epoll_ctl(this->_epoll_fd, EPOLL_CTL_ADD, bot_fd, &event) < 0)
    {
        close(bot_fd);
        return (false);
    }
    this->_requests[bot_fd] = req; // store request data in map
    return (true);
}

// send() - send remaining request data TO ollama through bot socket.
// recv() - receive response data FROM ollama through bot socket.
bool Bot::handle_request_data(int bot_fd, int event)
{
    t_request &req = this->_requests[bot_fd];
    epoll_event epoll_event;
    char buffer[4096];
    ssize_t bytes;
    size_t remaining;

    if (event == EPOLLOUT) // if socket is ready for writing -> send request to ollama
    {
        remaining = req.send_buffer.size() - req.bytes_sent; // how many bytes still need to be sent
        if (remaining > 0)
        {
            bytes = send(bot_fd, req.send_buffer.c_str() + req.bytes_sent, remaining, MSG_NOSIGNAL);
            if (bytes <= 0)
                return (false);
            req.bytes_sent += bytes; // add bytes to total bytes sent
        }
        if (req.bytes_sent == req.send_buffer.size()) // when entire request has been sent -> wait for response
        {
            epoll_event.events = EPOLLIN;
            epoll_event.data.fd = bot_fd;
            if (epoll_ctl(this->_epoll_fd, EPOLL_CTL_MOD, bot_fd, &epoll_event) < 0) // update epoll to monitor socket for reading
                return (false);
        }
    }
    else if (event == EPOLLIN) // if socket is ready for reading -> receive response from ollama
    {
        bytes = recv(bot_fd, buffer, sizeof(buffer), 0);
        if (bytes < 0)
            return (false);
        if (bytes == 0)
        {
            req.response_complete = true; // if ollama closed connection after sending response -> response is complete
            return (true);
        }
        req.recv_buffer.append(buffer, bytes); // add received bytes to response buffer
    }
    return (true);
}

bool Bot::handle_event(Server &server, int bot_fd, int events)
{
    t_request &req = this->_requests[bot_fd];
    std::string answer;

    if (events & EPOLLERR)
    {
        this->close_requests(bot_fd); // close socket if epoll has an error
        return (false);
    }
    if ((events & EPOLLOUT) && !this->handle_request_data(bot_fd, EPOLLOUT)) // send http request to ollama
    {
        this->close_requests(bot_fd);
        return (false);
    }
    if ((events & EPOLLIN) && !this->handle_request_data(bot_fd, EPOLLIN)) // receive http response from ollama
    {
        this->close_requests(bot_fd);
        return (false);
    }
    if (req.response_complete)
    {
        answer = this->extract_response(req.recv_buffer);
        if (answer != "Ollama returned an error." && answer != "No response from Ollama.")
        {
            this->_memory[req.client_fd] += "User: " + req.question + " ";
			this->_memory[req.client_fd] += "Assistant: " + answer + " ";
        }
        server.send_response(req.client_fd, answer); // send completed response to client
        this->close_requests(bot_fd);
        return (true);
    }
    if ((events & EPOLLHUP) && !(events & EPOLLIN))
    {
        this->close_requests(bot_fd); // close socket if ollama disconnects unexpectedly
        return (false);
    }
    return (true);
}

std::string Bot::extract_response(const std::string &json)
{
    std::string result;
    size_t start;

    if (json.find("HTTP/1.1 200") != 0)
        return ("Ollama returned an error.");
    start = json.find("\"response\":\"");
    if (start == std::string::npos)
        return ("No response from Ollama.");
    start += 12;
    while (start < json.size())
    {
        if (json[start] == '"') // end of response string
            break;
        if (json[start] == '\\' && start + 1 < json.size())
        {
            start++;
            if (json[start] == 'n')
                result += '\n';
            else if (json[start] == 'r')
                result += '\r';
            else if (json[start] == 't')
                result += '\t';
            else
                result += json[start];
        }
        else
            result += json[start];
        start++;
    }
    return (result);
}
