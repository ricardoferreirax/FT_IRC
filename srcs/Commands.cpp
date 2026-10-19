/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Commands.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/05 20:08:43 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/19 22:05:32 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Server.hpp"
#include "../includes/Utils.hpp"

void Server::execute_cmd(Client &client, const std::string &cmd, const std::vector<std::string> &params)
{
    typedef int (Server::*Cmd)(Client &, const std::vector<std::string> &); // type cmd points to a member function of Server
    std::map<std::string, Cmd> cmds; // map each cmd to its corresponding member function
    int error;

    cmds["PASS"] = &Server::handle_pass;
    cmds["NICK"] = &Server::handle_nick;
    cmds["USER"] = &Server::handle_user;
    cmds["PRIVMSG"] = &Server::handle_privmsg;
    cmds["JOIN"] = &Server::handle_join;
    cmds["PING"] = &Server::handle_ping;
    cmds["QUIT"] = &Server::handle_quit;
    cmds["FAST"] = &Server::handle_fast;
    if (cmds.find(cmd) == cmds.end()) // check if cmd is in the map
    {
        this->send_error(client, 421, cmd); // unknown command error
        return;
    }
    if ((cmd == "PRIVMSG" || cmd == "JOIN" || cmd == "MIKU") && !client.get_registered())
    {
        this->send_error(client, 451, cmd); // not registered error
        return;
    }
    error = (this->*cmds[cmd])(client, params);
    if (error != 0)
    {
        this->send_error(client, error, cmd);
        return;
    }
    if (cmd == "QUIT")
        return;
    if (!client.get_registered() && client.can_register())
    {
        client.set_registered(true);
        std::cout << "\n=====================================================" << std::endl;
        std::cout << "          CLIENT " << client.get_client_fd() << " REGISTERED!" << std::endl;
        std::cout << "=====================================================" << std::endl;
        this->send_msg(client, registration_reply(client, this->_name, this->_date), EPOLLIN | EPOLLOUT);
    }
}

int Server::handle_pass(Client &client, const std::vector<std::string> &params)
{
    if (client.get_registered())
        return (462);
    if (client.get_auth())
    {
        this->send_msg(client, ":" + this->_name + " NOTICE * :PASS already accepted\r\n", EPOLLIN | EPOLLOUT);
        return (0);
    }
    if (params.size() != 1)
        return (461);
    if (params[0] != this->_pass)
        return (464);
    client.set_auth(true);
    this->send_msg(client, ":" + this->_name + " NOTICE * :PASS accepted\r\n" + "\r\n", EPOLLIN | EPOLLOUT);
    return (0);
}

int Server::handle_nick(Client &client, const std::vector<std::string> &params)
{
    std::map<int, Client>::iterator it;
    std::string nick;
    int client_fd;

    client_fd = client.get_client_fd();
    if (!client.get_auth())
    {
        this->send_msg(client, ":" + this->_name + " NOTICE * :PASS must be accepted before NICK\r\n", EPOLLIN | EPOLLOUT);
        return (0);
    }
    if (params.empty())
        return (431); // no nickname given error
    nick = params[0];
    if (!is_valid_nick(nick))
    {
        this->send_error(client, 432, nick); // invalid nickname error
        return (0);
    }
    if (same_nick(client.get_nick(), nick))
    {
        this->send_msg(client, ":" + this->_name + " NOTICE " + client.get_nick() + " :You are already using this nickname\r\n", EPOLLIN | EPOLLOUT);
        return (0);
    }
    for (it = this->_clients.begin(); it != this->_clients.end(); ++it)
    {
        if (it->first != client_fd && same_nick(it->second.get_nick(), nick))
        {
            this->send_error(client, 433, nick); // nickname already in use error
            return (0);
        }
    }
    client.set_nick(nick);
    this->send_msg(client, ":" + this->_name + " NOTICE * :Set nickname to " + nick + "\r\n" + "\r\n", EPOLLIN | EPOLLOUT);
    return (0);
}

int Server::handle_user(Client &client, const std::vector<std::string> &params)
{
    std::string username;
    std::string mode;

    if (client.get_registered())
        return (462); // error if client is already registered
    if (client.get_nick().empty())
    {
        this->send_msg(client, ":" + this->_name + " NOTICE * :NICK must be set before USER\r\n", EPOLLIN | EPOLLOUT);
        return (0);
    }
    if (params.size() < 4)
        return (461); // not enough parameters error
    username = params[0];
    mode = params[1];
    if (!is_valid_user_mode(mode))
    {
        this->send_msg(client, ":" + this->_name + " NOTICE " + client.get_nick() + " :Invalid USER mode: only numbers are allowed\r\n", EPOLLIN | EPOLLOUT);
        return (0);
    }
    client.set_user(username);
    this->send_msg(client, ":" + this->_name + " NOTICE * :Set username to " + username + "\r\n" + "\r\n", EPOLLIN | EPOLLOUT);
    return (0);
}


