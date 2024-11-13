/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpResponse.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: minakim <minakim@student.42berlin.de>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/06/30 16:23:00 by sanghupa          #+#    #+#             */
/*   Updated: 2024/11/13 02:40:03 by minakim          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "webserv.hpp"
#include "HttpResponse.hpp"
#include "ErrorResponse.hpp"
#include "Location.hpp"
#include "Context.hpp"

////////////////////////////////////////////////////////////////////////////////
/// @brief 42 pdf
/// Your HTTP response status codes must be accurate.
/// You server must have default error pages if none are provided.
/// Limit client body size.
////////////////////////////////////////////////////////////////////////////////

HttpResponse::HttpResponse(const Context& context)
	: _statusCode(200), _statusMessage("OK"), _context(const_cast<Context&>(context))
{
}

HttpResponse::HttpResponse(const Context& context, const std::string& filePath)
	: _statusCode(200), _statusMessage("OK"), _context(const_cast<Context&>(context))
{
	initializefromFile(filePath);
}

/// @brief Copy constructor for the HttpResponse class.
/// @param other 
HttpResponse::HttpResponse(const HttpResponse& other)
	: _context(other._context)
{
	_statusCode = other._statusCode;
	_statusMessage = other._statusMessage;
	_headers = other._headers;
	_body = other._body;
}

HttpResponse& HttpResponse::operator=(const HttpResponse& other)
{
	if (this != &other)
	{
		_statusCode = other._statusCode;
		_statusMessage = other._statusMessage;
		_headers = other._headers;
		_body = other._body;
		_bodyLength = other._bodyLength;
		_context = other._context;
	}
	return (*this);
}

/// @brief Destructor for the HttpResponse class.
HttpResponse::~HttpResponse()
{
}

////////////////////////////////////////////////////////////////////////////////
/// Public member functions: toString
////////////////////////////////////////////////////////////////////////////////

std::string	HttpResponse::generateResponseToString() const
{
	return (getResponseLine() + _getHeadersString() + "\r\n\r\n" + _body);
}

////////////////////////////////////////////////////////////////////////////////
/// Public member functions: set default headers
/// @fn setDefaultHeadersImpl:
///		Basic implementation of setting the default headers for an HttpResponse object.
/// @fn setDefaultHeaders(HttpResponse& resp): Static function
///		Use in static function to set the default headers for an HttpResponse object.
/// @fn setDefaultHeaders():
///		Read the Object's body and set the default headers for the response.
////////////////////////////////////////////////////////////////////////////////

/// @brief Use in static function to set the default headers for an HttpResponse object.
/// @param resp The object to set the headers for.
/// @param bodyContent The content to be included in the response body.
void	HttpResponse::setDefaultHeaders(HttpResponse& resp)
{
	resp._setDefaultHeadersImpl();
}

/// @brief Read the Object's body and set the default headers for the response.
/// @param bodyContent The content to be included in the response body.
void	HttpResponse::setDefaultHeaders()
{
	this->_setDefaultHeadersImpl();
}

/// @brief Basic implementation of setting the default headers for an HttpResponse object.
void	HttpResponse::_setDefaultHeadersImpl()
{
	if (_bodyLength <= 0)
		*this = internalServerError_500(_context);
	setHeader("Content-Length", toString(_bodyLength));
	setHeader("Content-Type", "text/html");
	setHeader("Connection", "close");
}

////////////////////////////////////////////////////////////////////////////////
/// Public member functions: initializefromFile
////////////////////////////////////////////////////////////////////////////////
/// @brief Creates an Static HttpResponse object by reading the contents of a file.
/// @param filePath The path to the file to be read.
/// @return HttpResponse The created HttpResponse object.
/// @warning If the file cannot be opened, the response point a 404 error().
void	HttpResponse::initializefromFile(const std::string& filePath)
{
	_fileToBody(filePath);
	if (_body.empty())
		return ;
	if (_statusCode == 200)
		setDefaultHeaders();
}

////////////////////////////////////////////////////////////////////////////////
/// Private member functions
////////////////////////////////////////////////////////////////////////////////
/// @brief Reads the contents of a file into a string.
/// If the file cannot be opened, the response point a 404 error().
/// If the file is empty, the response point a 500 error().
/// @param filePath The path to the file to be read.
/// @return Return the file content as a string.
/// If there is any error, return an empty string.
void	HttpResponse::_fileToBody(const std::string& filePath)
{
	std::ifstream	file(filePath.c_str(), std::ios::binary | std::ios::ate);
	std::string		body = "";
	std::streamsize fileLength;

	if (!file.is_open())
	{
		*this = notFound_404(_context);
		return ;
	}
	fileLength = file.tellg();
	if (fileLength < 0)
	{
		*this = internalServerError_500(_context);
		return ;
	}
	file.seekg(0, std::ios::beg);
	if (static_cast<size_t>(fileLength) > _context.getServer().max_body_size)
	{
		*this = requestEntityTooLarge_413(_context);
		return ;
	}
	body.resize(fileLength);
	if (!file.read(&body[0], fileLength))
	{
		*this = internalServerError_500(_context);
		return ;
	}
	setBody(body);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief returns the headers as a string.
/// @return `std::string`, The headers as a string.
std::string	HttpResponse::_getHeadersString() const
{
	std::string headers;
	for (std::map<std::string, std::string>::const_iterator it =_headers.begin();
															it != _headers.end();
															it++)
	{
		headers += it->first + ": " + it->second + "\r\n";
	}
	return (headers);
}

/// @brief returns the status line as a string.
/// @return `std::string`, The status line as a string.
std::string HttpResponse::_getStatusLine() const
{
	std::stringstream statusLine;
	statusLine << "HTTP/1.1 " << _statusCode << " " << _statusMessage << "\r\n";
	return (statusLine.str());
}

////////////////////////////////////////////////////////////////////////////////
/// @brief Initializes the map of HTTP status codes to status messages.
/// @var `statusMap` Static, The map of status codes to status messages.
/// @return Return the map of status codes and status messages.
const std::map<int, std::string>& HttpResponse::_staticInitStatusMap()
{
	static std::map<int, std::string> statusMap;

	if (statusMap.empty())
	{
		statusMap[200] = "OK";
		statusMap[201] = "Created";
		statusMap[204] = "No Content";
		statusMap[301] = "Moved Permanently";
		statusMap[302] = "Found";
		statusMap[400] = "Bad Request";
		statusMap[401] = "Unauthorized";
		statusMap[403] = "Forbidden";
		statusMap[404] = "Not Found";
		statusMap[405] = "Method Not Allowed";
		statusMap[408] = "Request Timeout";
		statusMap[413] = "Request Entity Too Large";
		statusMap[418] = "I'm a Teapot";
		statusMap[500] = "Internal Server Error";
		statusMap[501] = "Not Implemented";
	}
	return (statusMap);
}

/// @brief Sets the HTTP status code and corresponding message for the response.
/// 
/// This function sets the `_statusCode` member variable and updates the `_statusMessage`
/// based on a predefined map of status codes. The map is initialized once and reused, 
/// ensuring a consistent set of status codes and messages across the application.
///
/// If the provided status code is not found in the map, an exception is thrown.
///
/// @param code The HTTP status code to set for the response.
/// @throws `std::runtime_error` if the status code is not found in the map.
void	HttpResponse::setStatusCode(int code)
{
	_statusCode = code;
	const std::map<int, std::string>& statusMap = _staticInitStatusMap();

	std::map<int, std::string>::const_iterator it = statusMap.find(code);
	if (it != statusMap.end())
		_statusMessage = it->second;
	else
		throw std::runtime_error("Unknown status code: " + toString(getStatusCode()));
}

////////////////////////////////////////////////////////////////////////////////

HttpResponse HttpResponse::response_withMessage
			(StaticMethod method, const Context& context, const std::string& message)
{
	HttpResponse resp = method(context);
	resp.setBody(resp._generateHtmlBody(message));
	return (resp);
}

/// @brief Generates a simple HTML response for a given HTTP status code.
/// @param code The HTTP status code for which to generate the response. 
/// @return Return the generated HTML response.
HttpResponse	HttpResponse::createSimpleHttpResponse(int code)
{
	HttpResponse resp(_context);
	resp.setStatusCode(code);
	resp.setBody(resp._generateHtmlBody());
	setDefaultHeaders(resp);
	return (resp);
}

/// @brief generates a simple HTML response body for a given HTTP status code.
/// @return `std::string`, The generated HTML response body.
std::string	HttpResponse::_generateHtmlBody()
{
	if (toString(getStatusCode()).empty() || getStatusMessage().empty())
		return ("");
	std::string body = 
		"<html>"
		"<body>"
		"<h1>" + toString(getStatusCode()) + " " + getStatusMessage() + "</h1>"
		"</body>"
		"</html>";
	return (body);
}

std::string	HttpResponse::_generateHtmlBody(const std::string& message)
{
	if (toString(getStatusCode()).empty() || getStatusMessage().empty())
		return ("");
	std::string body = 
		"<html>"
		"<body>"
		"<h1>" + toString(getStatusCode()) + " " + getStatusMessage() + "</h1>"
		+ message +
		"</body>"
		"</html>";
	return (body);
}

/// @brief constructs a page detail object for a given file path.
/// `t_page_detail` is a struct that contains the path of the file and a boolean flag indicating if the file is valid.
/// @param path 
/// @return `t_page_detail`
t_page_detail	HttpResponse::constructPageDetail(const std::string& path)
{
	t_page_detail	page;
	page.path = path;
	page.isValid = isFile(path);
	return (page);
}

////////////////////////////////////////////////////////////////////////////////
/// Protected member funtion: createErrorResponse(), error responses
////////////////////////////////////////////////////////////////////////////////

/// @brief use it to create an error response with status code.
/// @param code status code.
/// @return reutrn `ErrorResponse` object. 
HttpResponse HttpResponse::createErrorResponse(int code)
{
	if (checkStatusRange(code) != STATUS_ERROR)
		throw std::runtime_error("Invalid error code: " + toString(code));

	ErrorResponse	errorResp(_context);
	return (errorResp.generateErrorResponse(code));
}

////////////////////////////////////////////////////////////////////////////////
/// Public member function: createErrorResponse(), static error responses.
////////////////////////////////////////////////////////////////////////////////

/// @brief Static, use it to create an error response
/// @param code status code.
/// @param context reference to the context object.
/// @return reutrn `ErrorResponse` object. 
HttpResponse HttpResponse::createErrorResponse(int code, const Context& context)
{
	if (checkStatusRange(code) != STATUS_ERROR)
		throw std::runtime_error("Invalid error code: " + toString(code));

	ErrorResponse errorResp(context);
	return (errorResp.generateErrorResponse(code));
}

////////////////////////////////////////////////////////////////////////////////
/// Public member functions: static error responses.
////////////////////////////////////////////////////////////////////////////////

HttpResponse	HttpResponse::badRequest_400(const Context& context)
{
	return (createErrorResponse(400, context));
}

HttpResponse	HttpResponse::forbidden_403(const Context& context)
{
	return (createErrorResponse(403, context));
}

HttpResponse	HttpResponse::notFound_404(const Context& context)
{
	return ( createErrorResponse(404, context));
}

HttpResponse	HttpResponse::methodNotAllowed_405(const Context& context)
{
	return (createErrorResponse(405, context));
}

HttpResponse	HttpResponse::requestTimeout_408(const Context& context)
{
	return (createErrorResponse(408, context));
}

HttpResponse	HttpResponse::requestEntityTooLarge_413(const Context& context)
{
	return (createErrorResponse(413, context));
}

HttpResponse	HttpResponse::imaTeapot_418(const Context& context)
{
	return (createErrorResponse(418, context));
}

HttpResponse	HttpResponse::internalServerError_500(const Context& context)
{
	return (createErrorResponse(500, context));
}

HttpResponse	HttpResponse::notImplemented_501(const Context& context)
{
	return (createErrorResponse(501, context));
}


////////////////////////////////////////////////////////////////////////////////
/// Public member functions: static success responses.
////////////////////////////////////////////////////////////////////////////////

/// @brief Creates a successful HTTP response with status code 200.
/// @details All `Response` object constructors initialize the status code to 200 and the status message to "OK".
/// @return Return the generated HTML response.
HttpResponse	HttpResponse::success_200(const Context& context)
{
	HttpResponse resp(context);
	resp.setBody(resp._generateHtmlBody());
	setDefaultHeaders(resp);
	return (resp);
}

HttpResponse	HttpResponse::created_201(const Context& context, const std::string& fileName)
{
	HttpResponse	resp(context);
	std::string		message;

	message = "<p>File uploaded successfully: " + fileName + "</p>";
	// message += "<p>Location: " + context.getServer().upload_dir + "/" + fileName + "</p>";
	resp.setBody(resp._generateHtmlBody(message));
	setDefaultHeaders(resp);
	return (resp);
}

HttpResponse	HttpResponse::redirect_301(const Context& context, const std::string& location)
{
	// TODO: implement
	(void)location;
	HttpResponse resp(context);
	resp.setBody(resp._generateHtmlBody());
	setDefaultHeaders(resp);
	return (resp);
}

HttpResponse	HttpResponse::noContent_204(const Context& context)
{
	HttpResponse resp(context);
	resp.setStatusCode(204);
	setDefaultHeaders(resp);
	return (resp);
}

/// @brief Checks if the status code is within the valid range.
/// @param code status code.
/// @return return the status range, `e_status`.
/// @enum e_statue.
e_status	HttpResponse::checkStatusRange(int code)
{
	if (code < 100 || code > 599)
		return (STATUS_INVALID);
	if (code < 200)
		return (STATUS_INFORMATIONAL);
	if (code < 300)
		return (STATUS_SUCCESS);
	if (code < 400)
		return (STATUS_REDIRECTION);
	if (code < 600)
		return (STATUS_ERROR);
	return (STATUS_INVALID);
}

////////////////////////////////////////////////////////////////////////////////
/// Setters
////////////////////////////////////////////////////////////////////////////////

void	HttpResponse::setStatusCode(int code, const std::string statusMessage)
{
	_statusCode = code;
	_statusMessage = statusMessage;
}

void	HttpResponse::setHeader(const std::string key, const std::string value)
{
	_headers.insert(std::pair<std::string, std::string>(key, value));
}

void	HttpResponse::setBody(const std::string bodyContent)
{
	_body = bodyContent;
	_bodyLength = static_cast<size_t>(_body.size());
}

////////////////////////////////////////////////////////////////////////////////
/// Getters
////////////////////////////////////////////////////////////////////////////////

std::string	HttpResponse::getBody()
{
	return (_body);
}

size_t	HttpResponse::getBodyLength()
{
	return (_bodyLength);
}

int HttpResponse::getStatusCode() const
{
	return (_statusCode);
}

std::string HttpResponse::getStatusMessage() const
{
	return (_statusMessage);
}

std::string HttpResponse::getResponseLine() const
{
	std::ostringstream oss;
	oss << "HTTP/1.1 " << _statusCode << " " << _statusMessage << "\r\n";
	return (oss.str());
}
