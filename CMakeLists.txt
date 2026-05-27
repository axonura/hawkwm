cmake_minimum_required(VERSION 3.16)

project(hawkwm VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Qml WaylandCompositor)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(HAWKWM_DEBUG)
endif()

include_directories(include)
add_subdirectory(src)