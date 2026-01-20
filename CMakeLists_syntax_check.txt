cmake_minimum_required(VERSION 3.20)
project(ThemisDB_SyntaxCheck CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Check if key headers compile
add_library(training_service_registry OBJECT
    src/llm/lora_framework/training_service_registry.cpp
)

target_include_directories(training_service_registry PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Set C++20
set_target_properties(training_service_registry PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

message(STATUS "Syntax check configuration complete")
