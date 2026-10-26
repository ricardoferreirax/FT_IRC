/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   BotUtils.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/24 22:41:21 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/24 22:59:45 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef BOTUTILS_HPP
# define BOTUTILS_HPP

# include <string>

enum	e_json
{
    JSON_ESCAPE,
    JSON_UNESCAPE
};

std::string nb_to_str(size_t nb);
std::string convert_json(const std::string &text, e_json type);

#endif
