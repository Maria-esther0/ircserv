#include "client.hpp"
#include <unordered_map>

Client::Client() : _fd(-1), _authenticated(false), _buffer("")
{}

std::map<std::string, Client::MemberFunctionPtr> Client::functionMap = Client::handle_map();

std::map<std::string, Client::MemberFunctionPtr> Client::handle_map() {
	std::map<std::string, MemberFunctionPtr> map;
	map["NICK"] = &Client::nick_command;
	map["PING"] = &Client::ping_command;
	map["PASS"] = &Client::pass_command;
	map["CAP"] = &Client::cap_command;
	map["USER"] = &Client::user_command;
	map["JOIN"] = &Client::join_command;
	map["MODE"] = &Client::mode_command;
	map["OPER"] = &Client::oper_command;
	map["WHO"] = &Client::who_command;
	map["PRIVMSG"] = &Client::privmsg_command;
	map["TOPIC"] = &Client::topic_command;
	map["KICK"] = &Client::kick_command;
	map["PART"] = &Client::part_command;
	map["QUIT"] = &Client::quit_command;
	map["MSG"] = &Client::msg_command;
	map["INVITE"] = &Client::invite_command;
	return map;
}

std::string Client::getNick() {
	return _nick;
}

std::string Client::getUser() {
	return _user;
}

void Client::setRealName(const std::string &rname) {
	_realname = rname;
}

std::string Client::getRealName() {
	return _realname;
}

void Client::setHost(const std::string &host) {
	_hostname = host;
}

std::string Client::getHost() {
	return _hostname;
}

bool Client::handleCommand(std::string &command, std::vector<std::string> &args, Server &server) {
	std::map<std::string, MemberFunctionPtr>::iterator it = functionMap.find(args[0]);
	if (it != functionMap.end()) {
		MemberFunctionPtr func = it->second;
		(this->*func)(command, args, server);  // Call the member function
	} else {
		std::cout << "Function not found: " << command << std::endl;
		return false;
	}
	return true;
}

int Client::getFd() const {
	return _fd;
}

void Client::setFd(int fd) {
	_fd = fd;
}

void Client::setIpAddr(const std::string& addr) {
	_ipAddr = addr;
}

std::vector<std::string> get_commands(std::string& commandBuffer) {
	std::vector<std::string> commands;

	size_t newlinePos = commandBuffer.find('\n');

	while (newlinePos != std::string::npos) {
		std::string command = commandBuffer.substr(0, newlinePos);
		commandBuffer.erase(0, newlinePos + 1);
		commands.push_back(command);
		newlinePos = commandBuffer.find('\n');
	}
	return commands;
}

bool startswith(const std::string& str, const std::string& needle) {
	if (str.length() < needle.length())
		return false;
	for (size_t i = 0; i < needle.length(); ++i) {
		if (str[i] != needle[i])
			return false;
	}
	return true;
}

std::vector<std::string> arg_extraction(const std::string& command) {
	std::string commandCopy = command;
	size_t pos = 0;
	while ((pos = commandCopy.find('\r', pos)) != std::string::npos) {
		commandCopy[pos] = '\0';
	}

	std::vector<std::string> args;
	std::istringstream iss(commandCopy);
	std::string arg;
	while (iss >> arg) {
		args.push_back(util::trimSpace(std::string(arg.c_str())));
	}
	return args;
}

void Client::setNick(const std::string &nick) {
	/* validate nick here */
	_nick = nick;
}

void Client::setUser(const std::string &user) {
	/* validate user here */
	_user = user;
}

static void print_args(const std::vector<std::string> &args) {
	std::cout << "Args:\n";
	for (size_t i = 0; i < args.size(); ++i) {
		std::cout << (i + 1) << ": " << args[i];
		if (i < args.size() - 1) {
			std::cout << "\n";
		} else {
			std::cout << std::endl;
		}
	}
}

