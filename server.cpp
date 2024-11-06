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
	struct pollfd fds[100];
	int client_count = 0;

	fds[0].fd = server_fd;
	fds[0].events = POLLIN;

	while (true) {
		int poll_count = poll(fds, client_count + 1, -1);
		if (poll_count < 0) {
			std::cerr << "Erreur: poll() échoué - " << strerror(errno) << std::endl;
			continue;
		}

		if (fds[0].revents & POLLIN) {
			struct sockaddr_in client_address;
			socklen_t client_len = sizeof(client_address);
			int new_client = accept(server_fd, (struct sockaddr*)&client_address, &client_len);
			if (new_client < 0) {
				std::cerr << "Erreur: Accept échoué - " << strerror(errno) << std::endl;
				continue;
			}

			setNonBlocking(new_client);
			
			// Activer SO_KEEPALIVE pour maintenir la connexion active
			int optval = 1;
			setsockopt(new_client, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval));

			std::cout << "Nouveau client connecté!" << std::endl;
			client_count++;
			fds[client_count].fd = new_client;
			fds[client_count].events = POLLIN;
		}

		// Gestion des autres événements pour les clients connectés
		for (int i = 1; i <= client_count; i++) {
			if (fds[i].revents & POLLIN) {
				handleClient(fds[i].fd);

				if (fds[i].fd == -1) {
					for (int j = i; j < client_count; ++j) {
						fds[j] = fds[j + 1];
					}
					--client_count;
					--i;
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

	// Trouver le client correspondant au client_fd
	Client& client = clientMap[client_fd];

	// Log de debug pour chaque commande reçue
	std::cerr << "Commande reçue : " << command << " de client_fd : " << client_fd << std::endl;

	if (command == "CAP") {
		std::string subCommand;
		iss >> subCommand;
		if (subCommand == "LS") {
			std::string response = ":server CAP * LS :\r\n";
			send(client_fd, response.c_str(), response.size(), 0);
		}
		return;
	}

	if (command == "PASS") {
		std::string password;
		iss >> password;
		if (!checkPassword(client_fd, password)) {
			return; // Fermer la connexion si le mot de passe est incorrect
		}
		client.passReceived = true; // Indiquer que le mot de passe a été reçu
	} else if (command == "NICK") {
		if (!client.passReceived) {
			std::cerr << "Erreur : Le mot de passe doit être envoyé avant NICK.\n";
			return;
		}
		std::string nickname;
		iss >> nickname;
		setNickname(client_fd, nickname);
	} else if (command == "USER") {
		if (!client.passReceived) {
			std::cerr << "Erreur : Le mot de passe doit être envoyé avant USER.\n";
			return;
		}
		std::string username, realname;
		iss >> username;
		std::getline(iss, realname);
		setUser(client_fd, username, realname);

		// Envoyer une séquence complète de messages de bienvenue après l'enregistrement de l'utilisateur
		std::string welcomeMsg1 = ":server 001 " + client.nickname + " :Bienvenue sur le serveur IRC\r\n";
		std::string welcomeMsg2 = ":server 002 " + client.nickname + " :Votre host est server, version 1.0\r\n";
		std::string welcomeMsg3 = ":server 003 " + client.nickname + " :This server was created today\r\n";
		std::string welcomeMsg4 = ":server 004 " + client.nickname + " server 1.0 iowghraAsORTVSxNCWqBzvd\r\n";
		
		send(client_fd, welcomeMsg1.c_str(), welcomeMsg1.size(), 0);
		send(client_fd, welcomeMsg2.c_str(), welcomeMsg2.size(), 0);
		send(client_fd, welcomeMsg3.c_str(), welcomeMsg3.size(), 0);
		send(client_fd, welcomeMsg4.c_str(), welcomeMsg4.size(), 0);
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
		kickUser(client_fd, channel, user);
	} else if (command == "INVITE") {
		std::string channel, user;
		iss >> channel >> user;
		inviteUser(client_fd, channel, user);
	} else if (command == "MODE") {
		std::string channel, mode, parameter;
		iss >> channel >> mode;
		if (iss >> parameter) {
			setChannelMode(client_fd, channel, mode, parameter);
		} else {
			setChannelMode(client_fd, channel, mode);
		}
	} else if (command == "TOPIC") {
		std::string channel, topic;
		iss >> channel;
		std::getline(iss, topic);
		topic = topic.empty() ? "" : topic.substr(1); // Supprime l'espace initial de `getline`
		topicChannel(client_fd, channel, topic);
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
	char buffer[1024];
	int bytesRead = recv(client_fd, buffer, sizeof(buffer), 0);
	if (bytesRead < 0) {
		if (errno == EWOULDBLOCK || errno == EAGAIN) {
			return; // Pas de données à lire pour le moment
		} else {
			std::cerr << "Erreur de lecture de client " << client_fd << ": " << strerror(errno) << std::endl;
			close(client_fd);
			removeClient(client_fd);
			return;
		}
	} else if (bytesRead == 0) {
		std::cout << "Le client " << client_fd << " s'est déconnecté." << std::endl;
		close(client_fd);
		removeClient(client_fd);
		return;
	}

	// Traiter les données reçues
	std::string message(buffer, bytesRead);
	processCommand(client_fd, message);
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
		channelMap[channelName].operators.insert(client_fd); // Le créateur est opérateur
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

	Channel &channel = channelMap[channelName];

	if (channel.operators.find(client_fd) == channel.operators.end()) {
		std::cerr << "Erreur: Le client " << client_fd << " n'est pas opérateur du canal " << channelName << "." << std::endl;
		return;
	}

	int user_fd = -1;
	for (std::map<int, Client>::iterator it = clientMap.begin(); it != clientMap.end(); ++it) {
		if (it->second.nickname == user) {
			user_fd = it->first;
			break;
		}
	}

	if (user_fd == -1 || channel.clients.find(user_fd) == channel.clients.end()) {
		std::cerr << "Erreur: L'utilisateur " << user << " n'est pas dans le canal " << channelName << std::endl;
		return;
	}

	std::string notifyMsg = ":" + clientMap[client_fd].nickname + " KICK " + channelName + " " + user + " :Expulsé par l'opérateur\r\n";
	for (std::set<int>::iterator it = channel.clients.begin(); it != channel.clients.end(); ++it) {
		int member_fd = *it;
		send(member_fd, notifyMsg.c_str(), notifyMsg.size(), 0);
	}

	channel.clients.erase(user_fd);
	std::string kickMessage = "Vous avez été expulsé du canal " + channelName + ".\r\n";
	send(user_fd, kickMessage.c_str(), kickMessage.size(), 0);

	std::cout << "Utilisateur " << user << " expulsé du canal " << channelName << " par " << client_fd << std::endl;
}

void Server::inviteUser(int client_fd, const std::string& channelName, const std::string& user) {
	if (channelMap.find(channelName) == channelMap.end()) {
		std::cerr << "Erreur : Le canal " << channelName << " n'existe pas." << std::endl;
		return;
	}

	Channel &channel = channelMap[channelName];

	if (channel.operators.find(client_fd) == channel.operators.end()) {
		std::cerr << "Erreur : Le client " << client_fd << " n'est pas opérateur du canal " << channelName << "." << std::endl;
		return;
	}

	int user_fd = -1;
	for (std::map<int, Client>::iterator it = clientMap.begin(); it != clientMap.end(); ++it) {
		if (it->second.nickname == user) {
			user_fd = it->first;
			break;
		}
	}

	if (user_fd == -1) {
		std::cerr << "Erreur : L'utilisateur " << user << " n'est pas connecté." << std::endl;
		return;
	}

	std::string inviteMessage = "Vous avez été invité à rejoindre le canal " + channelName + ".\r\n";
	send(user_fd, inviteMessage.c_str(), inviteMessage.size(), 0);
	
	std::string confirmMsg = ":server 341 " + clientMap[client_fd].nickname + " " + user + " " + channelName + " :Invitation envoyée\r\n";
	send(client_fd, confirmMsg.c_str(), confirmMsg.size(), 0);

	std::cout << "Utilisateur " << user << " invité à rejoindre le canal " << channelName << " par " << client_fd << std::endl;
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

void Server::setChannelMode(int client_fd, const std::string& channelName, const std::string& mode, const std::string& parameter) {
	if (channelMap.find(channelName) == channelMap.end()) {
		std::cerr << "Erreur : Le canal " << channelName << " n'existe pas." << std::endl;
		return;
	}

	Channel &channel = channelMap[channelName];

	if (channel.operators.find(client_fd) == channel.operators.end()) {
		std::cerr << "Erreur : Le client " << client_fd << " n'est pas opérateur du canal " << channelName << "." << std::endl;
		return;
	}

	if (mode == "+i") {
		channel.inviteOnly = true;
		std::cout << "Le mode +i (invitation seulement) est activé pour le canal " << channelName << std::endl;
	} else if (mode == "-i") {
		channel.inviteOnly = false;
		std::cout << "Le mode -i (invitation seulement) est désactivé pour le canal " << channelName << std::endl;
	} else if (mode == "+t") {
		channel.topicRestricted = true;
		std::cout << "Le mode +t (sujet restreint) est activé pour le canal " << channelName << std::endl;
	} else if (mode == "-t") {
		channel.topicRestricted = false;
		std::cout << "Le mode -t (sujet restreint) est désactivé pour le canal " << channelName << std::endl;
	} else if (mode == "+k" && !parameter.empty()) {
		channel.password = parameter;
		std::cout << "Le mot de passe pour le canal " << channelName << " est défini." << std::endl;
	} else if (mode == "-k") {
		channel.password.clear();
		std::cout << "Le mot de passe pour le canal " << channelName << " est supprimé." << std::endl;
	} else if (mode == "+l" && !parameter.empty()) {
		channel.userLimit = std::stoi(parameter);
		std::cout << "Limite d'utilisateurs pour le canal " << channelName << " est définie à " << channel.userLimit << std::endl;
	} else if (mode == "-l") {
		channel.userLimit = -1;
		std::cout << "Limite d'utilisateurs pour le canal " << channelName << " est supprimée." << std::endl;
	} else {
		std::cerr << "Mode inconnu ou paramètre manquant pour le mode " << mode << std::endl;
	}
}

void Server::topicChannel(int client_fd, const std::string& channelName, const std::string& topic) {
	// Vérification de l'existence du canal
	if (channelMap.find(channelName) == channelMap.end()) {
		std::cerr << "Erreur : Le canal " << channelName << " n'existe pas." << std::endl;
		return;
	}

	Channel &channel = channelMap[channelName];

	// Vérification des permissions si le mode +t est activé
	if (channel.topicRestricted && channel.operators.find(client_fd) == channel.operators.end()) {
		std::cerr << "Erreur : Seuls les opérateurs peuvent modifier le sujet dans le canal " << channelName << " lorsque le mode +t est activé." << std::endl;
		return;
	}

	// Définir ou afficher le sujet du canal
	if (topic.empty()) {
		std::string topicMsg = "Sujet actuel pour le canal " + channelName + " : " + channel.topic + "\r\n";
		send(client_fd, topicMsg.c_str(), topicMsg.size(), 0);
	} else {
		// Mettre à jour le sujet
		channel.topic = topic;
		std::string topicUpdateMsg = ":" + clientMap[client_fd].nickname + " TOPIC " + channelName + " :" + topic + "\r\n";
		
		// Notifier tous les membres du canal du nouveau sujet
		for (std::set<int>::iterator it = channel.clients.begin(); it != channel.clients.end(); ++it) {
			int member_fd = *it;
			send(member_fd, topicUpdateMsg.c_str(), topicUpdateMsg.size(), 0);
		}

		std::cout << "Sujet du canal " << channelName << " mis à jour par le client " << client_fd << std::endl;
	}
}
