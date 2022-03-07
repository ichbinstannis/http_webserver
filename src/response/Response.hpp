/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ymanfryd <marvin@42.fr>                    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/02/20 19:07:21 by ymanfryd          #+#    #+#             */
/*   Updated: 2022/02/20 19:07:27 by ymanfryd         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTP_WEBSERVER_RESPONSE_HPP
#define HTTP_WEBSERVER_RESPONSE_HPP

#include <map>
#include "../config/ServerConfig.hpp"
#include "../../inc/webserv.hpp"
#include "../request/RequestParser.hpp"
#include "../config/Location.hpp"

class RequestParser;
class ServerConfig;
class Location;

class Response {
    private:
            RequestParser                   request_;
            std::string                     response_;
            std::string                     requestRoute_;
            std::string                     requestMethod_;
            std::string                     responseHeaders_;
            std::string                     responseContentType_;
            std::string                     responseBody_;
            int                             responseCode_;
            size_t                          contentLength_;
            std::vector<std::string>        requestPath_;
            std::map<int, std::string>      responseCodes_;
            std::map<std::string, Location> locations_;
            std::set<std::string>           locationMethods_;
            std::vector<std::string>        supportedMethods_;
            std::vector<std::string>        locationIndex_;
            std::string                     locationRoot_;
            std::string                     requestedFile_;
            std::map<int, std::string>		errorPages_;

            void                            setResponse();
            void                            setResponseCodes();
            void                            setResponseHeaders();
            void                            setContentType();
            void                            setLocationMethods(const std::set<std::string>& locationMethods);
            void                            setLocationIndex(const std::set<std::string>& locationIndex);
            void                            setLocationRoot(const std::string& locationRoot);
            void                            setResponseCode(int code);
            void                            setContentLength(size_t len);
            void                            setResponseBody(const std::string& body);

            void                            createResponse();
            void                            readLocationData();
            void                            trimRequestPath();
            std::string                     findMaxPossibleLocation(const std::string& location);
            std::string                     getScreen();

            Response(): responseCode_(0), contentLength_(0) {};
    public:

            const std::string  &getResponse()          const;
            const std::string  &getResponseBody()      const;
            const std::string  &getResponseHeaders()   const;
            const size_t       &getContentLength()     const;
            const int          &getResponseCode()      const;

            Response(RequestParser &request, ServerConfig &config);
            Response(const Response &other);
            Response &operator=(const Response &other);
            ~Response() {}

            static std::string readContent(const std::string &filename);
};

#endif //HTTP_WEBSERVER_RESPONSE_HPP
