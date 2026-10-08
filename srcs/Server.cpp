/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/28 05:42:35 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/08 17:18:30 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Server.hpp"

// port: where server will listen for incoming tcp connections
// pass: password clients will need during irc registration
Server::Server(int port, const std::string &pass)
{
	this->_pass = pass;
	this->_port = port;
	this->_listen_fd = -1;
	this->_epoll_fd = -1;
}

Server::~Server()
{
    std::map<int, Client>::iterator it;

    for (it = this->_clients.begin(); it != this->_clients.end(); ++it)
    {
        close(it->first);
    }
    if (this->_epoll_fd != -1)
        close(this->_epoll_fd);
    if (this->_listen_fd != -1)
        close(this->_listen_fd);
    std::cout << "[SERVER] Closed!" << std::endl;
}

// creates and prepares tcp listening socket used by irc server
void Server::start_socket()
{
	int	socket_opt;
	sockaddr_in server_addr;

    this->_listen_fd = socket(AF_INET, SOCK_STREAM, 0); // create ipv4 tcp listening socket
    if (this->_listen_fd < 0)
        throw std::runtime_error("IRC: socket() failed.");
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);  // configure socket to accept connections on any local network interface
    server_addr.sin_port = htons(this->_port);       // configure socket to listen on specified port (convert to network byte order)
    socket_opt = 1;
    if (setsockopt(this->_listen_fd, SOL_SOCKET, SO_REUSEADDR, &socket_opt, sizeof(socket_opt)) < 0) // enable SO_REUSEADDR so listening address can be reused after restarting the server
        throw std::runtime_error("IRC: setsockopt() failed.");
    if (fcntl(this->_listen_fd, F_SETFL, O_NONBLOCK) < 0) // configure socket as non blocking while performing socket operations
        throw std::runtime_error("IRC: fcntl() failed.");
    if (bind(this->_listen_fd, reinterpret_cast<sockaddr *>(&server_addr), sizeof(server_addr)) < 0) // associate socket with configured address and port so it can receive incoming connections
        throw std::runtime_error("IRC: bind() failed.");
    if (listen(this->_listen_fd, SOMAXCONN) < 0)         // changes socket into a listening socket so it can receive incoming tcp connection
        throw std::runtime_error("IRC: listen() failed.");
    std::cout << "[SERVER] Listening on 0.0.0.0:" << this->_port << std::endl;
}

// creates epoll to monitor listening socket and connected client sockets.
// listening socket is monitored for new connections, while client sockets are monitored for incoming and outgoing data.
// epoll_wait() blocks until one or more registered sockets become ready.
void Server::monitor_epoll_events()
{
    epoll_event event;
    epoll_event events[10];
    int ready_events; // nb of ready events returned by epoll_wait()
    int current_fd;

    this->_epoll_fd = epoll_create1(0); // create epoll to monitor socket events
    if (this->_epoll_fd < 0)
        throw std::runtime_error("IRC: epoll_create1() failed.");
    event.events = EPOLLIN; // monitor listening socket for new incoming connections
    event.data.fd = this->_listen_fd; // store listening socket fd inside event so it can be identified later
    if (epoll_ctl(this->_epoll_fd, EPOLL_CTL_ADD, this->_listen_fd, &event) < 0) // add listening socket to epoll interest list
        throw std::runtime_error("IRC: epoll_ctl() failed.");
    std::cout << "\n[SERVER] Waiting for connections..." << std::endl;
    while (running) // keep monitoring socket events till server is stopped
    {
        ready_events = epoll_wait(this->_epoll_fd, events, 10, -1); // wait till at least one registered socket has an event
        if (ready_events < 0)
        {
            if (!running) // stop server when ctrl-c
                break;
            throw std::runtime_error("IRC: epoll_wait() failed.");
        }
        for (int i = 0; i < ready_events; i++) // process every event returned by epoll_wait()
        {
            current_fd = events[i].data.fd; // get fd that generated current event
            if (current_fd == this->_listen_fd && (events[i].events & EPOLLIN)) // if returned event is from listening socket and is EPOLLIN
                this->accept_client(); // EPOLLIN on listening socket means a new client is waiting to connect
            else if (current_fd != this->_listen_fd)
			{
			    if (events[i].events & EPOLLIN) // if returned event is from a client socket and is EPOLLIN
			        this->handle_client_data(current_fd, RECV); // means new data is waiting to be read from this client
			    if (this->_clients.find(current_fd) == this->_clients.end()) // if client was disconnected don't try to send data to it
			        continue; // skip to next event
			    if (events[i].events & EPOLLOUT) // if returned event is from a client socket and is EPOLLOUT
			        this->handle_client_data(current_fd, SEND); // means client socket is ready to send data
			}
        }
    }
}