static std::string get_messages(const std::string& command) {
	size_t colon_pos = command.rfind(':');
	if (colon_pos == std::string::npos) {
		return ""; // No colon found, return empty string
	}
	std::string message = command.substr(colon_pos + 1);
	if (!message.empty() && message[message.size() - 1] == '\r') {
		message.erase(message.size() - 1);
	}
	return message;
}

static std::string get_channel_name(std::string arg) {
	if (arg.empty()) {
		return "";
	}
	if (startswith(arg, "#")) {
		return arg.substr(1);
	} else {
		return arg;
	}
}

void Client::nick_command(std::string &command, std::vector<std::string> &args, Server &server)  {
	if (args.size() > 1) {
		if (server.doesNickExist(args[1])) {
			sendReply(ERR_NICKNAMEINUSE(_nick, args[1]));
			return;
		}
		std::string oldNick = _nick;
		setNick(args[1]);
		if (!_authenticated) {
			registerClient(server);
		}
		if (_authenticated) {
			server.sendToAll(RPL_NICK(oldNick, _user, _hostname, _nick));
		}
	} else {
		sendReply(ERR_NEEDMOREPARAMS(std::string("NICK")));
	}
}

void Client::pass_command(std::string &command, std::vector<std::string> &args, Server &server)  {
	(void) command;
	(void) server;
	if (args.size() > 1) {
		_pwd = args[1];
	} else {
		sendReply(ERR_NEEDMOREPARAMS(std::string("PASS")));
	}
}

void Client::ping_command(std::string &command, std::vector<std::string> &args, Server &server)  {
	(void) command;
	(void) args;
	(void) server;
	sendReply(RPL_PING);
}

void Client::cap_command(std::string &command, std::vector<std::string> &args, Server &server)  {
	(void) command;
	(void) server;
	if (args[0] == "CAP" && args.size() > 2 && args[1] == "LS" && args[2] == "302") {
		sendReply(RPL_CAP);
		return;
	}
	if (args[0] == "CAP" && args.size() > 1 && args[1] == "REQ") {
		sendReply(RPL_CAP_REQ);
		return;
	}
}

void Client::user_command(std::string &command, std::vector<std::string> &args, Server &server)  {
	(void) command;
	if (args.size() > 4) {
		if (_authenticated) {
			sendReply(ERR_ALREADYREGISTERED);
			return;
		}
		setUser(args[1]);
		if (startswith(args[4], ":")) {
			setRealName(args[4].substr(1));
		}
		setHost(args[3]);
		registerClient(server);
	} else {
		sendReply(ERR_NEEDMOREPARAMS(std::string("USER")));
	}
}

void Client::join_command(std::string &command, std::vector<std::string> &args, Server &server)  {
	(void) command;
	if (args.size() > 1 && get_channel_name(args[1]).size() > 0) {
		if (!server.channelExists(get_channel_name(args[1]))) {
			server.createChannel(get_channel_name(args[1]));
		}
		Channel &chan = server.getChannelByName(get_channel_name(args[1]));
		if (chan.isClientInChannel(_fd)) {
			sendReply(ERR_USERONCHANNEL(_nick, chan.getName()));
			return;
		}
		if (chan.hasMode('l') && chan.getUserLimit() > -1 && chan.getUserLimit() == (int)chan.getClientFds().size()) {
			sendReply(ERR_CHANNELISFULL(_nick, chan.getName()));
			return;
		}
		if (chan.hasMode('k') && !chan.getPassword().empty()) {
			if (args.size() < 3 || (args.size() > 2 && chan.getPassword() != args[2])) {
				sendReply(ERR_BADCHANNELKEY(_nick, chan.getName()));
				return;
			}
		}
		if (chan.hasMode('i')) {
			if (!chan.isFdInvited(_fd)) {
				sendReply(RPL_MUST_BE_INVITED(_nick, chan.getName()));
				return;
			}
		}
		chan.joinChannel(_fd, server);
	} else {
		sendReply(ERR_NEEDMOREPARAMS(std::string("JOIN")));
	}
}

