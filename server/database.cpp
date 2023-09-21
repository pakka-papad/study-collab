#include <map>
#include <mutex>
#include <vector>
#include <string>
#include <regex>
#include <set>
#include "account.hpp"
#include "group.hpp"
#include "server_constants.cpp"

class Database {
    private:
    std::mutex mtx;
    Database(){
        try{
            std::filesystem::create_directory(serverDirectory + fileStore);
        } catch(const std::filesystem::filesystem_error &e){
            
        }
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
        try{
            std::filesystem::create_directory(serverDirectory + fileStore + groupId + "/");
        } catch(const std::filesystem::filesystem_error &e){
            return false;
        }
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

    bool addToGroup(const std::string email, const std::string groupId){
        mtx.lock();
        bool accountExists = (accounts.find(email) != accounts.end());
        bool groupExists = (groups.find(groupId) != groups.end());
        if(!accountExists || !groupExists){
            mtx.unlock();
            return false;
        }
        groups[groupId]->members.insert(email);
        mtx.unlock();
        return true;
    }

    std::vector<std::pair<Connection*,std::string>> getGroupMemberConnectionsAndEmails(const std::string groupId){
        std::vector<std::pair<Connection*,std::string>> res;
        mtx.lock();
        if(groups.count(groupId) == 0){
            mtx.unlock();
            return res;
        }
        for(auto &member: groups[groupId]->members){
            if(accounts.count(member) != 0 && accounts[member]->activeConn != NULL){
                res.push_back({accounts[member]->activeConn, member});
            }
        }
        mtx.unlock();
        return res;
    }

    bool saveFile(const std::string &groupId, const std::string &fileName, const std::string &chunk, bool lastChunk){
        mtx.lock();
        if(groups.count(groupId) == 0){
            mtx.unlock();
            return false;
        }
        auto file = groups[groupId]->openFiles[fileName];
        if(file == NULL){
            std::string path = serverDirectory + fileStore + groupId + "/" + fileName;
            file = new std::ofstream(path.c_str(), std::ios::binary);
            groups[groupId]->openFiles[fileName] = file;
        }
        if(file == NULL) {
            if(lastChunk){
                groups[groupId]->openFiles.erase(fileName);
            }
            mtx.unlock();
            return false;
        }
        file->write(chunk.c_str(), chunk.size());
        if(lastChunk){
            file->close();
            delete file;
            groups[groupId]->openFiles.erase(fileName);
            groups[groupId]->sharedFiles.push_back(fileName);
        }
        mtx.unlock();
        return true;
    }

    std::vector<std::string> getAllFiles(const std::string groupId){
        std::vector<std::string> res;
        mtx.lock();
        if(groups.count(groupId) == 0){
            mtx.unlock();
            return res;
        }
        res = groups[groupId]->sharedFiles;
        mtx.unlock();
        return res;
    }
};