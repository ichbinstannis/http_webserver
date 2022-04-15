#pragma once

#include "webserv.hpp"
#include "Config.hpp"
#include "RequestParser.hpp"
#include "Response.hpp"
#include "Logger.hpp"

extern pthread_mutex_t g_write;
class ServerConfig;
class RequestParser;
class Response;
class Logger;

typedef struct s_reqData {
	std::string		reqString;
	size_t			reqLength;
	Response		*response;
	char			*responseStr;
	size_t			responseSize;
	bool			isTransfer;
	bool			isMultipart;
	bool			foundHeaders;
	std::string		method;
	std::string		bound;
	std::string		finalBound;
	size_t			chunkInd;
}	t_reqData;

class Server {
private:
	int 						_listenSocket;
	int							_timeout;
	sockaddr_in					_servAddr;
	std::vector<struct pollfd>	_fds;
	std::map<long, t_reqData>	_clients;
	std::set<int>				_fdToDel;
	std::stringstream 			_message;
	Server(const Server &other);
	void						isChunked(std::string headers, s_reqData *req);
	bool						findReqEnd(t_reqData &req);
	void						pollError(pollfd &pfd);
	void						clearConnections();
	Server						&operator = (const Server &other);

public:
	class ServerConfig			webConfig;
	int							serverID;
	pthread_t					tid;
	Server();
	~Server();
	void						setTimeout(int timeout);
	int							getListenSocket(void) const;
	int							getTimeout(void) const;
	void						initiate(const char *ipAddr, int port);
	void						acceptConnection(void);
	void						closeConnection(int socket);
	void						receiveRequest(pollfd &pfd);
	void						sendResponse(pollfd &pfd);
	void						runServer(int timeout);
	void						initReqDataStruct(int clientFD);
	bool						endByTimeout(t_reqData &req);
};

char	*getCstring(const std::string &cppString);