void Client::mode_command(std::string &command, std::vector<std::string> &args, Server &server)  {
	(void) command;
	(void) args;
	(void) server;
	if (args.size() > 1) {

		std::string name = get_channel_name(args[1]);
		if (server.channelExists(name)) {
			Channel &chan = server.getChannelByName(name);
			if (args.size() > 2) {
				std::string mode = "";
				if (args[2].size() > 1) {
					mode = args[2].substr(1, 2);
				}
				if (!chan.isClientInChannel(_fd)) {
					sendReply(ERR_USERNOTINCHANNEL(_nick, chan.getName()));
					return;
				}
				if (startswith(args[2], "b")) {
					sendReply(END_BAN(_nick, chan.getName()));
				} else if (startswith(args[2], "+")) {
					chan.setChannelMode(mode, *this, args, server);
				} else if (startswith(args[2], "-")) {
					chan.unsetChannelMode(mode, *this, args, server);
				}
			} else {
				std::string modes = "+";
				std::set<char>::iterator it_;
				std::cout << "The size of chan modes is " << chan.getModes().size() << std::endl;
				for (it_ = chan.getModes().begin(); it_ != chan.getModes().end(); it_ ++) {
					if (*it_ == 'k' && chan.getPassword().empty()) {
						continue;
					}
					modes += *it_;
				}
				for (size_t i = 0; i < modes.size(); ++i) {
					if (modes[i] == 'l' && chan.getUserLimit() > 0) {
						modes += " ";
						modes += util::streamItoa(chan.getUserLimit());
					}
					if (modes[i] == 'k' && !chan.getPassword().empty()) {
						modes += " ";
						modes += chan.getPassword();
					}
				}
				sendReply(RPL_CHANNEL_MODES(_nick, chan.getName(), modes));
			}
		} else if (server.doesNickExist(args[1])) {
			if (args.size() > 2) {
				std::string mode = "";
				if (args[2].size() > 1) {
					mode = args[2].substr(1, 2);
				}
				sendReply(RPL_UNKNOWN_USER_MODE(_nick, mode));
			} else {
				if (args[1] == _nick) {
					std::string mode = "+";
					if (server.fdIsGlobalOp(_fd)) {
						mode += "o";
					}
					sendReply(RPL_NICK_MODES(_nick, mode));
				} else {
					sendReply(RPL_CANT_MODE_OTHER(_nick));
				}
			}
		} else {
			sendReply(ERR_NOSUCHNICK(_nick, args[1]));
		}
	} else {
		sendReply(ERR_NEEDMOREPARAMS(std::string("MODE")));
	}
}

void Client::oper_command(std::string &command, std::vector<std::string> &args, Server &server)  {
	(void) command;
	if (args.size() > 2) {
		if (args[2] != "1234") {
			sendReply(ERR_PASSWDMISMATCH);
		} else {
			sendReply(RPL_MODE_OPER(_nick));
			sendReply(RPL_OPER(_nick));
			server.op(_fd);
		}
	} else {
		sendReply(ERR_NEEDMOREPARAMS(std::string("OPER")));
	}
}

void Client::who_command(std::string &command, std::vector<std::string> &args, Server &server)  {
	(void) command;
	if (args.size() > 1) {
		if (!server.channelExists(get_channel_name(args[1]))) {
			sendReply(ERR_NOSUCHCHANNEL(args[1]));
			return;
		}
		Channel &chan = server.getChannelByName(get_channel_name(args[1]));
		std::cout << "Amount of users in channel for WHO: " << chan.getClientFds().size() << std::endl;
		for (size_t i = 0; i < chan.getClientFds().size(); i ++) {
			if (!server.fdExists(chan.getClientFds()[i])) {
				continue;
			}
			Client &client = server.getClientByFd(chan.getClientFds()[i]);
			std::string flags = "H";
			if (chan.isOperator(chan.getClientFds()[i], server)) {
				flags = "H@";
			}
			sendReply(RPL_WHO(_nick, chan.getName(), client.getUser(), client.getHost(), client.getNick(), flags, client.getRealName()));
		}
		sendReply(END_WHO(_nick, chan.getName()));
	} else {
		sendReply(ERR_NEEDMOREPARAMS(std::string("WHO")));
	}
}

