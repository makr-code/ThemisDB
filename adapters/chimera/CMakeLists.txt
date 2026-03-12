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
        message(STATUS "PostgreSQL adapter: enabled with libpqxx")
        message(STATUS "  PostgreSQL version: ${PostgreSQL_VERSION}")
        add_library(chimera_postgresql_adapter
            ${CMAKE_SOURCE_DIR}/src/chimera/postgresql_adapter.cpp
        )
        target_include_directories(chimera_postgresql_adapter
            PUBLIC ${CMAKE_SOURCE_DIR}/include
        )
        target_link_libraries(chimera_postgresql_adapter
            PUBLIC chimera_interface
            PRIVATE libpqxx::pqxx PostgreSQL::PostgreSQL
        )
    else()
        message(WARNING "libpqxx not found; PostgreSQL adapter built in simulation mode (no live server required)")
        add_library(chimera_postgresql_adapter
            ${CMAKE_SOURCE_DIR}/src/chimera/postgresql_adapter.cpp
        )
        target_include_directories(chimera_postgresql_adapter
            PUBLIC ${CMAKE_SOURCE_DIR}/include
        )
        target_link_libraries(chimera_postgresql_adapter
            PUBLIC chimera_interface
        )
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
        message(STATUS "MongoDB adapter: enabled with real mongocxx driver")
        add_library(chimera_mongodb_adapter
            ${CMAKE_SOURCE_DIR}/src/chimera/mongodb_adapter.cpp
        )
        target_include_directories(chimera_mongodb_adapter
            PUBLIC ${CMAKE_SOURCE_DIR}/include
        )
        target_compile_definitions(chimera_mongodb_adapter
            PUBLIC THEMIS_ENABLE_MONGODB
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
# Weaviate Adapter (Optional - requires HTTP client for production use)
# ==============================================================================

option(BUILD_WEAVIATE_ADAPTER "Build Weaviate adapter" OFF)

if(BUILD_WEAVIATE_ADAPTER)
    # Production deployments should link against an HTTP client library
    # (e.g. cpp-httplib or cpr) for live Weaviate REST/GraphQL API access.
    # The adapter also works in simulation mode without any external driver.
    message(STATUS "Weaviate adapter: enabled (simulation mode; production requires HTTP client)")
    add_library(chimera_weaviate_adapter
        ${CMAKE_SOURCE_DIR}/src/chimera/weaviate_adapter.cpp
    )
    target_include_directories(chimera_weaviate_adapter
        PUBLIC ${CMAKE_SOURCE_DIR}/include
    )
    target_link_libraries(chimera_weaviate_adapter
        PUBLIC chimera_interface
    )
endif()

# ==============================================================================
# Qdrant Adapter (Optional - requires HTTP client for production use)
# ==============================================================================

option(BUILD_QDRANT_ADAPTER "Build Qdrant adapter" OFF)

if(BUILD_QDRANT_ADAPTER)
    # Detect cpp-httplib (header-only) and nlohmann/json
    find_package(httplib CONFIG QUIET)
    find_package(nlohmann_json CONFIG QUIET)

    if(httplib_FOUND AND nlohmann_json_FOUND)
        message(STATUS "Qdrant adapter: enabled with cpp-httplib + nlohmann/json (real HTTP calls)")
        add_library(chimera_qdrant_adapter
            ${CMAKE_SOURCE_DIR}/src/chimera/qdrant_adapter.cpp
        )
        target_include_directories(chimera_qdrant_adapter
            PUBLIC ${CMAKE_SOURCE_DIR}/include
        )
        target_compile_definitions(chimera_qdrant_adapter
            PUBLIC THEMIS_ENABLE_QDRANT
        )
        target_link_libraries(chimera_qdrant_adapter
            PUBLIC chimera_interface
            PRIVATE httplib::httplib nlohmann_json::nlohmann_json
        )
    else()
        message(WARNING "cpp-httplib or nlohmann/json not found; Qdrant adapter built in simulation mode (no live server required)")
        add_library(chimera_qdrant_adapter
            ${CMAKE_SOURCE_DIR}/src/chimera/qdrant_adapter.cpp
        )
        target_include_directories(chimera_qdrant_adapter
            PUBLIC ${CMAKE_SOURCE_DIR}/include
        )
        target_link_libraries(chimera_qdrant_adapter
            PUBLIC chimera_interface
        )
    endif()
endif()

# ==============================================================================
# Pinecone Adapter (Optional - requires HTTP client for production use)
# ==============================================================================

option(BUILD_PINECONE_ADAPTER "Build Pinecone adapter" OFF)

if(BUILD_PINECONE_ADAPTER)
    # Production deployments should link against an HTTP client library
    # (e.g. cpp-httplib or cpr) for live Pinecone REST API access.
    # The adapter also works in simulation mode without any external driver.
    message(STATUS "Pinecone adapter: enabled (simulation mode; production requires HTTP client)")
    add_library(chimera_pinecone_adapter
        ${CMAKE_SOURCE_DIR}/src/chimera/pinecone_adapter.cpp
    )
    target_include_directories(chimera_pinecone_adapter
        PUBLIC ${CMAKE_SOURCE_DIR}/include
    )
    target_link_libraries(chimera_pinecone_adapter
        PUBLIC chimera_interface
    )
endif()

# ==============================================================================
# Elasticsearch Adapter (Optional - requires HTTP client for production use)
# ==============================================================================

option(BUILD_ELASTICSEARCH_ADAPTER "Build Elasticsearch adapter" OFF)

if(BUILD_ELASTICSEARCH_ADAPTER)
    # Production deployments should link against an HTTP client library
    # (e.g. cpp-httplib or cpr) for live Elasticsearch REST API access.
    # The adapter also works in simulation mode without any external driver.
    message(STATUS "Elasticsearch adapter: enabled (simulation mode; production requires HTTP client)")
    add_library(chimera_elasticsearch_adapter
        ${CMAKE_SOURCE_DIR}/src/chimera/elasticsearch_adapter.cpp
    )
    target_include_directories(chimera_elasticsearch_adapter
        PUBLIC ${CMAKE_SOURCE_DIR}/include
    )
    target_link_libraries(chimera_elasticsearch_adapter
        PUBLIC chimera_interface
    )
endif()

# ==============================================================================
# Neo4j Adapter (Optional - requires Bolt/HTTP client for production use)
# ==============================================================================

option(BUILD_NEO4J_ADAPTER "Build Neo4j adapter" OFF)

if(BUILD_NEO4J_ADAPTER)
    # Detect cpp-httplib and nlohmann/json for the HTTP Transactional API path.
    find_package(httplib CONFIG QUIET)
    find_package(nlohmann_json CONFIG QUIET)

    if(httplib_FOUND AND nlohmann_json_FOUND)
        message(STATUS "Neo4j adapter: enabled with cpp-httplib + nlohmann/json (real Cypher/HTTP calls)")
        add_library(chimera_neo4j_adapter
            ${CMAKE_SOURCE_DIR}/src/chimera/neo4j_adapter.cpp
        )
        target_include_directories(chimera_neo4j_adapter
            PUBLIC ${CMAKE_SOURCE_DIR}/include
        )
        target_compile_definitions(chimera_neo4j_adapter
            PUBLIC THEMIS_ENABLE_NEO4J
        )
        target_link_libraries(chimera_neo4j_adapter
            PUBLIC chimera_interface
            PRIVATE httplib::httplib nlohmann_json::nlohmann_json
        )
    else()
        message(WARNING "cpp-httplib or nlohmann/json not found; Neo4j adapter built in simulation mode (no live server required)")
        add_library(chimera_neo4j_adapter
            ${CMAKE_SOURCE_DIR}/src/chimera/neo4j_adapter.cpp
        )
        target_include_directories(chimera_neo4j_adapter
            PUBLIC ${CMAKE_SOURCE_DIR}/include
        )
        target_link_libraries(chimera_neo4j_adapter
            PUBLIC chimera_interface
        )
    endif()
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
        ${CMAKE_SOURCE_DIR}/include/chimera/postgresql_adapter.hpp
        ${CMAKE_SOURCE_DIR}/include/chimera/weaviate_adapter.hpp
        ${CMAKE_SOURCE_DIR}/include/chimera/qdrant_adapter.hpp
        ${CMAKE_SOURCE_DIR}/include/chimera/pinecone_adapter.hpp
        ${CMAKE_SOURCE_DIR}/include/chimera/elasticsearch_adapter.hpp
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
message(STATUS "  Qdrant adapter: ${BUILD_QDRANT_ADAPTER}")
message(STATUS "  Pinecone adapter: ${BUILD_PINECONE_ADAPTER}")
message(STATUS "  Elasticsearch adapter: ${BUILD_ELASTICSEARCH_ADAPTER}")
message(STATUS "  Neo4j adapter: ${BUILD_NEO4J_ADAPTER}")
message(STATUS "  Template adapter: provided")
message(STATUS "  Tests: ${BUILD_CHIMERA_TESTS}")
message(STATUS "")
