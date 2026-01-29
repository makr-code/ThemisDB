# Ethics AI Plugin - Native C++ Implementation
cmake_minimum_required(VERSION 3.20)

project(ethics_ai_plugin VERSION 1.0.0)

message(STATUS "Configuring Ethics AI Plugin")

# Plugin source files
set(PLUGIN_SOURCES
    ethics_ai_types.cpp
    ethics_ai_plugin.cpp
    philosophy_loader.cpp
    argument_store.cpp
    rag_context_engine.cpp
    discourse_engine.cpp
    ethics_evaluator.cpp
)

# Create plugin library
add_library(ethics_ai_plugin SHARED ${PLUGIN_SOURCES})

# Include directories
target_include_directories(ethics_ai_plugin
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Dependencies
target_link_libraries(ethics_ai_plugin
    PRIVATE
        # Core ThemisDB libraries (to be linked during build)
        # These will be resolved when building with main project
)

# Find yaml-cpp for philosophy profile loading
find_package(yaml-cpp QUIET)
if(yaml-cpp_FOUND)
    target_link_libraries(ethics_ai_plugin PRIVATE yaml-cpp)
    target_compile_definitions(ethics_ai_plugin PRIVATE HAVE_YAML_CPP)
    message(STATUS "  - yaml-cpp found: Enabling YAML philosophy profiles")
else()
    message(WARNING "  - yaml-cpp not found: YAML loading will be limited")
endif()

# Set plugin properties
set_target_properties(ethics_ai_plugin PROPERTIES
    PREFIX ""
    SUFFIX "${CMAKE_SHARED_LIBRARY_SUFFIX}"
    OUTPUT_NAME "ethics_ai_plugin"
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    POSITION_INDEPENDENT_CODE ON
)

# Compiler flags
if(MSVC)
    target_compile_options(ethics_ai_plugin PRIVATE /W4)
else()
    target_compile_options(ethics_ai_plugin PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Install plugin
install(TARGETS ethics_ai_plugin
    LIBRARY DESTINATION lib/themisdb/plugins
    RUNTIME DESTINATION lib/themisdb/plugins
    ARCHIVE DESTINATION lib/themisdb/plugins
)

# Install headers
install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/plugins/ethics_ai
    DESTINATION include/plugins
    FILES_MATCHING PATTERN "*.h"
)

# Install philosophy profiles (from plugin directory)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/philosophies
    DESTINATION lib/themisdb/plugins/ethics_ai/
    FILES_MATCHING PATTERN "*.yaml" PATTERN "*.yml" PATTERN "*.md"
)

# Install plugin metadata
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/ethics_ai_plugin.json.in
    ${CMAKE_CURRENT_BINARY_DIR}/ethics_ai_plugin.json
    @ONLY
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ethics_ai_plugin.json
    DESTINATION lib/themisdb/plugins/
)

# Build examples (optional)
if(THEMIS_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

message(STATUS "Ethics AI Plugin configuration complete")