void Client::privmsg_command(std::string &command, std::vector<std::string> &args, Server &server)  {
	if (args.size() > 2) {
		if (server.channelExists(get_channel_name(args[1]))) {
			Channel &chan = server.getChannelByName(get_channel_name(args[1]));
			if (!chan.isClientInChannel(_fd)) {
				sendReply(ERR_USERNOTINCHANNEL(_nick, chan.getName()));
				return;
			}
			std::string message = get_messages(command);
			for (size_t i = 0; i < chan.getClientFds().size(); i ++) {
				if (!server.fdExists(chan.getClientFds()[i])) {
					continue;
				}
				Client &client = server.getClientByFd(chan.getClientFds()[i]);
				if (client.getFd() != _fd) {
					client.sendReply(RPL_PRIVMSG(getNick(), getUser(), getHost(), chan.getName(), message));
				}
			}
		} else if (server.doesNickExist(args[1])) {
			if (!server.doesNickExist(args[1])) {
				sendReply(ERR_NOSUCHNICK(_nick, args[1]));
				return;
			}
			std::string message = get_messages(command);
			Client &c = server.findClientByNick(args[1]);
			c.sendReply(PRIV_MSG_DM(_nick, _user, _hostname, c.getNick(), message));
		} else {
			sendReply(ERR_NOSUCHNICK(_nick, args[1]));
			return;
		}

	} else {
		sendReply(ERR_NEEDMOREPARAMS(std::string("PRIVMSG")));
	}
}

void Client::topic_command(std::string &command, std::vector<std::string> &args, Server &server)  {
	(void) command;
	(void) args;
	(void) server;
	if (args.size() < 2) {
		sendReply(ERR_NEEDMOREPARAMS(std::string("TOPIC")));
		return;
	}

	if (!server.channelExists(get_channel_name(args[1]))) {
		sendReply(ERR_NOSUCHCHANNEL(args[1]));
		return;
	}

	Channel &chan = server.getChannelByName(get_channel_name(args[1]));
	if (args.size() > 2) {
		if (!chan.isClientInChannel(_fd)) {
			sendReply(ERR_USERNOTINCHANNEL(_nick, chan.getName()));
			return;
		}
		if (chan.hasMode('t') && !chan.isOperator(_fd, server)) {
			sendReply(ERR_CHANOPRIVSNEEDED(chan.getName(), _nick));
			return;
		}
		std::string topic = get_messages(command);
		chan.setChannelTopic(topic, *this, server);
	} else {
		sendReply(RPL_TOPIC_COMMAND(_nick, chan.getName(), chan.getTopic()));
	}
}

void Client::kick_command(std::string &command, std::vector<std::string> &args, Server &server)  {
	(void) command;
	if (args.size() > 2) {
		if (!server.channelExists(get_channel_name(args[1]))) {
			sendReply(ERR_NOSUCHCHANNEL(args[1]));
			return;
		}
		Channel &chan = server.getChannelByName(get_channel_name(args[1]));
		if (!chan.isClientInChannel(_fd)) {
			sendReply(ERR_USERNOTINCHANNEL(_nick, chan.getName()));
			return;
		}
		if (!chan.isOperator(_fd, server)) {
			sendReply(ERR_CHANOPRIVSNEEDED(chan.getName(), _nick));
			return;
		}
		if (server.doesNickExist(args[2])) {
			Client &target = server.findClientByNick(args[2]);
			if (chan.isClientInChannel(target.getFd())) {
				chan.broadcastMessage(RPL_KICK(_nick, _user, _hostname, chan.getName(), target.getNick()), server);
				chan.removeAllAssociations(target.getFd());
				if (chan.getClientFds().empty()) {
					server.removeChannel(chan.getName());
				}
			} else {
				sendReply(ERR_NOSUCHNICK(_nick, args[2]));
				return;
			}
		} else {
			sendReply(ERR_NOSUCHNICK(_nick, args[2]));
			return;
		}
	} else {
		sendReply(ERR_NEEDMOREPARAMS(std::string("KICK")));
	}
}

