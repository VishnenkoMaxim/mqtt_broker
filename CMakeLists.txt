cmake_minimum_required(VERSION 3.18)
include(CMakePrintHelpers)

set(CMAKE_CXX_STANDARD 17)
set(PATCH_VERSION "1" CACHE INTERNAL "Patch version")
set(PROJECT_VERSION 0.0.${PATCH_VERSION})
project(mqtt_broker VERSION ${PROJECT_VERSION})

configure_file(version.h.in version.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(headers)
include_directories(headers/libconfig)

link_directories(libs)
link_directories(/usr/local/lib)
link_directories(${CMAKE_BINARY_DIR})

add_executable(mqtt_broker main.cpp)
add_library(functions src/functions.cpp)

set_target_properties(mqtt_broker functions PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
target_include_directories(functions PRIVATE "${CMAKE_BINARY_DIR}")
target_link_libraries(mqtt_broker PRIVATE spdlog functions config++ stdc++ m)

add_subdirectory(test)

target_compile_options(mqtt_broker PRIVATE -Wall -Wextra -pedantic)
target_compile_options(functions PRIVATE -Wall -Wextra -pedantic)

install(TARGETS mqtt_broker RUNTIME DESTINATION bin)
set(CPACK_GENERATOR DEB)

set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")

set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Vishnenko Maxim")

include(CPack)