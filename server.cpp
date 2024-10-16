#include "server.hpp"
#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <poll.h>

Server::Server(int port) : port(port) {
	// Crée le socket du serveur
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == 0) {
		std::cerr << "Erreur: Création du socket échouée" << std::endl;
		_exit(1);
	}

	// Configuration de l'adresse du serveur
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(port);

	// Associe le socket à l'adresse et au port
	if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
		std::cerr << "Erreur: Bind échoué" << std::endl;
		close(server_fd);
		_exit(1);
	}

	// Le serveur écoute les connexions entrantes
	if (listen(server_fd, 5) < 0) {
		std::cerr << "Erreur: Listen échoué" << std::endl;
		close(server_fd);
		_exit(1);
	}
}

Server::~Server() {
	close(server_fd);
	for (size_t i = 0; i < clients.size(); ++i) {
		close(clients[i]);
	}
}

void Server::start() {
	std::cout << "Le serveur est en écoute sur le port " << port << std::endl;
	acceptClients();
}

void Server::acceptClients() {
	while (true) {
		struct sockaddr_in client_address;
		socklen_t client_len = sizeof(client_address);

		int new_client = accept(server_fd, (struct sockaddr*)&client_address, &client_len);
		if (new_client < 0) {
			std::cerr << "Erreur: Accept échoué" << std::endl;
			continue;
		}

		std::cout << "Nouveau client connecté!" << std::endl;
		clients.push_back(new_client);

		// Démarrer la gestion de ce client dans un thread séparé ou via poll()
		handleClient(new_client);
	}
}

void Server::handleClient(int client_fd) {
	char buffer[1024] = {0};
	int valread = read(client_fd, buffer, 1024);
	if (valread > 0) {
		std::cout << "Message reçu: " << buffer << std::endl;
		send(client_fd, "Bienvenue sur le serveur IRC!\n", strlen("Bienvenue sur le serveur IRC!\n"), 0);
	}
	close(client_fd);
}
