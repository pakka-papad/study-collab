#include <iostream>
#include <ncurses.h>
#include <vector>
#include <string>
#include <unistd.h>
#include "menu.cpp"
#include "connection.hpp"
#include "../constants.hpp"
#include <nlohmann/json.hpp>

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