// client socket is made non-blocking, added to epoll and a Client object is created
// to store all data and state associated with this connection
void Server::accept_client()
{
	int client_fd;
	epoll_event	event;

    client_fd = accept(this->_listen_fd, NULL, NULL); // accept pending tcp connection and create new socket to communicate with client
    if (client_fd < 0)
        throw std::runtime_error("IRC: accept() failed.");
    if (fcntl(client_fd, F_SETFL, O_NONBLOCK) < 0) // make client socket non-blocking so socket operations don't block server
    {
        close(client_fd);
        throw std::runtime_error("IRC: fcntl() failed.");
    }
    event.events = EPOLLIN; // initially monitor client socket only for incoming data
    event.data.fd = client_fd; // store fd to server identify which client generated event
    if (epoll_ctl(this->_epoll_fd, EPOLL_CTL_ADD, client_fd, &event) < 0) // add client socket to epoll interest list
    {
        close(client_fd);
        throw std::runtime_error("IRC: epoll_ctl() failed.");
    }
	this->_clients[client_fd] = Client(client_fd); // create and store client object associated with its socket fd
    std::cout << "\n==========================================" << std::endl;
    std::cout << "        [CLIENT " << client_fd << "] CONNECTED!" << std::endl;
    std::cout << "==========================================\n" << std::endl;
}

// one recv() may contain a partial irc msg or multiple, only msgs ending with "\r\n" are processed,
// incomplete data stays in client's buffer and will be completed by a future recv().
// pending server replies are stored in send_buffer till they can be sent to this client, when output buffer becomes empty, 
// EPOLLOUT is removed because there is nothing left to send.
void Server::handle_client_data(int client_fd, client_event type)
{
    std::string &recv_buffer = this->_clients[client_fd].get_recv_buffer(); // buffer stores received data for this client
    std::string &send_buffer = this->_clients[client_fd].get_send_buffer(); // buffer has data waiting to be sent to this client
    char buffer[1024]; // temp buffer stores bytes received from client socket
    ssize_t bytes; // nb bytes received from or sent to client socket
    epoll_event event;

    if (type == RECV)
    {
        bytes = recv(client_fd, buffer, sizeof(buffer), 0); // receive/reads tcp data from client socket
        if (bytes < 0)
        {
            std::cerr << "[CLIENT " << client_fd << "] recv() failed." << std::endl;
            return;
        }
        if (bytes == 0) // if client has closed its connection -> recv() returns 0
        {
            this->disconnect_client(client_fd);
            return;
        }
        recv_buffer.append(buffer, bytes); // append received bytes to this client's buffer
        this->process_message(client_fd, recv_buffer); // process every complete irc msg stored in buffer
    }
    else if (type == SEND)
    {
        if (send_buffer.empty()) // nothing is waiting to be sent to this client
            return;
        bytes = send(client_fd, send_buffer.c_str(), send_buffer.size(), 0); // send/writes part of pending data from buffer to client socket
        if (bytes < 0)
        {
            std::cerr << "[CLIENT " << client_fd << "] send() failed." << std::endl;
            return;
        }
        send_buffer.erase(0, bytes); // remove only successfully sent bytes from buffer
        if (send_buffer.empty()) // if all pending data was sent socket don't need be monitored for writing
        {
            event.events = EPOLLIN; // keep monitoring client socket for new incoming data
            event.data.fd = client_fd; // store fd in event
            if (epoll_ctl(this->_epoll_fd, EPOLL_CTL_MOD, client_fd, &event) < 0) // update epoll and stop monitoring EPOLLOUT for this client
                throw std::runtime_error("IRC: epoll_ctl() failed.");
        }
    }
}

