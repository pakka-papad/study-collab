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

class Login: public Screen {
    public:
    Screen* display() {
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
        int res = Connection::getConnection()->sendMessage(&loginMsg);
        return NULL;
    }
};

class Auth: public Screen {
    private:
    std::vector<std::string> options = {
        "Login",
        "Sign Up"
    };
    public:
    Screen* display() {
        clear();
        int selectedOption = showMenu(options);
        Screen* res;
        if(selectedOption == 0){
            res = new Login();
        } else if(selectedOption == 1){

        }
        return res;
    }
};