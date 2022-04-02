#include "Server.hpp"

Server::Server() {
	memset(&_servAddr, 0, sizeof(_servAddr));
	// memset(_fds, 0, sizeof(_fds));
}

Server::~Server() { //todo rework cleaning
	pthread_mutex_lock(&g_write);
	for (size_t i = 0; i < _fds.size(); i++) {
		if (_fds[i].fd >= 0)
		{
			close(_fds[i].fd);
			std::cerr << BgMAGENTA << "Web server [" << this->serverID << "]: connection closed on socket "
						<< _fds[i].fd << " (D)"  << std::endl;
			Logger::printCriticalMessage(&_message);
		}
	}
	std::cerr << "Server " << this->serverID << " is down" << RESET << std::endl;
	pthread_mutex_unlock(&g_write);
}

void	Server::setTimeout(int timeout) {
	this->_timeout = timeout;
}

int		Server::getListenSocket(void) const {
	return (this->_listenSocket);
}

int		Server::getTimeout(void) const {
	return (this->_timeout);
}

// int		Server::getNumberFds(void) const {
// 	return (this->_numberFds);
// }

void	Server::initiate(const char *ipAddr, int port) {
	this->_listenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (_listenSocket < 0) {
		_message << "socket() failed" << " on server " << this->serverID;
		Logger::printCriticalMessage(&_message);
		exit(-1);
	}
	int optval = 1;
	int ret = setsockopt(this->_listenSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&optval, sizeof(optval));
	if (ret < 0) {
		_message << "setsockopt() failed" << " on server " << this->serverID;
		Logger::printCriticalMessage(&_message);
		close(this->_listenSocket);
		exit(-1);
	}
	ret = fcntl(this->_listenSocket, F_SETFL, O_NONBLOCK);
	if (ret < 0) {
		_message << "fcntl() failed" << " on server " << this->serverID;
		Logger::printCriticalMessage(&_message);
		close(this->_listenSocket);
		exit(-1);
	}
	this->_servAddr.sin_family = AF_INET;
	this->_servAddr.sin_addr.s_addr = inet_addr(ipAddr);
	this->_servAddr.sin_port = htons(port);
	ret = bind(this->_listenSocket, (struct sockaddr *)&this->_servAddr, sizeof(this->_servAddr));
	if (ret < 0) {
		_message << "bind() failed" << " on server " << this->serverID;
		Logger::printCriticalMessage(&_message);
		close(this->_listenSocket);
		exit(-1);
	}
	ret = listen(this->_listenSocket, BACKLOG);
	if (ret < 0) {
		_message << "listen() failed" << " on server " << this->serverID;
		Logger::printCriticalMessage(&_message);
		close(this->_listenSocket);
		exit(-1);
	}
	return ;
}

void	Server::initiate() {
	const char *ipAddr = webConfig.getHost().c_str();
	int	port = webConfig.getPort();
	this->_listenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (_listenSocket < 0) {
		_message << "socket() failed" << " on server " << this->serverID;
		Logger::printCriticalMessage(&_message);
		exit(-1);
	}
	int optval = 1;
	int ret = setsockopt(this->_listenSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&optval, sizeof(optval));
	if (ret < 0) {
		_message << "setsockopt() failed" << " on server " << this->serverID;
		Logger::printCriticalMessage(&_message);
		close(this->_listenSocket);
		exit(-1);
	}
	ret = fcntl(this->_listenSocket, F_SETFL, O_NONBLOCK);
	if (ret < 0) {
		_message << "fcntl() failed" << " on server " << this->serverID;
		Logger::printCriticalMessage(&_message);
		close(this->_listenSocket);
		exit(-1);
	}
	this->_servAddr.sin_family = AF_INET;
	this->_servAddr.sin_addr.s_addr = inet_addr(ipAddr);
	this->_servAddr.sin_port = htons(port);
	ret = bind(this->_listenSocket, (struct sockaddr *)&this->_servAddr, sizeof(this->_servAddr));
	if (ret < 0) {
		_message << "bind() failed" << " on server " << this->serverID;
		Logger::printCriticalMessage(&_message);
		close(this->_listenSocket);
		exit(-1);
	}
	ret = listen(this->_listenSocket, BACKLOG);
	if (ret < 0) {
		_message << "listen() failed" << " on server " << this->serverID;
		Logger::printCriticalMessage(&_message);
		close(this->_listenSocket);
		exit(-1);
	}
	return ;
}

void	Server::initReqDataStruct(int clientFD) {
	t_reqData req;

	req.reqString = "";
	req.reqLength = 0;
	req.responseStr = NULL;
	req.responseSize = 0;
	req.isTransfer = false;
	req.isMultipart = false;
	req.foundHeaders = false;
	req.method = "";	
	req.chunkInd = 0;
	req.lastTime = clock();
	req.response = NULL;
	_clients[clientFD] = req;
}

