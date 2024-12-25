cmake_minimum_required(VERSION 3.10)

project(SystemdGUI)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT DEFINED CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

if(CMAKE_BUILD_TYPE EQUAL "Debug")
    add_compile_options("$<BUILD_INTERFACE:-Wall;-ggdb>")
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtkmm-3.0)
include_directories(${GTK3_INCLUDE_DIRS})
link_directories(${GTK3_LIBRARY_DIRS})

# Suppress deprecation warnings
add_definitions(${GTK3_CFLAGS_OTHER})

include_directories(include)
add_executable(systemd-control-panel
    src/main.cpp
    src/utils.cpp
    src/ServiceData.cpp
    src/ServiceRow.cpp
    src/SelectItemWindow.cpp
    src/ControlPanelWindow.cpp
)

target_link_libraries(systemd-control-panel ${GTK3_LIBRARIES})
