#include <map>
#include <string>
#include <regex>
#include "account.hpp"

class Database {
    private:
    Database(){

    }
    std::map<std::string,Account*> accounts; // [email] -> [Account]

    public:
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    static Database* getDatabse() {
        static Database db;
        return &db;
    }

    bool loginOrRegisterUser(Connection* conn, const std::string &email, const std::string &password){
        // regex match on email
        auto it = accounts.find(email);
        if(it == accounts.end()){
            Account* acc = new Account();
            acc->email = email;
            acc->password = password;
            acc->activeConn = conn;
            accounts[email] = acc;
            return true;
        } else {
            if(it->second->password != password) return false;
            accounts[email]->activeConn = conn;
            return true;
        }
    }

    void logoutUser(std::string email){
        if(email.empty()) return;
        auto it = accounts.find(email);
        if(it == accounts.end()) return;
        delete it->second->activeConn;
        it->second->activeConn = NULL;
    }



};