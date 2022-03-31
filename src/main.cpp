#include "webserv.hpp"
#include "Server.hpp"
#include "Config.hpp"

bool g_status = false;
pthread_mutex_t g_write;
std::stringstream message;

static void	interruptHandler(int sig_int) {
	(void)sig_int;
	g_status = true;
	message << BgMAGENTA << "\nAttention! Interruption signal caught.\n";
	Logger::printCriticalMessage(&message);
}

static void	*routine(void *webserv) {
	message << "Run server[" << reinterpret_cast<Server *>(webserv)->serverID << "]\n" << reinterpret_cast<Server *>(webserv)->webConfig;
	Logger::printDebugMessage(&message);
	reinterpret_cast<Server *>(webserv)->initiate(reinterpret_cast<Server *>(webserv)->webConfig.getHost().c_str(), reinterpret_cast<Server *>(webserv)->webConfig.getPort()); // когда будет Config, метод сменится на .initiate(void)
	reinterpret_cast<Server *>(webserv)->runServer(-1);
	return (NULL);
}

int main(int argc, char *argv[]) {
	message << "C++ version is " << __cplusplus << std::endl << std::endl;
	Logger::printCriticalMessage(&message);
	signal(SIGINT, interruptHandler);
	Config config(argc, argv);
	std::vector<ServerConfig> servers = config.getServers();
	Server webserv[servers.size()];
	for (size_t i = 0; i < servers.size(); i++) {
		webserv[i].webConfig = servers[i];
		webserv[i].serverID = i;
		if (pthread_create(&webserv[i].tid, NULL, &routine, &webserv[i]) != 0)
			exit (EXIT_FAILURE);
		pthread_detach(webserv[i].tid);
	}
	while(!g_status) {
		if (g_status) {
			message << BgMAGENTA << "Closing connections... ";
			Logger::printCriticalMessage(&message);
		}
	}
	pthread_mutex_destroy(&g_write);
	return 0;
}
