#include "server.hpp"

bool Server::_gotSig = false;

Server::Server() {
	_serverSocketFd = -1;
	_port = -1;
}

void Server::startServer(char *portArg, char *pwdArg) {
	std::cout << "Initialisation du serveur...\n";
	setupSignalHandlers();
	_port = validateAndParsePort(portArg);
	_password = std::string(pwdArg);
	_serverSocketFd = -1;
	initializeServerSocket();
	std::cout << "Serveur <" << _serverSocketFd << "> Connecté" << std::endl;
	std::cout << "En attente d'une connexion...\n";

	while (!Server::_gotSig) {
		// Attente d'un événement
		int pollResult = poll(&_fds[0], _fds.size(), -1);
		if (pollResult == -1) {
			if (Server::_gotSig) {
				break; // Sortir de la boucle si un signal est reçu
			}
			throw std::runtime_error("poll() a échoué: " + std::string(strerror(errno)));
		}

		for (size_t i = 0; i < _fds.size(); ++i) {
			if (_fds[i].revents & POLLIN) {
				if (_fds[i].fd == _serverSocketFd) {
					// Nouveau client
					acceptNewClient();
				} else {
					// Données d'un client existant
					handleClientData(_fds[i].fd);
				}
			}
		}
	}

	closeAllSockets();
	std::cout << "Au revoir ~" << std::endl;
}

int Server::validateAndParsePort(const char *arg) {
	if (arg == nullptr) {
		return -1;
	}

	for (const char *p = arg; *p != '\0'; ++p) {
		if (!std::isdigit(*p)) {
			return -1;
		}
	}

	return std::atoi(arg);
}

void Server::handleSignal(int signum) {
	const char *signalName;
	switch (signum) {
		case SIGINT:
			signalName = "SIGINT";
			break;
		case SIGQUIT:
			signalName = "SIGQUIT";
			break;
		default:
			signalName = "UNKNOWN";
			break;
	}

	std::cout << std::endl << "Received signal " << signalName << " (" << signum << ")" << std::endl;
	Server::_gotSig = true;
}

void Server::setupSignalHandlers() {
	// Configuration de la structure sigaction pour gérer les signaux
	struct sigaction sa;
	sa.sa_handler = handleSignal;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	// Enregistrement du gestionnaire pour SIGINT
	if (sigaction(SIGINT, &sa, NULL) == -1) {
		std::cerr << "Failed to bind signal SIGINT" << std::endl;
		exit(EXIT_FAILURE);
	}

	// Enregistrement du gestionnaire pour SIGQUIT
	if (sigaction(SIGQUIT, &sa, NULL) == -1) {
		std::cerr << "Failed to bind signal SIGQUIT" << std::endl;
		exit(EXIT_FAILURE);
	}
}

void Server::disconnectClient(int fd) {
	std::cout << "Removing client " << fd << std::endl;

	// Retirer le descripteur de fichier de la liste des fds
	for (std::vector<struct pollfd>::iterator it = _fds.begin(); it != _fds.end(); ++it) {
		if (it->fd == fd) {
			_fds.erase(it);
			break;
		}
	}

	// Retirer le client de la liste des clients
	for (std::vector<Client>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
		if (it->getFd() == fd) {
			_clients.erase(it);
			break;
		}
	}

	// Retirer le client de tous les canaux
	for (std::vector<Channel>::iterator it = _channels.begin(); it != _channels.end(); ++it) {
		it->removeAllAssociations(fd);
	}

	// Fermer le descripteur de fichier
	if (close(fd) == -1) {
		std::cerr << "Failed to close client socket: " << strerror(errno) << std::endl;
	}
}

