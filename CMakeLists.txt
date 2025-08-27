cmake_minimum_required(VERSION 3.22)
project(whiteboard LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Находим все исходные файлы
file(GLOB_RECURSE SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
)

# Находим все заголовочные файлы
file(GLOB_RECURSE HEADERS 
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
)

# Добавляем заголовки в исполняемый файл
add_executable(whiteboard ${SOURCES} ${HEADERS})

# Устанавливаем директории включения
target_include_directories(whiteboard PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src  # Добавлено
)

# Явно включаем AUTOMOC для цели
set_target_properties(whiteboard PROPERTIES AUTOMOC ON)

# Подключаем Qt модули
find_package(Qt6 REQUIRED COMPONENTS 
    Core Gui Widgets 
    Quick QuickControls2 
    WebSockets Network
)

target_link_libraries(whiteboard PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::WebSockets
    Qt6::Network  # Добавлено
)