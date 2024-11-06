#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <vector>
#include <map>
#include <set>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

//colors
#define RED "\033[0;31m"
#define WHITE "\033[0;37m"
#define GREEN "\033[0;32m"
#define YELLOW "\033[0;33m"

// Définir les structures Client et Channel
struct Client {
	int fd;
	std::string nickname;
	std::string username;
	std::string realname;
	bool registered;
	bool passReceived; // Pour vérifier si le mot de passe a été reçu

	Client() : fd(-1), registered(false), passReceived(false) {} // Initialisation par défaut
	Client(int fd) : fd(fd), registered(false), passReceived(false) {}
};

struct Channel {
	std::string name;
	std::set<int> clients;
	std::set<int> operators;

	bool inviteOnly;         // Mode `i` : invitation seulement
	bool topicRestricted;    // Mode `t` : sujet restreint aux opérateurs
	std::string password;    // Mode `k` : mot de passe du canal
	int userLimit;           // Mode `l` : limite d’utilisateurs

	Channel() : inviteOnly(false), topicRestricted(false), userLimit(-1) {}
	Channel(const std::string& name) : name(name), inviteOnly(false), topicRestricted(false), userLimit(-1) {}
};



class Server {
private:
	int server_fd; // Descripteur de fichier pour le serveur
	int port; // Port sur lequel le serveur écoute
	struct sockaddr_in address; // Adresse du serveur
	std::vector<int> clients; // Liste des descripteurs de fichiers clients
	void catch_signal();
	static bool _signal;

	std::string serverPassword; // Mot de passe du serveur
	std::map<int, Client> clientMap; // Associe les FDs aux instances Client
	std::map<std::string, Channel> channelMap; // Associe les noms de canaux aux instances Channel

	void setNonBlocking(int fd);
	void removeClient(int client_fd);
	bool checkPassword(int client_fd, const std::string& password);

	// Méthodes pour traiter les commandes IRC
	void processCommand(int client_fd, const std::string& message);
	void setNickname(int client_fd, const std::string& nickname);
	void setUser(int client_fd, const std::string& username, const std::string& realname);
	void joinChannel(int client_fd, const std::string& channel);
	void partChannel(int client_fd, const std::string& channelName);
	void sendMessage(int client_fd, const std::string& recipient, const std::string& message);
	void kickUser(int client_fd, const std::string& channelName, const std::string& user);
	void inviteUser(int client_fd, const std::string& channelName, const std::string& user);
	void setChannelMode(int client_fd, const std::string& channelName, const std::string& mode, const std::string& parameter = "");

public:
	Server(int port, const std::string &password);
	~Server();
	void start(); // Méthode pour démarrer le serveur
	void acceptClients(); // Accepter les connexions clients
	void handleClient(int client_fd); // Gérer la communication avec un client

	static void check_signal(int signal);
	static int get_port(char *ag); // Récupérer le port à partir des arguments


	// commandes specifiques aux operateurs de canaux
// 	void kick(int client_fd, const std::string& command); // expulser un utilisateur d'un canal
// 	void invite(int client_fd, const std::string& command); // inviter un utilisateur dans un canal
// 	void topic(int client_fd, const std::string& command); // changer le sujet d'un canal
// 	void mode(int client_fd, const std::string& command); // changer le mode d'un canal
};

#endif // SERVER_HPP
