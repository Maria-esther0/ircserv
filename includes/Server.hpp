#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream>
#include <string>

class Server
{
    public:
        Server();
        Server(int ac, char** av);
        ~Server();
        void run();
        void printUsage();
        void printError(std::string error);
};

#endif