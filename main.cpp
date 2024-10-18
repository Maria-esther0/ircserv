#include "server.hpp"

int main(int ac, char **av)
{
    if (ac != 3)
    {
        std::cerr<<RED<< "ERROR !"<<std::endl<<WHITE<<"Usage: " << av[0] << " <port> <password>\n";
        _exit(1);
    }    
    
    if (Server::get_port(av[1]) == -1)
    {
        std::cerr<<RED<<"ERROR !"<<std::endl<<WHITE<<"Invalid port!\n";
        _exit(1);
    }
    
    int port = 6667; // Port standard pour IRC
    Server ircServer(port);
    try
    {    ircServer.start(av[1], av[2]);}
    catch(const std::exception& e)
    {
        ircServer.~Server();
        std::cerr << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
