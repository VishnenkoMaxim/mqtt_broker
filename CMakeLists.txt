cmake_minimum_required(VERSION 3.10)
include(CMakePrintHelpers)

set(CMAKE_CXX_STANDARD 17)
set(PATCH_VERSION "1" CACHE INTERNAL "Patch version")
set(PROJECT_VERSION 0.0.${PATCH_VERSION})
set(CMAKE_CXX_COMPILER "/usr/bin/clang")
project(mqtt_broker VERSION ${PROJECT_VERSION})

option(STATIC_BUILD "static build" ON)
option(DEBUG_BUILD "debug build" OFF)

configure_file(version.h.in version.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(headers)
include_directories(headers/libconfig)

link_directories(libs)
link_directories(/usr/local/lib)
link_directories(${CMAKE_BINARY_DIR})

add_library(functions src/functions.cpp)
add_library(command src/command.cpp)

set(CMAKE_DEBUG_POSTFIX _g)

if (STATIC_BUILD)
    add_library(mqtt_protocol STATIC src/mqtt_protocol.cpp src/mqtt_variable_header.cpp src/mqtt_fixed_header.cpp src/mqtt_topic.cpp)
    set(CMAKE_EXE_LINKER_FLAGS " -static")
else()
    add_library(mqtt_protocol SHARED src/mqtt_protocol.cpp src/mqtt_variable_header.cpp src/mqtt_fixed_header.cpp src/mqtt_topic.cpp)
endif()

add_executable(mqtt_broker main.cpp src/broker.cpp src/client.cpp src/handlers.cpp src/topic_storage.cpp
        src/mqtt_packet_handler.cpp src/mqtt_error_handler.cpp)

if (DEBUG_BUILD)
    set_target_properties(mqtt_broker command functions mqtt_protocol PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON
                            DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})
else()
    set_target_properties(mqtt_broker command functions mqtt_protocol PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
endif()

target_include_directories(functions PRIVATE "${CMAKE_BINARY_DIR}")
target_include_directories(mqtt_protocol PRIVATE "${CMAKE_BINARY_DIR}")
target_include_directories(command PRIVATE "${CMAKE_BINARY_DIR}")

target_link_libraries(command PRIVATE functions)
target_link_libraries(mqtt_broker PRIVATE functions command mqtt_protocol config++ stdc++ pthread m)

#add_subdirectory(test)

target_compile_options(mqtt_broker PRIVATE -Wall -Wextra -pedantic)
target_compile_options(functions PRIVATE -Wall -Wextra -pedantic)
target_compile_options(mqtt_protocol PRIVATE -Wall -Wextra -pedantic)
target_compile_options(command PRIVATE -Wall -Wextra -pedantic)

install(TARGETS mqtt_broker RUNTIME DESTINATION /home)
set(CPACK_GENERATOR DEB)

set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")

set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Vishnenko Maxim")

include(CPack)