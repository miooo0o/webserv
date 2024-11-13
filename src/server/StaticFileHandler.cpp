/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   StaticFileHandler.cpp                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: minakim <minakim@student.42berlin.de>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/06/30 16:23:00 by sanghupa          #+#    #+#             */
/*   Updated: 2024/11/13 02:34:22 by minakim          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "StaticFileHandler.hpp"
#include "HttpResponse.hpp"
#include "HttpRequest.hpp"
#include "Location.hpp"
#include "Context.hpp"

////////////////////////////////////////////////////////////////////////////////
/// @brief `relativePath` is used as an initial step to find the file that corresponds
///	       to the client's request URI
/// @brief `resolvedPath` is used to return the final file path that the server can provide.
///		   It is used to handle the request, saved as _relativePath in the class.


StaticFileHandler::StaticFileHandler()
	: _relativePath(""), _mimeTypes()
{
}

StaticFileHandler::StaticFileHandler(const StaticFileHandler& other)
{
	_relativePath = other._relativePath;
}

StaticFileHandler& StaticFileHandler::operator=(const StaticFileHandler& other)
{
	if (this != &other)
	{
		_relativePath = other._relativePath;
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
HttpResponse StaticFileHandler::handleGet(const Context& context)
{
	_initMimeTypes();
	int status = _verifyHeaders(context);
	if (HttpResponse::checkStatusRange(status) != STATUS_SUCCESS)
		return (HttpResponse::createErrorResponse(status, context));
	if (context.getRequest().getUri() == "/")
		return (_handleRoot(context));
	_setRelativePath(_buildPathWithUri(context));
	if (isDir(_relativePath))
	{
		if (context.getLocation().isListdir())
			return (_handleDirListing(context));
		return (_handleDirRequest(context));
	}
	else if (isFile(_relativePath))
		return (_handleFileRequest(context));
	return (_handleNotFound(context));
}

////////////////////////////////////////////////////////////////////////////////
/// Public methods: handlepost
////////////////////////////////////////////////////////////////////////////////

HttpResponse StaticFileHandler::handlePost(const Context& context)
{
	_initMimeTypes();
	int status = _verifyHeaders(context);
	if (HttpResponse::checkStatusRange(status) != STATUS_SUCCESS)
		return (HttpResponse::createErrorResponse(status, context));
	_setRelativePath(_buildUploadPath(context));
	if (createDir(_relativePath))
		return (_processUploadFile(context));
	return (HttpResponse::notFound_404(context));
}

////////////////////////////////////////////////////////////////////////////////
/// Public methods: handleDelete
////////////////////////////////////////////////////////////////////////////////
HttpResponse StaticFileHandler::handleDelete(const Context& context)
{
	_initMimeTypes();
	int status = _verifyHeaders(context);
	if (HttpResponse::checkStatusRange(status) != STATUS_SUCCESS)
		return (HttpResponse::createErrorResponse(status, context));
	
	// TODO: check logic for delete delete
	_setRelativePath(_buildPathWithUri(context));
	if (isFile(_relativePath) || isDir(_relativePath))
	{
		if (!hasWritePermission(_relativePath))
			return (HttpResponse::forbidden_403(context));
		if (!hasReadPermission(_relativePath) && deleteFileOrDir(_relativePath))
			return (HttpResponse::noContent_204(context));
		return (HttpResponse::internalServerError_500(context));
	}
	return (_handleNotFound(context));
}

////////////////////////////////////////////////////////////////////////////////
/// Public methods: handleCgiGet
////////////////////////////////////////////////////////////////////////////////

HttpResponse StaticFileHandler::handleCgiGet(const Context& context)
{
	(void)context;
	return (HttpResponse::notImplemented_501(context));
}

////////////////////////////////////////////////////////////////////////////////
/// Public methods: handleCgiPost
////////////////////////////////////////////////////////////////////////////////

HttpResponse StaticFileHandler::handleCgiPost(const Context& context)
{
	(void)context;
	return (HttpResponse::notImplemented_501(context));
}


////////////////////////////////////////////////////////////////////////////////
HttpResponse	StaticFileHandler::_processUploadFile(const Context& context)
{

	std::cout << "TEST | body type: " << context.getRequest().getBodyType() << std::endl;
	
	HttpRequest::e_body_type bodyType = context.getRequest().getBodyType();
	if (bodyType == HttpRequest::FORM_DATA)
		return (_handleFormDataBody(context));
	return (HttpResponse::internalServerError_500(context));
}

HttpResponse StaticFileHandler::_handleFormDataBody(const Context& context)
{
    FormData formData(context.getRequest());

	if (!_validateMimeType(formData.getFilename()))
		return (HttpResponse::forbidden_403(context));

	std::cout << CYAN << formData << RESET << std::endl;

	if (!formData.isValid())
		return (HttpResponse::badRequest_400(context));
	if (formData.getFilename().empty())
		return (HttpResponse::badRequest_400(context));
    std::string filePath = _buildFilePath(formData.getFilename());
    if (writeFile(filePath, formData.getContent()))
        return (HttpResponse::created_201(context, FormData::clearFileName(formData.getFilename())));
    return (HttpResponse::internalServerError_500(context));
}

bool StaticFileHandler::writeFile(const std::string& filePath, const std::string& content) const
{
	std::ofstream file(filePath.c_str());
	if (!file.is_open())
		return (false);
	file << content;
	file.close();
	return (true);
}

////////////////////////////////////////////////////////////////////////////////
/// Private methods
////////////////////////////////////////////////////////////////////////////////

/// @brief Verifies the headers of the request.
/// @param context 
/// @return 
int	StaticFileHandler::_verifyHeaders(const Context& context) const
{
	const std::string&							method = context.getRequest().getMethod();
	const std::map<std::string, std::string>&	headers = context.getRequest().getHeaders();

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

int StaticFileHandler::_validatePostHeaders
	(const Context& context, const std::map<std::string, std::string>& headers) const
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

bool	StaticFileHandler::_hasTargetHeader
		(const std::string& target, const std::map<std::string, std::string>& headers) const
{
	if (headers.find(target) == headers.end())
		return (false);
	return (true);
}


/// @brief _handleDirRequest, private method to handle the directory request
/// This method is called when the directory listing is not `true`,
/// but the `uri` in the request is a directory.
/// the method modifies the request to append the default file name.
/// @warning this method depends on `_handleFileRequest` method
HttpResponse StaticFileHandler::_handleDirRequest(const Context& context)
{
	HttpRequest	modifiedRequest = context.getRequest();
	Context&	moditiedContext = const_cast<Context&>(context);
	
	_setRelativePath(_buildPathWithIndex(context));
	std::cout << " relative path that with index is built: " << _relativePath << std::endl;
	if (!isFile(_relativePath))
		return (_handleNotFound(context));
	modifiedRequest.setUri(getRelativePath());
	moditiedContext.setRequest(modifiedRequest);
	return (_handleFileRequest(moditiedContext));
}

////////////////////////////////////////////////////////////////////////////////

/// @brief Validates server and location root paths.
/// Throws an exception if either path is empty.
/// @param serverRoot Root path of the server.
/// @param locationRoot Root path of the location.
void StaticFileHandler::_validateRootPaths
	(const std::string& serverRoot, const std::string& locationRoot) const
{
	if (serverRoot.empty() || locationRoot.empty())
		throw std::runtime_error("Root path is empty");
}

/// @brief Constructs the full path based on server root, location root, and additional path components.
/// Uses the default index file if none is specified in the context.
/// @param context The context containing server and location configurations.
/// @param additionalPath Additional path components to append.
/// @return Constructed path with server and location roots.
std::string StaticFileHandler::_buildFullPath
	(const Context& context, const std::string& additionalPath) const
{
	std::string serverRoot = context.getServer().root;
	std::string locationRoot = context.getLocation().getRootPath();

	_validateRootPaths(serverRoot, locationRoot);
	return ("." + serverRoot + locationRoot + "/" + additionalPath);
}

/// @brief Builds the path to the index file or specified default file.
/// @param context The context for retrieving index settings.
/// @return Path to the default or specified index file.
std::string StaticFileHandler::_buildPathWithIndex(const Context& context) const
{
	std::string indexFile = context.getLocation().getIndex();
	if (indexFile.empty())
		indexFile = INDEX_HTML;
	return (_buildFullPath(context, indexFile));
}

/// @brief Constructs the full path with the URI requested by the client.
/// @param context The context containing server and location configurations.
/// @return Full path including server root, location root, and request URI.
std::string StaticFileHandler::_buildPathWithUri(const Context& context) const
{
	return (_buildFullPath(context, context.getRequest().getUri()));
}

/// @brief Constructs the path with upload directory.
/// @param context The context containing server, location, and upload configurations.
/// @return Full path for upload handling.
std::string StaticFileHandler::_buildUploadPath(const Context& context) const
{
	std::string serverRoot = context.getServer().root;
	std::string uploadRoot = context.getServer().upload_dir;
	std::string uri = context.getRequest().getUri();
	std::string uploadPath = context.getLocation().getUploadPath();

	std::cout << GREEN << "TEST | uploadRoot: " << uploadRoot << " uri: " << uri 
	<< "\n" << "." + serverRoot + uploadRoot << RESET << std::endl;

	return ("." + serverRoot + uploadRoot);
}

std::string StaticFileHandler::_buildFilePath(const std::string& fileName) const
{
	return (_relativePath + "/" + FormData::clearFileName(fileName));
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
	resp.setBody(_genDirListingHtml(getRelativePath()));

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
	HttpResponse resp(context, _relativePath);
	resp.setHeader("Content-Type", resolveMimeType(_relativePath));
	return (resp);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief _handleRoot, private method to handle the root request
/// This method is called when the requested URI is the root directory.
/// It builds the full path of the default file and checks if the file exists.
HttpResponse StaticFileHandler::_handleRoot(const Context& context)
{
	_setRelativePath(_buildPathWithIndex(context));
	if (!isFile(_relativePath))
		return (_handleNotFound(context));
	return (_createResponseForFile(context));
}


HttpResponse StaticFileHandler::_handleNotFound(const Context& context)
{
	return (HttpResponse::notFound_404(context));
}


////////////////////////////////////////////////////////////////////////////////
/// Setters
////////////////////////////////////////////////////////////////////////////////

void StaticFileHandler::_setRelativePath(const std::string& relativePath)
{
	_relativePath = relativePath;
}

////////////////////////////////////////////////////////////////////////////////
/// Getters
////////////////////////////////////////////////////////////////////////////////

std::string	StaticFileHandler::getRelativePath() const
{
	return (_relativePath);
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

bool StaticFileHandler::_validateMimeType(const std::string& filename) const
{
	
	std::string ext = filename.substr(filename.find_last_of(".") + 1);
	std::map<std::string, std::string>::const_iterator it = _mimeTypes.find(ext);
	if (it == _mimeTypes.end())
		return (false);
	return (true);
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