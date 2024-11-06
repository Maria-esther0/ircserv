#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <vector>
#include <map>
#include <set>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

// Définir les structures Client et Channel
struct Client {
	int fd;
	std::string nickname;
	time_t lastPing;
	std::string username;
	std::string realname;
	bool registered;    // Indique si le client est entièrement authentifié
	bool passReceived;  // Pour vérifier si le mot de passe a été reçu
	bool nickReceived;  // Pour vérifier si le pseudo (NICK) a été reçu
	bool userReceived;  // Pour vérifier si le nom d'utilisateur (USER) a été reçu

	Client() : fd(-1), registered(false), passReceived(false), nickReceived(false), userReceived(false) {}
	Client(int fd) : fd(fd), registered(false), passReceived(false), nickReceived(false), userReceived(false) {}
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
	int server_fd;
	int port;
	struct sockaddr_in address;  // Adresse du serveur
	std::vector<int> clients;    // Liste des descripteurs de fichier des clients
	std::string serverPassword;
	std::string serverName;
	std::map<int, Client> clientMap;
	std::map<std::string, Channel> channelMap;

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
	void sendWelcomeMessages(Client &client);
	std::string getServerCreationDate();
	void sendPingToClients();
	void disconnectInactiveClients();

public:
	Server(int port, const std::string &password, const std::string &name = "myircserver");
	~Server();
	void start();
	void acceptClients();
	void handleClient(int client_fd);
};

#endif // SERVER_HPP
