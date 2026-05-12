cmake_minimum_required(VERSION 3.21)
project(generic_installer_framework VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(giuf_core
    src/core/app_controller.cpp
)

target_include_directories(giuf_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(giuf-cli
    tools/giuf-cli/main.cpp
)

target_link_libraries(giuf-cli PRIVATE giuf_core)
