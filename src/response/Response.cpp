/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ymanfryd <ymanfryd@student.21-school.ru    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/02/20 19:06:46 by ymanfryd          #+#    #+#             */
/*   Updated: 2022/03/22 12:11:23 by ymanfryd         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Response.hpp"

Response::Response(RequestParser &request, ServerConfig &config) :
        RequestParser_(request),
        ServerConfig_(config),
        requestRoute_(request.getRoute()),
        requestMethod_(request.getMethod()),
        requestBody_(request.getBody()),
        responseCode_(0),
        ClientMaxBodySize_(config.getClientMaxBodySize()),
        contentLength_(0),
        requestPath_(request.getPath()),
        locations_(config.getLocations()),
        supportedMethods_(request.getSupportedMethods()),
        errorPages_(config.getErrorPages()),
        cgiRequested_(false) {
    setResponseCodes();
    createResponse();
}

Response::Response(const Response &other) {
    *this = other;
}

Response &Response::operator=(const Response &other) {
    if (this != &other) {
        RequestParser_          = other.RequestParser_;
        ServerConfig_           = other.ServerConfig_;
        Location_               = other.Location_;
        requestRoute_           = other.requestRoute_;
        requestPath_            = other.requestPath_;
        requestMethod_          = other.requestMethod_;
        requestedFile_          = other.requestedFile_;
        response_               = other.response_;
        responseCode_           = other.responseCode_;
        responseBody_           = other.responseBody_;
        responseHeaders_        = other.responseHeaders_;
        responseContentType_    = other.responseContentType_;
        responseCodes_          = other.responseCodes_;
        contentLength_          = other.contentLength_;
        supportedMethods_       = other.supportedMethods_;
        locations_              = other.locations_;
        locationMethods_        = other.locationMethods_;
        locationIndex_          = other.locationIndex_;
        locationRoot_           = other.locationRoot_;
        locationRedirection_    = other.locationRedirection_;
        errorPages_             = other.errorPages_;
        ClientMaxBodySize_      = other.ClientMaxBodySize_;
        requestBody_            = other.requestBody_;
        cgiRequested_           = other.cgiRequested_;
    }
    return *this;
}

/**************************/
/******** GETTERS *********/
/**************************/

const std::string &Response::getResponse() const {
    return this->response_;
}

const int &Response::getResponseCode() const {
    return this->responseCode_;
}

const size_t &Response::getContentLength() const {
    return this->contentLength_;
}

const std::string &Response::getResponseBody() const {
    return this->responseBody_;
}

const std::string &Response::getResponseHeaders() const {
    return this->responseHeaders_;
}

/**************************/
/******** SETTERS *********/
/**************************/

void Response::setResponse() {
    response_ = responseHeaders_ + "\n\r\n\r" + responseBody_;
}

void Response::setResponseCode(int code) {
    responseCode_ = code;
}

void Response::setContentType() {
    size_t routeEnd = requestPath_.size() - 1;
    size_t dotPos = requestPath_[routeEnd].find('.');
    if (dotPos != std::string::npos) {
        std::string extension = requestPath_[routeEnd].substr(dotPos);
        if (extension == ".css") {
            requestedFile_ = requestPath_[routeEnd];
            responseContentType_ = "text/css\n";
            trimRequestPath();
        } else if (extension == ".js") {
            requestedFile_ = requestPath_[routeEnd];
            responseContentType_ = "application/javascript\n";
            trimRequestPath();
        } else if (extension == ServerConfig_.getCgiExt())
            requestedFile_ = requestPath_[routeEnd];
         else
            responseContentType_ = "text/html\n";
    } else
        responseContentType_ = "text/html\n";
}

void Response::setResponseHeaders() {
    responseHeaders_ = "HTTP/1.1 ";
    responseHeaders_ += responseCodes_.find(responseCode_)->second;
    if (responseCode_ == 301)
        responseHeaders_ += "Location: " + locationRedirection_ + requestRoute_ + "\n";
    responseHeaders_ += "Content-Type: " + responseContentType_;
    responseHeaders_ += "Content-Length: ";
    responseHeaders_ += std::to_string(contentLength_);
}

