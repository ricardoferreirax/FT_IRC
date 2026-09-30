/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/27 23:58:32 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/09/30 18:20:12 by rmedeiro         ###   ########.fr       */
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
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/epoll.h>

class Server
{
    private:
        std::string _pass;
        int _port;
        int _listen_fd;
        int _epoll_fd;

        std::vector<int> _client_fds;  // stores the fds of all currently connected clients so server knows which clients are active and manage them accordingly.
        std::map<int, std::string> _client_buffers;

        void add_to_epoll(int fd);
        void accept_client();
        void receive_data(int client_fd);
        void process_buffer(int client_fd);
        void disconnect_client(int client_fd);

    public:
        Server(int port, const std::string &pass);
        ~Server();

        void start_socket();
        void start_epoll();
};

#endif
