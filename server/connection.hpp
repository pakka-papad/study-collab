#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include <sys/socket.h>
#include <mutex>
#include "message.hpp"

class Connection {
    public:
    sockaddr_in clientAddress;
    int clientSocket;
    std::mutex sendMutex;
};

#endif 
