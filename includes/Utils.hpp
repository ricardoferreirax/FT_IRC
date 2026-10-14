/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Utils.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/08 16:14:29 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/12 16:43:27 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef UTILS_HPP
# define UTILS_HPP

# include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include "Channels.hpp"

bool validate_args(char **av, int &port);
bool is_valid_nick(const std::string &nick);
bool same_nick(const std::string &nick1, const std::string &nick2);
bool get_param(std::string &remaining, std::string &param);
bool is_valid_user_mode(const std::string &mode);
std::vector<std::string> ft_split(const std::string &s, char delim);
std::string generateRandomString(int length);

t_cmode	default_mode(void);

#endif
