#include <iostream>
#include <ncurses.h>
#include <vector>
#include <string>
#include <unistd.h>
#include "menu.cpp"
#include "connection.hpp"
#include "../constants.hpp"
#include <nlohmann/json.hpp>
#include <mutex>
#include <functional>
#include <atomic>
#include <filesystem>
#include "chat.hpp"
#include "chat_db.cpp"
#include "safe_queue.cpp"
#include "progress.cpp"

class Screen {
    public:
    virtual Screen* display() = 0;
};

class CreateGroup: public Screen {
    public:
    Screen* display(){
        bool again = true;
        auto conn = Connection::getConnection();
        std::string gropupName;
        while(again){
            std::cout << "Enter group name: ";
            std::cin >> gropupName;
            nlohmann::json data;
            data["group_name"] = gropupName;
            Message reqMsg(CREATE_GROUP_REQUEST,data.dump());
            conn->sendMessage(&reqMsg);
            auto reply = conn->msgQ.pop();
            if(reply->code == CREATE_GROUP_SUCCESS){
                std::cout << "Group created successfully" << std::endl;
                again = false;
                sleep(3);
            } else {
                char tryAgain;
                std::cout << "Try again? [y|n]: ";
                std::cin >> tryAgain;
                if(tryAgain == 'n'){
                    again = false;
                }
            }
        }
        return NULL;
    }   
};

class JoinGroup: public Screen {
    public:
    Screen* display(){
        auto conn = Connection::getConnection();
        std::cout << "Fetching groups list..." << std::endl;

        Message req(REQUEST_NON_PARTICIPATING_GROUPS,"{}");
        conn->sendMessage(&req);
        auto reply = conn->msgQ.pop();
        nlohmann::json data = nlohmann::json::parse(reply->message);
        std::vector<std::string> groups;
        std::vector<std::string> groupIds;
        std::string groupName, createdBy, groupId;
        for(auto &group: data){
            groupName = group["group_name"];
            createdBy = group["created_by"];
            groupId = group["group_id"];
            groups.push_back(groupName + " (created by: " + createdBy + ")");
            groupIds.push_back(groupId);
        }

        bool again = true;
        while(again){
            int grpPos = showMenu(groups);
            clear();
            if(grpPos == -1) return NULL;
            nlohmann::json joinData;
            joinData["group_id"] = groupIds[grpPos];
            Message joinReq(JOIN_GROUP_REQUEST, joinData.dump());
            conn->sendMessage(&joinReq);
            auto joinRep = conn->msgQ.pop();
            if(joinRep->code == JOIN_GROUP_SUCCESS){
                std::cout << "Group joined successfully" << std::endl;
                sleep(3);
                again = false;
            } else {
                char tryAgain;
                std::cout << "Try again? [y|n]: ";
                std::cin >> tryAgain;
                if(tryAgain == 'n'){
                    again = false;
                }
            }
        }
        return NULL;
    }
};

class ChatRoom: public Screen {
    private:
    std::mutex mtx;
    public:
    Screen* display(){
        auto conn = Connection::getConnection();
        std::cout << "Fetching groups list..." << std::endl;

        Message req(REQUEST_PARTICIPATING_GROUPS,"{}");
        conn->sendMessage(&req);
        auto reply = conn->msgQ.pop();
        nlohmann::json data = nlohmann::json::parse(reply->message);
        std::vector<std::string> groups;
        std::vector<std::string> groupIds;
        std::string groupName, createdBy, groupId;
        for(auto &group: data){
            groupName = group["group_name"];
            createdBy = group["created_by"];
            groupId = group["group_id"];
            groups.push_back(groupName + " (created by: " + createdBy + ")");
            groupIds.push_back(groupId);
        }

        int grpPos = showMenu(groups);
        if(grpPos == -1) return NULL;
        system("clear");
        auto chats = ChatDB::getChatDB()->fetchChats(groupIds[grpPos]);
        std::cout << "Type \"quit\" (without quotes) to quit the chat room" << std::endl;
        for(auto &chat: chats){
            std::cout << "(" << chat.createdBy << ") " << chat.message << std::endl;
        }
        std::function<void(Chat,std::string)> listener = [&](Chat newChat, std::string chatGroupId){
            if(chatGroupId != groupIds[grpPos]) return;
            mtx.lock();
            std::cout << "(" << newChat.createdBy << ") "  << newChat.message << std::endl;
            mtx.unlock();
        };
        ChatDB::getChatDB()->addListener(&listener);

        while(true){
            std::string input;
            getline(std::cin, input);
            if(input == "quit"){
                break;
            }
            if(input == "") continue;
            nlohmann::json chatData;
            chatData["group_id"] = groupIds[grpPos];
            chatData["message"] = input;
            Message chatMsg(SEND_MESSAGE, chatData.dump());
            Connection::getConnection()->sendMessage(&chatMsg);
        }

        
        mtx.lock();
        ChatDB::getChatDB()->removeListener();
        mtx.unlock();
        return NULL;
    }
};

