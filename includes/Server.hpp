/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/27 23:58:32 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/08 16:14:41 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
# define SERVER_HPP

#include <csignal>
#include <cstdlib>
#include <cctype>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <map>
#include <netinet/in.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

#include "Client.hpp"

extern volatile sig_atomic_t running;

enum client_event
{
	RECV,
    SEND
};

class Server
{
    private:
        std::string _pass;
        int _port;
        int _listen_fd;
        int _epoll_fd;

        std::map<int, Client> _clients; // store all connected clients with their socket fd corresponding to their client
		
        void accept_client();
		void handle_client_data(int client_fd, client_event event);
        void process_message(int client_fd, std::string &client_buffer);
		void send_reply(int client_fd, const std::string &reply);
        void disconnect_client(int client_fd);

        void handle_cmd(int client_fd, const std::string &cmd, const std::string &params);
        void handle_pass(int client_fd, const std::string &params);
        void handle_nick(int client_fd, const std::string &params);
        void handle_user(int client_fd, const std::string &params);
        void register_client(int client_fd);

    public:
        Server(int port, const std::string &pass);
        ~Server();

        void start_socket();
        void monitor_epoll_events();
};

void handle_signal(int signal);
void setup_signals(void);

#endif
