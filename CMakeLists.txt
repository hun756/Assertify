cmake_minimum_required(VERSION 3.10)
project(MyLibrary)

# Add the includeassert.hpp file to the include directory
include_directories(include)

# Add the source files for your library
add_library(MyLibrary test/test_main.cpp)

# Add the test files in the test folder
file(GLOB TEST_FILES test/*.cpp)

# Create an executable for each test file
foreach(TEST_FILE ${TEST_FILES})
    get_filename_component(TEST_NAME ${TEST_FILE} NAME_WE)
    add_executable(${TEST_NAME} ${TEST_FILE})
    target_link_libraries(${TEST_NAME} MyLibrary)
endforeach()