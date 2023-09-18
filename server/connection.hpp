#include <sys/socket.h>

class Connection {
    public:
    sockaddr_in clientAddress;
    int clientSocket;
};