void Server::process_message(int client_fd, std::string &client_buffer)
{
    std::string msg;
    std::string cmd;
    std::string params;
    size_t pos;
    size_t space;

    pos = client_buffer.find("\r\n");
    while (pos != std::string::npos) // while there is at least one complete irc msg in buffer
    {
        msg = client_buffer.substr(0, pos); // extract msg from buffer ignoring "\r\n"
		if (msg.empty()) // if msg is emptyremove it from buffer and continue to next msg
		{
		    client_buffer.erase(0, pos + 2);
		    pos = client_buffer.find("\r\n"); // find next complete msg in buffer
		    continue;
		}
        space = msg.find(' '); // find space that separate cmd from params
        if (space == std::string::npos)
        {
            cmd = msg;
            params = "";
        }
        else
        {
            cmd = msg.substr(0, space); // extract cmd w/out params after space
            params = msg.substr(space + 1); // extract params after space
        }
        this->send_reply(client_fd, "Msg: " + msg + " | Cmd: " + cmd + " | Params: " + params + "\r\n");
        this->handle_cmd(client_fd, cmd, params); // process irc msg by executing its cmd with its params
		if (this->_clients.find(client_fd) == this->_clients.end()) // if client was disconnected don't try to send data to it
			return;
		this->send_reply(client_fd, "\r\n");
        client_buffer.erase(0, pos + 2);
        pos = client_buffer.find("\r\n");
    }
}

void Server::send_reply(int client_fd, const std::string &reply)
{
    epoll_event event;

    this->_clients[client_fd].get_send_buffer() += reply; // += allows multiple replies to not replace each other 
    event.events = EPOLLIN | EPOLLOUT; // monitor client for both incoming data and writting
    event.data.fd = client_fd; // store client fd in event
    if (epoll_ctl(this->_epoll_fd, EPOLL_CTL_MOD, client_fd, &event) < 0) // update client events to add EPOLLOUT
        throw std::runtime_error("IRC: epoll_ctl() failed.");
}

void Server::register_client(int client_fd)
{
    std::string nick; // store nick of client that completed registration

    if (this->_clients[client_fd].get_registered()) // if client is already registered
	{
        return;
	}
	if (this->_clients[client_fd].can_register())
	{
		this->_clients[client_fd].set_registered(true); // mark client as registered
		nick = this->_clients[client_fd].get_nick(); // store nick that has completed regist
		std::cout << "\n==========================================" << std::endl;
		std::cout << "     [CLIENT " << client_fd << "] REGISTERED!" << std::endl;
		std::cout << "==========================================\n" << std::endl;
		this->send_reply(client_fd, "\r\n");
		this->send_reply(client_fd, ":ircserv 001 " + nick + " :Welcome to the IRC server\r\n");
	}
}

void Server::handle_cmd(int client_fd, const std::string &cmd, const std::string &params)
{
    if ((cmd == "PASS" || cmd == "USER") && this->_clients[client_fd].get_registered())
    {
        this->send_reply(client_fd, ":ircserv 462 * :You may not reregister\r\n"); // ERR_ALREADYREGISTRED
        return;
    }
    if (cmd == "PASS")
        this->handle_pass(client_fd, params);
    else if (cmd == "NICK")
        this->handle_nick(client_fd, params);
    else if (cmd == "USER")
        this->handle_user(client_fd, params);
	else if (cmd == "QUIT")
	{
		this->handle_quit(client_fd, params);
		return;
	}
    else
    {
        this->send_reply(client_fd, ":ircserv 421 * " + cmd + " :Unknown command\r\n"); // ERR_UNKNOWNCOMMAND
        return;
    }
    this->register_client(client_fd);
}

// removes disconnected client from epoll, deletes its client object and closes socket associated with the connection
// EPOLL_CTL_DEL removes socket from epoll interest list
void Server::disconnect_client(int client_fd)
{
    if (epoll_ctl(this->_epoll_fd, EPOLL_CTL_DEL, client_fd, NULL) < 0) // stop monitoring client socket
        throw std::runtime_error("IRC: epoll_ctl() failed.");
    this->_clients.erase(client_fd); // remove client from server container
    close(client_fd);
    std::cout << "==========================================" << std::endl;
    std::cout << "     [CLIENT " << client_fd << "] DISCONNECTED!" << std::endl;
    std::cout << "==========================================\n" << std::endl;
}
