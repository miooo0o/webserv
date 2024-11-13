/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: minakim <minakim@student.42berlin.de>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/06/30 16:23:46 by sanghupa          #+#    #+#             */
/*   Updated: 2024/11/13 15:24:21 by minakim          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
# define SERVER_HPP

// # include "webserv.hpp"
# include <vector>
# include "Config.hpp"
# include "Location.hpp"
# include "RequestHandler.hpp"
# include "HttpRequest.hpp"
# include "HttpResponse.hpp"

class	Config;
class	Location;
// class	RequestHandler;

struct	ServerConfig;

struct ListenInfo
{
	std::string listen;
	std::string	host;
	int			port;
	int			fd;
};

class	Server
{
	public:
		Server(Config& config);
		~Server();

		void						start();
		void						stop();

		bool						isRunning() const;

	private:
		bool						_running;
		std::vector<ListenInfo>		_listenInfos;
		std::map<int, ListenInfo> 	_clients;
		std::vector<pollfd>			_pollfds;
		RequestHandler				_requestHandler;

		int							_setupListeningSocket(const std::string host, int port);

		ServerConfig&				_fetchConfig(int target);
		int							_setupListenInfos();
		int							_setupListenSockets();
		int							_acceptNewConnection(int target);
		int							_handleClientData(int target, size_t i);

		Config&						_config;
		std::vector<ServerConfig*>	_serverConfigs;

	private: // TODO: update / delete 
	ssize_t	_receiveData(int target,std::string& str);

};

#endif