void Response::setResponseBody(const std::string &body) {
    responseBody_ = body;
}

void Response::setContentLength(size_t len) {
    contentLength_ = len;
}

void Response::setLocationMethods(const std::set<std::string> &locationMethods) {
    for (size_t j = 0; j < supportedMethods_.size(); j++) {
        if (locationMethods.count(supportedMethods_[j])) {
            locationMethods_.insert(supportedMethods_[j]);
        }
    }
}

void Response::setLocationIndex(const std::set<std::string> &locationIndex) {
    std::set<std::string>::iterator it;
    for (it = locationIndex.begin(); it != locationIndex.end(); it++) {
        locationIndex_.push_back(it->data());
    }
}

void Response::setLocationRoot(const std::string &locationRoot) {
    locationRoot_ = locationRoot;
}

void Response::setLocationRedirection(const std::string& locationRedirection) {
    locationRedirection_ = locationRedirection;
}

void Response::setResponseCodes() {
    responseCodes_.insert(std::pair<int, std::string>(200, "200 OK\n"));
    responseCodes_.insert(std::pair<int, std::string>(201, "201 Created\n"));
    responseCodes_.insert(std::pair<int, std::string>(301, "301 Moved Permanently\n"));
    responseCodes_.insert(std::pair<int, std::string>(400, "400 Bad Request\n"));
    responseCodes_.insert(std::pair<int, std::string>(404, "404 Not Found\n"));
    responseCodes_.insert(std::pair<int, std::string>(405, "405 Method Not Allowed\n"));
    responseCodes_.insert(std::pair<int, std::string>(413, "413 Request Entity Too Large\n"));
    responseCodes_.insert(std::pair<int, std::string>(500, "500 Internal Server Error\n"));
    responseCodes_.insert(std::pair<int, std::string>(502, "502 Bad Gateway\n"));
    responseCodes_.insert(std::pair<int, std::string>(503, "503 Service Unavailable\n"));
}

/**************************/
/******** HELPERS *********/
/**************************/


void Response::createResponse() {
    std::string body;

    readLocationData();
    setContentType();

    if (!locationRedirection_.empty())
        setResponseCode(301);
    else if (checkPathForLocation() == -1)
        setResponseCode(404);
    else if (!locationMethods_.count(requestMethod_) && !cgiRequested_)
        setResponseCode(405);
    else if (requestBody_.length() > ClientMaxBodySize_ && !cgiRequested_)
        setResponseCode(413);
    else if (!checkContentLength())
        setResponseCode(400);
    else if (!cgiRequested_ && requestMethod_ != "PUT" && requestMethod_ != "POST")
        setResponseCode(200);
    else if (!cgiRequested_)
        setResponseCode(201);
    if (!cgiRequested_) {
        if (responseCode_ == 201)
            savePostBody();
        if (responseBody_.empty()) {
            body = readContent(getScreen());
            if (responseBody_.empty()) {
                setContentLength(body.size());
                setResponseHeaders();
                setResponseBody(body);
            } else {
                setContentLength(responseBody_.size());
                setResponseHeaders();
            }
        } else {
            setContentLength(responseBody_.size());
            setResponseHeaders();
        }
        setResponse();
    }
}

bool Response::checkContentLength() {
    size_t contentLength = RequestParser_.getContentLength();
    if (contentLength) {
        size_t bodyLength = RequestParser_.getBody().length();
//        std::cout << BgGREEN << contentLength << " | " << bodyLength << "|"<<  RequestParser_.getBody() << "|"<< RESET << std::endl;
        if (contentLength != bodyLength)
            return false;
    }
    return true;
}

void Response::checkFileRequested() {
    size_t routeEnd = requestPath_.size() - 1;
    size_t dotPos = requestPath_[routeEnd].find('.');
    if (dotPos != std::string::npos) {
        std::string extension = requestPath_[routeEnd].substr(dotPos);
        if (extension == ".css") {
            requestedFile_ = requestPath_[routeEnd];
            responseContentType_ = "text/css\n";
        } else if (extension == ".js") {
            requestedFile_ = requestPath_[routeEnd];
            responseContentType_ = "application/javascript\n";
        }
    }
}

