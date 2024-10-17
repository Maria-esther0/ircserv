#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <vector>
#include <map>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <iostream>
#include <stdlib.h>
#include <stdio.h>

//colors
#define RED "\033[0;31m"
#define WHITE "\033[0;37m"
#define GREEN "\033[0;32m"
#define YELLOW "\033[0;33m"

class Server {
private:
	int server_fd; // Descripteur de fichier pour le serveur
	int port; // Port sur lequel le serveur écoute
	struct sockaddr_in address; // Adresse du serveur
	std::vector<int> clients; // Liste des descripteurs de fichiers clients

public:
	Server(int port);
	~Server();
	void start(); // Méthode pour démarrer le serveur
	void acceptClients(); // Accepter les connexions clients
	void handleClient(int client_fd); // Gérer la communication avec un client

	static int get_port(char *ag); // Récupérer le port à partir des arguments

	// commandes specifiques aux operateurs de canaux
// 	void kick(int client_fd, const std::string& command); // expulser un utilisateur d'un canal
// 	void invite(int client_fd, const std::string& command); // inviter un utilisateur dans un canal
// 	void topic(int client_fd, const std::string& command); // changer le sujet d'un canal
// 	void mode(int client_fd, const std::string& command); // changer le mode d'un canal
};

#endif // SERVER_HPP
