#include <string>
#include "connection.hpp"

class Account {
    public:
    std::string email;
    std::string password;
    Connection* activeConn;
};