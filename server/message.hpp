#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <string>

class Message {
    public:
    char code;
    std::string message;
    Message(char code, std::string message): code(code), message(message) {

    }
};

#endif