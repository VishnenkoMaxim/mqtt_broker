project(Google_tests)

add_subdirectory(googletest)
include_directories(googletest/googletest/include)
include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

add_executable(google_tests_run tests.cpp)

target_link_libraries(google_tests_run gtest gtest_main functions mqtt_protocol spdlog config++)