#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <vector>
#include <map>
#include <set>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

// Définir les structures Client et Channel
struct Client {
	int fd;
	std::string nickname;
	std::string username;
	std::string realname;
	bool registered;

	Client() : fd(-1), registered(false) {} // Constructeur par défaut
	Client(int fd) : fd(fd), registered(false) {}
};

struct Channel {
	std::string name;
	std::set<int> clients;

	Channel() {} // Constructeur par défaut
	Channel(const std::string& name) : name(name) {}
};

class Server {
private:
	int server_fd;
	int port;
	struct sockaddr_in address;
	std::vector<int> clients;
	
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

public:
	Server(int port, const std::string &password);
	~Server();
	void start(); // Méthode pour démarrer le serveur
	void acceptClients(); // Accepter les connexions clients
	void handleClient(int client_fd); // Gérer la communication avec un client
};

#endif // SERVER_HPP
