#include "Util.hpp"
#include <sstream>
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>
#include "HttpRequest.hpp"


////////////////////////////////////////////////////////////////////////////////
/// trim
////////////////////////////////////////////////////////////////////////////////
/// @brief Trims the string by removing leading and trailing whitespace.
/// @details `WHITESPACE`: Whitespace includes: space, tab, carriage return, and newline.
/// @param str `const std::string&`, The string to be trimmed.
/// @return The trimmed string.
std::string trim(const std::string& str)
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
/// toString
////////////////////////////////////////////////////////////////////////////////

std::string	toString(const int value)
{
	std::ostringstream oss;
	oss << value;
	return (oss.str());
}

std::string toString(const size_t value)
{
	std::ostringstream oss;
	oss << value;
	return (oss.str());
}

std::string toString(const ssize_t value)
{
	std::ostringstream oss;
	oss << value;
	return (oss.str());
}

std::string toString(const std::vector<std::string>& values)
{
	std::string result = "";
	for (std::vector<std::string>::const_iterator it = values.begin(); it != values.end(); ++it)
		result += *it;
	return (result);
}

size_t		toSizeT(const std::string& value)
{
	std::istringstream	iss(value);
	size_t				result;
	iss >> result;
	return (result);
}



////////////////////////////////////////////////////////////////////////////////
/// Utility functions: File, Directory
////////////////////////////////////////////////////////////////////////////////

/// @brief Checks if a file exists.
/// @param path The path of the file.
bool	isFile(const std::string path)
{
	struct stat buffer;
	if (stat(path.c_str(), &buffer) != 0)
		return (false);
	if (S_ISREG(buffer.st_mode))
		return (true);
	return (false);
}

/// @brief Checks if a file exists.
/// @param path The path of the file.
bool	isDir(const std::string path)
{
	struct stat buffer;

	if (stat(path.c_str(), &buffer) != 0)
		return (false);
	if (S_ISDIR(buffer.st_mode))
		return (true);
	return (false);
}

bool	createFile(const std::string& path)
{
	std::ofstream	file(path.c_str());
	if (file.is_open())
	{
		file.close();
		return (true);
	}
	return (false);
}

bool	deleteFile(const std::string& path)
{
	if (remove(path.c_str()) == 0)
		return (true);
	return (false);
}

bool	createDir(const std::string& path)
{
	if (isDir(path))
		return (true);
	if (mkdir(path.c_str(), 0777) == 0)
		return (true);
	return (false);
}

bool	deleteDir(const std::string& path)
{
	if (rmdir(path.c_str()) == 0)
		return (true);
	return (false);
}

////////////////////////////////////////////////////////////////////////////////
/// Utility functions: Permissions
////////////////////////////////////////////////////////////////////////////////

bool	hasWritePermission(const std::string& path)
{
	if (access(path.c_str(), W_OK) == 0)
		return (true);
	return (false);
}

bool	hasReadPermission(const std::string& path)
{
	if (access(path.c_str(), R_OK) == 0)
		return (true);
	return (false);
}

bool	hasExecutePermission(const std::string& path)
{
	if (access(path.c_str(), X_OK) == 0)
		return (true);
	return (false);
}

bool	deleteFileOrDir(const std::string& path)
{
	if (remove(path.c_str()) == 0)
		return (true);
	return (false);
}

////////////////////////////////////////////////////////////////////////////////
/// FormData
////////////////////////////////////////////////////////////////////////////////

FormData::FormData(const HttpRequest& request)
	: _content(""), _boundary(""), _isValid(false)
{
	_content.clear();
	_parse(request.getBody(), request.getContentType());
}

FormData::~FormData()
{
}

////////////////////////////////////////////////////////////////////////////////

void	FormData::_parse(std::string body, const std::string& contentType)
{
	if (body.empty() || contentType.empty())
		return ;
	if (!_extractBoundary(contentType))
		return ;
	_isValid = _parseRequest(body, _boundary);

}

bool FormData::_parseRequest(const std::string& body, const std::string& boundary)
{
	std::string		requestLine;
	FormData::Parts parts;
	if (!_checkBoundary(body, boundary))
		return (false);
	requestLine = _extractReqeustLine(body, boundary);
	if (requestLine.empty())
		return (false);
	parts = _splitToParts(requestLine);

	return (_getFormData(parts));
}

FormData::Parts	FormData::_splitToParts(const std::string& requestLine)
{
	std::istringstream	iss(requestLine);
    FormData::Parts		parts;
    std::string			line;

    if (std::getline(iss, line) && line.find("Content-Disposition") != std::string::npos)
        parts.disposition = line;
	else
		return (parts);
	while (std::getline(iss, line) && line != "\r")
		parts.headerLine += line + "\n";
	while (std::getline(iss, line))
		parts.body += line + "\n";
    return (parts);
}

bool FormData::_getFormData(FormData::Parts& parts)
{
	if (parts.disposition.empty() || parts.headerLine.empty() || parts.body.empty())
		return (false);
	if (!_parseContentDisposition(parts.disposition))
		return (false);
	if (!_parseHeaders(parts.headerLine))
		return (false);
	_content = parts.body;
	return (true);
}

