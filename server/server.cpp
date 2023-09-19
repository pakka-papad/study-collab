#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <mutex>
#include <nlohmann/json.hpp>
#include "connection.hpp"
#include "message.hpp"
#include "database.cpp"
#include "../constants.hpp"

pthread_mutex_t mtx;

void sendMessage(Connection* conn, const Message &msg, std::string &email){
    std::string str;
    str.push_back(msg.code);
    int len = msg.message.size();
    str.push_back(((len >> 24) & 0xFF));
    str.push_back(((len >> 16) & 0xFF));
    str.push_back(((len >> 8) & 0xFF));
    str.push_back((len & 0xFF));
    str.append(msg.message);

    pthread_mutex_lock(&mtx);
    std::cerr << "Sending message on socket " << conn->clientSocket << " code:" << int(msg.code) << " msg:" << msg.message << std::endl;
    pthread_mutex_unlock(&mtx);

    if(send(conn->clientSocket, &str[0], str.size(), 0) == -1){
        pthread_mutex_lock(&mtx);
        std::cerr << "Closed socket " << conn->clientSocket << ". Could not send data." << std::endl;
        pthread_mutex_unlock(&mtx);
        Database::getDatabse()->logoutUser(email);
        close(conn->clientSocket);
    }
}

void handleMessage(Connection* conn, const Message &msg, std::string &email){
    switch(msg.code){
        case LOGIN_REQUEST: {
            nlohmann::json data = nlohmann::json::parse(msg.message);
            std::string reqEmail = data["email"];
            std::string password = data["password"];
            bool success = Database::getDatabse()->loginOrRegisterUser(conn, reqEmail, password);
            if(success) {
                email = reqEmail;
                pthread_mutex_lock(&mtx);
                std::cerr << "Socket " << conn->clientSocket << " now set to email " << email << std::endl;
                pthread_mutex_unlock(&mtx);
            }
            Message replyMsg((success ? LOGIN_SUCCESS : LOGIN_FAILED),"{}");
            sendMessage(conn, replyMsg, email);
            break;
        }
        case CREATE_GROUP_REQUEST: {
            nlohmann::json data = nlohmann::json::parse(msg.message);
            std::string groupName = data["group_name"];
            bool success = Database::getDatabse()->createGroup(email, groupName);
            Message replyMsg((success ? CREATE_GROUP_SUCCESS : CREATE_GROUP_FAILED), "{}");
            sendMessage(conn, replyMsg, email);
            break;
        }
        case REQUEST_ALL_GROUPS: {
            auto groups = Database::getDatabse()->fetchAllGroups(email);
            nlohmann::json res;
            for(auto &group: groups){
                res.push_back(group.toJson());
            }
            Message reply(REPLY_ALL_GROUPS, res.dump());
            sendMessage(conn, reply, email);
        }
        case REQUEST_PARTICIPATING_GROUPS: {
            auto groups = Database::getDatabse()->fetchParticipatingGroups(email);
            nlohmann::json res;
            for(auto &group: groups){
                res.push_back(group.toJson());
            }
            Message reply(REPLY_PARTICIPATING_GROUPS, res.dump());
            sendMessage(conn, reply, email);
        }
        case REQUEST_NON_PARTICIPATING_GROUPS: {
            auto groups = Database::getDatabse()->fetchNonParticipatingGroups(email);
            nlohmann::json res;
            for(auto &group: groups){
                res.push_back(group.toJson());
            }
            Message reply(REPLY_NON_PARTICIPATING_GROUPS, res.dump());
            sendMessage(conn, reply, email);
            break;
        }
        case JOIN_GROUP_REQUEST: {
            nlohmann::json data = nlohmann::json::parse(msg.message);
            std::string groupId = data["group_id"];
            bool success = Database::getDatabse()->addToGroup(email, groupId);
            Message reply((success ? JOIN_GROUP_SUCCESS : JOIN_GROUP_FAILED), "{}");
            sendMessage(conn, reply, email);
        }
        default:
            break;
    }
}

void* handleConnection(void* arg){
    Connection* conn = static_cast<Connection*>(arg);
    pthread_mutex_lock(&mtx);
    std::cerr << "Client connected. Socket-id: " << conn->clientSocket << std::endl;
    pthread_mutex_unlock(&mtx);

    std::string email;

    const int BUF_SIZE = 2048;

    char buffer[BUF_SIZE+1];
    buffer[BUF_SIZE] = '\0';

    char code = 0;
    int msgLen = 0;
    std::string msg;
    int nextByte = 0;

    while(true){
        int bytesRead = recv(conn->clientSocket, buffer, BUF_SIZE, 0);
        if (bytesRead == -1) {
            close(conn->clientSocket);
            pthread_mutex_lock(&mtx);
            std::cerr << "Closed socket " << conn->clientSocket << std::endl;
            pthread_mutex_unlock(&mtx);
            Database::getDatabse()->logoutUser(email);
            pthread_exit(NULL);
            return NULL;
        }
        if(bytesRead == 0) continue;
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
                    Message newMsg(code,msg);
                    pthread_mutex_lock(&mtx);
                    std::cerr << "Message on socket: " << conn->clientSocket << " len:" << msgLen << " code:" << int(code) << " msg:" << msg << std::endl;
                    pthread_mutex_unlock(&mtx);
                    handleMessage(conn, newMsg, email);

                    nextByte = 0;
                    code = 0;
                    msgLen = 0;
                    msg.clear();
                }
            }
        }
    }

    return NULL;
}

int main(){
    freopen("logs.txt", "w", stderr);

    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        std::cerr << "Error creating server socket." << std::endl;
        return EXIT_FAILURE;
    }

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    serverAddress.sin_addr.s_addr = INADDR_ANY; 

    if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
        std::cerr << "Error binding socket." << std::endl;
        close(serverSocket);
        return EXIT_FAILURE;
    }

    if (listen(serverSocket, 6) == -1) {
        std::cerr << "Error listening for connections." << std::endl;
        close(serverSocket);
        return 1;
    }

    std::cout << "Server is listening on port 8080..." << std::endl;

    while(true){
        sockaddr_in clientAddress;
        socklen_t clientAddressLength = sizeof(clientAddress);
        int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddress, &clientAddressLength);

        if (clientSocket == -1) {
            std::cerr << "Error accepting client connection." << std::endl;
            close(serverSocket);
            return EXIT_FAILURE;
        }

        pthread_t clientThread;
        Connection* args = new Connection();
        args->clientAddress = clientAddress;
        args->clientSocket = clientSocket;
        if (pthread_create(&clientThread, NULL, handleConnection, args) != 0) {
            std::cerr << "Error creating thread." << std::endl;
            close(clientSocket);
            continue;
        }

        // int time = 0;
        // while(time < 10){
        //     time++;
        //     std::string message = "Hello, client! " + std::to_string(time);
        //     if (send(clientSocket, &message[0], message.size(), 0) == -1) {
        //         std::cerr << "Error sending data to client." << std::endl;
        //         close(clientSocket);
        //     }
        // }
        // close(clientSocket);

    }
   
    close(serverSocket);
}