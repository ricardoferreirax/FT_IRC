/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Utils.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/08 16:14:29 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/21 18:00:04 by rmedeiro         ###   ########.fr       */
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

int		validate_port(const std::string &port);
bool	is_valid_nick(const std::string &nick);
bool	same_nick(const std::string &nick1, const std::string &nick2);
void	parse_params(std::string params, std::vector<std::string> &parsed);
bool	is_valid_user_mode(const std::string &mode);
void	parse_msg(const std::string &msg, std::string &cmd, std::vector<std::string> &params);

std::vector<std::string> ft_split(const std::string &s, char delim);
std::string	registration_reply(const Client &client, const std::string &server_name, const std::string &date);
std::string	generateRandomString(int length);
std::string nb_to_str(size_t nb);
std::string extract_response(const std::string &json);
std::string escape_json(const std::string &text);

t_cmode	default_mode(void);

#endif