int Server::handle_privmsg(Client &client, const std::vector<std::string> &params)
{
    std::map<int, Client>::iterator it;
    std::string target;
    std::string msg;
    std::string prefix;

    if (params.empty())
        return (411); // no recipient given error
    if (params.size() < 2 || params[1].empty()) // check if msg text is empty
        return (412);
    target = params[0];
    msg = params[1];
    if (same_nick(target, "Miku")) // send msgs addressed to miku directly to ollama
    {
        if (!this->_bot.start_request(client.get_client_fd(), msg)) // if bot is unavailable -> send error to client
            this->send_msg(client, ":Miku!bot@ircserv PRIVMSG " + client.get_nick() + " :Miku is unavailable.\r\n", EPOLLIN | EPOLLOUT);
        return (0);
    }
    for (it = this->_clients.begin(); it != this->_clients.end(); ++it)
    {
        if (same_nick(it->second.get_nick(), target)) // if target matches a client's nickname
        {
            prefix = ":" + client.get_nick() + "!" + client.get_user() + "@" + client.get_host_ip();
            this->send_msg(it->second, prefix + " PRIVMSG " + it->second.get_nick() + " :" + msg + "\r\n", EPOLLIN | EPOLLOUT);
            return (0);
        }
    }

    this->send_error(client, 401, target); // no such nick/channel error
    return (0);
}


// ping is used to check if connection between client and server is still active.
int Server::handle_ping(Client &client, const std::vector<std::string> &params)
{
    std::string token;

    if (params.empty() || params[0].empty())
        return (409); // no origin specified error
    token = params[0];
    this->send_msg(client, ":" + this->_name + " PONG " + this->_name + " :" + token + "\r\n" + "\r\n", EPOLLIN | EPOLLOUT);
    return (0);
}

int Server::handle_quit(Client &client, const std::vector<std::string> &params)
{
    std::map<int, Client>::iterator it;
    std::string reason;
    std::string msg;
    std::string client_name;
    int client_fd;

    client_fd = client.get_client_fd();
    if (params.empty() || params[0].empty())
        reason = "Client quit";
    else
        reason = params[0];
    if (client.get_nick().empty())
        client_name = "unregistered client";
    else
        client_name = client.get_nick();
    msg = ":" + client.get_nick() + "!" + client.get_user() + "@" + client.get_host_ip() + " QUIT :" + reason + "\r\n";
    for (it = this->_clients.begin(); it != this->_clients.end(); ++it)
    {
        if (it->first == client_fd) // skip sending QUIT msg to client that sent it
            continue;
        this->send_msg(it->second, msg, EPOLLIN | EPOLLOUT); // send QUIT msg to all other clients
    }
    this->send_msg(client, "Closing connection for " + client_name + ": " + reason + "\r\n", EPOLLIN | EPOLLOUT);
    client.set_quit(true);
    return (0);
}

int Server::handle_join(Client &client, const std::vector<std::string> &params)
{
    std::map<std::string, Channel>::iterator it;
    std::vector<std::string> c_names;
    std::vector<std::string> keys;
    std::string chan_name;
    std::string key;
    size_t i;
    int ret;

    if (params.empty() || params.size() > 2)
        return (461);
    c_names = ft_split(params[0], ',');
    if (params.size() > 1)
        keys = ft_split(params[1], ',');
    i = 0;
    while (i < c_names.size())
    {
        chan_name = c_names[i];
        if (chan_name.empty() || (chan_name[0] != '#' && chan_name[0] != '&'))
        {
            this->send_msg(client, "Invalid channel name: must start with # or &\r\n\r\n", EPOLLIN | EPOLLOUT);
            i++;
            continue;
        }
        if (i < keys.size())
            key = keys[i];
        else
            key = "";
        it = this->_channels.find(chan_name);
        if (it == this->_channels.end())
        {
            Channel ch(chan_name, default_mode(), client);
            this->_channels.insert(std::pair<std::string, Channel>(chan_name, ch));
            this->send_msg(client, "You created Channel " + chan_name + "\r\n" + "\r\n", EPOLLIN | EPOLLOUT);
        }
        else
        {
            ret = it->second.joinChannel(client, key);
            if (ret == 1)
                this->send_msg(client, "NOTICE: User is already in Channel\r\n", EPOLLIN | EPOLLOUT);
            else if (ret == 2)
                this->send_msg(client, "Channel is full: " + chan_name + " ! Get yo ass outta here\r\n" + "\r\n", EPOLLIN | EPOLLOUT);
            else if (ret == 3)
                this->send_msg(client, ":" + this->_name + " 473 " + client.get_nick() + " " + chan_name + " :You weren't invited to the boys club. Access denied\r\n", EPOLLIN | EPOLLOUT);
            else if (ret == 4)
                this->send_msg(client, "Channel requires a key: " + chan_name + " ! What's the pass? This is boys only club" + "\r\n", EPOLLIN | EPOLLOUT);
            else
                this->send_msg(client, "You joined " + chan_name + "\r\n" + "\r\n", EPOLLIN | EPOLLOUT);
        }
        i++;
    }
    return (0);
}