void Server::closeAllSockets() {
	for (size_t i = 0; i < _clients.size(); ++i) { // Fermer tous les clients
		int clientFd = _clients[i].getFd();
		if (clientFd != -1) {
			std::cout << "Client <" << clientFd << "> Disconnected" << std::endl;
			if (close(clientFd) == -1) {
				std::cerr << "Failed to close client socket: " << strerror(errno) << std::endl;
			}
		}
	}
	if (_serverSocketFd != -1) { // Fermer le socket du serveur
		std::cout << "Server <" << _serverSocketFd << "> Disconnected" << std::endl;
		if (close(_serverSocketFd) == -1) {
			std::cerr << "Failed to close server socket: " << strerror(errno) << std::endl;
		}
	}
	_serverSocketFd = -1; ///////////////////////////
}

void Server::initializeServerSocket() {
	struct sockaddr_in addr;
	struct pollfd newPoll;
	addr.sin_family = AF_INET; // Utilisation de l'IPv4
	addr.sin_port = htons(this->_port); // Conversion du port en ordre de bytes réseau (big endian)
	addr.sin_addr.s_addr = INADDR_ANY; // Permet aux clients de se connecter à n'importe quelle adresse utilisée par la machine locale

	_serverSocketFd = socket(AF_INET, SOCK_STREAM, 0);
	if (_serverSocketFd == -1) {
		throw std::runtime_error("Failed to create socket: " + std::string(strerror(errno)));
	}

	// setsockopt: Permet de relancer le serveur immédiatement après l'avoir tué
	int optval = 1;
	if (setsockopt(_serverSocketFd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1) {
		throw std::runtime_error("Failed to set option (SO_REUSEADDR) on socket: " + std::string(strerror(errno)));
	}

	// Configurer le socket en mode non-bloquant
	if (fcntl(_serverSocketFd, F_SETFL, O_NONBLOCK) == -1) {
		throw std::runtime_error("Failed to set option (O_NONBLOCK) on socket: " + std::string(strerror(errno)));
	}

	if (bind(_serverSocketFd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
		throw std::runtime_error("Failed to bind socket: " + std::string(strerror(errno)));
	}

	if (listen(_serverSocketFd, SOMAXCONN) == -1) {
		throw std::runtime_error("listen() failed: " + std::string(strerror(errno)));
	}

	newPoll.fd = _serverSocketFd; // Ajouter le socket serveur au pollfd
	newPoll.events = POLLIN; // POLLIN pour lire les données
	newPoll.revents = 0; // Initialiser revents à 0
	_fds.push_back(newPoll); // Ajouter le socket serveur au pollfd
}

void Server::acceptNewClient() {
	Client cli;
	struct sockaddr_in cliAddress;
	struct pollfd newPoll;
	socklen_t len = sizeof(cliAddress);

	int incofd = accept(_serverSocketFd, (sockaddr *)&cliAddress, &len); // accept the new client
	if (incofd == -1) {
		std::cerr << "accept() failed: " << strerror(errno) << std::endl;
		return;
	}

	// set the socket option (O_NONBLOCK) for non-blocking socket
	if (fcntl(incofd, F_SETFL, O_NONBLOCK) == -1) {
		std::cerr << "fcntl() failed: " << strerror(errno) << std::endl;
		close(incofd);
		return;
	}

	newPoll.fd = incofd;
	newPoll.events = POLLIN;
	newPoll.revents = 0;

	cli.setFd(incofd);
	cli.setIpAddr(inet_ntoa(cliAddress.sin_addr));
	_clients.push_back(cli);
	_fds.push_back(newPoll);

	std::cout << "Client <" << incofd << "> Connected" << std::endl;
}

void Server::handleClientData(int fd) {
	const int BUFFER_SIZE = 1024;
	char buff[BUFFER_SIZE];
	std::memset(buff, 0, sizeof(buff));

	// Receives data into buffer
	ssize_t bytes = recv(fd, buff, sizeof(buff) - 1, 0);

	// Check if client disconnected
	if (bytes <= 0) {
		std::cout << "Client <" << fd << "> Disconnected" << std::endl;
		try {
			disconnectClient(fd);
		} catch (std::runtime_error &e) {
			std::cerr << "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@Error: " << e.what() << std::endl;
		}
		// disconnectClient(fd);
		
	} else {
		buff[bytes] = '\0';
		std::cout << "Client <" << fd << "> Data:\n" << "`" << buff << "`" << std::endl;

		// Process data
		if (fdExists(fd)) {
			getClientByFd(fd).processClientBuffer(buff, *this);
		}
	}
}

bool Server::isPasswordValid(const std::string &pwd) const {
	return pwd == _password;
}

Client& Server::findClientByNick(const std::string& nick) {
	for (std::vector<Client>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
		if (it->getNick() == nick) {
			return *it;
		}
	}
	throw std::runtime_error("Client not found (by nick)");
}

bool Server::doesNickExist(const std::string &nick) {
	for (size_t i = 0; i < _clients.size(); i++){
		if (_clients[i].getNick() == nick) {
			return (true);
		}
	}
	return (false);
}

bool Server::channelExists(const std::string &name) {
	for (size_t i = 0; i < _channels.size(); i++){
		if (_channels[i].getName() == name) {
			return (true);
		}
	}
	return (false);
}

Channel &Server::getChannelByName(const std::string &name) {
	for (size_t i = 0; i < _channels.size(); i++){
		if (_channels[i].getName() == name) {
			return (_channels[i]);
		}
	}
	throw std::runtime_error("Channel not found");
}

Channel &Server::createChannel(const std::string &name) {
	Channel res(name);
	_channels.push_back(res);
	return (_channels[_channels.size() - 1]);
}

void Server::removeChannel(const std::string &name) {
	for (size_t i = 0; i < _channels.size(); i++){
		if (_channels[i].getName() == name) {
			_channels.erase(_channels.begin() + i);
			break;
		}
	}
}

void Server::sendToClient(const std::string &rep, int fd) {
	std::string rep_ = rep.substr(0, 510);
	std::cout << "attempting to reply with " << rep_ << " to " << fd <<std::endl;
	rep_ += "\r\n";
	ssize_t sentBytes = send(fd, rep_.c_str(), rep_.length(), 0);
	if (sentBytes < 0) {
		std::cerr << "Error sending reply to client <" << fd << ">" << std::endl;
	}
}

void Server::sendToAll(const std::string &rep) {
	std::string rep_ = rep.substr(0, 510);
	std::cout << "attempting to reply with " << rep_ << " to all" << std::endl;
	rep_ += "\r\n";
	for (size_t i = 0; i < _clients.size(); i ++) {
		ssize_t sentBytes = send(_clients[i].getFd(), rep_.c_str(), rep_.length(), 0);
		if (sentBytes < 0) {
			std::cerr << "Error sending reply to client <" << _clients[i].getFd() << ">" << std::endl;
		}
	}
}

bool Server::fdExists(int fd) {
	for (size_t i = 0; i < _clients.size(); i++){
		if (_clients[i].getFd() == fd) {
			return (true);
		}
	}
	return (false);
}

Client &Server::getClientByFd(int fd) {
	for (size_t i = 0; i < _clients.size(); i++){
		if (_clients[i].getFd() == fd) {
			return (_clients[i]);
		}
	}
	throw std::runtime_error("Client not found (by fd)");
}

void Server::op(int fd) {
	_globalOpFds.push_back(fd);
}

bool Server::fdIsGlobalOp(int fd) {
	for (size_t i = 0; i < _globalOpFds.size(); ++i) {
		if (_globalOpFds[i] == fd) {
			return true;
		}
	}
	return false;
}

std::vector<Channel> &Server::getChannels() {
	return _channels;
}

void Server::debugPrintFds() {
	std::string res = "Fds:";
	for (size_t i = 0; i < _fds.size(); ++i) {
		res += "`";
		res += util::streamItoa(_fds[i].fd);
		res += "`, ";
	}
	std::cout << res << std::endl;
}

void Server::debugPrintChannels() {
	std::string res = "Channels:";
	for (size_t i = 0; i < _channels.size(); ++i) {
		res += "`";
		res += _channels[i].getName();
		res += "`, ";
	}
	std::cout << res << std::endl;
}
