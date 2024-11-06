#ifndef IRCSERV_MESSAGES_HPP
#define IRCSERV_MESSAGES_HPP

#define ERR_NONICKNAMEGIVEN						"431 :No nickname given\r\n"
#define ERR_NICKNAMEINUSE(nick)					"433 * " + nick + " :Nickname is already in use\r\n"
#define RPL_WELCOME(nick, user)					"001 * :Welcome to the Internet Relay Network " + nick + "!" + user + "@42.lausanne.ch\r\n"
#define ERR_ALREADYREGISTERED					"462 :Unauthorized command (already registered)\r\n"
#define ERR_NEEDMOREPARAMS(client, cmd)			"461 " + client + " " + cmd " :Not enough parameters\r\n"
#define ERR_PASSWDMISMATCH						"464 * :Password incorrect\r\n"
#define CH_NICK(o_nick, n_nick)					o_nick + " changed his nickname to " + n_nick + "\r\n"
#define ERR_BADCHANNELKEY(channel)				"475 " + channel + " :Cannot join channel (+k)\r\n"
#define RPL_NAMREPLY(client, channels, admin)	"353 " + client + " = " + channels + " :" + admin + "\r\n"
#define RPL_NAMREPLY(client, channels, admin)	"353 " + client + " = " + channels + " :" + admin + "\r\n"
#define RPL_ENDOFNAMES(client, channel)			"366 " + client + " " + channel + " :End of /NAMES list.\r\n"
#define ERR_NOSUCHCHANNEL(source, channel)		"403 " + source + " " + channel + " :No such channel\r\n"
#define ERR_NOSUCHNICK(source, target)			"401 " + source + " " + target + ":No such nick/channel\r\n"
#define ERR_NOTONCHANNEL(channel)				"442 " + channel + " :You're not on that channel\r\n"
#define ERR_UNKNOWNCOMMAND(source, command)		"421 " + source + " " + command + " :Unknown command\r\n"
#define ERR_NOORIGIN							"409 :No origin specified\r\n"
#define ERR_NOSUCHSERVER(serv_addr)				"402 " + serv_addr + " :No such server\r\n"
#define KICK_MSG(target, channel)				":" " Kick " + target + " from " + channel + "\r\n"
#define ERR_CHANOPRIVSNEEDED(client, channel)	"482 " + client + " " + channel + " :You're not channel operator\r\n"
#define ERR_USERNOTINCHANNEL(nick, channel)		"441 " + nick + " " + channel + " :They aren't on that channel\r\n"
#define RPL_MODE(source, channel, modes, args)	":" + source + " MODE " + channel + " " + modes + " " + args + "\r\n"
#define ERR_CANNOTSENDTOCHAN(source, channel)	"404 " + source + " " + channel + " :Cannot send to channel\r\n"
#define ERR_CHANNELISFULL(channel)				"471 " + channel + " :Cannot join channel (+l)\r\n"

// Confirmation
#define CNF_JOIN(client, channel)				":" + client + " JOIN :" + channel + "\r\n"
#define CNF_PRIVMSG(source, target, message)	":" + source + " PRIVMSG " + target + " :" + message + "\r\n"

#define ERR_NOCHANMODES(channel)				"477 " + channel + " :Channel doesn't support modes\r\n"

#endif //IRCSERV_MESSAGES_HPP
