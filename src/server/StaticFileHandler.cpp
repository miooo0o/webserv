/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   StaticFileHandler.cpp                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: minakim <minakim@student.42berlin.de>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/06/30 16:23:00 by sanghupa          #+#    #+#             */
/*   Updated: 2024/11/05 11:27:17 by minakim          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "StaticFileHandler.hpp"
#include "HttpResponse.hpp"
#include "HttpRequest.hpp"
#include "Location.hpp"
#include "Context.hpp"

////////////////////////////////////////////////////////////////////////////////

StaticFileHandler::StaticFileHandler()
	: _handledPath("")
{
}

StaticFileHandler::StaticFileHandler(const StaticFileHandler& other)
{
	_handledPath = other._handledPath;
}

StaticFileHandler& StaticFileHandler::operator=(const StaticFileHandler& other)
{
	if (this != &other)
	{
		_handledPath = other._handledPath;
	}
	return (*this);
}

StaticFileHandler::~StaticFileHandler()
{}

////////////////////////////////////////////////////////////////////////////////
/// Public methods: handleget
////////////////////////////////////////////////////////////////////////////////

/// @brief handleget, public method to handle the request
/// This method is called by the handleget to handle the request.
/// It checks if the requested URI is a directory or a file and calls the
/// appropriate method to handle the request.
/// @param request HttpRequest object as reference
/// @param location Location object as reference
HttpResponse StaticFileHandler::handleget(const Context& context)
{
	_initMimeTypes();
	int status = _verifyHeaders(context);
	if (HttpResponse::checkStatusRange(status) != STATUS_SUCCESS)
		return (HttpResponse::createErrorResponse(status, context));

	if (context.getRequest().getUri() == "/")
		return (_handleRoot(context));
	_setHandledPath(_buildPathWithUri(context));
	if (isDir(_handledPath))
	{
		if (context.getLocation().isListdir())
			return (_handleDirListing(context));
		return (_handleDirRequest(context));
	}
	else if (isFile(_handledPath))
		return (_handleFileRequest(context));
	return (_handleNotFound(context));
}

////////////////////////////////////////////////////////////////////////////////
/// Public methods: handlepost
////////////////////////////////////////////////////////////////////////////////

HttpResponse StaticFileHandler::handlepost(const Context& context)
{
	_initMimeTypes();
	int status = _verifyHeaders(context);
	if (HttpResponse::checkStatusRange(status) != STATUS_SUCCESS)
		return (HttpResponse::createErrorResponse(status, context));
	HttpResponse resp(context);
	return (resp);
}

////////////////////////////////////////////////////////////////////////////////
/// Private methods: verify headers
////////////////////////////////////////////////////////////////////////////////

int	StaticFileHandler::_verifyHeaders(const Context& context) const
{
	const std::string&							method = context.getRequest().getMethod();
	const std::map<std::string, std::string>&	headers = context.getRequest().getHeaders();

	std::cout << RED << "TEST | HttpRequest->_uri: " << context.getRequest().getUri() << RESET << std::endl;

	if (method == "GET")
		return (_validateGetHeaders(headers));
	else if (method == "POST")
		return (_validatePostHeaders(context, headers));
	else if (method == "DELETE")
		return (_validateDeleteHeaders(headers));
	else
		throw std::runtime_error("Wrong method checked during verify headers. " + method);
}

int StaticFileHandler::_validateGetHeaders(const std::map<std::string, std::string>& headers) const
{
	if (!_hasTargetHeader("Host", headers))
		return (400);
	return (200);
}

int StaticFileHandler::_validatePostHeaders(const Context& context, const std::map<std::string, std::string>& headers) const
{
	if (context.getRequest().getBody().empty())
		return (400);
	if (!_hasTargetHeader("Content-Length", headers))
		return (411);
	if (!_hasTargetHeader("Content-Type", headers))
		return (400);
	return (200);
}

int StaticFileHandler::_validateDeleteHeaders(const std::map<std::string, std::string>& headers) const
{
	(void)headers;
	return (false);
}

bool	StaticFileHandler::_hasTargetHeader(const std::string& target, const std::map<std::string, std::string>& headers) const
{
	if (headers.find(target) == headers.end())
		return (false);
	return (true);
}


////////////////////////////////////////////////////////////////////////////////
/// Private methods
////////////////////////////////////////////////////////////////////////////////

/// @brief _handleDirRequest, private method to handle the directory request
/// This method is called when the directory listing is not `true`,
/// but the `uri` in the request is a directory.
/// the method modifies the request to append the default file name.
/// @warning this method depends on `_handleFileRequest` method
HttpResponse StaticFileHandler::_handleDirRequest(const Context& context)
{
	HttpRequest	modifiedRequest = context.getRequest();
	Context&	moditiedContext = const_cast<Context&>(context);
	
	_setHandledPath(_buildAbsolutePathWithIndex(context));

	std::cout << CYAN << "TEST | diractory: StaticFileHandler->_handledpath: " << _handledPath << RESET << std::endl;
	
	if (!isFile(_handledPath))
		return (_handleNotFound(context));
	modifiedRequest.setUri(getFullPath());
	moditiedContext.setRequest(modifiedRequest);
	return (_handleFileRequest(moditiedContext));
}

std::string StaticFileHandler::_buildAbsolutePathWithIndex(const Context& context) const
{
	std::string index		= INDEX_HTML;
	std::string readedIndex = context.getLocation().getIndex();
	if (!readedIndex.empty())
		index = readedIndex;
	return (_handledPath + index);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief _handleDirListing, private method to handle the directory listing
/// This method is called when the directory listing is `true`,
/// and the `uri` in the request is a directory.
/// @warning not implemented yet
HttpResponse StaticFileHandler::_handleDirListing(const Context& context)
{
	return (_createDirListingResponse(context));
}

/// @brief Create a `response` object for a directory listing.
/// @return `HttpResponse`
HttpResponse StaticFileHandler::_createDirListingResponse(const Context& context) const
{
	HttpResponse		resp(context);
	resp.setBody(_genDirListingHtml(getFullPath()));

	if (resp.getBody().empty() || resp.getBodyLength() <= 0)
		return (HttpResponse::internalServerError_500(context));
	resp.setDefaultHeaders();
	return (resp);
}

/// @brief Generates an HTML page with a directory listing.
/// @param path 
/// @return Create a list of directories and files in HTML and export it to `std::string`.
std::string	StaticFileHandler::_genDirListingHtml(const std::string& path) const
{
	std::stringstream	html;
	std::string			body;

	html.clear();
    html << "<html><head><title>Directory Listing</title></head><body>";
    html << "<h2>Directory Listing for " << path << "</h2><ul>";
    
    html << _genListing(path);

    html << "</ul></body></html>";
	body = html.str();
	return (body);
}

std::string StaticFileHandler::_genListing(const std::string& path) const
{
	DIR*			dir = opendir(path.c_str());
    struct dirent*	entry;
	std::string		listing;

	if (dir == NULL)
		return ("<li>Error opening directory</li>");
	else
	{
		while ((entry = readdir(dir)) != NULL)
		{
			// TODO: implement: sort the list
			std::string name = entry->d_name;
			if (name != "." && name != "..")
			{
				if (entry->d_type == DT_DIR)
					listing += "<li><a href=\"" + name + "/\">" + name + "</a></li>";
				else
					listing += "<li><a href=\"" + name + "\">" + name + "</a></li>";
			}
		}
		closedir(dir);
		return (listing);
	}
}

////////////////////////////////////////////////////////////////////////////////
HttpResponse StaticFileHandler::_handleFileRequest(const Context& context)
{
	return (_createResponseForFile(context));
}

HttpResponse StaticFileHandler::_createResponseForFile(const Context& context) const
{
	HttpResponse resp(context, _handledPath);
	resp.setHeader("Content-Type", resolveMimeType(_handledPath));
	return (resp);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief _handleRoot, private method to handle the root request
/// This method is called when the requested URI is the root directory.
/// It builds the full path of the default file and checks if the file exists.
HttpResponse StaticFileHandler::_handleRoot(const Context& context)
{
	_setHandledPath(_buildAbsolutePathWithRoot(context));
	if (!isFile(_handledPath))
		return (_handleNotFound(context));
	return (_createResponseForFile(context));
}

////////////////////////////////////////////////////////////////////////////////
std::string StaticFileHandler::_buildAbsolutePathWithRoot(const Context& context) const
{
	std::string	locationRoot	= context.getLocation().getRootPath();
	std::string serverRoot		= context.getServer().root;
	std::string	defaultFile		= context.getLocation().getIndex();

	if (locationRoot.empty())
		throw std::runtime_error("Root path is empty");
	if (defaultFile.empty())
		defaultFile = INDEX_HTML;
	std::string fullPath = "." + serverRoot + locationRoot + "/" + defaultFile;
	return (fullPath);
}

std::string StaticFileHandler::_buildPathWithUri(const Context& context) const
{
	std::string	locationRoot	= context.getLocation().getRootPath();
	std::string serverRoot		= context.getServer().root;

	if (locationRoot.empty() || serverRoot.empty())
		throw std::runtime_error("Root path is empty");
	std::string fullPath = "." + serverRoot + locationRoot + context.getRequest().getUri();

	std::cout << CYAN << "TEST | URI build: absolute path: " << fullPath << RESET << std::endl;

	return(fullPath);
}

HttpResponse StaticFileHandler::_handleNotFound(const Context& context)
{
	return (HttpResponse::notFound_404(context));
}


////////////////////////////////////////////////////////////////////////////////
/// Setters
////////////////////////////////////////////////////////////////////////////////

void	StaticFileHandler::_setHandledPath(const std::string& fullPath)
{
	_handledPath = fullPath;
}

////////////////////////////////////////////////////////////////////////////////
/// Getters
////////////////////////////////////////////////////////////////////////////////

std::string	StaticFileHandler::getFullPath() const
{
	return (_handledPath);
}

////////////////////////////////////////////////////////////////////////////////
/// Mime types
////////////////////////////////////////////////////////////////////////////////
/// @brief Initializes the MIME types map from the configuration.
/// If the configuration does not provide MIME types, default values are used.
void	StaticFileHandler::_initMimeTypes()
{
	Config&	config = Config::getInstance();
	
	config.getMimeTypeMap(); 
	if (config.getMimeTypeMap().size() > 0)
		_mimeTypes = config.getMimeTypeMap();
	else
	{
		_mimeTypes.insert(std::make_pair("html", "text/html"));
		_mimeTypes.insert(std::make_pair("css", "text/css"));
		_mimeTypes.insert(std::make_pair("js", "application/javascript"));
		_mimeTypes.insert(std::make_pair("png", "image/png"));
		_mimeTypes.insert(std::make_pair("jpg", "image/jpeg"));
		_mimeTypes.insert(std::make_pair("gif", "image/gif"));
		_mimeTypes.insert(std::make_pair("txt", "text/plain"));
	}
} 

/// @brief Returns the MIME type of a given file based on its file extension.
/// @example resolveMimeType("index.html") ext = "html", and returns "text/html"
/// @param path The path of the file.
/// @return The MIME type of the file.
std::string StaticFileHandler::resolveMimeType(const std::string path) const
{
	std::string::size_type dotPos = path.find_last_of(".");
	if (dotPos == std::string::npos)
		return ("text/plain");
	std::string ext = path.substr(dotPos + 1);
	std::map<std::string, std::string>::const_iterator it = _mimeTypes.find(ext);
	if (it != _mimeTypes.end())
		return (it->second);
	return ("application/octet-stream");
}