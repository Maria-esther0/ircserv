#ifndef UTIL_HPP
#define UTIL_HPP

#include <string>
#include <sstream>

class util {
public:
	static std::string trimSpace(const std::string &str);
	static std::string removeFirstChar(const std::string &str);
	static std::string streamItoa(int n);
};


#endif //FT_IRC_UTIL_H