bool FormData::_parseHeaders(const std::string& line)
{
    std::map<std::string, std::string> headers;
    std::istringstream iss(line);
    std::string headerLine;

    while (std::getline(iss, headerLine))
    {
        std::string::size_type colonPos = headerLine.find(':');
		std::string key		= trim(headerLine.substr(0, colonPos));
        std::string value	= trim(headerLine.substr(colonPos + 1));
        if (!key.empty() && !value.empty())
            headers[key] = value;
    }
    if (headers.empty() || headers.find("Content-Type") == headers.end())
        return (false);
    _headers = headers;
    return (true);
}

bool FormData::_parseContentDisposition(const std::string& line)
{
    std::map<std::string, std::string> disposition;
    std::istringstream iss(line);
    std::string token;

    std::getline(iss, token, ';');
    while (std::getline(iss, token, ';'))
    {
        token = trim(token);
        std::string::size_type equalPos = token.find('=');
        if (equalPos != std::string::npos)
        {
            std::string key = trim(token.substr(0, equalPos));
            std::string value = trim(token.substr(equalPos + 1));
            if (!value.empty() && value[0] == '"' && value[value.size() - 1] == '"')
                value = value.substr(1, value.size() - 2);
            disposition[key] = value;
        }
    }
    if (disposition.find("name") == disposition.end())
        return false;
    _disposition = disposition;
    return true;
}

////////////////////////////////////////////////////////////////////////////////

bool FormData::_extractBoundary(const std::string& contentType)
{
	if (!_hasBoundary(contentType))
		return (false);
	_boundary = _trimBoundary(_getBoundaryString(contentType));
	return (!_boundary.empty());
}

bool	FormData::_hasBoundary(const std::string& contentType)
{
	if (contentType.empty())
		return (false);
	return (contentType.find("boundary=") != std::string::npos);
}

std::string	FormData::_getBoundaryString(const std::string& contentType)
{
	const std::string		boundaryPrefix = "boundary=";
	std::string::size_type	pos = contentType.find(boundaryPrefix);
	return (pos == std::string::npos ? "" : contentType.substr(pos + boundaryPrefix.length()));
}

std::string FormData::_trimBoundary(const std::string& boundary)
{
	std::string::size_type endPos = boundary.find("\r\n");
	return (endPos != std::string::npos ? boundary.substr(0, endPos) : boundary);
}

bool FormData::_checkBoundary(const std::string& body, const std::string& boundary)
{
	std::string fullBoundary = "--" + boundary;
	return body.find(fullBoundary) == 0;
}

std::string FormData::_extractReqeustLine(const std::string& requestBody, const std::string& boundary)
{
	std::string fullBoundary = "--" + boundary;
	std::string::size_type pos = requestBody.find("\r\n", fullBoundary.size()) + 2;
	std::string::size_type nextPos = requestBody.find(fullBoundary, pos);

	if (!_hasOneData(pos, nextPos))
		return ("");
	return (requestBody.substr(pos, nextPos - pos));
}

bool FormData::_isEndBoundary(const std::string& body, const std::string& boundary)
{
	std::string fullBoundary	= "--" + boundary;
	std::string endBoundary		= fullBoundary + "--";
	std::string::size_type pos = body.find(endBoundary, body.find(fullBoundary) + fullBoundary.size());
	return (pos != std::string::npos);
}

bool	FormData::_hasOneData(std::string::size_type pos, std::string::size_type nextPos)
{
	return (pos != std::string::npos && nextPos != std::string::npos);
}

std::string FormData::clearFileName(const std::string& fileName)
{
	char replacement = REPLACEMENT_FILENAME_CHAR;
	std::string cleanedFileName = fileName;
	if (fileName.empty())
		return ("");
	for (std::string::size_type i = 0; i < cleanedFileName.size(); ++i)
	{
		if (std::isspace(cleanedFileName[i]))
			cleanedFileName[i] = replacement;
	}
	return (cleanedFileName);
}

////////////////////////////////////////////////////////////////////////////////

const std::map<std::string, std::string>& FormData::getDisposition() const
{
	return (_disposition);
}

const std::map<std::string, std::string>& FormData::getHeaders() const
{
	return (_headers);
}

const std::string& FormData::getContent() const
{
	return (_content);
}

const std::string& FormData::getBoundary() const
{
	return (_boundary);
}

bool FormData::isValid() const
{
	return (_isValid);
}

std::string FormData::getFilename() const
{
	std::map<std::string, std::string>::const_iterator it = \
					 _disposition.find("filename");
	return ((it != _disposition.end()) ? it->second : "");
}

std::string FormData::getContentType() const
{
	std::map<std::string, std::string>::const_iterator it = \
					 _headers.find("Content-Type");
	return ((it != _headers.end()) ? it->second : "");
}



std::ostream& operator<<(std::ostream& os, const FormData& formData) {
    // _disposition 출력
    os << "Disposition:" << std::endl;
    for (std::map<std::string, std::string>::const_iterator it = formData.getDisposition().begin();
         it != formData.getDisposition().end(); ++it) {
        os << "  " << it->first << ": " << it->second << std::endl;
    }

    // _headers 출력
    os << "Headers:" << std::endl;
    for (std::map<std::string, std::string>::const_iterator it = formData.getHeaders().begin();
         it != formData.getHeaders().end(); ++it) {
        os << "  " << it->first << ": " << it->second << std::endl;
    }

    // _content, _boundary, _isValid 출력
    os << "Content: " << formData.getContent() << std::endl;
    os << "Boundary: " << formData.getBoundary() << std::endl;
    os << "Is Valid: " << (formData.isValid() ? "true" : "false") << std::endl;

    return os;
}