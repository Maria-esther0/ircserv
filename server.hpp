#ifndef SERVER_HPP
#define SERVER_HPP

#include <arpa/inet.h>
#include <cctype>
#include <csignal>
#include <cstdlib>
#include <fcntl.h>
#include <iostream>
#include <poll.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <vector>
#include <cstring>
#include <unistd.h>
#include <algorithm>
#include <string>
#include <set>
#include "client.hpp"
#include "channel.hpp"

/* colors */
#define RED "\033[0;31m"
#define WHI "\033[0;37m"
#define GRE "\033[0;32m"
#define YEL "\033[0;33m"

class Client;
class Channel;

class Server {
private:
	int _port;
	int _serverSocketFd;
	std::string _password;
	std::vector<Client> _clients;
	std::vector<Channel> _channels;
	std::vector<struct pollfd> _fds;
	std::vector<int> _globalOpFds;

	/* Stop server when we get a ctrl+c or ctrl+\ */
	static bool _gotSig;

	void setupSignalHandlers();

public:
	Server();

	void startServer(char *portArg, char *pwdArg);
	void initializeServerSocket();
	void acceptNewClient();
	void closeAllSockets();
	void disconnectClient(int fd);
	void handleClientData(int fd);

	static int validateAndParsePort(const char *arg);
	static void handleSignal(int signum);

	bool isPasswordValid(const std::string &pwd) const;

	bool doesNickExist(const std::string &nick);
	Client& findClientByNick(const std::string& nick);
	bool fdExists(int fd);
	Client &getClientByFd(int fd);

	bool channelExists(const std::string &name);
	Channel &getChannelByName(const std::string &name);
	std::vector<Channel> &getChannels();

	Channel &createChannel(const std::string &name);
	void removeChannel(const std::string &name);

	void sendToClient(const std::string &rep, int fd);
	void sendToAll(const std::string &rep);

	void op(int fd);
	bool fdIsGlobalOp(int fd);

	void debugPrintFds();
	void debugPrintChannels();

	std::string hostname;
};


#endif //FT_IRC_SERVER_H
