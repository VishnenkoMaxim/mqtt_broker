project(Google_tests)
include(CMakePrintHelpers)

add_subdirectory(googletest)
include_directories(googletest/googletest/include)
include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

add_executable(google_tests_run tests.cpp)
#add_library(mqtt_variable_header ../src/mqtt_variable_header.cpp)

target_link_libraries(google_tests_run gtest gtest_main functions command mqtt_protocol mqtt_variable_header spdlog config++)