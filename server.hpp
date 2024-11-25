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
#include <arpa/inet.h>

// Définir les structures Client et Channel
struct Client {
	int fd;
	bool is_authenticated;
	std::string nickname;
	time_t lastPing;
	std::string username;
	std::string realname;
	bool registered;    // Indique si le client est entièrement authentifié
	bool passReceived;  // Pour vérifier si le mot de passe a été reçu
	bool nickReceived;  // Pour vérifier si le pseudo (NICK) a été reçu
	bool userReceived;  // Pour vérifier si le nom d'utilisateur (USER) a été reçu

	Client() : fd(-42), is_authenticated(false), registered(false), passReceived(false), nickReceived(false), userReceived(false){}
	Client(int fd) : fd(fd), is_authenticated(false), registered(false), passReceived(false), nickReceived(false), userReceived(false) {}
};

struct Channel {
	std::string name;
	std::set<int> clients;
	std::set<int> operators;
	bool inviteOnly;         // Mode `i` : invitation seulement
	bool topicRestricted;    // Mode `t` : sujet restreint aux opérateurs
	std::string password;    // Mode `k` : mot de passe du canal
	int userLimit;           // Mode `l` : limite d’utilisateurs
	std::string topic;       // Sujet du canal

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
	std::string serverName;

	void setNonBlocking(int fd);
	void removeClient(int client_fd);
	bool checkPassword(int client_fd, const std::string& password);
	void processCommand(int client_fd, const std::string& message);
	void setNickname(int client_fd, const std::string& nickname);
	void setUser(int client_fd, const std::string& username, const std::string& realname);
	void joinChannel(int client_fd, const std::string& channel);
	void partChannel(int client_fd, const std::string& channelName);
	void sendMessage(int client_fd, const std::string& recipient, const std::string& message);
	void kickUser(int client_fd, const std::string& channelName, const std::string& user);
	void inviteUser(int client_fd, const std::string& channelName, const std::string& user);
	void setChannelMode(int client_fd, const std::string& channelName, const std::string& mode, const std::string& parameter = "");
	void topicChannel(int client_fd, const std::string& channelName, const std::string& topic);
	void sendWelcomeMessages(Client &client, int client_fd);
	std::string getServerCreationDate();
	void sendPingToClients();
	void disconnectInactiveClients();
	bool CAP_LS;

public:
	Server(int port, const std::string &password, const std::string &name = "myircserver");
	~Server();
	void start(); // Méthode pour démarrer le serveur
	void acceptClients(); // Accepter les connexions clients
	void handleClient(int client_fd); // Gérer la communication avec un client
	// void handleConnection(int clientSocket);
	static void check_signal(int signal);
	static int get_port(char *ag); // Récupérer le port à partir des arguments
	bool                        getCapStatus();
    void                        setCapStatus(bool value);

	// commandes specifiques aux operateurs de canaux
// 	void kick(int client_fd, const std::string& command); // expulser un utilisateur d'un canal
// 	void invite(int client_fd, const std::string& command); // inviter un utilisateur dans un canal
// 	void topic(int client_fd, const std::string& command); // changer le sujet d'un canal
// 	void mode(int client_fd, const std::string& command); // changer le mode d'un canal
};

#endif // SERVER_HPP
