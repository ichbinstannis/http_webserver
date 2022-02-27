#ifndef SERVERCONFIG_HPP
#define SERVERCONFIG_HPP

#include <vector>
#include <map>
#include <set>
#include <iostream>
#include "Location.hpp"

#define DEFAULT_ERROR_PAGE "error.html"
#define DEFAULT_PORT 8080

class ServerConfig {
private:
	std::string	host;
	std::string	serverName;
	int			port;
	std::string	errorPage;
	std::map<std::string, Location>	locations;
public:
	ServerConfig();

	const std::string &getHost() const;
	const std::string &getServerName() const;
	const int &getPort() const;
	const std::string &getErrorPage() const;
	const std::map<std::string, Location> &getLocations() const;
};

std::ostream& operator<< (std::ostream &out, const ServerConfig &sc);





#endif