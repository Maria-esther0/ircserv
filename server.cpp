#include "server.hpp"
#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <poll.h>
#include <fcntl.h>
#include <set>
#include <sstream>
#include <ctime>

Server::Server(int port, const std::string &password, const std::string &name)
	: port(port), serverPassword(password), serverName(name) {
	
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == -1) {
		std::cerr << "Erreur: impossible de créer le socket" << std::endl;
		exit(EXIT_FAILURE);
	}

	// Initialisation de l'adresse
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(port);

	if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
		std::cerr << "Erreur: impossible de binder le socket" << std::endl;
		exit(EXIT_FAILURE);
	}

	if (listen(server_fd, 10) < 0) {
		std::cerr << "Erreur: écoute impossible sur le socket" << std::endl;
		exit(EXIT_FAILURE);
	}
}

// void handleConnection(int clientSocket) {
// 	// Step 3: Use protocol messages
// 	std::string welcomeMessage = RPL_WELCOME; // Assuming PROTOCOL_WELCOME_MESSAGE is defined in messages.hpp
// 	send(clientSocket, welcomeMessage.c_str(), welcomeMessage.size(), 0);

// 	// Handle incoming messages
// 	char buffer[1024];
// 	int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
// 	if (bytesReceived > 0) {
// 		std::string receivedMessage(buffer, bytesReceived);
// 		if (receivedMessage == PROTOCOL_HELLO_MESSAGE) { // Assuming PROTOCOL_HELLO_MESSAGE is defined in messages.hpp
// 			// Respond to hello message
// 			std::string response = PROTOCOL_HELLO_RESPONSE;
// 			send(clientSocket, response.c_str(), response.size(), 0);
// 		}
// 	}
// }

Server::~Server() {
	close(server_fd);
	for (size_t i = 0; i < clients.size(); ++i) {
		close(clients[i]);
	}
}

bool Server::checkPassword(int client_fd, const std::string &password) {
	(void) client_fd;
	return password == serverPassword;
}

#include <ctime>
#include <iostream>

