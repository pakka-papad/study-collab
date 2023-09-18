#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "tui_controller.cpp"

void* runUiThread(void* args){
    auto controller = TuiController::getTuiController();
    controller->run();
    return NULL;
}

int main(int argc, char * argv[]){
    if(argc > 1){
        freopen(argv[1], "w", stderr);
    }

    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1) {
        std::cerr << "Error creating client socket." << std::endl;
        return EXIT_FAILURE;
    }

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
        std::cerr << "Error connecting to the server." << std::endl;
        close(clientSocket);
        return 1;
    }

    Connection::getConnection()->init(clientSocket);

    
    pthread_t uiThreadId;
    pthread_create(&uiThreadId, NULL, runUiThread, NULL);

    pthread_join(uiThreadId, NULL);

    return EXIT_SUCCESS;
}