std::string Response::handleChunkedBody() {
    std::string newBody;
    std::string restBody = requestBody_;
    size_t lineEnd;
    while (restBody.length()) {
        lineEnd = restBody.find("\n");
        if (lineEnd != std::string::npos) {
            std::string chunkSize = restBody.substr(0, lineEnd - 1);
            int decChunkSize = hexToDec(chunkSize);
            newBody += restBody.substr(lineEnd + 1, decChunkSize);
            restBody = restBody.substr(decChunkSize + lineEnd + 1);
        } else
            break;
    }

    return newBody;
}

void Response::savePostBody() {
    int filesCount = 0;
    struct dirent *d;
    DIR *dh = opendir(locationRoot_.c_str());
    while ((d = readdir(dh)) != NULL)
    { filesCount++; }
    std::string filesCountStr = numberToString(filesCount - 1);
    std::string filename = requestedFile_.empty() ? filesCountStr : requestedFile_.substr(0, requestedFile_.length() - 1);
    std::ofstream	postBodyFile(locationRoot_ + "/" + filename);
    std::string encoding = RequestParser_.getHeaders().find("Transfer-Encoding")->second;
    if (encoding == "chunked" && requestBody_.length())
        postBodyFile << handleChunkedBody();
    else
        postBodyFile << requestBody_;
    postBodyFile.close();
    closedir(dh);
}

std::string Response::findMaxPossibleLocation(const std::string &location) {
    std::string route = requestRoute_;

    for (size_t i = route.size(); i > 0; i--) {
        if (route == location)
            return route;
        route = route.substr(0, i);
    }
    return "";
}

void Response::readLocationData() {
    std::map<std::string, Location>::iterator iterator;
    std::string maxPossibleLocation;

    for (iterator = locations_.begin(); iterator != locations_.end(); iterator++) {
        if (!findMaxPossibleLocation(iterator->first).empty())
            maxPossibleLocation = findMaxPossibleLocation(iterator->first);
    }
    if (maxPossibleLocation.empty())
        maxPossibleLocation = "/";
    for (iterator = locations_.begin(); iterator != locations_.end(); iterator++) {
        if (iterator->first == maxPossibleLocation) {
            Location_ = iterator->second;
            setLocationRedirection(iterator->second.getRedirection());
            setLocationRoot(iterator->second.getAlias());
            setLocationMethods(iterator->second.getMethods());
            setLocationIndex(iterator->second.getIndex());
            int locationBodySize = iterator->second.getClientMaxBodySize();
            if (locationBodySize > 0)
                ClientMaxBodySize_ = locationBodySize;
        }
    }
    if (maxPossibleLocation != "/")
        requestRoute_ = requestRoute_.substr(maxPossibleLocation.length() - 1);
}


