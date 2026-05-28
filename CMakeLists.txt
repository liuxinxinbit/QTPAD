cmake_minimum_required(VERSION 3.16)

project(qtpad VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt5 REQUIRED COMPONENTS Widgets)

add_executable(qtpad
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/virtualxboxcontroller.cpp
    src/virtualxboxcontroller.h
)

target_link_libraries(qtpad PRIVATE Qt5::Widgets)

target_compile_options(qtpad PRIVATE -Wall -Wextra -Wpedantic)
