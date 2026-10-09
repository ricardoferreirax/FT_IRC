/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/07 12:28:03 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/10/07 13:47:28 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLIENT_HPP
# define CLIENT_HPP

# include <string>

class Client
{
  private:
	std::string _recv_buffer;
	std::string _send_buffer;
	std::string _nick;
	std::string _user;
	int _fd;
	bool _auth;
	bool _registered;

  public:
	Client(int fd);
	~Client();

	std::string &get_recv_buffer();
		// returns reference to client's receive buffer so server can append new data to it
	std::string &get_send_buffer();
		// returns reference to client's send buffer so server can append new data to it
};

#endif
