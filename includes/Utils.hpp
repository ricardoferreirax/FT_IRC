/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Utils.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/08 16:14:29 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/15 10:49:47 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef UTILS_HPP
# define UTILS_HPP

#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include "Channels.hpp"

bool validate_args(char **av, int &port);
bool is_valid_nick(const std::string &nick);
bool same_nick(const std::string &nick1, const std::string &nick2);
void parse_params(std::string params, std::vector<std::string> &parsed);
bool is_valid_user_mode(const std::string &mode);

e_cmd parse_message(const std::string &msg, std::vector<std::string> &params);
std::vector<std::string> ft_split(const std::string &s, char delim);
std::string generateRandomString(int length);

t_cmode	default_mode(void);

#endif
