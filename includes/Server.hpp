/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/27 23:58:32 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/05 18:31:44 by rmedeiro         ###   ########.fr       */
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

extern volatile sig_atomic_t running;

class Server
{
    private:
        std::string _pass;
        int _port;
        int _listen_fd;
        int _epoll_fd;

        std::vector<int> _client_fds; // stores fds of all currently connected clients
        std::map<int, std::string> _client_buffers; // stores the receive buffer associated with each client fd

        void accept_client();
        void receive_data(int client_fd);
        void process_buffer(int client_fd);
        void disconnect_client(int client_fd);

    public:
        Server(int port, const std::string &pass);
        ~Server();

        void start_socket();
        void prepare_epoll();
        void handle_events();
};

void handle_signal(int signal);
void setup_signals();

#endif