void Client::invite_command(std::string &command, std::vector<std::string> &args, Server &server)  {
	(void) command;
	(void) args;
	(void) server;
	if (args.size() > 2) {
		std::cout << "INVITE: checking if channel `" << get_channel_name(args[2]) << "` exists: " << server.channelExists(get_channel_name(args[2])) << std::endl;
		if (!server.channelExists(get_channel_name(args[2]))) {
			sendReply(ERR_NOSUCHCHANNEL(args[2]));
			return;
		}
		Channel &chan = server.getChannelByName(get_channel_name(args[2]));
		if (!chan.isClientInChannel(_fd)) {
			sendReply(ERR_USERNOTINCHANNEL(_nick, chan.getName()));
			return;
		}
		if (server.doesNickExist(args[1])) {
			Client &target = server.findClientByNick(args[1]);
			chan.inviteClient(target.getFd());
			sendReply(RPL_INVITE(_nick, target.getNick(), target.getUser(), target.getHost(), chan.getName()));
			target.sendReply(INVITE(_nick, _user, _hostname, target.getNick(), chan.getName()));
		} else {
			sendReply(ERR_NOSUCHNICK(_nick, args[1]));
			return;
		}
	} else {
		sendReply(ERR_NEEDMOREPARAMS(std::string("INVITE")));
	}
}

void Client::part_command(std::string &command, std::vector<std::string> &args, Server &server) {
	if (args.size() > 1) {
		std::string channelName = get_channel_name(args[1]);
		if (!server.channelExists(channelName)) {
			sendReply(ERR_NOSUCHCHANNEL(args[1]));
			return;
		}
		Channel &chan = server.getChannelByName(channelName);
		if (!chan.isClientInChannel(_fd)) {
			sendReply(ERR_USERNOTINCHANNEL(_nick, chan.getName()));
			return;
		}
		chan.broadcastMessage(RPL_PART(_nick, _user, _hostname, chan.getName(), get_messages(command)), server);
		chan.part(_fd, server); // Utiliser la fonction part pour retirer le client du canal
	} else {
		sendReply(ERR_NEEDMOREPARAMS(std::string("PART")));
	}
}

void Client::quit_command(std::string &command, std::vector<std::string> &args, Server &server)  {
	(void) command;
	(void) args;
	// std::vector<Channel> &channels = server.getChannels();
	// std::vector<std::string> chanNamesWithFd;
	// for (size_t i = 0; i < channels.size(); ++i) {
	// 	if (channels[i].isClientInChannel(_fd)) {
	// 		chanNamesWithFd.push_back(channels[i].getName());
	// 	}
	// }
	// std::string msg = get_messages(command);
	// for (size_t i = 0; i < chanNamesWithFd.size(); ++i) {
	// 	if (!server.channelExists(chanNamesWithFd[i])) {
	// 		continue;
	// 	}
	// 	Channel &chan = server.getChannelByName(chanNamesWithFd[i]);
	// 	chan.broadcastMessage(RPL_QUIT(_nick, _user, _hostname, channels[i].getName(), msg), server);
	// 	chan.removeAllAssociations(_fd);
	// 	if (chan.getClientFds().empty()) {
	// 		server.removeChannel(chan.getName());
	// 	}
	// }
	// sendReply(ERROR(msg));
	// server.disconnectClient(_fd);
}

