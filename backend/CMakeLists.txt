cmake_minimum_required(VERSION 3.22)
project(whiteboard_server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(APPLE)
    list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew")
endif()

find_package(Boost REQUIRED COMPONENTS json)

add_executable(whiteboard_server server.cpp)

find_package(Threads REQUIRED)
target_link_libraries(whiteboard_server PRIVATE
    Boost::json
    Boost::headers
    Threads::Threads
)
