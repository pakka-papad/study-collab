#include <atomic>
#include "message.hpp"
#include "safe_queue.cpp"

class Connection {
    private:
    bool isInitialized = false;
    int socket;
    pthread_t listenerThreadId;

    void listenIncomingMessages();

    static void* listen(void* args){
        Connection* conn = static_cast<Connection*>(args);
        conn->listenIncomingMessages();
        return NULL;
    }

    Connection(){}

    public:
    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;

    SafeQueue<Message*> msgQ;

    static Connection* getConnection() {
        static Connection conn;
        return &conn;
    }

    void init(int socket){
        this->socket = socket;
        isInitialized = true;
        pthread_create(&listenerThreadId, NULL, listen, this);
    }

    int sendMessage(Message* msg);

    void terminate();

};