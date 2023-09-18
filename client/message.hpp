#include <string>

class Message {
    public:
    char code;
    std::string message;
    Message(char code, std::string message): code(code), message(message) {

    }
};