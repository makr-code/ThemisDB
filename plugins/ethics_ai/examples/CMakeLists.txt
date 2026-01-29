# Ethics AI Plugin Examples
cmake_minimum_required(VERSION 3.20)

# Only build examples if explicitly requested
if(NOT THEMIS_BUILD_EXAMPLES)
    return()
endif()

message(STATUS "Building Ethics AI Plugin Examples")

# Example 1: Basic Usage
add_executable(ethics_ai_example_basic
    example_basic_usage.cpp
)

target_include_directories(ethics_ai_example_basic
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/plugins/ethics_ai
)

target_link_libraries(ethics_ai_example_basic
    PRIVATE
        ethics_ai_plugin
)

set_target_properties(ethics_ai_example_basic PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    OUTPUT_NAME "ethics_ai_basic_usage"
)

# Install examples
install(TARGETS ethics_ai_example_basic
    RUNTIME DESTINATION bin/examples
)

message(STATUS "  - Basic Usage Example")
