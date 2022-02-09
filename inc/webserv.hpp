#pragma once

#include <cassert>
#include <cerrno>
#include <cstring>
#include <iostream> // For cout
#include <cstdlib> // For exit() and EXIT_FAILURE

#include <sys/socket.h> // For socket functions
#include <netinet/in.h> // For sockaddr_in
#include <unistd.h> // For read
#include <arpa/inet.h> // For inet_addr("127.0.0.1")
#include <fcntl.h>// for open
#include <sys/stat.h> // for struct stat
#include <vector>
#include <errno.h>
#include <stdio.h> // for perror
#include <signal.h>
#include <poll.h>

#include "utils.hpp"
#include "ErrorHandle.hpp"

# define RESET "\033[0m" // sets color to default
# define RED "\033[31m"
# define GREEN "\033[32m"
# define YELLOW "\033[33m"

# define PORT 8888
# define BACKLOG 100 // the maximum number of connections that will be queued
# define BUFFER_SIZE 10240
# define AMMOUNT_FDS 101