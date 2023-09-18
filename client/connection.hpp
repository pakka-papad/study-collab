#include "message.hpp"

class Connection {
    private:
    bool isInitialized = false;
    int socket;
    Connection(){ }
    public:
    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;

    static Connection* getConnection() {
        static Connection conn;
        return &conn;
    }

    void init(int socket){
        this->socket = socket;
        isInitialized = true;
    }

    int sendMessage(Message* msg);

};