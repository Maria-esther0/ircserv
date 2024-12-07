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
	static std::map<std::string, MemberFunctionPtr> handle_map();
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
	void nick_command(std::string &command, std::vector<std::string> &args, Server &server);
	void ping_command(std::string &command, std::vector<std::string> &args, Server &server);
	void pass_command(std::string &command, std::vector<std::string> &args, Server &server);
	void cap_command(std::string &command, std::vector<std::string> &args, Server &server);
	void user_command(std::string &command, std::vector<std::string> &args, Server &server);
	void join_command(std::string &command, std::vector<std::string> &args, Server &server);
	void mode_command(std::string &command, std::vector<std::string> &args, Server &server);
	void oper_command(std::string &command, std::vector<std::string> &args, Server &server);
	void who_command(std::string &command, std::vector<std::string> &args, Server &server);
	void privmsg_command(std::string &command, std::vector<std::string> &args, Server &server);
	void topic_command(std::string &command, std::vector<std::string> &args, Server &server);
	void kick_command(std::string &command, std::vector<std::string> &args, Server &server);
	void part_command(std::string &command, std::vector<std::string> &args, Server &server);
	void quit_command(std::string &command, std::vector<std::string> &args, Server &server);
	void msg_command(std::string &command, std::vector<std::string> &args, Server &server);
	void invite_command(std::string &command, std::vector<std::string> &args, Server &server);
};

#endif //FT_IRC_CLIENT_H
