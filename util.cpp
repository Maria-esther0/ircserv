#include "util.hpp"

std::string util::trimSpace(const std::string &str) {
	if (str.empty()) {
		return (str);
	}
	std::string::size_type i, j;
	i = 0;
	while (i<str.size() && isspace(str[i])) {
		i ++;
	}
	if (i == str.size()) {
		return std::string(); // empty string
	}
	j = str.size() - 1;
	while (isspace(str[j])) {
		j --;
	}
	return (str.substr(i, j - i + 1));
}

std::string util::removeFirstChar(const std::string &str) {
	if (str.empty()) {
		return str;
	}
	const char *newStr = str.c_str() + 1;
	return std::string(newStr);
}

std::string util::streamItoa(int n) {
	std::stringstream ss;
	ss << n;
	return ss.str();
}
