#include "server.hpp"

int main() {
    int port = 6667; // Port standard pour IRC
    Server ircServer(port);
    ircServer.start();
    return 0;
}
