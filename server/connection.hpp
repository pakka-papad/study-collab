#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include <sys/socket.h>

class Connection {
    public:
    sockaddr_in clientAddress;
    int clientSocket;
};

#endif 
