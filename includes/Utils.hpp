/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Utils.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/08 16:14:29 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/10 00:07:41 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef UTILS_HPP
# define UTILS_HPP

# include <string>

bool is_valid_nick(const std::string &nick);
bool same_nick(const std::string &nick1, const std::string &nick2);
bool get_param(std::string &remaining, std::string &param);

#endif
