/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rmedeiro <rmedeiro@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/27 23:58:32 by rmedeiro          #+#    #+#             */
/*   Updated: 2026/09/28 06:35:23 by rmedeiro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
# define SERVER_HPP

# include <cstdlib>
# include <iostream>
# include <netinet/in.h>
# include <stdexcept>
# include <string>
# include <sys/socket.h>
# include <unistd.h>

class Server
{
	private:
		std::string _pass;
		int _port;
		int _listenFd;

	public:
		Server(int port, const std::string &pass);
		~Server();

	void initListener();
};

#endif
