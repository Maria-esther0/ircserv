#include "server.hpp"
#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <poll.h>
#include <fcntl.h>
#include <set>
#include <sstream>

Server::Server(int port, const std::string &password) : port(port), serverPassword(password) {
	// Crée le socket du serveur
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == 0) {
		std::cerr << "Erreur: Création du socket échouée - " << strerror(errno) << std::endl;
		exit(EXIT_FAILURE);
	}

	// Configurer le socket serveur en mode non-bloquant
	setNonBlocking(server_fd);

	// Configuration de l'adresse du serveur
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(port);

	// Associe le socket à l'adresse et au port
	if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
		std::cerr << "Erreur: Bind échoué sur le port " << port << " - " << strerror(errno) << std::endl;
		close(server_fd);
		exit(EXIT_FAILURE);
	}

	// Le serveur écoute les connexions entrantes
	if (listen(server_fd, 5) < 0) {
		std::cerr << "Erreur: Listen échoué - " << strerror(errno) << std::endl;
		close(server_fd);
		exit(EXIT_FAILURE);
	}
}

bool Server::checkPassword(int client_fd, const std::string &password) {
	if (password == serverPassword) {
		std::cout << "Mot de passe correct pour le client " << client_fd << std::endl;
		return true;
	} else {
		std::cerr << "Mot de passe incorrect pour le client " << client_fd << std::endl;
		close(client_fd);
		removeClient(client_fd);
		return false;
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
	struct pollfd fds[100]; // Peut gérer jusqu'à 100 clients simultanés pour cet exemple
	int client_count = 0;

	fds[0].fd = server_fd; // Le serveur lui-même (pour accepter de nouvelles connexions)
	fds[0].events = POLLIN; // Attendre les événements d'entrée

	while (true) {
		int poll_count = poll(fds, client_count + 1, -1); // Attendre des événements
		if (poll_count < 0) {
			std::cerr << "Erreur: poll() échoué - " << strerror(errno) << std::endl;
			continue;
		}

		// Vérifier si un nouveau client se connecte
		if (fds[0].revents & POLLIN) {
			struct sockaddr_in client_address;
			socklen_t client_len = sizeof(client_address);
			int new_client = accept(server_fd, (struct sockaddr*)&client_address, &client_len);
			if (new_client < 0) {
				std::cerr << "Erreur: Accept échoué - " << strerror(errno) << std::endl;
				continue;
			}

			// Configurer le socket client en mode non-bloquant
			setNonBlocking(new_client);

			std::cout << "Nouveau client connecté!" << std::endl;
			client_count++;
			fds[client_count].fd = new_client; // Ajouter le nouveau client au tableau de poll
			fds[client_count].events = POLLIN; // Attendre les messages de ce client
		}

		// Gérer les messages des clients existants
		for (int i = 1; i <= client_count; i++) {
			if (fds[i].revents & POLLIN) {
				handleClient(fds[i].fd);

				// Supprimer les clients déconnectés
				if (fds[i].fd == -1) {
					for (int j = i; j < client_count; ++j) {
						fds[j] = fds[j + 1];
					}
					--client_count;
					--i; // Ajuster l'index
				}
			}
		}
	}
}

void Server::removeClient(int client_fd) {
	close(client_fd);
	clientMap.erase(client_fd);
	std::cout << "Client " << client_fd << " déconnecté et supprimé." << std::endl;
}

void Server::setNonBlocking(int fd) {
	int flags = fcntl(fd, F_GETFL, 0);
	fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}
/*Implementation IRC*/

void Server::processCommand(int client_fd, const std::string &message) {
	std::istringstream iss(message);
	std::string command;
	iss >> command;

	if (command == "PASS") {
		std::string password;
		iss >> password;
		if (!checkPassword(client_fd, password)) {
			return; // Fermer la connexion si le mot de passe est incorrect
		}
	} else if (command == "NICK") {
		std::string nickname;
		iss >> nickname;
		setNickname(client_fd, nickname);
	} else if (command == "USER") {
		std::string username, realname;
		iss >> username;
		std::getline(iss, realname);
		setUser(client_fd, username, realname);
	} else if (command == "JOIN") {
		std::string channel;
		iss >> channel;
		joinChannel(client_fd, channel);
	} else if (command == "PART") {
		std::string channel;
		iss >> channel;
		partChannel(client_fd, channel);
	} else if (command == "KICK") {
		std::string channel, user;
		iss >> channel >> user;
		kickUser(client_fd, channel, user); // Appel de la fonction kickUser
	} else if (command == "PRIVMSG") {
		std::string recipient, messageBody;
		iss >> recipient;
		std::getline(iss, messageBody);
		sendMessage(client_fd, recipient, messageBody);
	} else if (command == "QUIT") {
		close(client_fd);
		removeClient(client_fd);
	} else {
		std::cerr << "Commande inconnue reçue de " << client_fd << ": " << message << std::endl;
	}
}

void Server::handleClient(int client_fd) {
	char buffer[1024] = {0};
	int valread = read(client_fd, buffer, 1024);
	
	if (valread > 0) {
		std::string message(buffer);
		processCommand(client_fd, message);
	} else if (valread == 0) {
		// Déconnexion propre
		std::cout << "Client déconnecté proprement !" << std::endl;
		close(client_fd);
		removeClient(client_fd);
	} else {
		std::cerr << "Erreur: lecture échouée pour le client " << client_fd << " - " << strerror(errno) << std::endl;
	}
}

void Server::setNickname(int client_fd, const std::string& nickname) {
	if (clientMap.find(client_fd) == clientMap.end()) {
		// Si le client n'est pas enregistré, créer une nouvelle entrée
		clientMap[client_fd] = Client(client_fd);
	}

	// Assigner le pseudo
	clientMap[client_fd].nickname = nickname;
	std::cout << "Client " << client_fd << " a défini son pseudo en " << nickname << std::endl;
}

void Server::setUser(int client_fd, const std::string& username, const std::string& realname) {
	if (clientMap.find(client_fd) == clientMap.end()) {
		// Si le client n'est pas enregistré, créer une nouvelle entrée
		clientMap[client_fd] = Client(client_fd);
	}

	// Assigner le nom d'utilisateur et le nom réel
	clientMap[client_fd].username = username;
	clientMap[client_fd].realname = realname;
	clientMap[client_fd].registered = true; // Marquer comme enregistré
	std::cout << "Client " << client_fd << " s'est enregistré comme utilisateur : " << username << " (" << realname << ")" << std::endl;
}

void Server::joinChannel(int client_fd, const std::string& channelName) {
	// Si le canal n'existe pas, le créer
	if (channelMap.find(channelName) == channelMap.end()) {
		channelMap[channelName] = Channel(channelName);
	}

	// Ajouter le client au canal
	channelMap[channelName].clients.insert(client_fd);
	std::cout << "Client " << client_fd << " a rejoint le canal : " << channelName << std::endl;
}

void Server::sendMessage(int client_fd, const std::string& recipient, const std::string& message) {
	if (channelMap.find(recipient) != channelMap.end()) {
		// Envoyer le message à tous les membres du canal
		std::set<int>& members = channelMap[recipient].clients;
		for (std::set<int>::iterator it = members.begin(); it != members.end(); ++it) {
			int member_fd = *it;
			if (member_fd != client_fd) { // Ne pas renvoyer le message à l'expéditeur
				send(member_fd, message.c_str(), message.size(), 0);
			}
		}
		std::cout << "Message envoyé au canal " << recipient << " par " << client_fd << std::endl;
	} else {
		// Vérifier si le destinataire est un utilisateur
		for (std::map<int, Client>::iterator it = clientMap.begin(); it != clientMap.end(); ++it) {
			int fd = it->first;
			Client& client = it->second;
			if (client.nickname == recipient) {
				send(fd, message.c_str(), message.size(), 0);
				std::cout << "Message privé envoyé à " << recipient << " par " << client_fd << std::endl;
				return;
			}
		}
		std::cerr << "Erreur: destinataire inconnu " << recipient << std::endl;
	}
}

void Server::kickUser(int client_fd, const std::string& channelName, const std::string& user) {
	if (channelMap.find(channelName) == channelMap.end()) {
		std::cerr << "Erreur: Le canal " << channelName << " n'existe pas." << std::endl;
		return;
	}
	
	// Remplacer "auto" par le type explicite
	Channel &channel = channelMap[channelName];
	int user_fd = -1;

	// Remplacer la boucle basée sur les plages par une boucle classique
	std::map<int, Client>::iterator it;
	for (it = clientMap.begin(); it != clientMap.end(); ++it) {
		if (it->second.nickname == user) {
			user_fd = it->first;
			break;
		}
	}

	if (user_fd == -1 || channel.clients.find(user_fd) == channel.clients.end()) {
		std::cerr << "Erreur: L'utilisateur " << user << " n'est pas dans le canal." << std::endl;
		return;
	}

	channel.clients.erase(user_fd);
	std::string kickMessage = "Vous avez été expulsé du canal " + channelName + ".\n";
	send(user_fd, kickMessage.c_str(), kickMessage.size(), 0);
	std::cout << "Utilisateur " << user << " expulsé du canal " << channelName << " par " << client_fd << std::endl;
}

void Server::partChannel(int client_fd, const std::string& channelName) {
	if (channelMap.find(channelName) == channelMap.end()) {
		std::cerr << "Erreur: Le canal " << channelName << " n'existe pas." << std::endl;
		return;
	}

	Channel &channel = channelMap[channelName];
	if (channel.clients.find(client_fd) != channel.clients.end()) {
		channel.clients.erase(client_fd);
		std::cout << "Client " << client_fd << " a quitté le canal : " << channelName << std::endl;

		// Si le canal est vide, le supprimer
		if (channel.clients.empty()) {
			channelMap.erase(channelName);
			std::cout << "Canal " << channelName << " supprimé car il est vide." << std::endl;
		}
	} else {
		std::cerr << "Erreur: Le client " << client_fd << " n'est pas dans le canal " << channelName << std::endl;
	}
}
