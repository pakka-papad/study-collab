#include <iostream>
#include <ncurses.h>
#include <vector>
#include <string>
#include "menu.cpp"
#include "connection.hpp"
#include "../constants.hpp"
#include <nlohmann/json.hpp>

class Screen {
    public:
    virtual Screen* display() = 0;
};

class TaskChoice: public Screen {
    private:
    std::vector<std::string> options = {
        "Create group",
        "Enter chat room",
        "Share file",
        "Download file"
    };
    public:
    Screen* display() {
        int selectedOption = showMenu(options);
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
