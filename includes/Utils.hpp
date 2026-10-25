/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Utils.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/08 16:14:29 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/25 17:59:15 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef UTILS_HPP
# define UTILS_HPP

# include "Channels.hpp"
# include <cstddef>
# include <cstdlib>
# include <ctime>
# include <iostream>
# include <sstream>
# include <string>
# include <sys/types.h>
# include <vector>

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

std::string registration_reply(const Client &client, const std::string &server_name, const std::string &date);
std::string generateRandomString(int length);
std::vector<std::string> ft_split(const std::string &s, char delim);

t_cmode	default_mode(void);

#endif
