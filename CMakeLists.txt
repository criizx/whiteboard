cmake_minimum_required(VERSION 3.22)
project(whiteboard LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_PREFIX_PATH "/usr/lib/cmake/Qt5")

find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets Quick QuickControls2 WebSockets)

# Файлы проекта
set(SOURCES
        main.cpp
        ui/include/mainwindow.h
        ui/src/mainwindow.cpp
        drawing_logic/include/canvaswidget.h
        drawing_logic/src/canvaswidget.cpp
        drawing_logic/include/drawer.h
        drawing_logic/src/drawer.cpp
        drawing_logic/src/DrawableObject.cpp
        drawing_logic/include/DrawableObject.h
)


# ⬇️ Сначала создаём исполняемый файл
add_executable(whiteboard ${SOURCES} ${RESOURCES})

# ⬇️ Потом указываем include директории
target_include_directories(whiteboard PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/drawing_logic/include
)

# ⬇️ Потом связываем с Qt
target_link_libraries(whiteboard
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Quick
        Qt5::QuickControls2
        Qt5::WebSockets
)
