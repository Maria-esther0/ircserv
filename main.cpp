#include "server.hpp"


int main(int argc, char *argv[]) {
	std::cout << "Hello from ircserv!\n";

	// Vérification du nombre d'arguments
	if (argc != 3) {
		std::cerr << "Usage: " << argv[0] << " <port> <password>\n";
		return 1;
	}

	// Validation du port
	if (Server::validateAndParsePort(argv[1]) == -1) {
		std::cerr << "Invalid port!\n";
		return 1;
	}

	Server server;

	// Initialisation du serveur
	server.startServer(argv[1], argv[2]);
	std::cout << "The Server Closed!" << std::endl;

	return 0;
}
