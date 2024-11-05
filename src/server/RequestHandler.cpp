/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestHandler.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: minakim <minakim@student.42berlin.de>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/06/30 16:23:00 by sanghupa          #+#    #+#             */
/*   Updated: 2024/11/05 11:50:19 by minakim          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "RequestHandler.hpp"
#include "HttpResponse.hpp"
#include "HttpRequest.hpp"
#include "Location.hpp"
#include "Config.hpp"
#include "Context.hpp"

RequestHandler:: RequestHandler()
{}

RequestHandler::~RequestHandler()
{}

////////////////////////////////////////////////////////////////////////////////
/// Public Methods
////////////////////////////////////////////////////////////////////////////////

/// @brief Processes the incoming HTTP request and returns an appropriate response.
///
/// This function handles an incoming HTTP request by first logging the request details. It then
/// locates the corresponding `Location` object based on the request URI. If no matching location is
/// found, it returns a `404 Not Found` response. If the HTTP method is not allowed for the located
/// `Location`, it returns a `405 Method Not Allowed` response. Otherwise, it delegates the request
/// processing to the appropriate handler based on the method (`GET`, `POST`, `DELETE`, etc.).
///
/// @param request The `HttpRequest` object representing the incoming request.
/// @return An `HttpResponse` object containing the result of processing the request. 
/// 		1. Returns `404 Not Found` if no location is found.
///			2. `405 Method Not Allowed` if the method is not allowed.
///			3. or the result of `_processRequest` if the method is valid and the location is found.
HttpResponse	RequestHandler::handleRequest(const Context& context)
{

	// std::cout << "\r" << request.getMethod() << " | " << request.getUri() << " | " <<
	// 	request.getVersion() << std::endl;

	if (_isCGIReqeust(context))
		return (_handleCGIRequest(context));
	else if (_isAllowedMethod(context))
		return (_processStandardMethods(context));
	return (HttpResponse::methodNotAllowed_405(context));
}

////////////////////////////////////////////////////////////////////////////////
/// Private Methods
////////////////////////////////////////////////////////////////////////////////

/// @brief Checks if the request is a CGI request at the current location.
/// @param context 
/// @return bool
bool	RequestHandler::_isCGIReqeust(const Context& context) const
{


	std::map<std::string, std::string> cgiMap = context.getLocation().getCgi();
	
	std::cout << YELLOW << "TEST | location.getCgi(): " << RESET << std::flush;
	if (context.getLocation().getCgi().empty())
	{
		std::cout << YELLOW << "No CGI configurations found." << RESET << std::endl;
		return (false);
	}
	std::cout << YELLOW << "CGI configurations found." << RESET << std::endl;


	for (std::map<std::string, std::string>::const_iterator it = cgiMap.begin(); it != cgiMap.end(); ++it)
	{
    	std::cout << YELLOW << " CGI       key: " << it->first << " -> value: " << it->second << RESET << std::endl;
	}
	return (true);
}

/// @brief Not implemented.
/// @param context 
/// @return 
HttpResponse	RequestHandler::_handleCGIRequest(const Context& context)
{
	// env, fork...
	return (HttpResponse::notImplemented_501(context));
}

////////////////////////////////////////////////////////////////////////////////
/// @brief This function checks if the request method is in the list of allowed methods.
/// When the Location object is created, it is initialized with a list of allowed methods.
/// if user did not specify the allowed methods at `.conf file`, it is initialized with {"GET", "POST", "DELETE"}.
bool	RequestHandler::_isAllowedMethod(const Context& context) const
{
	const std::string &requestMethod = context.getRequest().getMethod();
	const std::vector<std::string> &allowedMethods =  context.getLocation().getAllowedMethods();
	
	if (!allowedMethods.empty() 
		&& std::find(allowedMethods.begin(), allowedMethods.end(), requestMethod) != allowedMethods.end())
			return (true);
	return (false);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief Processes the HTTP request based on its method.
///
/// This function handles an HTTP request by dispatching it to the appropriate method handler
/// (`GET`, `POST`, or `DELETE`). If the method is not supported, it returns a `501 Not Implemented`
/// response.
///
/// @param request The `HttpRequest` object representing the incoming HTTP request.
/// @param location The `Location` object associated with the request's URI.
/// @return An `HttpResponse` object containing the result of processing the request based on the method.
///         If the method is not supported, returns a `501 Not Implemented` response.
HttpResponse	RequestHandler::_processStandardMethods(const Context& context)
{
	const HttpRequest& request = context.getRequest();

	if (request.getMethod() == "GET")
		return (_handleGet(context));
	else if (request.getMethod() == "POST")
		return (_handlePost(context));
	else if (request.getMethod() == "DELETE")
		return (_handleDelete(context));
	else
		return (HttpResponse::notImplemented_501(context));
}

HttpResponse RequestHandler::_handleGet(const Context& context)
{
	return (_staticFileHandler.handleget(context));
}

HttpResponse RequestHandler::_handlePost(const Context& context)
{
	return (_staticFileHandler.handlepost(context));
}

HttpResponse RequestHandler::_handleDelete(const Context& context)
{
	return (HttpResponse::notImplemented_501(context));
}


////////////////////////////////////////////////////////////////////////////////

// TODO: Redirections
// Redirections	https://developer.mozilla.org/en-US/docs/Web/HTTP/Redirections
// request | get -> response -> 301 -> request | get -> response 
/*
<head>
  <meta http-equiv="Refresh" content="0; URL=https://example.com/" />
</head>
*/
