/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   BotUtils.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/24 22:40:12 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/24 22:59:35 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/BotUtils.hpp"
#include "../includes/Bot.hpp"
#include "../../includes/Utils.hpp"

std::string nb_to_str(size_t nb)
{
	std::string res;
	
	if (nb == 0)
		return ("0");
	while (nb > 0)
	{
		res.insert(res.begin(), '0' + (nb % 10));
		nb /= 10;
	}
	return (res);
}

void Bot::log_debug(const std::string &name, const std::string &text)
{
	std::string log_path = ".logs/" + str_lower(this->_name) + ".log";
	std::ofstream debug_file;
	if(name.empty() && text.empty())
	{
		debug_file.open(log_path.c_str(), std::ios::out | std::ios::trunc);
		return;
	}
	else
		debug_file.open(log_path.c_str(), std::ios::app);
	if (!debug_file.is_open())
		return ;
	debug_file << "======== " << name << " ========\n";
	debug_file << text << "\n\n";
	debug_file.close();
}

std::string convert_json(const std::string &text, e_json type)
{
	std::string res;
	
	for (size_t i = 0; i < text.size(); i++)
	{
		if (type == JSON_ESCAPE) // convert normal text to json
		{
			if (text[i] == '\\')
				res += "\\\\"; // escape backslash
			else if (text[i] == '"')
				res += "\\\""; // quotes
			else if (text[i] == '\n')
				res += "\\n"; // nl
			else if (text[i] == '\r')
				res += "\\r"; // carriage
			else if (text[i] == '\t')
				res += "\\t"; // tab
			else
				res += text[i]; // if normal char ->add it
		}
		else if (type == JSON_UNESCAPE) // convert json to normal text
		{
			if (text[i] == '\\' && i + 1 < text.size()) // if is an escape char and not last
			{
				++i; // skip backslash
				if (text[i] == 'n')
					res += '\n'; // nl
				else if (text[i] == 'r')
					res += '\r'; // carriage
				else if (text[i] == 't')
					res += '\t'; // tab
				else
					res += text[i]; // escaped quote or backslash
			}
			else
				res += text[i]; // if normal char ->add it
		}
	}
	return (res);
}
