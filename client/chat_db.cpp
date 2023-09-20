#ifndef CHAT_DB
#define CHAT_DB

#include <map>
#include <mutex>
#include <vector>
#include <functional>
#include "chat.hpp"

class ChatDB {
    private:
    std::mutex mtx;
    std::map<std::string,std::vector<Chat*>> db; // [groupId] -> [list of chats]
    std::function<void(Chat,std::string)>* listener;
    ChatDB(){

    }
    public:
    ChatDB(const ChatDB&) = delete;
    ChatDB& operator=(const ChatDB&) = delete;

    static ChatDB* getChatDB(){
        static ChatDB chatDb;
        return &chatDb;
    }

    void addListener(std::function<void(Chat,std::string)>* listener){
        mtx.lock();
        this->listener = listener;
        mtx.unlock();
    }

    void removeListener(){
        mtx.lock();
        listener = NULL;
        mtx.unlock();
    }

    void saveChat(Chat* chat, const std::string groupId){
        mtx.lock();
        db[groupId].push_back(chat);
        if(listener != NULL){
            (*listener)(*chat, groupId);
        }
        mtx.unlock();
    }

    std::vector<Chat> fetchChats(const std::string groupId){
        std::vector<Chat> res;
        mtx.lock();
        if(db.count(groupId) == 0) {
            mtx.unlock();
            return res;
        }
        for(auto &chat: db[groupId]){
            res.push_back(*chat);
        }
        mtx.unlock();
        return res;
    }
};

#endif