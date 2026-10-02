cmake_minimum_required(VERSION 3.15)
project(themis_docs_builder VERSION 1.0.0 LANGUAGES CXX)

# C++17 required
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
option(BUILD_TESTS "Build unit tests" OFF)
option(WITH_YAML "Enable YAML configuration support" ON)

# Find required packages. Prefer a package config when available, but also
# support the common Debian/Ubuntu system installation where only the shared
# library and header files are present.
find_package(RocksDB QUIET CONFIG)
if(NOT RocksDB_FOUND)
    find_path(ROCKSDB_INCLUDE_DIR NAMES rocksdb/db.h PATH_SUFFIXES include)
    find_library(ROCKSDB_LIBRARY NAMES rocksdb)
    if(ROCKSDB_INCLUDE_DIR AND ROCKSDB_LIBRARY)
        add_library(RocksDB::rocksdb UNKNOWN IMPORTED)
        set_target_properties(RocksDB::rocksdb PROPERTIES
            IMPORTED_LOCATION "${ROCKSDB_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${ROCKSDB_INCLUDE_DIR}"
        )
        set(RocksDB_FOUND TRUE)
    endif()
endif()

if(NOT RocksDB_FOUND)
    message(FATAL_ERROR "RocksDB is required for themis_docs_builder. Install librocksdb-dev or configure a RocksDB package directory.")
endif()

# Optional: YAML-CPP for configuration file support
if(WITH_YAML)
    find_package(yaml-cpp QUIET)
endif()

# Include directories
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${ROCKSDB_INCLUDE_DIR}
)

# Source files
set(SOURCES
    src/main.cpp
    src/docs_builder.cpp
    src/document_parser.cpp
    src/rocksdb_writer.cpp
    src/validator.cpp
)

# Header files
set(HEADERS
    include/docs_builder.h
    include/document_parser.h
    include/rocksdb_writer.h
    include/validator.h
)

# Executable
add_executable(themis_docs_builder ${SOURCES} ${HEADERS})

if(WITH_YAML AND yaml-cpp_FOUND)
    target_compile_definitions(themis_docs_builder PRIVATE WITH_YAML_SUPPORT)
endif()

# Link libraries
target_link_libraries(themis_docs_builder
    PRIVATE
        RocksDB::rocksdb
        pthread
)

if(WITH_YAML AND yaml-cpp_FOUND)
    target_link_libraries(themis_docs_builder PRIVATE yaml-cpp)
endif()

# Compiler warnings
if(MSVC)
    target_compile_options(themis_docs_builder PRIVATE /W4)
else()
    target_compile_options(themis_docs_builder PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Install target
install(TARGETS themis_docs_builder
    RUNTIME DESTINATION bin
)

# Install documentation
install(FILES README.md DESTINATION share/doc/themis_docs_builder)

# Install example configuration
install(FILES config/builder_config.example.yaml
    DESTINATION share/themis_docs_builder/config
)

# Tests (if enabled)
if(BUILD_TESTS)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
        enable_testing()
        add_subdirectory(tests)
    else()
        message(STATUS "ThemisDB docs builder tests not present; skipping test target registration")
    endif()
endif()

# Print configuration summary
message(STATUS "ThemisDB Documentation Builder Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  YAML Support: ${WITH_YAML}")
message(STATUS "  Build Tests: ${BUILD_TESTS}")