void	Server::runServer(int timeout) {
	struct pollfd	*fdsBeginPointer;
	pollfd			new_Pollfd = {_listenSocket, POLLIN, 0};

	_fds.push_back(new_Pollfd);
	this->setTimeout(timeout);
	while (true) {
		if (DEBUG > 1) {
			_message <<"Waiting on poll() [server " << this->serverID << "]...\n";
			Logger::printDebugMessage(&_message);
		}
		fdsBeginPointer = &_fds[0];
		int ret = poll(fdsBeginPointer, _fds.size(), _timeout);
		if (ret < 0) {
			_message << "poll() failed" << " on server " << this->serverID;
			Logger::printCriticalMessage(&_message);
			break;
		}
		if (ret == 0) {
			_message << "poll() timed out. End program." << " on server " << this->serverID; ;
			Logger::printCriticalMessage(&_message);
			break;
		}
		for (size_t i = 0; i < _fds.size(); i++) {
			if (_fds[i].revents == 0)
				continue;
			if (_fds[i].revents) {
				if (_fds[i].revents & POLLIN && _fds[i].fd == _listenSocket)
					acceptConnection();
				else if (_fds[i].revents & POLLIN && _fds[i].fd != _listenSocket)
					receiveRequest(i);
				else if (_fds[i].revents & POLLOUT && _fds[i].fd != _listenSocket)
					sendResponse(i);
				else
					pollError(i);
			}
		}
		endByTimeout();
		clearConnections();
		// usleep(1000); // todo включить если тестируете через curl большие файлы
	}
	return ;
}


void	Server::acceptConnection(void) {
	int newFd = accept(_listenSocket, NULL, NULL);
	if (newFd < 0)
		return ;
	int ret = fcntl(newFd, F_SETFL, O_NONBLOCK);
	if (ret < 0) {
		_message << "fcntl() failed" << " on server " << this->serverID;
		Logger::printCriticalMessage(&_message);
		close(_listenSocket);
		exit(-1);
	}
	if (DEBUG >= 0) {
		_message << "New incoming connection:\t" << newFd << " on server " << this->serverID;
		Logger::printCriticalMessage(&_message);
	}

	pollfd	newConnect = {newFd, POLLIN, 0};
	_fds.push_back(newConnect);
	initReqDataStruct(newFd);
	return ;
}

void	Server::clearConnections() {
	int fd;
	std::vector<struct pollfd>::iterator it;

	it = _fds.begin();

	for (size_t i = 0; i < _fds.size(); i++)
		if (_fdToDel.count(_fds[i].fd))
		{
			fd = _fds[i].fd;
			close(fd);
			if (DEBUG > 0) {
				_message << "Connection has been closed:\t" << fd << " on server " << this->serverID;
				Logger::printInfoMessage(&_message);
			}
			if (_clients[fd].response)
				delete _clients[fd].response;
			_fdToDel.erase(fd);
			_clients.erase(fd);
			_fds.erase(it + i);
			--i;
		}
}


void	Server::receiveRequest(int i) {
	int	fd = _fds[i].fd;
	if (DEBUG > 1) {
		_message << "Event detected on descriptor:\t" << fd << " on server " << this->serverID;
		Logger::printDebugMessage(&_message);
	}
	int ret = 0;
	char buffer[BUFFER_SIZE];
	ret = recv(fd, buffer, BUFFER_SIZE, 0);
	_clients[fd].lastTime = clock();
	
	if (ret > 0)
	{
		std::string tail = std::string(buffer, ret);
		_clients[fd].reqLength += ret;
		_clients[fd].reqString += tail;
		if (DEBUG > 0) {
			_message << _clients[fd].reqLength << " bytes received from sd:\t" << fd << " on server " << this->serverID;
			Logger::printCriticalMessage(&_message);
		}
		// memset(buffer, 0, BUFFER_SIZE);
		if (findReqEnd(_clients[fd]))
			_fds[i].events = POLLOUT;
	}
	if (ret == 0 || ret == -1) {
		
		_fdToDel.insert(fd);
		if (!ret and DEBUG > 1) {
			_message << "Request to close connection:\t" << fd << " on server " << this->serverID;
			Logger::printDebugMessage(&_message);;
		}
		else if (DEBUG > 1) {
			_message << "recv() failed" << " on server " << this->serverID;
			Logger::printDebugMessage(&_message);
		}
	}
	return ;
}