void Server::start()
{
	std::cout << "Le serveur est en écoute sur le port " << port << std::endl;
	
	time_t lastPingTime = time(NULL);

	while (true) {
		// 1. Utiliser select() pour surveiller les activités des sockets
		fd_set readfds;
		FD_ZERO(&readfds);
		FD_SET(server_fd, &readfds);
		int max_fd = server_fd;

		// Ajouter tous les clients existants au set de descripteurs
		for (std::map<int, Client>::iterator it = clientMap.begin(); it != clientMap.end(); ++it) {
			FD_SET(it->first, &readfds);
			if (it->first > max_fd) {
				max_fd = it->first;
			}
		}

		struct timeval timeout;
		timeout.tv_sec = 1; // Intervalle court pour vérifier régulièrement
		timeout.tv_usec = 0;

		int activity = select(max_fd + 1, &readfds, NULL, NULL, &timeout);
		if (activity < 0 && errno != EINTR) {
			std::cerr << "Erreur de select()" << std::endl;
			exit(EXIT_FAILURE);
		}

		// 2. Traiter les nouvelles connexions et messages des clients existants
		if (FD_ISSET(server_fd, &readfds)) {
			acceptClients();
		}

		for (std::map<int, Client>::iterator it = clientMap.begin(); it != clientMap.end(); ++it) {
			int client_fd = it->first;
			if (FD_ISSET(client_fd, &readfds)) {
				handleClient(client_fd);
			}
		}

		// 3. Appeler sendPingToClients toutes les 60 secondes
		time_t currentTime = time(NULL);
		if (currentTime - lastPingTime >= 60) {
			sendPingToClients();
			lastPingTime = currentTime;
		}

		// 4. Appeler disconnectInactiveClients pour déconnecter les clients inactifs
		disconnectInactiveClients();
	}
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

bool isValidUTF8(const std::string &str) {
	int bytesToProcess = 0;
	for (std::string::size_type i = 0; i < str.size(); ++i) {
		unsigned char c = static_cast<unsigned char>(str[i]);
		
		if (bytesToProcess == 0) {
			if ((c & 0x80) == 0) continue;                 // 0xxxxxxx (ASCII)
			else if ((c & 0xE0) == 0xC0) bytesToProcess = 1; // 110xxxxx
			else if ((c & 0xF0) == 0xE0) bytesToProcess = 2; // 1110xxxx
			else if ((c & 0xF8) == 0xF0) bytesToProcess = 3; // 11110xxx
			else return false;  // Caractère non valide pour UTF-8
		} else {
			if ((c & 0xC0) != 0x80) return false; // Les octets suivants doivent être 10xxxxxx
			--bytesToProcess;
		}
	}
	return bytesToProcess == 0; // Vérifie que la chaîne s'est terminée correctement
}

/*Implementation IRC*/

// Fonction principale de traitement des commandes
void Server::processCommand(int client_fd, const std::string &message) {
	// Vérifier l'encodage UTF-8
	if (!isValidUTF8(message)) {
		std::string errorMsg = ":server 400 " + clientMap[client_fd].nickname + " :Invalid UTF-8 encoding\r\n";
		send(client_fd, errorMsg.c_str(), errorMsg.size(), 0);
		return;
	}
	if (client_fd < 0 ) {
		std::cerr << "Erreur: client non enregistré" << std::endl;
		return;
	}
	std::istringstream iss(message);
	std::string command;
	iss >> command;

	Client &client = clientMap[client_fd];
	std::cout << "[DEBUG]: Client: " << &client << " its fd: " << client_fd << std::endl;
	if (!client.registered) {
		if (command != "PASS" && command != "NICK" && command != "USER") {
			std::string errorMsg = ":server 451 :You have not registered\r\n";
			send(client_fd, errorMsg.c_str(), errorMsg.size(), 0);
			return;
		}
	}
	// Commande CAP pour la négociation des capacités
	std::cout << "[DEBUG] Received command: " << command << " VS  Received message: " << message << std::endl;
	if (command == "CAP") {
		std::string subcommand;
		iss >> subcommand;

		if (subcommand == "LS") {
			std::string capResponse = ":server CAP * LS :\r\n"; // Liste vide des capacités
			send(client_fd, capResponse.c_str(), capResponse.size(), 0);
		} else if (subcommand == "REQ") {
			std::string capRequested;
			iss >> capRequested;
			std::string capAckResponse = ":server CAP * NAK :" + capRequested + "\r\n";
			send(client_fd, capAckResponse.c_str(), capAckResponse.size(), 0);
		} else if (subcommand == "END") {
			std::string capEndResponse = ":server CAP * END\r\n";
			send(client_fd, capEndResponse.c_str(), capEndResponse.size(), 0);
		}
		return;
	}

	if (command == "PASS") {
		std::string password;
		iss >> password;
		if (!checkPassword(client_fd, password)) {
			return;
		}
		client.passReceived = true;
	} else if (command == "NICK") {
		if (!client.passReceived && !serverPassword.empty()) {
			return;
		}
		std::string nickname;
		iss >> nickname;
		setNickname(client_fd, nickname);
	} else if (command == "USER") {
		std::cout << "[DEBUG] entering USER" << std::endl;
		// if (!client.passReceived) {

		// 	return;
		// }
		std::string username, realname;
		iss >> username;
		client.is_authenticated = true;
		std::getline(iss, realname);
		setUser(client_fd, username, realname);
		std::cout << "[DEBUG] Finish up USER" << std::endl;
		sendWelcomeMessages(client, client_fd);
	} else if (command == "PING") {
		std::string pongMessage = "PONG :" + command.substr(5) + "\r\n"; // Réponse au PING
		send(client_fd, pongMessage.c_str(), pongMessage.size(), 0);
	} else {
		if (!client.registered) {
			return;
		}

		if (command == "/JOIN") {
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
			// Commande inconnue
			std::string errorMsg = ":server 421 " + client.nickname + " " + command + " :Unknown command\r\n";
			send(client_fd, errorMsg.c_str(), errorMsg.size(), 0);
			std::cerr << "Commande inconnue reçue de " << client_fd << ": " << command << std::endl;
		}
	}
}

void Server::handleClient(int client_fd) {
	char buffer[1024] = {0};
	int valread = read(client_fd, buffer, 1024);
	std::
	std::cout << "[DEBUG] ################## fun ##################" << std::endl;

	if (valread > 0) {
		std::cout << "[DEBUG] Reeceived message: " << buffer << std::endl;
		// send(client_fd, "Bienvenue sur le serveur IRC!\n", strlen("Bienvenue sur le serveur IRC!\n"), 0);

		std::string message(buffer);
		std::stringstream ss(message);
		std::string to;

		while(std::getline(ss,to,'\n')){
			std::cout << "[DEBUG] handling commmand: " << to << std::endl;
			processCommand(client_fd, to);
		}
		// processCommand(client_fd, message);
	} else if (valread == 0) {
		// Déconnexion propre
		std::cout << "Client déconnecté proprement !" << std::endl;
		close(client_fd);
		removeClient(client_fd);
		return;
	}

	// Traiter les données reçues
	// std::string message(buffer, valread); //valread = strlen(buffer)
	// processCommand(client_fd, message);
}

void Server::setNickname(int client_fd, const std::string& nickname) {
	if (clientMap.find(client_fd) == clientMap.end()) {
		// Si le client n'est pas enregistré, créer une nouvelle entrée
		clientMap[client_fd] = Client(client_fd);
	}

	// Assigner le pseudo
	clientMap[client_fd].nickname = nickname;
	std::cout << "[DEBUG] Client set their nickname: " << nickname << std::endl;
	// std::cout << "Client " << client_fd << " a défini son pseudo en {send back message}" << nickname << std::endl;
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
		channelMap[channelName].operators.insert(client_fd); // Le premier utilisateur est opérateur
	}

	Channel& channel = channelMap[channelName];
	
	// Vérifier les conditions du canal (mode `+i`, limite d’utilisateurs, etc.)
	if (channel.inviteOnly && channel.operators.find(client_fd) == channel.operators.end()) {
		send(client_fd, ":server 473 :Cannot join channel (+i)\r\n", 39, 0); // Erreur d'accès au canal sur invitation seulement
		return;
	}

	if (channel.userLimit > 0 && channel.clients.size() >= static_cast<size_t>(channel.userLimit)) {
		send(client_fd, ":server 471 :Cannot join channel (+l)\r\n", 39, 0); // Erreur si le canal a atteint sa limite d'utilisateurs
		return;
	}

	// Ajouter le client au canal
	channel.clients.insert(client_fd);
	std::cout << "Client " << client_fd << " a rejoint le canal : " << channelName << std::endl;

	// Message de confirmation JOIN pour les autres membres du canal
	std::string joinMsg = ":" + clientMap[client_fd].nickname + " JOIN :" + channelName + "\r\n";
	for (std::set<int>::iterator it = channel.clients.begin(); it != channel.clients.end(); ++it) {
	int member_fd = *it;
	send(member_fd, joinMsg.c_str(), joinMsg.size(), 0);
}
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
		std::cerr << "Erreur : Le canal " << channelName << " n'existe pas." << std::endl;
		return;
	}

	Channel& channel = channelMap[channelName];
	if (channel.clients.find(client_fd) == channel.clients.end()) {
		std::cerr << "Erreur : Le client n'est pas dans le canal " << channelName << std::endl;
		return;
	}

	// Envoyer le message PART à tous les membres du canal
	std::string partMsg = ":" + clientMap[client_fd].nickname + " PART :" + channelName + "\r\n";
	for (std::set<int>::iterator it = channel.clients.begin(); it != channel.clients.end(); ++it) {
	int member_fd = *it;
	send(member_fd, partMsg.c_str(), partMsg.size(), 0);
}

	// Retirer le client du canal
	channel.clients.erase(client_fd);
	std::cout << "Client " << client_fd << " a quitté le canal : " << channelName << std::endl;

	// Supprimer le canal si vide
	if (channel.clients.empty()) {
		channelMap.erase(channelName);
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

std::string Server::getServerCreationDate() {
	// Supposons que vous ayez stocké la date de démarrage du serveur
	time_t now = time(0);
	char buffer[80];
	struct tm * timeinfo = localtime(&now);
	strftime(buffer, 80, "%c", timeinfo);
	return std::string(buffer);
}


void Server::sendWelcomeMessages(Client &client, int client_fd) {
	std::string nick = client.nickname;
	std::string user = client.username;
	std::string host = "localhost"; // Remplacez par le nom de votre serveur ou l'IP

	std::cout << "[DEBUG] Send welcome message to: " << nick << " fd: " << client_fd << std::endl;
	std::string msg001 = "001 " + nick + " :Welcome to ircserv \r\n";

	send(client_fd, msg001.c_str(), msg001.size(), 0);
}

void Server::sendPingToClients() {
	std::string pingMessage = "PING :server\r\n";
	for (std::map<int, Client>::iterator it = clientMap.begin(); it != clientMap.end(); ++it) {
		int client_fd = it->first;
		send(client_fd, pingMessage.c_str(), pingMessage.size(), 0);
	}
}

void Server::disconnectInactiveClients() {
	time_t currentTime = time(NULL);
	for (std::map<int, Client>::iterator it = clientMap.begin(); it != clientMap.end(); ) {
		if (currentTime - it->second.lastPing > 120) { // 120 secondes de délai
			int client_fd = it->first;
			close(client_fd);
			it = clientMap.erase(it);
			std::cout << "Client " << client_fd << " déconnecté pour inactivité." << std::endl;
		} else {
			++it;
		}
	}
}

int Server::get_port(char *ag)
{
	char *c = ag;
	while (*ag)
	{
		if (!std::isdigit(*ag))
		{
			return (-1);
		}
		ag++;
	}
	// printf("Port: %d\n", std::atoi(c));
	return std::atoi(c);
}

/*
 * Gestion des signaux pour crt+c et crt+\
*/
void Server::catch_signal()
{
	if (signal(SIGINT, Server::check_signal) == SIG_ERR) // crt+c
	{
		std::cerr << "Failed to bind signal SIGINT" << std::endl;
		_exit(1);
	}
	if (signal(SIGQUIT, Server::check_signal) == SIG_ERR) /* crtl+\ */
	{
		std::cerr << "Failed to bind signal SIGQUIT" << std::endl;
		_exit(1);
	}
}
void Server::check_signal(int signal)
{
	std::cout << "Signal reçu: " << signal << std::endl;
	//Server::_signal = true;
	_exit(0);

}
