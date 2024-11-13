#ifndef UTIL_HPP
# define UTIL_HPP

# include <string>
# include <vector>
# include <sstream>
# include <map>
# include <sys/types.h> 

# define REPLACEMENT_FILENAME_CHAR	'_'
# define WHITESPACE					" \t\r\n"

std::string		trim(const std::string& str);

std::string		toString(const int value);
std::string		toString(const size_t value);
std::string		toString(const ssize_t value);
std::string		toString(const std::vector<std::string>& values);

size_t			toSizeT(const std::string& value);


bool	isDir(const std::string path);
bool	createDir(const std::string& path);
bool	deleteDir(const std::string& path);

bool	isFile(const std::string path);
bool	createFile(const std::string& path);
bool	deleteFile(const std::string& path);

bool	hasWritePermission(const std::string& path);
bool	hasReadPermission(const std::string& path);
bool	hasExecutePermission(const std::string& path);
bool	deleteFileOrDir(const std::string& path);

class HttpRequest;
class FormData
{

struct Parts
{
	std::string	disposition;
	std::string headerLine;
	std::string body;
	Parts() : disposition(""), headerLine(""), body("") {}
};

public:
	FormData(const HttpRequest& request);
	~FormData();

    const std::map<std::string, std::string>&	getDisposition() const;
    const std::map<std::string, std::string>&	getHeaders() const;
    const std::string& 							getContent() const;
    const std::string&							getBoundary() const;
    bool										isValid() const;
    std::string									getFilename() const;
    std::string									getContentType() const;




	static std::string			clearFileName(const std::string& fileName);

private:
	std::map<std::string, std::string>	_disposition; // Content-Disposition 
    std::map<std::string, std::string>	_headers;     // Content-Type
    std::string							_content;     // body
	std::string							_boundary;    // boundary
	bool								_isValid;     // is valid

	void						_parse(std::string body, const std::string& contentType);
	bool						_parseRequest(const std::string& body, const std::string& boundary);
	FormData::Parts				_splitToParts(const std::string& requestLine);
	bool						_getFormData(FormData::Parts& parts);

	bool						_hasBoundary(const std::string& contentType);
	bool         		    	_extractBoundary(const std::string& contentType);
	bool						_hasOneData(std::string::size_type pos, std::string::size_type nextPos);
	std::string					_trimBoundary(const std::string& boundary);
	std::string					_getBoundaryString(const std::string& contentType);


	bool						_checkBoundary(const std::string& body, const std::string& boundary);
	bool						_isEndBoundary(const std::string& body, const std::string& boundary);
	std::string					_extractReqeustLine(const std::string& body, const std::string& boundary);

	bool						_parseHeaders(const std::string& line);
    bool						_parseContentDisposition(const std::string& line);

private: // Not used
	FormData();
};

std::ostream& operator<<(std::ostream& os, const FormData& formData);


#endif