#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include "server.hpp"

class Server;
class Client;

class Channel {
private:
	Channel();

	int _userLimit;
	std::vector<int> _clientFds;
	std::vector<int> _opFds;
	std::vector<int> _invitedFds;
	std::set<char> _modes;
	std::string _name;
	std::string _topic;
	std::string _password;

public:

	Channel(const std::string &name);

	std::string getName();
	std::set<char> &getModes();
	std::vector<int> getClientFds();
	std::string getPassword();

	int getUserLimit() const;

	bool isFdInvited(int fd) const;
	bool hasMode(const char &mode);
	bool isClientInChannel(int fd) const;
	bool isOperator(int fd, Server &server);

	const std::string &getTopic() const;

	void setName(const std::string &name);
	void setChannelMode(const std::string &mode, Client &who, std::vector<std::string> args, Server &server);
	void unsetChannelMode(const std::string &mode, Client &who, std::vector<std::string> args, Server &server);
	void joinChannel(int fd, Server &server);
	void part(int fd, Server &server);
	void removeOperator(int fd);
	void disconnectClient(int fd);
	void removeAllAssociations(int fd);
	void inviteClient(int fd);
	void removeInvite(int fd);
	void broadcastMessage(const std::string &rep, Server &server);
	void setChannelTopic(const std::string &topic, Client &who, Server &server);
};


#endif //FT_IRC_CHANNEL_H
