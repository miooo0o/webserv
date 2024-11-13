/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: minakim <minakim@student.42berlin.de>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/06/30 16:23:46 by sanghupa          #+#    #+#             */
/*   Updated: 2024/11/14 00:49:53 by minakim          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "webserv.hpp"
#include "Server.hpp"
#include "Context.hpp"

Server::Server(Config& config)
	: _config(config)
{
	_running = false;
	_serverConfigs = config.getServers();
}

Server::~Server()
{
	stop();
}

void	Server::start()
{
	if (_setupListenInfos() != 1)
	{
		stop();
		return ;
	}

	_running = true;

	if (_setupListenSockets() != 1)
	{
		stop();
		return ;
	}

	try
	{
		while (_running) 
		{
			if (g_sigint == true)
				break;
			int pollcount = poll(_pollfds.data(), _pollfds.size(), -1);
			if (pollcount <= 0) 
			{
				// std::cerr << "Error: poll failed" << std::endl;
				// Logger::Error("Server error: poll failed");
				// break;
				continue ;
			}
			for (size_t i = 0; i < _pollfds.size(); i++)
			{
				int target = _pollfds[i].fd;
				if (_pollfds[i].revents & POLLIN)
				{
					if ((_clients.find(target) != _clients.end()) && (_clients[target].fd == target))
					{
						if (_acceptNewConnection(target) != 1)
							break ;
					}
					else
					{
						if (_handleClientData(target, i) != 1)
							continue ;
					}
				}
			}
		}
		stop();
	} 
	catch (const std::exception& e)
	{
		// Print the error message
		std::cerr << "Error: " << e.what() << std::endl;
		// Logger::Error("Server error: %s", e.what());
		stop();
	}
}

void	Server::stop()
{
	if (_running)
	{
		_running = false;
		_pollfds.clear();
		// Logger::info("Server stopped");
		std::cout << "\rServer stopped" << std::endl;
	}
}

bool	Server::isRunning() const
{
	return (_running);
}

ServerConfig& Server::_fetchConfig(int target)
{
	ServerConfig& serverConfig = *_config.getServerByListen(_clients[target].listen);	
	return (serverConfig);
}

int Server::_setupListenInfos()
{
	// Hosts and ports to manage
	for (size_t i = 0; i < _serverConfigs.size(); i++)
	{
		ListenInfo info = {
			_serverConfigs[i]->listen,
			_serverConfigs[i]->host,
			_serverConfigs[i]->port,
			-1
		};
		_listenInfos.push_back(info);
	}
	return (1);
}

int Server::_setupListenSockets()
{
	try
	{
		// Setup listen sockets
		for (size_t i = 0; i < _listenInfos.size(); i++)
		{
			int listen_socket = _setupListeningSocket(_listenInfos[i].host, _listenInfos[i].port);
			_listenInfos[i].fd = listen_socket;
			_pollfds.push_back((struct pollfd){listen_socket, POLLIN, 0});

			// Track listening sockets
			_clients[listen_socket] = _listenInfos[i];
			std::cout << "Listening on " << _listenInfos[i].host << ":" << _listenInfos[i].port << std::endl;
		}
		return (1);
	}
	catch (const std::exception& e)
	{
		// Print the error message
		std::cerr << "Error: " << e.what() << std::endl;
		// Logger::Error("Server error: %s", e.what());
		stop();
		return (0);
	}
}

int	Server::_acceptNewConnection(int target)
{
	// New connection on a listening socket
	sockaddr_in client_addr;
	socklen_t client_addr_len = sizeof(client_addr);
	int client_socket = accept(target, (sockaddr*)&client_addr, &client_addr_len);
	if (client_socket < 0)
	{
		std::cerr << "Error: accept failed" << std::endl;
		// Logger::Error("Server error: accept failed");
		// break ;
		// continue;
		return (0);
	}

	int flags = fcntl(client_socket, F_GETFL, 0);
	if (flags == -1)
	{
		throw std::runtime_error("Failed to get file descriptor flags");
	}
	flags |= O_NONBLOCK;
	if (fcntl(client_socket, F_SETFL, flags) < 0)
	{
		throw std::runtime_error("Failed to set file descriptor flags to non-blocking");
	}

	// Track new client connection
	_pollfds.push_back((struct pollfd){client_socket, POLLIN | POLLOUT, 0});
	_clients[client_socket] = _clients[target];
	std::cout << "Client connected from " << _clients[client_socket].host << ":" << _clients[client_socket].port << std::endl;
	// Logger::info("Client connected from %s:%d", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
	return (1);
}

int	Server::_handleClientData(int target, size_t i)
{
	ServerConfig&	serverConfig = _fetchConfig(target);

	char buffer[serverConfig.max_body_size];
	ssize_t			count = read(target, buffer, sizeof(buffer));

	if (count <= 0)
	{
		close(target);
		_clients.erase(target);
		_pollfds.erase(_pollfds.begin() + i);
		return (0);
	}
	
	std::string		readed(buffer, count);
	std::cout << YELLOW << "TEST | requestData (server.cpp, buffer to std::string)" << std::endl;
	std::cout << "buffer: " << count << ", string size: "<< readed.size() << std::endl;
	std::cout << RESET << "\nrequestData\n\n" << YELLOW << readed << "\n\n" << RESET << std::endl;


	HttpRequest		request(readed);
	Context			contextFromTarget(serverConfig, request);
	HttpResponse	response = _requestHandler.handleRequest(contextFromTarget);
	std::string		responseData = response.generateResponseToString();
	// std::string	responseData = handle_request(requestData);
	////////////////////////////////////////////////////////////////////////////////////////
	
	// Send the response
	write(target, responseData.c_str(), responseData.size());
	close(target);
	_clients.erase(target);
	_pollfds.erase(_pollfds.begin() + i); 
	// --i;
	return (1);
}
