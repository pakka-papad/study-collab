#include <string>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <nlohmann/json.hpp>
#include "connection.hpp"
#include "../constants.hpp"
#include "chat_db.cpp"

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

    std::cerr << "Sending message code:" << int(msg->code) << " msg:" << msg->message << std::endl;
    int sendRes = send(this->socket, &str[0], str.size(), 0);

    if(sendRes == -1){
        std::cerr << "Socket closed" << std::endl;
        close(this->socket);
        return -1;
    }
   
    return 0;
}

void Connection::listenIncomingMessages(){
    std::cerr << "Started listening on socket " << socket << std::endl;

    const int BUF_SIZE = 2048;

    char buffer[BUF_SIZE+1];
    buffer[BUF_SIZE] = '\0';

    char code = 0;
    int msgLen = 0;
    std::string msg;
    int nextByte = 0;

    while(true){
        int bytesRead = recv(socket, buffer, BUF_SIZE, 0);
        if (bytesRead == -1) {
            close(socket);
            std::cerr << "Closed socket " << socket << std::endl;
            pthread_exit(NULL);
        }
        if(bytesRead == 0) continue;
        std::cerr << "bytesRead: " << bytesRead << std::endl;
        for(int i = 0; i < bytesRead; i++){
            if(nextByte == 0){
                code = buffer[i];
                nextByte++;
            } else if(nextByte == 1){
                msgLen = 0;
                msgLen += (uint8_t)buffer[i];
                nextByte++;
            } else if(nextByte == 2){
                msgLen = (msgLen << 8);
                msgLen += (uint8_t)buffer[i];
                nextByte++;
            } else if(nextByte == 3){
                msgLen = (msgLen << 8);
                msgLen += (uint8_t)buffer[i];
                nextByte++;
            } else if(nextByte == 4){
                msgLen = (msgLen << 8);
                msgLen += (uint8_t)buffer[i];
                msg.clear();
                nextByte++;
            } else {
                msg.push_back(buffer[i]);
                nextByte++;
                if(msg.size() == msgLen){
                    // handle message
                    if(code == NEW_MESSAGE){
                        nlohmann::json data = nlohmann::json::parse(msg);
                        Chat* chat = new Chat();
                        chat->message = data["chat"]["message"];
                        chat->createdBy = data["chat"]["created_by"];
                        chat->createdAt = data["chat"]["created_at"];
                        std::string groupId = data["group_id"];
                        ChatDB::getChatDB()->saveChat(chat, groupId);
                    } else {
                        Message* newMsg = new Message(code,msg);
                        msgQ.push(newMsg);
                    }
                    std::cerr << "Received message len:" << msgLen << " code:" << int(code) << " msg:" << msg << std::endl;

                    nextByte = 0;
                    code = 0;
                    msgLen = 0;
                    msg.clear();
                }
            }
        }
    }
    pthread_exit(NULL);
}

void Connection::terminate(){
    Message logout(LOGOUT,"{}");
    sendMessage(&logout);
    pthread_cancel(listenerThreadId);
    close(socket);
}