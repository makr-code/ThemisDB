cmake_minimum_required(VERSION 3.20)
project(ThemisDB_CursorSyntaxCheck CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Create object library to check cursor.cpp syntax
add_library(cursor_check OBJECT
    src/utils/cursor.cpp
)

target_include_directories(cursor_check PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

set_target_properties(cursor_check PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

message(STATUS "Cursor syntax check configuration complete")
