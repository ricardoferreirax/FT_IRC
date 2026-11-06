/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Utils.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/08 16:14:29 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/11/06 13:26:34 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef UTILS_HPP
#define UTILS_HPP

#include "Client.hpp"
#include <cstddef>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <vector>

typedef struct	s_ulimit
{
	size_t	u_limit;
	size_t	u_count;
}	t_ulimit;

typedef struct	s_mode
{
	std::string	topic;
	std::string	key;
	t_ulimit	users;
	bool		invite_only;
	bool		topic_restricted;
}	t_cmode;

typedef struct	s_modecmd
{
	std::string param;
	bool		needs_param;
	bool		is_add;
	char		flag;
}	t_modecmd;

typedef struct	s_banmask
{
	std::string nick;
	std::string user;
	std::string host;
	bool 		valid;
	s_banmask(): nick("*"), user("*"), host("*"), valid(true) {};
}	t_banmask;

enum	log_type
{
	TCP_RECEIVE,
	TCP_SEND,
	IRC_CMD
};

int		validate_port(const std::string &port);
bool	is_valid_nick(const std::string &nick);
bool	same_nick(const std::string &nick1, const std::string &nick2);
void	parse_params(std::string params, std::vector<std::string> &parsed);
bool	is_valid_user_mode(const std::string &mode);
void	parse_msg(const std::string &msg, std::string &cmd, std::vector<std::string> &params);
void	log_tcp(log_type type, int fd, ssize_t bytes, const std::string &data);
bool 	match_wildcard(std::string pattern, std::string match, size_t pi = 0, size_t mi = 0);
bool 	match_ban(Client &client, t_banmask ban);

std::vector<std::string> ft_split(const std::string &s, char delim);

std::string registration_reply(const Client &client, const std::string &server_name, const std::string &date);
std::string get_error_param(const std::string &cmd, int error, const std::vector<std::string> &params);
std::string generateRandomString(int length);

t_banmask	parseBanMask(std::string banmask);
t_cmode		default_mode(void);

#endif
