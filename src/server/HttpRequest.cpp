/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpRequest.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: minakim <minakim@student.42berlin.de>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/06/30 16:23:00 by sanghupa          #+#    #+#             */
/*   Updated: 2024/11/13 01:45:46 by minakim          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "webserv.hpp"
#include "HttpRequest.hpp"

/// @brief Default constructor, Not used as public constructor.
HttpRequest::HttpRequest()
	: _body(""), _type(NONE), _contentLenght(false, NOT_SET)
{
}

/// @brief Constructs an HttpRequest object by parsing the provided request data string.
/// @details This constructor initializes the HttpRequest object by parsing the raw request data,
/// typically read from the server. The data is parsed by calling the `parse()` method, which
/// processes and stores the request details within the object.
/// @param data A `std::string` containing the raw request data, with a maximum size defined by
/// `max-read-size` in the configuration file.
/// @note Ensure that `data` respects the size limit specified in the configuration to avoid overflow.
/// TODO: @minakim consider adding more specific control the size of the data
///		- read the data (as much max-header-size) and then parse it
///		- if the data is bigger than max-header-size, return an error
///     - after parsing the header, read the body (as much max-body-size) and then parse it (public method)
HttpRequest::HttpRequest(std::string& data)
	: _body(""), _type(NONE), _contentLenght(false, NOT_SET), _queryStrings(false, std::map<std::string, std::string>())
{
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

/// @brief Processes the request body based on the content type.
/// @details The body is processed based on the content type and stored in the `_body` member.
/// @details The body is stored as a string.
/// @note the webserv supports the following body types:
///			- `application/json`		-> RAW
///			- `text/plain`				-> RAW
///			- `multipart/form-data`		-> FORM_DATA
///			- `chunked`					-> CHUNKED
/// TODO: check what kind of body types are there, and how to handle them.
/// @param bodyLines 
/// @return true if the body was processed successfully, false otherwise.
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
	if (!_processUriQuery())
		return (false);
	return (true);
}

////////////////////////////////////////////////////////////////////////////////

/// @brief Processes the URI and extracts the query string.
/// @details The query string is separated from the URI and stored in a map.
/// 		 The URI is updated to remove the query string.
///			 The query string is stored as a map of key-value pairs.
bool HttpRequest::_processUriQuery()
{
	size_t		pos	= _uri.find("?");
	if (pos == std::string::npos)
		return (true);
	return (_processQueryStrings(pos));
}

bool	HttpRequest::_processQueryStrings(size_t pos)
{
	std::string	uriQuery = _uri;
	std::string	querys;

	_queryStrings.first = true;
	if (!_setUriWithoutQuery(pos))
		return (false);
	setUri(_uri.substr(0, pos));
	querys = uriQuery.substr(pos + 1);
	if (querys.empty())
		return (false);
	if (!_parseQueryParameters(querys))
		return (false);
	return (true);
}

/// @brief Removes the query string from the URI.
/// @param pos The position of the query string in the URI.
bool	HttpRequest::_setUriWithoutQuery(size_t pos)
{
	_uri = _uri.substr(0, pos);
	if (_uri.empty())
		return (false);
	return (true);
}

/// @brief Parses the query string and extracts the key-value pairs.
/// @param queryString The query string to be parsed.
bool	HttpRequest::_parseQueryParameters(const std::string& queryString)
{
	std::string			key, value;
	std::string			addAmpersandAtLast = queryString + '&';
	std::istringstream	iss(addAmpersandAtLast);

	while (_parseNextQueryParameter(iss, key, value))
		_queryStrings.second.insert(std::make_pair(key, value));
	if (_queryStrings.first && _queryStrings.second.empty())
		throw std::runtime_error("method found query string but failed to parse it");
	return (true);
}

bool HttpRequest::_parseNextQueryParameter
					(std::istringstream& iss, std::string& key, std::string& value)
{
    return std::getline(iss, key, '=') && std::getline(iss, value, '&');
}

/// @brief Checks if the request has query strings.
/// @return bool
bool	HttpRequest::hasQueryStrings() const
{
	return (_queryStrings.first);
}

/// @brief Returns the query strings as a map of key-value pairs.
/// @return a map of key-value pairs.
std::map<std::string, std::string>	HttpRequest::getQueryStringsMap() const
{
	if (!hasQueryStrings())
		return (std::map<std::string, std::string>());
	return (_queryStrings.second);
}

/// @brief Returns the query strings as a vector of strings.
/// @return a vector of strings in the format `key=value`.
std::vector<std::string>	HttpRequest::getQueryStrings() const
{
	if (!hasQueryStrings())
		return (std::vector<std::string>());
	std::vector<std::string>	queryStrings;
	for (std::map<std::string, std::string>::const_iterator	
			it = _queryStrings.second.begin();
			it != _queryStrings.second.end();
			++it)
		queryStrings.push_back(it->first + "=" + it->second);
	return (queryStrings);
}

////////////////////////////////////////////////////////////////////////////////

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
		_contentLenght = std::make_pair(true, toSizeT(_headers["Content-Length"]));
	return (true);
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
	return (_contentLenght.second);
}

HttpRequest::e_body_type	HttpRequest::getBodyType() const
{
	return (_type);
}

std::string HttpRequest::getContentType() const
{
	std::map<std::string, std::string>::const_iterator it = _headers.find("Content-Type");
	if (it != _headers.end())
		return (it->second);
	return ("");
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
        return (_contentLenght.first);
}

// TODO: if this fucntion is not used, remove it
/// @brief Sets the body of the request.
/// @param bodyLines 
/// @param type 
void	HttpRequest::setBody(const std::vector<std::string>& bodyLines, e_body_type type)
{
	if (!hasBody() || getContentLength() == 0)
		return;

	std::string bodyLinesToString = toString(bodyLines);
	// TODO: if contect-length is bigger than max-body-size, need to handle it
	// if (bodyLinesToString.length() != getContentLength())
	// 	throw std::runtime_error(
	// 	"HTTP method [" + getMethod() + "] at URI [" + getUri() + "] encountered a body length mismatch: "
	// 	"Expected Content-Length = " + toString(getContentLength()) + 
	// 	", but received body length = " + toString(bodyLinesToString.length()) + ".");
	_body = bodyLinesToString;
	_type = type;
}

/// @brief Sets the body of the request.
/// @param bodyLines 
/// @param type 
void	HttpRequest::setBody(const std::string& bodyLines, e_body_type type)
{
	if (!hasBody() || getContentLength() == 0)
		return;
	// TODO: if contect-length is bigger than max-body-size, need to handle it
	// if (bodyLines.length() != getContentLength())
	// 	throw std::runtime_error(
	// 	"HTTP method [" + getMethod() + "] at URI [" + getUri() + "] encountered a body length mismatch: "
	// 	"Expected Content-Length = " + toString(getContentLength()) + 
	// 	", but received body length = " + toString(bodyLines.length()) + ".");

	_type = type;
	_body = bodyLines;
}

/// @brief if the request has a body, set the content length.
/// @param contentLength content-length
void	HttpRequest::setContentLength(const ssize_t& contentLength)
{
	if (hasBody())
		_contentLenght.second = contentLength;
}
