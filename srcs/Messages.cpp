/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Messages.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/27 21:53:54 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/27 22:22:46 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Server.hpp"

void	Server::send_msg(Client &client, const std::string &msg)
{
	epoll_event	event;
	int			client_fd;

	client_fd = client.get_client_fd();
	client.get_send_buffer() += msg; // append msg to send buffer
	if (msg.size() < 2 || msg.substr(msg.size() - 2) != "\r\n") // if msg does not end with \r\n
		client.get_send_buffer() += "\r\n"; // end msg with \r\n
	client.get_send_buffer() += "\r\n"; 
	event.events = EPOLLIN | EPOLLOUT; // monitor socket for reading and writing
	event.data.fd = client_fd;
	if (epoll_ctl(this->_epoll_fd, EPOLL_CTL_MOD, client_fd, &event) < 0)
		throw std::runtime_error("IRC: epoll_ctl() failed.");
}

void	Server::send_reply(Client &client, int reply, const std::string &param, const std::string &text)
{
    std::map<int, std::string>::iterator it;
    std::map<int, std::string> 			 replies;
    std::string 						 nick;

    nick = client.get_nick();
    if (nick.empty())
        nick = "*";
    replies[RPL_WELCOME] = "001 " + nick + " :" + text;
    replies[RPL_YOURHOST] = "002 " + nick + " :" + text;
    replies[RPL_CREATED] = "003 " + nick + " :" + text;
    replies[RPL_MYINFO] = "004 " + nick + " " + param;
    replies[RPL_ISUPPORT] = "005 " + nick + " " + param + " :are supported by this server";
    replies[RPL_CHANNELMODEIS] = "324 " + nick + " " + param;
    replies[RPL_NOTOPIC] = "331 " + nick + " " + param + " :No topic is set";
    replies[RPL_TOPIC] = "332 " + nick + " " + param + " :" + text;
    replies[RPL_INVITING] = "341 " + nick + " " + param;
    replies[RPL_NAMREPLY] = "353 " + nick + " " + param + " :" + text;
    replies[RPL_ENDOFNAMES] = "366 " + nick + " " + param + " :End of /NAMES list";
    replies[RPL_HELPSTART] = "704 " + nick + " " + param + " :" + text;
    replies[RPL_HELPTXT] = "705 " + nick + " " + param + " :" + text;
    replies[RPL_ENDOFHELP] = "706 " + nick + " " + param + " :" + text;
    it = replies.find(reply);
    if (it == replies.end())
        return ;
    this->send_msg(client, ":" + this->_name + " " + it->second);
}

void Server::send_error(Client &client, int error, const std::string &param)
{
	std::map<int, std::string> errors; // each error code has a corresponding error msg
	std::string				   nick;

	nick = client.get_nick();
	if (nick.empty())
		nick = "*";
	errors[ERR_NOSUCHNICK] = "401 " + nick + " " + param + " :No such nick/channel";
	errors[ERR_NOSUCHCHANNEL] = "403 " + nick + " " + param + " :No such channel";
	errors[ERR_CANNOTSENDTOCHAN] = "404 " + nick + " " + param + " :Cannot send to channel";
	errors[ERR_NOORIGIN] = "409 " + nick + " :No origin specified";
	errors[ERR_NORECIPIENT] = "411 " + nick + " :No recipient given (" + param + ")";
	errors[ERR_NOTEXTTOSEND] = "412 " + nick + " :No text to send";
	errors[ERR_UNKNOWNCOMMAND] = "421 " + nick + " " + param + " :Unknown command";
	errors[ERR_NONICKNAMEGIVEN] = "431 " + nick + " :No nickname given";
	errors[ERR_ERRONEOUSNICKNAME] = "432 " + nick + " " + param + " :Erroneous nickname";
	errors[ERR_NICKNAMEINUSE] = "433 " + nick + " " + param + " :Nickname is already in use";
	errors[ERR_NOTONCHANNEL] = "442 " + nick + " " + param + " :You're not on that channel";
	errors[ERR_USERONCHANNEL] = "443 " + nick + " " + param + " :is already on channel";
	errors[ERR_NOTREGISTERED] = "451 " + nick + " :You have not registered";
	errors[ERR_NEEDMOREPARAMS] = "461 " + nick + " " + param + " :Not enough parameters";
	errors[ERR_ALREADYREGISTRED] = "462 " + nick + " :You may not reregister";
	errors[ERR_PASSWDMISMATCH] = "464 " + nick + " :Password incorrect";
    errors[ERR_CHANNELISFULL] = "471 " + nick + " " + param + " :Cannot join channel (+l)";
	errors[ERR_UNKNOWNMODE] = "472 " + nick + " " + param + " :is unknown mode char to me";
    errors[ERR_INVITEONLYCHAN] = "473 " + nick + " " + param + " :Cannot join channel (+i)";
    errors[ERR_BADCHANNELKEY] = "475 " + nick + " " + param + " :Cannot join channel (+k)";
	errors[ERR_BADCHANMASK] = "476 " + nick + " " + param + " :Bad Channel Mask";
	errors[ERR_CHANOPRIVSNEEDED] = "482 " + nick + " " + param + " :You're not channel operator";
	errors[ERR_HELPNOTFOUND] = "524 " + nick + " " + param + " :Help topic not found";
	errors[ERR_PASSREQUIRED] = "601 " + nick + " :Password must be accepted before nickname";
	errors[ERR_INVALIDUSERMODE] = "604 " + nick + " :Invalid USER mode: only numbers are allowed";
	if (errors.find(error) == errors.end())
		return ; // error code is not in map -> ignore it
	this->send_msg(client, ":" + this->_name + " " + errors[error]);
}

void	Server::send_notice(Client &client, int notice, const std::string &param)
{
	std::map<int, std::string>::iterator it;
	std::map<int, std::string> 			 notices;
	std::string							 nick;

	nick = client.get_nick();
	if (nick.empty())
		nick = "*";
	notices[PASSACCEPTED] = "PASS accepted";
	notices[NICKSET] = "Set nickname to " + param;
	notices[CHANNELCREATED] = "You created Channel " + param;
	notices[CHANNELJOINED] = "You joined " + param;
	notices[MIKUUNAVAILABLE] = "Miku is unavailable :/ Try again later.";
	it = notices.find(notice);
	if (it == notices.end())
		return ;
	this->send_msg(client, ":" + this->_name + " NOTICE " + nick + " :" + it->second);
}
