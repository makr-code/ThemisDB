# SPDX-License-Identifier: Apache-2.0 OR MIT
# CHIMERA Suite: Database Adapters Build Configuration

cmake_minimum_required(VERSION 3.15)

# Option to build CHIMERA adapters
option(BUILD_CHIMERA_ADAPTERS "Build CHIMERA database adapters" ON)
option(BUILD_CHIMERA_TESTS "Build CHIMERA adapter tests" ON)

if(NOT BUILD_CHIMERA_ADAPTERS)
    return()
endif()

# ==============================================================================
# CHIMERA Interface Library
# ==============================================================================

add_library(chimera_interface INTERFACE)
target_include_directories(chimera_interface INTERFACE
    ${CMAKE_SOURCE_DIR}/include
)
target_compile_features(chimera_interface INTERFACE cxx_std_17)

# ==============================================================================
# ThemisDB Adapter (Optional - requires ThemisDB)
# ==============================================================================

option(BUILD_THEMISDB_ADAPTER "Build ThemisDB adapter" OFF)

if(BUILD_THEMISDB_ADAPTER)
    # Note: This would require linking against ThemisDB libraries
    # For now, this is a header-only template
    message(STATUS "ThemisDB adapter: header-only template provided")
    message(STATUS "  Implementation requires linking against ThemisDB libraries")
endif()

# ==============================================================================
# PostgreSQL Adapter (Optional - requires libpqxx)
# ==============================================================================

option(BUILD_POSTGRESQL_ADAPTER "Build PostgreSQL adapter" OFF)

if(BUILD_POSTGRESQL_ADAPTER)
    find_package(PostgreSQL)
    find_package(libpqxx CONFIG)
    
    if(PostgreSQL_FOUND AND libpqxx_FOUND)
        message(STATUS "PostgreSQL adapter: enabled")
        message(STATUS "  PostgreSQL version: ${PostgreSQL_VERSION}")
        
        # Note: Actual implementation would be added here
        # add_library(chimera_postgresql_adapter postgresql_adapter.cpp)
        # target_link_libraries(chimera_postgresql_adapter 
        #     PUBLIC chimera_interface
        #     PRIVATE libpqxx::pqxx PostgreSQL::PostgreSQL
        # )
        
        message(STATUS "PostgreSQL adapter: header-only template provided")
        message(STATUS "  Implementation requires creating postgresql_adapter.cpp")
    else()
        message(WARNING "PostgreSQL or libpqxx not found, adapter disabled")
    endif()
endif()

# ==============================================================================
# MongoDB Adapter (Optional - requires mongocxx driver)
# ==============================================================================

option(BUILD_MONGODB_ADAPTER "Build MongoDB adapter" OFF)

if(BUILD_MONGODB_ADAPTER)
    find_package(libmongocxx CONFIG)
    find_package(libbsoncxx CONFIG)

    if(libmongocxx_FOUND AND libbsoncxx_FOUND)
        message(STATUS "MongoDB adapter: enabled")
        add_library(chimera_mongodb_adapter
            ${CMAKE_SOURCE_DIR}/src/chimera/mongodb_adapter.cpp
        )
        target_include_directories(chimera_mongodb_adapter
            PUBLIC ${CMAKE_SOURCE_DIR}/include
        )
        target_link_libraries(chimera_mongodb_adapter
            PUBLIC chimera_interface
            PRIVATE mongo::mongocxx_shared mongo::bsoncxx_shared
        )
    else()
        message(WARNING "mongocxx or bsoncxx not found; MongoDB adapter built without driver (simulation mode)")
        add_library(chimera_mongodb_adapter
            ${CMAKE_SOURCE_DIR}/src/chimera/mongodb_adapter.cpp
        )
        target_include_directories(chimera_mongodb_adapter
            PUBLIC ${CMAKE_SOURCE_DIR}/include
        )
        target_link_libraries(chimera_mongodb_adapter
            PUBLIC chimera_interface
        )
    endif()
endif()

# ==============================================================================
# Weaviate Adapter (Optional - requires HTTP client)
# ==============================================================================

option(BUILD_WEAVIATE_ADAPTER "Build Weaviate adapter" OFF)

if(BUILD_WEAVIATE_ADAPTER)
    # Note: Would require an HTTP client library like cpp-httplib or cpr
    message(STATUS "Weaviate adapter: header-only template provided")
    message(STATUS "  Implementation requires HTTP client library")
endif()

# ==============================================================================
# Template Adapter (Header-only)
# ==============================================================================

message(STATUS "Template adapter: provided as implementation guide")

# ==============================================================================
# Tests
# ==============================================================================

if(BUILD_CHIMERA_TESTS AND BUILD_TESTING)
    enable_testing()
    
    message(STATUS "CHIMERA adapter tests: enabled")
    message(STATUS "  Add test implementations in tests/chimera/")
    
    # Example: add_subdirectory(../../tests/chimera chimera_tests)
endif()

# ==============================================================================
# Installation
# ==============================================================================

# Install adapter headers
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/chimera/adapters
    FILES_MATCHING PATTERN "*.hpp"
)

# Install main interface header
install(
    FILES
        ${CMAKE_SOURCE_DIR}/include/chimera/database_adapter.hpp
        ${CMAKE_SOURCE_DIR}/include/chimera/themisdb_adapter.hpp
        ${CMAKE_SOURCE_DIR}/include/chimera/mongodb_adapter.hpp
    DESTINATION include/chimera
)

# Install documentation
install(
    FILES 
        ${CMAKE_CURRENT_SOURCE_DIR}/README.md
        ${CMAKE_SOURCE_DIR}/docs/chimera/ADAPTER_GUIDE.md
    DESTINATION share/doc/chimera
)

message(STATUS "")
message(STATUS "CHIMERA Adapters Configuration Summary:")
message(STATUS "  Interface library: enabled")
message(STATUS "  ThemisDB adapter: ${BUILD_THEMISDB_ADAPTER}")
message(STATUS "  PostgreSQL adapter: ${BUILD_POSTGRESQL_ADAPTER}")
message(STATUS "  MongoDB adapter: ${BUILD_MONGODB_ADAPTER}")
message(STATUS "  Weaviate adapter: ${BUILD_WEAVIATE_ADAPTER}")
message(STATUS "  Template adapter: provided")
message(STATUS "  Tests: ${BUILD_CHIMERA_TESTS}")
message(STATUS "")
