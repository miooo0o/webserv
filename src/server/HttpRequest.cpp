/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpRequest.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: minakim <minakim@student.42berlin.de>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/06/30 16:23:00 by sanghupa          #+#    #+#             */
/*   Updated: 2024/11/05 11:43:09 by minakim          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "webserv.hpp"
#include "HttpRequest.hpp"

////////////////////////////////////////////////////////////////////////////////
// Private methods Constructor
////////////////////////////////////////////////////////////////////////////////

HttpRequest::HttpRequest()
	: _body(""), _type(NONE), _contentLength(false, NOT_SET)
{
}

////////////////////////////////////////////////////////////////////////////////
// Public methods Constructor
////////////////////////////////////////////////////////////////////////////////

/// @brief Constructs an HttpRequest object, initializing and parsing the given request data.
/// @param data The raw HTTP request data read by the server, typically from a browser request.
/// 
/// This constructor initializes the HttpRequest object and parses the provided request data
/// to populate its attributes, including the request type, body, and content length. The 
/// parse result is indicated in the console output for debugging purposes.
HttpRequest::HttpRequest(std::string& data)
	: _body(""), _type(NONE), _contentLength(false, NOT_SET)
{
	/// FIXME: check logic to `_parseHttpRequestHeader`
	if (parse(data))
		std::cout << "TEST | HttpRequest | parse success" << std::endl;
	else
		std::cout << "TEST | HttpRequest | parse failed" << std::endl;
}

HttpRequest::~HttpRequest()
{}

////////////////////////////////////////////////////////////////////////////////
/// The current parse logic for my request is to first split the syntax into
/// request, header, and body lines, and then parse the split syntax separately.
/// I've separated the 'split' and 'parse' parts so that one function does one job. 
////////////////////////////////////////////////////////////////////////////////

/// @brief This function parses the request data and extracts
///			the method, path, version, headers, and body.
/// @param requestData The request data to be parsed. 
/// @return bool
bool HttpRequest::parse(const std::string& requestData)
{
	ReadedLines splitedRequestData = _splitRequestData(requestData);
	if (!_parseRequestLine(splitedRequestData.request))
		return (false);
	if (!_parseHeaders(splitedRequestData.headers))
		return (false);
	if (!_processRequestBody(splitedRequestData.bodyLines))
		return (false);
	return (true);
}

////////////////////////////////////////////////////////////////////////////////
// TODO: change to this logic for parsing the request

bool	HttpRequest::_parseHttpRequestHeader(const std::string& data)
{
	std::istringstream	iss(data);
	std::string			readline;

	if (data.empty())
		return (false);
	if (!std::getline(iss, readline))
		return (false);
	if (!_parseRequestLine(readline))
		return (false);
	if (_parseHeaders(_convertPartToHeaders(iss)))
		return (true);
	return (false);
}

bool	HttpRequest::parseHttpRequestBody(const std::string& data)
{
	if (!hasBody())
		return (true);
	if (getContentLength() <= 0)
		return (false);
	if (_headers["Content-Type"] == "application/json" || _headers["Content-Type"] == "text/plain")
		setBody(data, RAW);
	else if (_headers["Transfer-Encoding"] == "chunked")
		setBody(data, CHUNKED);
	else if (_headers["Content-Type"].find("multipart/form-data") != std::string::npos)
		setBody(data, FORM_DATA);
	return (true);
}

////////////////////////////////////////////////////////////////////////////////

bool	HttpRequest::_processRequestBody(const std::string& bodyLines)
{
	if (!hasBody())
		return (true);
	if (getContentLength() <= 0)
		return (false);
	if (_headers["Content-Type"] == "application/json" || _headers["Content-Type"] == "text/plain")
		setBody(bodyLines, RAW);
	else if (_headers["Transfer-Encoding"] == "chunked")
		setBody(bodyLines, CHUNKED);
	else if (_headers["Content-Type"].find("multipart/form-data") != std::string::npos)
		setBody(bodyLines, FORM_DATA);
	return (true);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief Separate the request data into request line, headers, and body.
/// @param requestData The request data to be separated.
/// @return A struct containing the separated request data.
ReadedLines HttpRequest::_splitRequestData(const std::string& requestData)
{
	ReadedLines			data;
	std::istringstream	iss(requestData);
	std::string			readline;
	std::string			drafts;

	if (requestData.empty())
		return (data);

	if (!std::getline(iss, readline))
		return (data);
	data.request = readline;
	data.headers = _convertPartToHeaders(iss);
	if (data.headers.empty())
		return (data);
	data.bodyLines = _convertPartToBodyLines(iss);
	return (data);
}

std::vector<std	::string> HttpRequest::_convertPartToHeaders(std::istringstream& iss)
{
	std::string					readline;
	std::vector<std	::string>	res;

	while (std::getline(iss, readline) && readline != "\r")
		res.push_back(readline);
	return (res);
}

std::string HttpRequest::_convertPartToBodyLines(std::istringstream& iss)
{
	std::string					readline;
	std::string					drafts;

	while (std::getline(iss, readline))
		drafts += readline + "\n";
	return (drafts);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief Parses the request line and extracts the method, path, and version.
/// @param requestLine `_method` `_uri` `_version`, example: GET /path/resource HTTP/1.1
/// @return bool
bool HttpRequest::_parseRequestLine(const std::string& requestLine)
{
	std::string			trimmedLine = trim(requestLine);
	std::istringstream	iss(trimmedLine);

	if (requestLine.empty() || trimmedLine.empty())
		return (false);
	iss >> _method >> _uri >> _version;
	if (_method.empty() || _uri.empty() || _version.empty())
		return (false);
	return (true);
}

/// @brief Parses the header lines and extracts the headers.
/// @param headerLines key:value pairs separated by `\r\n`
/// @return bool
bool HttpRequest::_parseHeaders(const std::vector<std::string> &headerLines)
{
	if (headerLines.empty())
		return (false);
	for (std::vector<std::string>::const_iterator	it = headerLines.begin();
													it != headerLines.end();
													++it)
	{
		const std::string& line = *it;
		if (line.empty())
			continue;
		size_t colonPos = line.find(':');
		if (colonPos == std::string::npos)
			return (false);
		std::string key = trim(line.substr(0, colonPos));
		if (key.empty())
			return (false);
		std::string value = trim(line.substr(colonPos + 1));
		if (value.empty())
			return (false);
		_headers.insert(std::make_pair(key, value));
	}
	if (_headers.find("Content-Length") != _headers.end())
		_contentLength = std::make_pair(true, toSizeT(_headers["Content-Length"]));
	return (true);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief Trims the string by removing leading and trailing whitespace.
/// @details `WHITESPACE`: Whitespace includes: space, tab, carriage return, and newline.
/// @param str `const std::string&`, The string to be trimmed.
/// @return The trimmed string.
std::string HttpRequest::trim(const std::string& str)
{
	std::string::size_type first = str.find_first_not_of(WHITESPACE);
	if (first == std::string::npos)
		return ("");
	std::string::size_type last = str.find_last_not_of(WHITESPACE);
	if (last == std::string::npos)
		return ("");
	return (str.substr(first, last - first + 1));
}

////////////////////////////////////////////////////////////////////////////////
/// Checker functions
////////////////////////////////////////////////////////////////////////////////

bool HttpRequest::isConnectionClose() const
{
	std::map<std::string, std::string>::const_iterator it = _headers.find("Connection");
	if (it != _headers.end() && it->second == "close")
		return (true);
	return (false);
}

////////////////////////////////////////////////////////////////////////////////
/// Getters
////////////////////////////////////////////////////////////////////////////////

std::string	HttpRequest::getMethod() const
{
	return (_method);
}

std::string	HttpRequest::getUri() const
{
	return (_uri);
}

std::string	HttpRequest::getVersion() const
{
	return (_version);
}

std::map<std::string, std::string>	HttpRequest::getHeaders() const
{
	return (_headers);
}

std::string	HttpRequest::getBody() const
{
	return (_body);
}

size_t	HttpRequest::getContentLength() const
{
	return (_contentLength.second);
}


////////////////////////////////////////////////////////////////////////////////
/// Setters
////////////////////////////////////////////////////////////////////////////////

void	HttpRequest::setUri(const std::string& uri)
{
	_uri = uri;
}

void	HttpRequest::setMethod(const std::string& method)
{
	_method = method;
}

void	HttpRequest::setVersion(const std::string& version)
{
	_version = version;
}

void	HttpRequest::setHeaders(const std::map<std::string, std::string>& headers)
{
	_headers = headers;
}

bool	HttpRequest::hasBody() const
{
        return (_contentLength.first);
}

void	HttpRequest::setBody(const std::vector<std::string>& bodyLines, e_body_type type)
{
	if (!hasBody() || getContentLength() == 0)
		return;
	std::string bodyLinesToString = toString(bodyLines);
	if (bodyLinesToString.length() != getContentLength())
		throw std::runtime_error(
		"HTTP method [" + getMethod() + "] at URI [" + getUri() + "] encountered a body length mismatch: "
		"Expected Content-Length = " + toString(getContentLength()) + 
		", but received body length = " + toString(bodyLinesToString.length()) + ".");
	_body = bodyLinesToString;
	_type = type;
}

void	HttpRequest::setBody(const std::string& bodyLines, e_body_type type)
{
	if (!hasBody() || getContentLength() == 0)
		return;
	if (bodyLines.length() != getContentLength())
		throw std::runtime_error(
		"HTTP method [" + getMethod() + "] at URI [" + getUri() + "] encountered a body length mismatch: "
		"Expected Content-Length = " + toString(getContentLength()) + 
		", but received body length = " + toString(bodyLines.length()) + ".");
	_body = bodyLines;
	_type = type;
}

void	HttpRequest::setContentLength(const ssize_t& contentLength)
{
	if (hasBody())
		_contentLength.second = contentLength;
}