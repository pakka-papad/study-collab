# Study Collab
A simple project aimed to build a client-server application on top of TCP sockets.  
The users can:
1. Create groups
2. Join groups
3. Chat
4. Share files
5. Download files

## Screenshots
<table>
  <tr>
    <td> <img src="https://github.com/pakka-papad/study-collab/assets/76241334/0f69bd97-884a-47dd-9cf4-f54102675c5e"/> </td>
    <td> <img src="https://github.com/pakka-papad/study-collab/assets/76241334/05117fea-2d00-4e69-9ab2-bba03c2068f0" /> </td>
  </tr>
  <tr>
    <td> Task chooser interface </td>
    <td> Join group interface </td>
  </tr>
  <tr>
    <td> <img src="https://github.com/pakka-papad/study-collab/assets/76241334/9cec586a-eeb9-423e-8e84-49a3d31eb9d9"/> </td>
    <td> <img src="https://github.com/pakka-papad/study-collab/assets/76241334/938b9c39-416e-44ce-b29f-952443213bed" /> </td>
  </tr>
  <tr>
    <td> Chat interface </td>
    <td> File upload interface </td>
  </tr>
  <tr> 
    <td> <img src="https://github.com/pakka-papad/study-collab/assets/76241334/d7a9b04b-2aa7-489d-8392-0f8aee1177bd" /> </td>
  </tr>
  <tr>
    <td> File download interface </td>
  </tr>
</table>

## Requirements
1. Linux (Tested on Ubuntu 22.04.1 LTS)
2. C++ 17
3. Curses library
4. OpenSSL library
5. [Nlohmann Json library](https://github.com/nlohmann/json)

## Build steps
Execute the commands at the root of the repository.
```
mkdir build
cd build
cmake ..
make
```
This will check for all the dependencies and generate the client and server binaries.

## Working

## Message passing overview
The client and server communicate via simple message passing through TCP. The messages are considered to be a stream of bytes. A sample message structure is as follows:

![Untitled-2023-09-30-2122](https://github.com/pakka-papad/study-collab/assets/76241334/174a9651-9079-4ba0-93df-e75542dee574)

A message consists of three parts:
1. Code: Predefined 1 byte mesaage codes used by both client and server. [See defined codes](https://github.com/pakka-papad/study-collab/blob/main/constants.hpp)
2. Length: Next 4 bytes together determine the length of the json data that follows it.
3. Json data: Variable length field that contains any additional message that is to be sent in json format. When no additional data is to be sent this field contains an empty json object.

## Client
The client is a multithreaded application which used two pthreads.  
One thread is responsible for displaying the information on the terminal window and pushing messages to socket when required.  
The other thread continuously listenes for incoming messages on the socket.  
On every message received (except chat messages), it pushes the message to a thread-safe queue. 
(A communication channel between the two threads. [Thread-safe queue](https://github.com/pakka-papad/study-collab/blob/main/client/safe_queue.cpp))  
When a client is run, it creates a directory *study-collab-client*. The file *.logs.txt* inside this contains all the log data for the client.
The sub-directory *downloads* contains all the downloaded files from the server into specific sub-directories for each group. A sub-directory for a group has title "group_id-group_name"


### Login
1. The client program sends the *LOGIN_REQUEST* code with **email** and **password** in json data.  
2. The server responds with either *LOGIN_SUCCESS* or *LOGIN_FAILED*.  

### Create group
1. The client program sends the *CREATE_GROUP_REQUEST* code with **group_name** in json data.  
2. The server responds with either *CREATE_GROUP_SUCCESS* or *CREATE_GROUP_FAILED*. In case of successful creation it also sends the **group_id** of the created group.  

### Join group
1. The client program first sends the *REQUEST_NON_PARTICIPATING_GROUPS* message to the server.  
2. The server responds with a list of all groups the client is not a part of in a json array format with code *REPLY_NON_PARTICIPATING_GROUPS*.  
3. The client then choose a group and sends the *REQUEST_JOIN_GROUP* code with **group_id** in the json data.  
4. The server responds with either *JOIN_GROUP_SUCCESS* or  *JOIN_GROUP_FAILED* code.  

### Chat
Chats feature only works for groups. Chats are not stored on the server.  
As the server receives a chat from some client, it immediately forwards the chat to all the members of the group currently online.  
As a client recieves a chat message (identified by code *NEW_MESSAGE*) it stores the chat in [ChatDB](https://github.com/pakka-papad/study-collab/blob/main/client/chat_db.cpp).  
For entering a chat room, the client:
1. Performs the *REQUEST_PARTICIPATING_GROUPS* negotiation
2. Loads the older chats form ChatDB
3. Adds a listener to ChatDB for listening to new incoming chats
4. Waits for user input to send a chat with code *SEND_MESSAGE*
5. On receiving input from user, sends a chat and goes back to 4

### File share
Files can be shared only in groups. Files are transferred between hosts in a chunked format. Each chunk (max size = 2046 bytes) is encoded in a Base64 format.  
For sharing a file, the client:
1. Performs the *REQUEST_PARTICIPATING_GROUPS* negotiation
2. Enters the path of the file to be shared
3. Breaks the file into chunks of size 2046 bytes (except the last one), encodes it into Base64 format and sends it with code *SAVE_FILE* and json data containing **chunk** (the Base64 encoding) and  **last** indicaing if it is the last chunk.
4. Server responds with either *SAVE_FILE_SUCCESS* or *SAVE_FILE_FAILED* code

### File download
For downloading a file, the client:
1. Performs the *REQUEST_PARTICIPATING_GROUPS* negotiation
2. Sends a message with code *REQUEST_FILES_LIST* and **group_id** in json data
3. Sends a message with code *REQUEST_FILE* and **group_id** and **filename** in json data
4. Server responds with code *SAVE_FILE* and sends the file in chunks encoded in Base64 format
5. The file is decoded and saved into a specific sub-directory

## Server
The server is a multithreaded application with each pthread handling one client.  
The server waits for new connections. On receiving a new connection, it creates a pthread and starts listening to incoming messages.
As a new message is recieved it processes the message and immediately responds to it.  
The server uses a in-memory [database](https://github.com/pakka-papad/study-collab/blob/main/server/database.cpp) to store all information about users, groups and files shared.  
When the server is run, a directory *study-collab-server* is created. The file *.logs.txt* inside this contains all the log data for the server.
The *store* sub-directory contains all the uploaded files arranged into sub-folders identified by their **group_id**.  
