/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/27 23:58:32 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/07 12:34:15 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
# define SERVER_HPP

#include <iostream>
#include <string>
#include <stdexcept>
#include <vector>
#include <map>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <csignal>
#include <sstream>

#include "Client.hpp"

extern volatile sig_atomic_t running;

class Server
{
    private:
        std::string _pass;
        int _port;
        int _listen_fd;
        int _epoll_fd;

		std::map<int, Client *> _clients; // stores client objects associated with each client fd

        std::vector<int> _client_fds; // stores fds of all currently connected clients
        std::map<int, std::string> _client_buffers; // stores the receive buffer associated with each client fd
		std::map<int, std::string> _client_output; // stores the output buffer associated with each client fd
		std::map<int, bool> _authenticated; // stores authentication status of each client fd
		std::map<int, std::string> _nicknames; // stores nickname associated with each client fd
		std::map<int, std::string> _usernames; // stores username associated with each client fd
		std::map<int, bool> _registered; // stores registration status of each client fd

        void accept_client();
        void receive_client_data(int client_fd);
		void send_client_data(int client_fd);
		void handle_cmd(int client_fd, const std::string &cmd, const std::string &params);
		void handle_pass(int client_fd, const std::string &params);
    	void handle_nick(int client_fd, const std::string &params);
    	void handle_user(int client_fd, const std::string &params);
		void register_client(int client_fd);
		
        void disconnect_client(int client_fd);

    public:
        Server(int port, const std::string &pass);
        ~Server();

        void start_socket();
        void monitor_epoll_events();
};

void handle_signal(int signal);
void setup_signals();

#endif
