#include <map>
#include <mutex>
#include <vector>
#include <string>
#include <regex>
#include "account.hpp"
#include "group.hpp"

class Database {
    private:
    std::mutex mtx;
    Database(){

    }

    int groupCounter = -1;
    std::map<std::string,Account*> accounts; // [email] -> [Account]
    std::map<std::string,Group*> groups; // [groupId] -> [Group]

    public:
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    static Database* getDatabse() {
        static Database db;
        return &db;
    }

    bool loginOrRegisterUser(Connection* conn, const std::string &email, const std::string &password){
        mtx.lock();
        // regex match on email
        auto it = accounts.find(email);
        if(it == accounts.end()){
            Account* acc = new Account();
            acc->email = email;
            acc->password = password;
            acc->activeConn = conn;
            accounts[email] = acc;
            mtx.unlock();
            return true;
        } else {
            if(it->second->password != password) {
                mtx.unlock();
                return false;
            }
            accounts[email]->activeConn = conn;
            mtx.unlock();
            return true;
        }
    }

    void logoutUser(std::string email){
        if(email.empty()) return;
        mtx.lock();
        auto it = accounts.find(email);
        if(it == accounts.end()) {
            mtx.unlock();
            return;
        }
        delete it->second->activeConn;
        it->second->activeConn = NULL;
        mtx.unlock();
    }

    bool createGroup(const std::string &email, const std::string &groupName){
        if(email.empty()) return false;
        mtx.lock();
        groupCounter++;
        std::string groupId = std::to_string(groupCounter);
        Group* grp = new Group();
        grp->groupId = groupId;
        grp->groupName = groupName;
        grp->createdBy = email;
        grp->members.insert(email);
        groups[groupId] = grp;
        mtx.unlock();
        return true;
    }

    std::vector<Group> fetchParticipatingGroups(const std::string &email){
        mtx.lock();
        std::vector<Group> res;
        for(auto &grp: groups){
            if(grp.second->members.count(email)){
                res.push_back(*grp.second);
            }
        }
        mtx.unlock();
        return res;
    }

    std::vector<Group> fetchNonParticipatingGroups(const std::string &email){
        mtx.lock();
        std::vector<Group> res;
        for(auto &grp: groups){
            if(grp.second->members.count(email) == 0){
                res.push_back(*grp.second);
            }
        }
        mtx.unlock();
        return res;
    }

    std::vector<Group> fetchAllGroups(const std::string &email){
        mtx.lock();
        std::vector<Group> res;
        for(auto &grp: groups){
            res.push_back(*grp.second);
        }
        mtx.unlock();
        return res;
    }


};