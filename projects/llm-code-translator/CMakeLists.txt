cmake_minimum_required(VERSION 3.20)

project(llm_code_translator VERSION 1.0.0 LANGUAGES CXX)

# C++20 Standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find dependencies
find_package(CURL REQUIRED)
find_package(nlohmann_json REQUIRED)

# Source files
set(SOURCES
    src/vllm_client.cpp
    src/execution_plan.cpp
    src/prompt_to_plan.cpp
    src/direct_executor.cpp
)

# Headers
set(HEADERS
    src/vllm_client.h
    src/execution_plan.h
    src/prompt_to_plan.h
    src/direct_executor.h
    src/direct_execution_engine.h
    src/jit_compiler.h
    src/llm_code_translator.h
)

# Create library
add_library(llm_code_translator STATIC ${SOURCES} ${HEADERS})

target_include_directories(llm_code_translator
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(llm_code_translator
    PUBLIC
        CURL::libcurl
        nlohmann_json::nlohmann_json
)

# Installation
install(TARGETS llm_code_translator
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(FILES ${HEADERS}
    DESTINATION include/llm_code_translator
)

# Examples (optional)
option(BUILD_EXAMPLES "Build example programs" OFF)
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Tests (optional)
option(BUILD_TESTS "Build test programs" OFF)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
