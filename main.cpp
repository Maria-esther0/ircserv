#include <csignal>
#include <iostream>
#include <cstdlib>
#include "server.hpp"

// Gestionnaire de signal pour une fermeture propre du serveur
void signalHandler(int signal) {
	std::cout << "Signal reçu : " << signal << ". Fermeture du serveur..." << std::endl;
	exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
	// Configuration des gestionnaires de signaux
	signal(SIGINT, signalHandler); // Intercepter Ctrl+C
	signal(SIGPIPE, SIG_IGN);      // Ignorer les erreurs de pipe brisé

	if (argc != 3) {
		std::cerr << "Usage: ./ircserv <port> <password>" << std::endl;
		return 1;
	}

	int port = atoi(argv[1]);
	std::string password = argv[2];

	Server ircServer(port, password);
	ircServer.start();

	return 0;
}
