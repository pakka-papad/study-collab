cmake_minimum_required(VERSION 3.10)

project(study-collab)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Curses REQUIRED)

add_executable(client client/connection.cpp client/client.cpp)
target_link_libraries(client PRIVATE ${CURSES_LIBRARIES})
add_executable(server server/server.cpp)