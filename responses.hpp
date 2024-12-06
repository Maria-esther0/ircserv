#ifndef RESPONSES_HPP
#define RESPONSES_HPP

#define ERR_NOTREGISTERED                                                       "451 :You have not registered"
#define ERR_ALREADYREGISTERED                                                   "462 :You may not register"
#define ERR_PASSWDMISMATCH                                                      "ERROR :Access denied: Bad password?"
#define ERR_NONICKNAMEGIVEN                                                     "431 :Nickname not given"
#define ERR_NICKNAMEINUSE(nick, attempted_nick)                                 "433 " + nick + " " + attempted_nick + " :Nickname already in use"

#define ERR_UNKNOWNCOMMAND(command)                                             "421 " + command + " :Unknown command"
#define ERR_NEEDMOREPARAMS(command)                                             "461 " + command + " :Not enough parameters"

#define ERR_TOOMANYCHANNELS(channel)                                            "405 " + channel + " :You have joined too many channels"
#define ERR_NOTONCHANNEL(channel)                                               "442 " + channel + " :You're not in that channel"
#define ERR_NOSUCHCHANNEL(channel)                                              "403 " + channel + " :No such channel"
#define ERR_BADCHANNELKEY(nick, channel)                                        "475 " + nick + " #" + channel + " :Cannot join channel (+k) -- Wrong channel key"
#define ERR_CHANNELISFULL(nick, channel)                                        "471 " + nick + " #" + channel + " :Cannot join channel (+l) -- Channel is full, try later\r"
#define ERR_CANNOTSENDTOCHAN(channel)                                           "404 " + channel + " :Cannot send to channel"
#define ERR_CHANOPRIVSNEEDED(channel, nick)                                     "482 " + nick + " #" + channel + " :You are not channel operator"

#define ERR_NOSUCHNICK(nick, target)                                            "401 " + nick + " " + target + " :No such nick or channel name"
#define ERR_USERNOTINCHANNEL(nickname, channel)                                 "441 " + nickname + " #" + channel + " :They aren't on that channel"
#define ERR_USERONCHANNEL(nickname, channel)                                    "443 " + nickname + " #" + channel + " :User already on that channel"


#define RPL_CAP                                                                 "CAP * LS :multi-prefix"
#define RPL_CAP_REQ                                                             "CAP * ACK :multi-prefix"

#define RPL_WELCOME(nick)                                                       "001 " + nick + " :Welcome to ft_irc"

/*
 * 353    RPL_NAMREPLY
							"( "=" / "*" / "@" ) <channel>
							 :[ "@" / "+" ] <nick> *( " " [ "@" / "+" ] <nick> )
				 - "@" is used for secret channels, "*" for private
					 channels, and "=" for others (public channels).*/
#define RPL_NAMREPLY(nick, channel, users)                                      "353 " + nick + " = #" + channel + " :" + users //3
#define RPL_ENDOFNAMES(nick, channel)                                           "366 " + nick + " #" + channel + " :End of /NAMES list." //4
#define RPL_JOIN(nick, user, host, channel)                                     ":" + nick + "!~" + user + "@" + host + " JOIN :#" + channel //1
#define SEND_TOPIC(nick, channel, topic)                                        "332 " + nick + " #" + channel + " :" + topic // 2

#define RPL_MOTD_MISSING                                                        "422 :MOTD file is missing"

/* Command Responses */


#define RPL_PART(nick, user, host, channel, message)                            ":" + nick + "!~" + user + "@" + host + " PART #" + channel + " :" + message
#define RPL_QUIT(nick, user, host, channel, message)                            ":" + nick + "!~" + user + "@" + host + " QUIT #" + channel + " :" + message
#define RPL_PING                                                                ":ft_irc PONG  ft_irc :ft_irc"
#define RPL_PRIVMSG(nick, user, host, channel, message)                         ":" + nick + "!~" + user + "@" + host + " PRIVMSG #" + channel + " :" + message
#define PRIV_MSG_DM(nick, user, host, target, message)                          ":" + nick + "!~" + user + "@" + host + " PRIVMSG #" + target + " :" + message
#define RPL_NOTICE(target, message)                                             ":NOTICE " + target + " :" + message
#define RPL_KICK(nick, user, host, channel, target)                             ":" + nick + "!~" + user + "@" + host + " KICK #" + channel + " " + target + " :"
#define RPL_MODE(nick, user, host, channel, mode, args)                         ":" + nick + "!~" + user + "@" + host + " MODE #" + channel + " " + mode + " " + args
#define RPL_MODE_OPER(user)                                                     "MODE " + user + " :+o"
#define RPL_OPER(user)                                                          "381 " + user + " :You are now an IRC Operator"
#define RPL_CHANNEL_MODES(user, channel, modes)                                 "324 " + user +" #" + channel + " " + modes
#define RPL_WHO(requesting_nick, channel, user, host, nick, flags, realname)    "352 " + requesting_nick + " #" + channel + " ~" + user + " " + host + " ft_irc " + nick +" H :0 " + realname
#define END_WHO(nick, channel)                                                  "315 " + nick + " #" + channel + " :End of WHO list"
#define END_BAN(nick, channel)                                                  "368 " + nick +" #" + channel + " :End of channel ban list"
#define RPL_TOPIC(nick, user, host, channel, topic)                             ":" + nick + "!~" + user + "@" + host + " TOPIC #" + channel + " :" + topic
#define RPL_TOPIC_COMMAND(nick, channel, topic)                                 "332 " + nick + " #" + channel + " :" + topic
#define RPL_INVITE(nick, target_nick, target_user, target_host, channel)        ":" + target_nick + "!~" + target_user + "@" + target_host + " 341 " + nick + " " + target_nick + " #" + channel
#define INVITE(nick, user, host, target, channel)                               ":" + nick + "!~" + user + "@" + host + " INVITE " + target + " #" + channel
#define RPL_MUST_BE_INVITED(nick, channel)                                      "473 " + nick + " #" + channel + " :Cannot join channel (+i) -- Invited users only"
#define RPL_NICK(nick, user, host, newnick)                                     ":" + nick + "!~" + user + "@" + host + " NICK :" + newnick
#define RPL_NICK_MODES(nick, mode)                                              "221 " + nick + " " + mode
#define RPL_CANT_MODE_OTHER(nick)                                               "502 " + nick + " :Can't set/get mode for other users"
#define RPL_UNKNOWN_USER_MODE(nick, modechar)                                   "501 " + nick + " :Unknown mode \"+" + modechar + "\""
#define ERR_UNKNOWN_CHANNEL_MODE(modechar, channel)                             "472 " + modechar + " :is unknown mode char to me for " + channel

#define ERROR(msg)                                                              "ERROR: " + msg

#endif //FT_IRC_// RESPONSES_H
