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
#include <fstream>
#include "chat.hpp"
#include "chat_db.cpp"
#include "safe_queue.cpp"
#include "progress.cpp"
#include "client_constants.cpp"
#include "../util.hpp"

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
                std::string groupId = nlohmann::json::parse(reply->message)["group_id"];
                try{
                    std::string dir = downloadsDir + groupId + "-" + gropupName;
                    std::filesystem::create_directory(dir);
                } catch(const std::filesystem::filesystem_error &e){

                }
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
        std::vector<std::string> groupIds, groupNames;
        std::string groupName, createdBy, groupId;
        for(auto &group: data){
            groupName = group["group_name"];
            createdBy = group["created_by"];
            groupId = group["group_id"];
            groups.push_back(groupName + " (created by: " + createdBy + ")");
            groupIds.push_back(groupId);
            groupNames.push_back(groupName);
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
                try{
                    std::string dir = downloadsDir + groupIds[grpPos] + "-" + groupNames[grpPos];
                    std::filesystem::create_directory(dir);
                } catch(const std::filesystem::filesystem_error &e){

                }
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
        

        std::ifstream file(path.c_str(), std::ios::binary);
        if(file.is_open() == false){
            std::cout << "Error opening file" << std::endl;
            sleep(1);
            return NULL;
        }

        std::filesystem::path pathObj(path);
        std::string filename = pathObj.filename().string();

        std::streampos begin = file.tellg();
        file.seekg(0, std::ios::end);
        std::streampos end = file.tellg();

        int64_t totalBytes = end - begin, totalBytesRead = 0;

        file.seekg(0, std::ios::beg);
        
        char buffer[2046];
        int bytesRead = 0;

        nlohmann::json chunkData;
        chunkData["filename"] = filename;
        chunkData["group_id"] = groupIds[grpPos];

        file.read(buffer, sizeof(buffer));
        bytesRead = file.gcount();
        std::string fileChunk;

        while(bytesRead > 0){
            chunkData["chunk"] = ByteArrayToBase64(buffer, bytesRead);
            chunkData["last"] = (file.eof() ? 1 : 0);
            Message chunk(SAVE_FILE, chunkData.dump());
            conn->sendMessage(&chunk);

            totalBytesRead += bytesRead;
            updateProgressBar(totalBytesRead, totalBytes);

            file.read(buffer, sizeof(buffer));
            bytesRead = file.gcount();
        }

        file.close();

        if(bytesRead == -1){
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

class DownloadFile: public Screen {
    public:
    Screen* display(){
        auto conn = Connection::getConnection();
        std::cout << "Fetching groups list..." << std::endl;

        Message grpsReq(REQUEST_PARTICIPATING_GROUPS,"{}");
        conn->sendMessage(&grpsReq);
        auto grpsReply = conn->msgQ.pop();
        nlohmann::json data = nlohmann::json::parse(grpsReply->message);
        std::vector<std::string> groups;
        std::vector<std::string> groupIds, groupNames;
        std::string groupName, createdBy, groupId;
        for(auto &group: data){
            groupName = group["group_name"];
            createdBy = group["created_by"];
            groupId = group["group_id"];
            groups.push_back(groupName + " (created by: " + createdBy + ")");
            groupIds.push_back(groupId);
            groupNames.push_back(groupName);
        }

        int grpPos = showMenu(groups);
        if(grpPos == -1) return NULL;
        system("clear");

        nlohmann::json filesReqData;
        filesReqData["group_id"] = groupIds[grpPos];
        Message filesReq(REQUEST_FILES_LIST,filesReqData.dump());
        conn->sendMessage(&filesReq);
        auto filesReply = conn->msgQ.pop();
        nlohmann::json fileData = nlohmann::json::parse(filesReply->message);
        std::vector<std::string> files;
        for(auto &file: fileData){
            files.push_back(file);
        }

        int filePos = showMenu(files);
        if(filePos == -1) return NULL;
        system("clear");

        nlohmann::json fileReqData;
        fileReqData["filename"] = files[filePos];
        fileReqData["group_id"] = groupIds[grpPos];
        Message fileReq(REQUEST_FILE,fileReqData.dump());
        conn->sendMessage(&fileReq);

        std::string filePath = downloadsDir + groupIds[grpPos] + "-" + groupNames[grpPos] + "/" + files[filePos];
        std::ofstream file(filePath.c_str(), std::ios::binary);

        int64_t recvBytes = 0;

        while(true){
            auto msg = conn->msgQ.pop();
            if(msg->code != SAVE_FILE) continue;
            nlohmann::json chunkData = nlohmann::json::parse(msg->message);
            std::string chunk = chunkData["chunk"];
            int last = chunkData["last"];
            recvBytes += chunk.size();
            auto bytes = Base64ToByteArray(chunk);
            file.write(bytes.c_str(), bytes.size());
            std::cout << "Received: " << recvBytes << " bytes\r";
            std::cout.flush();
            if(last){
                file.close();
                std::cout << "\nFile saved successfully" << std::endl;
                sleep(1);
                break;
            }
        }

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
            case 4:
                return new DownloadFile();
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
