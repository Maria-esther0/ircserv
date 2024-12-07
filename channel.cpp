#include "channel.hpp"

std::string Channel::getName() {
	return _name;
}

Channel::Channel() {
	_userLimit = -1;
}

Channel::Channel(const std::string &name) {
	_name = name;
	_userLimit = -1;
}

void Channel::setName(const std::string &name) {
	_name = name;
}

const std::string &Channel::getTopic() const {
	return _topic;
}

void Channel::setChannelMode(const std::string &mode, Client &who, std::vector<std::string> args, Server &server) {
	if (!isOperator(who.getFd(), server)) {
		who.sendReply(ERR_CHANOPRIVSNEEDED(_name, who.getNick()));
		return;
	}
	if (mode.empty()) {
		return;
	}
	char c_mode = mode.c_str()[0];

	switch (c_mode) {
		case 'k':
			if (args.size() > 3) {
				_password = args[3];
			}
			break;
		case 'o':
			if (args.size() > 3) {
				if (!server.doesNickExist(args[3])) {
					who.sendReply(ERR_NOSUCHNICK(who.getNick(), args[3]));
				} else {
					_opFds.push_back(server.findClientByNick(args[3]).getFd());
				}
			}
			break;
		case 'l':
			if (args.size() > 3) {
				char *end;
				long limit = std::strtol(args[3].c_str(), &end, 10);
				if (*end == '\0' && limit > 0) {
					_userLimit = static_cast<int>(limit);
				}
			}
			break;
		case 't':
		case 'i':
			// Aucun argument supplémentaire nécessaire pour ces modes
			break;
		default:
			who.sendReply(ERR_UNKNOWN_CHANNEL_MODE(mode, _name));
			return;
	}

	_modes.insert(c_mode);
	std::string modeArg = (args.size() > 3) ? args[3] : "";
	broadcastMessage(RPL_MODE(who.getNick(), who.getUser(), who.getHost(), _name, "+" + mode, modeArg), server);
}

bool Channel::hasMode(const char &mode) {
	return _modes.find(mode) != _modes.end();
}

void Channel::unsetChannelMode(const std::string &mode, Client &who, std::vector<std::string> args, Server &server) {
	if (!isOperator(who.getFd(), server)) {
		who.sendReply(ERR_CHANOPRIVSNEEDED(_name, who.getNick()));
		return;
	}
	if (mode.empty()) {
		return;
	}
	char c_mode = mode.c_str()[0];

	switch (c_mode) {
		case 'k':
			_password = "";
			break;
		case 'o':
			if (args.size() > 3) {
				if (!server.doesNickExist(args[3])) {
					who.sendReply(ERR_NOSUCHNICK(who.getNick(), args[3]));
				} else {
					removeOperator(server.findClientByNick(args[3]).getFd());
				}
			}
			break;
		case 'l':
			_userLimit = -1;
			break;
		case 't':
		case 'i':
			// Aucun argument supplémentaire nécessaire pour ces modes
			break;
		default:
			who.sendReply(ERR_UNKNOWN_CHANNEL_MODE(mode, _name));
			return;
	}

	_modes.erase(c_mode);
	std::string modeArg = (args.size() > 3) ? args[3] : "";
	broadcastMessage(RPL_MODE(who.getNick(), who.getUser(), who.getHost(), _name, "-" + mode, modeArg), server);
}

bool Channel::isOperator(int fd, Server &server) {
	if (server.fdIsGlobalOp(fd)) {
		return true;
	}
	for (std::vector<int>::iterator it = _opFds.begin(); it != _opFds.end(); ++it) {
		if (*it == fd) {
			return true;
		}
	}
	return false;
}

void Channel::setChannelTopic(const std::string &topic, Client &who, Server &server) {
	// Vérifier si le client est un opérateur du canal
	if (!isOperator(who.getFd(), server)) {
		who.sendReply(ERR_CHANOPRIVSNEEDED(_name, who.getNick()));
		return;
	}

	// Définir le sujet du canal
	_topic = topic;

	// Envoyer le nouveau sujet à tous les membres du canal
	broadcastMessage(RPL_TOPIC(who.getNick(), who.getUser(), who.getHost(), _name, topic), server);
}