void Client::msg_command(std::string &command, std::vector<std::string> &args, Server &server)  {
	if (args.size() > 2) {
		if (!server.doesNickExist(args[1])) {
			sendReply(ERR_NOSUCHNICK(_nick, args[1]));
			return;
		}
		std::string message = get_messages(command);
		Client &c = server.findClientByNick(args[1]);
		c.sendReply(PRIV_MSG_DM(_nick, _user, _hostname, c.getNick(), message));
	} else {
		sendReply(ERR_NEEDMOREPARAMS(std::string("PRIVMSG")));
	}
}

void Client::processClientBuffer(const char *newBuff, Server &server) {
	_buffer += std::string(newBuff);
	std::vector<std::string> commands = get_commands(_buffer);
	std::vector<std::string>::iterator it;

	for (it = commands.begin(); it != commands.end(); ++it) {
		std::vector<std::string> args = arg_extraction(*it);
		if (args.empty()) {
			continue;
		}
		print_args(args);
		// Commandes de débogage
		if (args[0] == "DEBUG:FDS") {
			server.debugPrintFds();
			continue;
		}
		if (args[0] == "DEBUG:CHANNELS") {
			server.debugPrintChannels();
			continue;
		}
		// Vérifier si le client est authentifié
		if (!_authenticated && args[0] != "PASS" && args[0] != "USER" && args[0] != "NICK") {
			std::cout << "Not registered, skipping command" << std::endl;
			sendReply(ERR_NOTREGISTERED);
			continue;
		}
		// Traiter la commande
		if (!handleCommand(*it, args, server)) {
			sendReply(ERR_UNKNOWNCOMMAND(args[0]));
		}
	}

	std::cout << "Remaining in buffer: " << _buffer << std::endl;
}

void Client::registerClient(Server &server) {
	// Vérifier si les champs utilisateur et pseudo sont vides
	if (_user.empty() || _nick.empty()) {
		return;
	}

	// Vérifier si le mot de passe est correct
	if (!server.isPasswordValid(_pwd)) {
		sendReply(ERR_PASSWDMISMATCH);
		return;
	}

	// Authentifier le client
	_authenticated = true;

	// Envoyer les messages de bienvenue et MOTD (Message of the Day)
	sendReply(RPL_WELCOME(_nick));
	sendReply(RPL_MOTD_MISSING);
}

void Client::sendReply(const std::string &rep) const {
    const size_t MAX_LENGTH = 510;

    // Truncate the message to the maximum length and append the carriage return/newline
    std::string truncatedRep = rep.substr(0, MAX_LENGTH);  // Truncate to MAX_LENGTH
    truncatedRep.append("\r\n");  // Append the CRLF sequence

    std::cout << "Tentative de réponse avec " << truncatedRep << " à " << _fd << std::endl;

    // Send the response to the client
    ssize_t sentBytes = send(_fd, truncatedRep.c_str(), truncatedRep.length(), 0);

    if (sentBytes < 0) {
        // More detailed error handling
        std::cerr << "Erreur lors de l'envoi de la réponse au client <" << _fd << "> : "
                  << strerror(errno) << std::endl;
    }
}


//void Client::sendReply(const std::string &rep) const {
//	const size_t MAX_LENGTH = 510;
//	std::string truncatedRep = rep.substr(0, MAX_LENGTH);
//	std::cout << "Tentative de réponse avec " << truncatedRep << " à " << _fd << std::endl;
//	truncatedRep += "\r\n";
//	ssize_t sentBytes = send(_fd, truncatedRep.c_str(), truncatedRep.length(), 0);
//	if (sentBytes < 0) {
//		std::cerr << "Erreur lors de l'envoi de la réponse au client <" << _fd << ">" << std::endl;
//	}
//}
