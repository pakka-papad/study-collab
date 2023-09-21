#include <iostream>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <filesystem>
#include "tui_controller.cpp"
#include "client_constants.cpp"

void* runUiThread(void* args){
    auto controller = TuiController::getTuiController();
    controller->run();
    return NULL;
}

int main(int argc, char * argv[]){

    std::string pid = std::to_string(getpid());
    try{
        std::filesystem::create_directory(rootDir);
        std::filesystem::create_directory(downloadsDir);
        std::string errorLogFile = rootDir + ".logs.txt";
        freopen(errorLogFile.c_str(), "w", stderr);
    } catch(const std::filesystem::filesystem_error &e){
        return EXIT_FAILURE;
    }

    std::string ipAddr = "127.0.0.1";
    if(argc > 1){
        ipAddr = argv[1];
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
    Connection::getConnection()->terminate();

    return EXIT_SUCCESS;
}