int Response::checkPathForLocation() {
    std::string stringFilename;
    if (locationRoot_[locationRoot_.size() - 1] == '/')
        stringFilename = locationRoot_ + requestRoute_.substr(1, requestRoute_.length() - 2);
    else
        stringFilename = locationRoot_ + "/" + requestRoute_.substr(1, requestRoute_.length() - 2);
    char *filename = const_cast<char *>(stringFilename.c_str());
    int openRes = open(filename, O_RDONLY);
    if (openRes == -1 && requestMethod_ != "PUT" && requestMethod_ != "POST")
        return -1;
    close(openRes);
    if (requestedFile_.find('.') != std::string::npos) {
        std::string extension = requestedFile_.substr(requestedFile_.find('.'));
        if (extension[extension.length() - 1] == '/')
            extension = extension.substr(0, extension.length() -1);
        if (extension == ServerConfig_.getCgiExt()) {
            cgiRequested_ = true;
            RequestParser_.setPathInfo(stringFilename);
            std::cout << BgRED << "CGI START" << RESET << std::endl;
            Cgi* cgi = new Cgi(ServerConfig_, Location_, RequestParser_);
            std::pair<int, std::string> cgiResult = cgi->execute();
            setResponseCode(cgiResult.first);
            response_ = cgiResult.second;
        } else if (requestMethod_ == "PUT" || requestMethod_ == "POST") {
            requestedFile_ = requestRoute_.substr(1);
            return 1;
        }
    } else if (requestMethod_ == "PUT" || requestMethod_ == "POST") {
        requestedFile_ = requestRoute_.substr(1);
        return 1;
    }
    std::string content = readContent(filename);
    if (!content.empty()) {
        setResponseBody(content);
        return 1;
    } else {
        if (Location_.getAutoindex()) {
            createAutoIndexPage(stringFilename.c_str());
            return 1;
        }
        std::string index;
        if (locationIndex_[0][0] != '.') {
            std::string file = stringFilename + locationIndex_[0];
            int fd = open(file.c_str(), O_RDONLY);
            if (fd == -1 && requestMethod_ != "PUT") {
                if (requestMethod_ == "PUT")
                   close(fd);
                return -1;
            }
            close(fd);
            index = readContent(stringFilename + "/" + locationIndex_[0]);
        }
        else
            index = findFileWithExtension(locationIndex_[0], stringFilename);
        setResponseBody(index);
        return 1;
    }
}

std::string Response::findFileWithExtension(std::string extension, std::string dir) {
    struct dirent *d;
    DIR *dh = opendir(dir.c_str());
    while ((d = readdir(dh)) != NULL)
    {
        std::string file = (std::string) d->d_name;
        size_t dotPos = file.find('.');
        if (dotPos != std::string::npos) {
            std::string fileExtension = file.substr(dotPos);
            if (fileExtension == extension) {
                closedir(dh);
                return readContent(dir + "/" + file);
            }

        }
    }
    closedir(dh);
    return "";
}

void    Response::createAutoIndexPage(const char *dir) {
    struct dirent *d;
    DIR *dh = opendir(dir);
    std::string strDir = (std::string)dir;
    std::string autoIndexPage = readContent("./src/screens/sample.html");
    autoIndexPage += "<body>\n <h1 class=\"autoIndexHeader\">";
    autoIndexPage += dir;
    autoIndexPage += "</h1>\n<div class=\"simpleContainer\">";
    strDir = strDir[strDir.size() - 1] == '/' ? strDir.substr(0, strDir.size() - 1) : strDir;
    while ((d = readdir(dh)) != NULL)
    {
        autoIndexPage += "<a class=\"autoIndexLink\"href=" + strDir + "/" + (std::string)d->d_name + ">";
        autoIndexPage +=  (std::string)d->d_name + "</a>\n";
    }
    autoIndexPage += "<div>\n</body>\n</html>\n";
    setResponseBody(autoIndexPage);
    closedir(dh);
}

std::string Response::readContent(const std::string &filename) {
    std::string buf;
    std::string content;
    std::ifstream inFile(filename);

    while (getline(inFile, buf))
        content += buf + "\n";
    inFile.close();
    return content;
}

std::string Response::getScreen() {
    std::string filename = locationRoot_;

    if (responseCode_ == 200 && !requestedFile_.length())
        filename += "/index.html";
    else if (responseCode_ == 200 && requestedFile_ == "style.css")
        filename = "./src/screens/style.css";
    else if (responseCode_ == 200 && requestedFile_ == "index.js")
        filename = "./src/screens/index.js";
    else if (errorPages_.find(responseCode_) != errorPages_.end())
        filename = "./errors/" + errorPages_[responseCode_];
    else if (responseCode_ != 200) {
        std::string errorDescription = responseCodes_.find(responseCode_)->second;
        setResponseBody(errorDescription);
        return errorDescription;
    }
    return filename;
}

void Response::trimRequestPath() {
    std::vector<std::string> newRequestPath;
    size_t routeEnd = requestPath_.size() - 1;
    if (routeEnd == 0) {
        newRequestPath.push_back("");
    }
    for (size_t i = 0; i < routeEnd; i++) {
        newRequestPath.push_back(requestPath_[i]);
    }
    requestPath_ = newRequestPath;
}
