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

//int main(int ac, char **av)
//{
//    if (ac != 3)
//    {
//        std::cerr<<RED<< "ERROR !"<<std::endl<<WHITE<<"Usage: " << av[0] << " <port> <password>\n";
//        _exit(1);
//    }

//    if (Server::get_port(av[1]) == -1)
//    {
//        std::cerr<<RED<<"ERROR !"<<std::endl<<WHITE<<"Invalid port!\n";
//        _exit(1);
//    }

//    int port = 6667; // Port standard pour IRC
//    Server ircServer(port);
//    try
//    {    ircServer.start(av[1], av[2]);}
//    catch(const std::exception& e)
//    {
//        ircServer.~Server();
//        std::cerr << e.what() << std::endl;
//        return 1;
//    }

//    return 0;