int Server::handle_fast(Client &client, const std::vector<std::string> &params)
{
    (void)params;

    if (!client.get_auth())
        return (451);
    client.set_nick("Nick");
    client.set_user("User");
    client.set_host_ip("localhost");
    return (0);
}

// void Server::handle_fast(Client &client)
// {
// 	std::string nick;
// 	std::string user;
// 	std::string host;

// 	nick = generateRandomString(9);
// 	user = generateRandomString(9);
// 	host = generateRandomString(9);
// 	client.set_auth(true);
// 	client.set_nick(nick);
// 	client.set_user(user);
// 	client.set_host_ip(host);
// 	if (client.can_register())
// 		client.set_registered(true);
// 	this->send_msg(client, "You shall be named: " + nick + "\n", EPOLLIN | EPOLLOUT);
// 	this->send_msg(client, "Your home kingdom shall be: " + host + "\n", EPOLLIN | EPOLLOUT);
// 	this->send_msg(client, "Thy user shall be sung as: " + user +  "\n", EPOLLIN | EPOLLOUT);
// 	this->send_msg(client, "Go forth, make haste, child of Mercury\r\n", EPOLLIN | EPOLLOUT);
// 	this->send_msg(client, registration_reply(client, this->_name, this->_date) + "\n", EPOLLIN | EPOLLOUT);
// }


// void Server::handle_join(Client &client, const std::vector<std::string> &params)
// {
// 	std::map<std::string, Channel>::iterator it;
// 	std::vector<std::string> c_names;
// 	std::vector<std::string> keys;
// 	std::string chan_name;
// 	std::string key;
// 	size_t i;
// 	int ret;

// 	if (params.empty())
// 	{
// 		this->send_msg(client, ":" + this->_name + " XXX * :Need more params\r\n", EPOLLIN | EPOLLOUT);
// 		return;
// 	}
// 	if (params.size() > 2)
// 	{
// 		this->send_msg(client, ":" + this->_name + " XXX * :Too many params\r\n", EPOLLIN | EPOLLOUT);
// 		return;
// 	}
// 	c_names = ft_split(params[0], ',');
// 	if (params.size() > 1)
// 		keys = ft_split(params[1], ',');
// 	i = 0;
// 	while (i < c_names.size())
// 	{
// 		chan_name = c_names[i];
// 		if (chan_name.empty() || (chan_name[0] != '#' && chan_name[0] != '&'))
// 		{
// 			this->send_msg(client, "You fucked up homie, channel can only start with # or &\r\n", EPOLLIN | EPOLLOUT);
// 			i++;
// 			continue;
// 		}
// 		if (i < keys.size())
// 			key = keys[i];
// 		else
// 			key = "";
// 		it = this->_channels.find(chan_name);
// 		if (it == this->_channels.end())
// 		{
// 			Channel ch(chan_name, default_mode(), client);
// 			this->_channels.insert(std::pair<std::string, Channel>(chan_name, ch));
// 			this->send_msg(client, "You created Channel " + chan_name + "\r\n" + "\n", EPOLLIN | EPOLLOUT);
// 		}
// 		else
// 		{
// 			ret = it->second.joinChannel(client, key);
// 			if (ret)
// 			{
// 				switch (ret)
// 				{
// 					case 1:
// 						this->send_msg(client, "NOTICE: User is already in Channel\r\n", EPOLLIN | EPOLLOUT);
// 						break;
// 					case 2:
// 						this->send_msg(client, ":" + this->getName() + " XXX * :" + chan_name + " This place is full, get yo ass outta here\r\n", EPOLLIN | EPOLLOUT);
// 						break;
// 					case 3:
// 						this->send_msg(client, ":" + this->getName() + " 473 * :" + chan_name + " You weren't invited to the boys club. Access denied\r\n", EPOLLIN | EPOLLOUT);
// 						break;
// 					case 4:
// 						this->send_msg(client, ":" + this->getName() + " XXX * :" + chan_name + " What's the pass? This is boys only club\r\n", EPOLLIN | EPOLLOUT);
// 						break;
// 				}
// 			}
// 			else
// 			{
// 				this->send_msg(client, "You joined " + chan_name + "\r\n" + "\n", EPOLLIN | EPOLLOUT);
// 			}
// 		}
// 		i++;
// 	}
// }
