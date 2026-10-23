/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Utils.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/08 16:14:29 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/23 18:29:06 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef UTILS_HPP
# define UTILS_HPP

# include "Channels.hpp"
# include <cstdlib>
# include <ctime>
# include <iostream>
# include <sstream>
# include <string>
# include <vector>

enum	e_json
{
    JSON_ESCAPE,
    JSON_UNESCAPE
};

int		validate_port(const std::string &port);
bool	is_valid_nick(const std::string &nick);
bool	same_nick(const std::string &nick1, const std::string &nick2);
void	parse_params(std::string params, std::vector<std::string> &parsed);
bool	is_valid_user_mode(const std::string &mode);
void	parse_msg(const std::string &msg, std::string &cmd, std::vector<std::string> &params);

std::string	registration_reply(const Client &client, const std::string &server_name, const std::string &date);
std::string	generateRandomString(int length);
std::string nb_to_str(size_t nb);
std::string convert_json(const std::string &text, e_json type);

std::vector<std::string> ft_split(const std::string &s, char delim);
t_cmode	default_mode(void);

#endif
