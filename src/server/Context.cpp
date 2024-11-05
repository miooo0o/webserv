#include "Context.hpp"

////////////////////////////////////////////////////////////////////////////////
/// Constructor & Destructor
////////////////////////////////////////////////////////////////////////////////

Context::Context(ServerConfig& config, HttpRequest& request)
	: _serverConfig(config), _request(request), _location(_findLocation(*this))
{
}

Context::Context(const Context& other)
	: _serverConfig(other._serverConfig), _request(other._request), _location(other._location)
{
}

Context& Context::operator=(const Context& other)
{
	if (this != &other)
	{
		_serverConfig = other._serverConfig;
		_location = other._location;
		_request = other._request;
	}
	return (*this);
}

Context::~Context()
{
}

////////////////////////////////////////////////////////////////////////////////
/// Public Methods: Getters
////////////////////////////////////////////////////////////////////////////////

const ServerConfig&	Context::getServer() const
{
	return (_serverConfig);
}

const Location&	Context::getLocation() const
{
	return (_location);
}

const HttpRequest&	Context::getRequest() const
{
	return (_request);
}

////////////////////////////////////////////////////////////////////////////////
/// Public Methods: setters
////////////////////////////////////////////////////////////////////////////////

void	Context::setRequest(HttpRequest& request)
{
	_request = request;
}

void	Context::setServer(ServerConfig& config)
{
	_serverConfig = config;
}

void	Context::setLocation(Location& location)
{
	_location = location;
}


////////////////////////////////////////////////////////////////////////////////
/// Private Methods
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/// Prototype to test
// TODO: Remove
void	test_different_path(const std::map<std::string, Location*>& locations);
////////////////////////////////////////////////////////////////////////////////


/// @brief finds the location object that matches the request URI.
/// @param context 
/// @return if the location is found, returns the location object.
///			Otherwise, returns the default location object.
///
/// @example
/// if location has = "/", "/path/", "/path/to/"
///	and	uri = 
///		- "/"					=> returns the default root location object.
///		- "/path"				=> returns the location object for "/path/".
///		- "/path/"				=> returns the location object for "/path/".
///		- "/path//////to/dir/"	=> returns the location object for "/path/to/".
///		- "/path/to//////////// => returns the location object for "/path/to/".
Location&	Context::_findLocation(Context& context) const
{
	const std::string& 					uri = context.getRequest().getUri();
	std::map<std::string, Location*>	locations = _serverConfig.map_locationObjs;


	// TODO: Remove Test function 
	// test_different_path(locations);

	std::string	matchedLocation = getMatchedLocation(uri, locations);
	if (!matchedLocation.empty() && locations.find(matchedLocation) != locations.end())
		return (*locations.at(matchedLocation));
	if (locations.find("/") == locations.end())
		throw std::runtime_error("No matching location found and root location is not defined.");
	return (*locations.at("/"));
}

/// @brief Finds the longest location path that matches the request URI.
///
/// This function extracts the path from the URI and searches for the most specific location
/// that matches this path. It starts with the full path and progressively checks shorter
/// paths up to the root, using the provided locations map from the `Config` instance.
///
/// @param path The path extracted from the URI to match against the locations.
/// @param locations A map of location paths to `Location` objects from the `Config` instance.
/// @return The longest matching `location path`. If no match is found, returns an `empty string`.
std::string getMatchedLocation(const std::string& path, const std::map<std::string, Location*>& locations)
{
	std::string	matched;
	std::string parentPath;

	if (path.empty())
		throw std::invalid_argument("Path cannot be empty");
	if (locations.empty())
		throw std::invalid_argument("Locations map cannot be empty");
	std::string::size_type	lastSlashPos = path.find_last_of('/');
	if (lastSlashPos == std::string::npos)
		return ("");
	std::string normalisedPath = normalisePath(path);
	lastSlashPos = normalisedPath.find_last_of('/');
	if (lastSlashPos == 0)
		return ("/");
	while (!normalisedPath.empty())
	{
		std::map<std::string, Location*>::const_iterator it = locations.find(normalisedPath);
		if (it != locations.end())
		{
			matched = normalisedPath;
			break;
		}
		normalisedPath = getParentPath(normalisedPath);
		if (normalisedPath == "/")
			break;
	}
	return (matched);
}

/// @brief Extracts the parent path from a given path.
/// @param path 
/// @return return the parent path of the given path.
std::string	getParentPath(const std::string& path)
{
	if (path == "/")
		return path;

	std::string trimmedPath = path;

	if (trimmedPath.length() > 1 && trimmedPath[trimmedPath.length() - 1] == '/')
		trimmedPath.erase(trimmedPath.length() - 1);
	// std::cout << "trimmedPath: " << trimmedPath << std::endl;
	
	std::string::size_type lastSlashPos = trimmedPath.find_last_of('/');
	if (lastSlashPos == std::string::npos)
		return ("");
	if (lastSlashPos == 0)
		return ("/");
	return (trimmedPath.substr(0, lastSlashPos + 1));
}

std::string normalisePath(const std::string& path)
{
	std::string normalisedPath = path;

	while (normalisedPath.find("//") != std::string::npos)
		normalisedPath.replace(normalisedPath.find("//"), 2, "/");
	if (normalisedPath.length() > 1 && normalisedPath[normalisedPath.length() - 1] != '/')
		normalisedPath += "/";
	return (normalisedPath);
}

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/// TODO: Remove 
/// Test Functions for the Context class 
////////////////////////////////////////////////////////////////////////////////
void test_path_unit(const std::string& path, const std::map<std::string, Location*>& locations)
{
	std::cout << "----------------------------------------------------------------------" << std::endl;
	std::cout << "TEST | path: " << path << std::endl;
	std::string matchedLocation = getMatchedLocation(path, locations);
	std::cout << "matchedLocation: " << matchedLocation << std::endl;
}

void test_different_path(const std::map<std::string, Location*>& locations)
{
	test_path_unit("/", locations);
	test_path_unit("///////////////", locations);
	test_path_unit("/image", locations);
	test_path_unit("/image/", locations);
	test_path_unit("/images", locations);
	test_path_unit("/images/", locations);
	test_path_unit("/images//////////////////", locations);
	test_path_unit("/images/not_exist/", locations);
	test_path_unit("/images/not_exist/////////////////////also_not_exist/", locations);
	test_path_unit("/images/test_dir/", locations);
	test_path_unit("/images/test_dir////////////////////", locations);
	test_path_unit("/images///////////////test_dir////////////////////", locations);
	test_path_unit("/images/test_dir/ho/", locations);
	std::cout << "--------------test unit done------------------------------------------" << std::endl << std::endl << std::endl;

}