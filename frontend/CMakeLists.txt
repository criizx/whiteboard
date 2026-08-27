cmake_minimum_required(VERSION 3.22)
project(whiteboard LANGUAGES CXX)

option(WHITEBOARD_ENABLE_P2P "Enable WebRTC peer-to-peer transport" ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)
list(APPEND SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")

file(GLOB_RECURSE HEADERS 
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
)

add_executable(whiteboard ${SOURCES} ${HEADERS})

set_target_properties(whiteboard PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_include_directories(whiteboard PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src  
)

set_target_properties(whiteboard PROPERTIES AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS 
    Core Gui Widgets 
    WebSockets Network
)

target_link_libraries(whiteboard PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::WebSockets
    Qt6::Network
)

if(WHITEBOARD_ENABLE_P2P)
    find_package(LibDataChannel CONFIG QUIET)
    if(NOT LibDataChannel_FOUND)
        include(FetchContent)
        set(NO_MEDIA ON CACHE BOOL "" FORCE)
        set(NO_WEBSOCKET ON CACHE BOOL "" FORCE)
        set(NO_EXAMPLES ON CACHE BOOL "" FORCE)
        set(NO_TESTS ON CACHE BOOL "" FORCE)
        set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
        FetchContent_Declare(libdatachannel
            GIT_REPOSITORY https://github.com/paullouisageneau/libdatachannel.git
            GIT_TAG v0.24.3
            GIT_SHALLOW TRUE
        )
        FetchContent_MakeAvailable(libdatachannel)
    endif()

    if(TARGET LibDataChannel::LibDataChannelStatic)
        target_link_libraries(whiteboard PRIVATE LibDataChannel::LibDataChannelStatic)
    elseif(TARGET LibDataChannel::LibDataChannel)
        target_link_libraries(whiteboard PRIVATE LibDataChannel::LibDataChannel)
    elseif(TARGET datachannel-static)
        target_link_libraries(whiteboard PRIVATE datachannel-static)
    else()
        target_link_libraries(whiteboard PRIVATE datachannel)
    endif()
    target_compile_definitions(whiteboard PRIVATE WHITEBOARD_HAS_WEBRTC=1)
endif()

install(TARGETS whiteboard
    BUNDLE DESTINATION . COMPONENT Whiteboard
    RUNTIME DESTINATION bin COMPONENT Whiteboard
)

qt_generate_deploy_app_script(
    TARGET whiteboard
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script} COMPONENT Whiteboard)
