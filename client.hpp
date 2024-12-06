#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <vector>
#include <iostream>
#include <map>
#include <sstream>
#include "server.hpp"
#include "responses.hpp"
#include "util.hpp"

class Server;

//typedef void (*MemberFunctionPtr)();


class Client {
private:
	int _fd;
	std::string _ipAddr;
	bool _authenticated; // true if has provided correct password
	std::string _buffer;
	std::string _pwd;
	std::string _nick;
	std::string _user;
	std::string _realname;
	std::string _servername;
	std::string _hostname;

	void setNick(const std::string &nick);
	void setUser(const std::string &user);
	void setRealName(const std::string &rname);
	void setHost(const std::string &host);
	void registerClient(Server &server);

public:
	typedef void (Client::*MemberFunctionPtr)(std::string &command, std::vector<std::string> &args, Server &server);
	static std::map<std::string, MemberFunctionPtr> createHandlerMap();
	static std::map<std::string, MemberFunctionPtr> functionMap;
	bool handleCommand(std::string &command, std::vector<std::string> &args, Server &server);

	Client();

	void sendReply(const std::string &rep) const;

	int getFd() const;
	std::string getNick();
	std::string getUser();
	std::string getRealName();
	std::string getHost();
	void setFd(int fd);
	void setIpAddr(const std::string &addr);
	void processClientBuffer(const char *newBuff, Server &server);
	
	// commands
	void handleCommandNick(std::string &command, std::vector<std::string> &args, Server &server);
	void handleCommandPing(std::string &command, std::vector<std::string> &args, Server &server);
	void handleCommandPass(std::string &command, std::vector<std::string> &args, Server &server);
	void handleCommandCap(std::string &command, std::vector<std::string> &args, Server &server);
	void handleCommandUser(std::string &command, std::vector<std::string> &args, Server &server);
	void handleCommandJoin(std::string &command, std::vector<std::string> &args, Server &server);
	void handleCommandMode(std::string &command, std::vector<std::string> &args, Server &server);
	void handleCommandOper(std::string &command, std::vector<std::string> &args, Server &server);
	void handleCommandWho(std::string &command, std::vector<std::string> &args, Server &server);
	void handleCommandPrivmsg(std::string &command, std::vector<std::string> &args, Server &server);
	void handleCommandTopic(std::string &command, std::vector<std::string> &args, Server &server);
	void handleCommandKick(std::string &command, std::vector<std::string> &args, Server &server);
	void handleCommandPart(std::string &command, std::vector<std::string> &args, Server &server);
	void handleCommandQuit(std::string &command, std::vector<std::string> &args, Server &server);
	void handleCommandMsg(std::string &command, std::vector<std::string> &args, Server &server);
	void handleCommandInvite(std::string &command, std::vector<std::string> &args, Server &server);
};

#endif //FT_IRC_CLIENT_H