void	Server::sendResponse(int i) {
	int	fd = _fds[i].fd;

	if (!_clients[fd].response) {
		try {
			RequestParser request = RequestParser(_clients[fd].reqString, _clients[fd].reqLength);
			if (DEBUG > 0) {
				if (request.getBody().length() > 10000)
					request.showHeaders();
				else {
					_message << "|" << YELLOW  << request.getRequest() << RESET"|";
					Logger::printInfoMessage(&_message);
				}
			}
			_clients[fd].reqString = "";
			_clients[fd].reqLength = 0;
			_clients[fd].foundHeaders = 0;
			_clients[fd].chunkInd = 0;
			_clients[fd].response =  new Response(request, webConfig);
			// _clients[fd].cgi = 
		}
		catch (RequestParser::UnsupportedMethodException &e) {
			_message << e.what();
			Logger::printCriticalMessage(&_message);
			return ;
		}
	}
	Response *response = _clients[fd].response;
	const char *responseStr;
	size_t responseSize;
	size_t chunkInd;
	if (_clients[fd].responseSize) {
		// не получилось отправить в прошлый раз всю строку
		responseStr = _clients[fd].responseStr;
		responseSize = _clients[fd].responseSize;
	}
	else if (_clients[fd].response->getChunked()) {
		// отправляем следующий чанк
		chunkInd = _clients[fd].chunkInd;
		
		responseStr = &(response->getChunks()[chunkInd][0]);
		responseSize = response->getChunks()[chunkInd].size();
		if (DEBUG > 1) {
			_message << "send chunk " << chunkInd << " data:\n" << response->getChunks()[chunkInd].substr(0, 130);
			Logger::printInfoMessage(&_message);
		}
			// usleep(50000);
		_clients[fd].chunkInd = ++chunkInd;
	}
	else {
		// отправляем весь ответ целиком
		responseStr = &response->getResponse()[0];
		responseSize = response->getResponse().size();
	}
	if (DEBUG > 1) {
		_message << CYAN << _clients[fd].response->getResponseCode() << RESET" with size="  << responseSize;
		Logger::printInfoMessage(&_message);
	}
	int ret = send(fd, responseStr, responseSize, 0);
	_clients[fd].lastTime = clock();
	_clients[fd].responseStr = (char *)responseStr + ret;
	_clients[fd].responseSize = responseSize - ret;
	// free (responseStr);
	if (ret < 0) {
		_message << "send() failed " << " on server " << this->serverID;
		Logger::printCriticalMessage(&_message);
		_fdToDel.insert(fd);
		return ;
	}
	if (!response->getChunked() or chunkInd == response->getChunks().size()) {
		delete _clients[fd].response;
		_clients[fd].response = NULL;
		_fds[i].events = POLLIN;
	}
	return ;
}


void Server::pollError(int i) {
	int	fd = _fds[i].fd;
	if (DEBUG > 0) {
		_message << "Error in fd = " << fd << RED ;
		if (_fds[i].revents & POLLNVAL)
			_message << " POLLNVAL";
		else if (_fds[i].revents & POLLHUP)
			_message << " POLLHUP";
		else if (_fds[i].revents & POLLERR)
			_message << " POLLERR";
		Logger::printInfoMessage(&_message);
	}
	_fdToDel.insert(fd);
}

void Server::isChunked(std::string headers, s_reqData *req) {
	size_t startPos = headers.find("Transfer-Encoding:");
	if (startPos != std::string::npos) {
		size_t endLine = headers.find("\n", startPos);
		std::string transferEncodingLine = headers.substr(startPos, endLine - startPos);
		req->isTransfer =  (transferEncodingLine.find("chunked") != std::string::npos);
	}
	startPos = headers.find("Content-Type:");
	if (startPos != std::string::npos) {
		size_t endLine = headers.find("\n", startPos);
		std::string typeLine = headers.substr(startPos, endLine - startPos);
		if (typeLine.find("multipart/form-data;") != std::string::npos) {
			req->isMultipart = true;
			size_t boundaryStart = typeLine.find("boundary=") + 9;
			size_t boundaryEnd = typeLine.length();
			req->bound = typeLine.substr(boundaryStart, boundaryEnd - boundaryStart - 1);
			req->finalBound = req->bound + "--";
		}
		// std::cout << BgBLUE << req->bound << RESET << std::endl;
	}
}

void Server::endByTimeout(void) {
	// void	Server::clearConnections() {
	int fd;
	size_t	now = clock();
	for (size_t i = 1; i < _fds.size(); i++) {
		fd = _fds[i].fd;
		if ((now - _clients[fd].lastTime) / CLOCKS_PER_SEC > KEEP_ALIVE) {
			if (DEBUG >= 0) {
				_message << "fd " << fd << " closed by timeout " << (now - _clients[fd].lastTime)/CLOCKS_PER_SEC;
				Logger::printCriticalMessage(&_message);
			}
			_fdToDel.insert(fd);
		}
	}
}

bool Server::findReqEnd(t_reqData &req) {
	size_t	pos;
	if (!req.foundHeaders) {
		size_t headersEnd = req.reqString.find(ENDH);
		if (headersEnd == std::string::npos) // todo заголовок еще не пришел до конца
			return false;
		req.foundHeaders = true;
		req.method = req.reqString.substr(0, req.reqString.find_first_of(' '));
		isChunked(req.reqString.substr(0, headersEnd), &req);
	}
	if (!req.isTransfer && !req.isMultipart)
		return true;
	pos = std::max(0, (int)req.reqString.size() - 10);
	if (req.isTransfer and req.reqString.find("0\r\n\r\n", pos) != std::string::npos)
		return true;
	if (req.isTransfer and req.method != "POST" and req.method != "PUT" and req.reqString.find(ENDH, pos) != std::string::npos)
		return true;
	if (req.isMultipart && req.reqString.find(req.finalBound) != std::string::npos)
		return true;
	return false;
}