void Channel::joinChannel(int fd, Server &server) {
	if (!server.fdExists(fd)) {
		return;
	}

	Client &c = server.getClientByFd(fd);
	c.sendReply(RPL_JOIN(c.getNick(), c.getUser(), c.getHost(), _name));
	broadcastMessage(RPL_JOIN(c.getNick(), c.getUser(), c.getHost(), _name), server);

	std::string users;
	if (_clientFds.empty()) {
		_opFds.push_back(fd);
	}
	_clientFds.push_back(fd);
	std::cout << "Ajout du fd " << fd << " aux fds..., taille maintenant " << _clientFds.size() << std::endl;

	for (size_t i = 0; i < _clientFds.size(); ++i) {
		if (server.fdExists(_clientFds[i])) {
			if (!users.empty()) {
				users += " ";
			}
			if (isOperator(_clientFds[i], server)) {
				users += "@";
			}
			users += server.getClientByFd(_clientFds[i]).getNick();
		}
	}

	c.sendReply(SEND_TOPIC(c.getNick(), _name, _topic));
	c.sendReply(RPL_NAMREPLY(c.getNick(), _name, users));
	c.sendReply(RPL_ENDOFNAMES(c.getNick(), _name));
}

void Channel::part(int fd, Server &server) {
	// Vérifier si le descripteur de fichier existe dans le serveur
	if (!server.fdExists(fd)) {
		return;
	}

	// Retirer le descripteur de fichier de la liste des clients du canal
	for (std::vector<int>::iterator it = _clientFds.begin(); it != _clientFds.end(); ++it) {
		if (*it == fd) {
			_clientFds.erase(it);
			break;
		}
	}

	// Retirer le descripteur de fichier de la liste des opérateurs du canal, si présent
	for (std::vector<int>::iterator it = _opFds.begin(); it != _opFds.end(); ++it) {
		if (*it == fd) {
			_opFds.erase(it);
			break;
		}
	}

	// Si le canal est vide, effectuer des actions de nettoyage si nécessaire
	if (_clientFds.empty()) {
		server.removeChannel(_name);
	}
}

bool Channel::isClientInChannel(int fd) const {
	for (std::vector<int>::const_iterator it = _clientFds.begin(); it != _clientFds.end(); ++it) {
		if (*it == fd) {
			return true;
		}
	}
	return false;
}

std::set<char> &Channel::getModes() {
	return _modes;
}

std::vector<int> Channel::getClientFds() {
	return _clientFds;
}

std::string Channel::getPassword() {
	return _password;
}

int Channel::getUserLimit() const {
	return _userLimit;
}

void Channel::removeOperator(int fd) {
	for (std::vector<int>::iterator it = _opFds.begin(); it != _opFds.end(); ++it) {
		if (*it == fd) {
			_opFds.erase(it);
			break;
		}
	}
}

void Channel::disconnectClient(int fd) {
	std::cout << "Retrait du client " << fd << " du canal " << std::endl;
	for (std::vector<int>::iterator it = _clientFds.begin(); it != _clientFds.end(); ++it) {
		if (*it == fd) {
			_clientFds.erase(it);
			break;
		}
	}
}

void Channel::removeAllAssociations(int fd) {
	// Retirer le descripteur de fichier de la liste des opérateurs
	removeOperator(fd);
	// Retirer le descripteur de fichier de la liste des clients
	disconnectClient(fd);
	// Retirer le descripteur de fichier de la liste des invités
	removeInvite(fd);
}

void Channel::inviteClient(int fd) {
	// Vérifier si le descripteur de fichier est déjà dans la liste des invités
	for (std::vector<int>::iterator it = _invitedFds.begin(); it != _invitedFds.end(); ++it) {
		if (*it == fd) {
			return; // Le descripteur de fichier est déjà invité, ne rien faire
		}
	}
	// Ajouter le descripteur de fichier à la liste des invités
	_invitedFds.push_back(fd);
}

void Channel::removeInvite(int fd) {
	for (std::vector<int>::iterator it = _invitedFds.begin(); it != _invitedFds.end(); ++it) {
		if (*it == fd) {
			_invitedFds.erase(it);
			break;
		}
	}
}

bool Channel::isFdInvited(int fd) const {
	for (std::vector<int>::const_iterator it = _invitedFds.begin(); it != _invitedFds.end(); ++it) {
		if (*it == fd) {
			return true;
		}
	}
	return false;
}

void Channel::broadcastMessage(const std::string &rep, Server &server) {
	// Tronquer le message à 510 caractères pour s'assurer qu'il respecte la limite de longueur
	std::string rep_ = rep.substr(0, 510);
	std::cout << "Tentative d'envoi de " << rep_ << " à tous les membres du canal " << _name << std::endl;

	// Envoyer le message à tous les clients du canal
	for (std::vector<int>::iterator it = _clientFds.begin(); it != _clientFds.end(); ++it) {
		server.sendToClient(rep_, *it);
	}
}
