#include <string>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include "connection.hpp"

int Connection::sendMessage(Message* msg){
    if(!isInitialized) return -2;
    
    std::string str;
    str.push_back(msg->code);
    int len = msg->message.size();
    str.push_back(((len >> 24) & 0xFF));
    str.push_back(((len >> 16) & 0xFF));
    str.push_back(((len >> 8) & 0xFF));
    str.push_back((len & 0xFF));
    str.append(msg->message);
    int x = 0;
    x += str[1];
    x = (x << 8);
    x += str[2];
    x = (x << 8);
    x += str[3];
    x = (x << 8);
    x += str[4];


    if(send(this->socket, &str[0], str.size(), 0) == -1){
        std::cerr << "Socket closed" << std::endl;
        close(this->socket);
        return -1;
    }
   
    return 0;
}