class ShareFile: public Screen {
    public:
    Screen* display(){
        auto conn = Connection::getConnection();
        std::cout << "Fetching groups list..." << std::endl;

        Message req(REQUEST_PARTICIPATING_GROUPS,"{}");
        conn->sendMessage(&req);
        auto reply = conn->msgQ.pop();
        nlohmann::json data = nlohmann::json::parse(reply->message);
        std::vector<std::string> groups;
        std::vector<std::string> groupIds;
        std::string groupName, createdBy, groupId;
        for(auto &group: data){
            groupName = group["group_name"];
            createdBy = group["created_by"];
            groupId = group["group_id"];
            groups.push_back(groupName + " (created by: " + createdBy + ")");
            groupIds.push_back(groupId);
        }

        int grpPos = showMenu(groups);
        if(grpPos == -1) return NULL;
        system("clear");
        
        std::string path;
        while(path == ""){
            system("clear");
            std::cout << "Enter file path: ";
            getline(std::cin, path);    
        }
        

        FILE* file = fopen(path.c_str(), "rb");
        if(file == NULL){
            std::cout << "Error opening file" << std::endl;
            sleep(1);
            return NULL;
        }

        std::filesystem::path pathObj(path);
        std::string filename = pathObj.filename().string();

        int64_t totalBytes = 0, bytesRead = 0;
        if(fseek(file, 0, SEEK_END) != 0){
            std::cout << "Error occurred. Aborting file transmission." << std::endl;
            fclose(file);
            sleep(1);
            return NULL;
        }

        totalBytes = ftell(file);

        if(totalBytes == -1){
            std::cout << "Error occurred. Aborting file transmission." << std::endl;
            fclose(file);
            sleep(1);
            return NULL;
        }

        if (fseek(file, 0, SEEK_SET) != 0) {
            std::cout << "Error occurred. Aborting file transmission." << std::endl;
            fclose(file);
            sleep(1);
            return NULL;
        }
        
        char buffer1[2048], buffer2[2048];
        int bytesRead1 = 0, bytesRead2 = 0;

        bytesRead1 = fread(buffer1, 1, sizeof(buffer1), file);
        std::string fileChunk;

        while(bytesRead1 > 0){
            bytesRead2 = (file != NULL ? fread(buffer2, 1, sizeof(buffer2), file) : 0);
            fileChunk.clear();
            for(int i = 0; i < bytesRead1; i++){
                fileChunk.push_back(buffer1[i]);
            }
            nlohmann::json data;
            data["filename"] = filename;
            data["group_id"] = groupIds[grpPos];
            data["chunk"] = fileChunk;
            data["last"] = (bytesRead2 <= 0 ? 1 : 0);
            Message chunk(SAVE_FILE, data.dump());
            conn->sendMessage(&chunk);

            bytesRead += bytesRead1;
            updateProgressBar(bytesRead, totalBytes);

            std::swap(buffer1, buffer2);
            std::swap(bytesRead1, bytesRead2);
        }

        if(bytesRead1 == -1){
            std::cout << "\nError sending file" << std::endl;
            sleep(1);
            return NULL;
        }

        std::cout << "\nFile sent" << std::endl;
        auto fileShareReply = conn->msgQ.pop();
        if(fileShareReply->code == SAVE_FILE_SUCCESS){
            std::cout << "File sent successfully" << std::endl;
        } else {
            std::cout << "Error occurred while sending file" << std::endl;
        }
        sleep(1);
        return NULL;
    }
};

class TaskChoice: public Screen {
    private:
    std::vector<std::string> options = {
        "Create group",
        "Join group",
        "Enter chat room",
        "Share file",
        "Download file"
    };
    public:
    Screen* display() {
        int selectedOption = showMenu(options);
        switch(selectedOption){
            case 0:
                return new CreateGroup();
                break;
            case 1:
                return new JoinGroup();
                break;
            case 2:
                return new ChatRoom();
                break;
            case 3:
                return new ShareFile();
                break;
            default:    
                return NULL;
                break;
        }
        return NULL;
    }
};

class Login: public Screen {
    public:
    Screen* display() {
        auto conn = Connection::getConnection();
        while(true){
            std::cout << "Enter your email: ";
            std::string email;
            std::cin >> email;
            std::cout << "Enter your password: ";
            std::string pwd;
            std::cin >> pwd;
            nlohmann::json data;
            data["email"] = email;
            data["password"] = pwd;
            Message loginMsg(LOGIN_REQUEST, data.dump());
            int res = conn->sendMessage(&loginMsg);
            Message* reply = conn->msgQ.pop();
            if(reply->code == LOGIN_SUCCESS) break;
            clear();
        }
        return new TaskChoice();
    }
};
