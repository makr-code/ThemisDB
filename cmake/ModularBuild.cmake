# Modular Build Configuration for ThemisDB
# This feature is planned for post-v1.3.0 release
# See docs/architecture/MODULARIZATION_PLAN.md for details

cmake_minimum_required(VERSION 3.20)

# Build mode option - defaults to legacy monolithic build until v1.3.0 is released
option(THEMIS_BUILD_MODULAR "Build as modular libraries instead of monolithic core (post-v1.3.0 feature)" OFF)

# Version check - only allow modular build after v1.3.0
if(THEMIS_BUILD_MODULAR)
    if(PROJECT_VERSION VERSION_LESS "1.3.0")
        message(WARNING 
            "THEMIS_BUILD_MODULAR requires v1.3.0 or later. Current version: ${PROJECT_VERSION}\n"
            "Modular build is disabled. See docs/architecture/MODULARIZATION_PLAN.md for details.\n"
            "Falling back to monolithic build.")
        set(THEMIS_BUILD_MODULAR OFF CACHE BOOL "Modular build disabled - version too old" FORCE)
    else()
        message(STATUS "Modular build enabled (v${PROJECT_VERSION} >= 1.3.0)")
    endif()
endif()

# Optional module configuration (only relevant when THEMIS_BUILD_MODULAR=ON)
if(THEMIS_BUILD_MODULAR)
    option(THEMIS_MODULE_LLM "Include LLM inference module (optional)" ON)
    option(THEMIS_MODULE_GEO "Include geospatial module (optional)" ON)
    option(THEMIS_MODULE_GRAPH "Include graph analytics module (optional)" ON)
    option(THEMIS_MODULE_CONTENT "Include content processors module (optional)" OFF)
    option(THEMIS_MODULE_TIMESERIES "Include time-series module" ON)
    option(THEMIS_MODULE_SHARDING "Include distributed sharding module" ON)
endif()

# Helper function to create a modular library target
# Usage: themis_add_module(module_name SOURCES file1.cpp file2.cpp ... DEPENDENCIES dep1 dep2 ...)
function(themis_add_module MODULE_NAME)
    cmake_parse_arguments(ARG "" "" "SOURCES;DEPENDENCIES" ${ARGN})
    
    # Create the library
    add_library(themis_${MODULE_NAME} SHARED ${ARG_SOURCES})
    
    # Set export macro
    string(TOUPPER ${MODULE_NAME} MODULE_NAME_UPPER)
    target_compile_definitions(themis_${MODULE_NAME} 
        PRIVATE THEMIS_${MODULE_NAME_UPPER}_EXPORTS
        PUBLIC THEMIS_${MODULE_NAME_UPPER}_ENABLED
    )
    
    # Include directories
    target_include_directories(themis_${MODULE_NAME}
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
        PRIVATE
            ${CMAKE_SOURCE_DIR}/src
    )
    
    # Link dependencies
    if(ARG_DEPENDENCIES)
        target_link_libraries(themis_${MODULE_NAME} PRIVATE ${ARG_DEPENDENCIES})
    endif()
    
    # Windows: Export all symbols for DLL
    if(MSVC)
        set_target_properties(themis_${MODULE_NAME} PROPERTIES
            WINDOWS_EXPORT_ALL_SYMBOLS ON
        )
    endif()
    
    # Installation
    install(TARGETS themis_${MODULE_NAME}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
    )
    
    message(STATUS "Module configured: themis_${MODULE_NAME}")
endfunction()

# Module source file lists (to be populated during modularization)
# These will be extracted from the current THEMIS_CORE_SOURCES

set(THEMIS_BASE_SOURCES
    # To be populated post-v1.3.0
    # src/storage/base_entity.cpp
    # Common types, interfaces, status codes
)

set(THEMIS_STORAGE_SOURCES
    # To be populated post-v1.3.0
    # src/storage/rocksdb_wrapper.cpp
    # src/index/secondary_index.cpp
    # Core storage engine and indexes
)

set(THEMIS_QUERY_SOURCES
    # To be populated post-v1.3.0
    # src/query/query_engine.cpp
    # src/query/aql_parser.cpp
    # Query engine and AQL
)

set(THEMIS_SECURITY_SOURCES
    # To be populated post-v1.3.0
    # src/security/*.cpp
    # Encryption, PKI, RBAC, JWT
)

set(THEMIS_SHARDING_SOURCES
    # To be populated post-v1.3.0
    # src/sharding/*.cpp
    # Distributed system components
)

set(THEMIS_LLM_SOURCES
    # To be populated post-v1.3.0
    # src/llm/*.cpp
    # LLM inference and prompt management
)

set(THEMIS_CONTENT_SOURCES
    # To be populated post-v1.3.0
    # src/content/*.cpp
    # Content management and processing
)

set(THEMIS_TIMESERIES_SOURCES
    # To be populated post-v1.3.0
    # src/timeseries/*.cpp
    # Time-series storage and compression
)

set(THEMIS_NETWORK_SOURCES
    # To be populated post-v1.3.0
    # src/server/*.cpp
    # src/network/*.cpp
    # HTTP/Wire protocol servers
)

set(THEMIS_GEO_SOURCES
    # To be populated post-v1.3.0
    # src/geo/*.cpp
    # Geospatial index and operations
)

set(THEMIS_GRAPH_SOURCES
    # To be populated post-v1.3.0
    # src/index/graph_index.cpp
    # Graph analytics and indexing
)

# Function to build modular architecture (post-v1.3.0)
function(themis_build_modular)
    message(STATUS "Building ThemisDB with modular architecture")
    
    # Core modules (always required)
    themis_add_module(base
        SOURCES ${THEMIS_BASE_SOURCES}
        DEPENDENCIES ""
    )
    
    themis_add_module(storage
        SOURCES ${THEMIS_STORAGE_SOURCES}
        DEPENDENCIES themis_base ${THEMIS_ROCKSDB_TARGET}
    )
    
    themis_add_module(query
        SOURCES ${THEMIS_QUERY_SOURCES}
        DEPENDENCIES themis_base themis_storage
    )
    
    themis_add_module(security
        SOURCES ${THEMIS_SECURITY_SOURCES}
        DEPENDENCIES themis_base OpenSSL::SSL OpenSSL::Crypto
    )
    
    themis_add_module(network
        SOURCES ${THEMIS_NETWORK_SOURCES}
        DEPENDENCIES themis_base themis_storage themis_query
    )
    
    # Optional modules
    if(THEMIS_MODULE_SHARDING)
        themis_add_module(sharding
            SOURCES ${THEMIS_SHARDING_SOURCES}
            DEPENDENCIES themis_base themis_storage themis_security
        )
    endif()
    
    if(THEMIS_MODULE_TIMESERIES)
        themis_add_module(timeseries
            SOURCES ${THEMIS_TIMESERIES_SOURCES}
            DEPENDENCIES themis_base themis_storage
        )
    endif()
    
    if(THEMIS_MODULE_LLM)
        themis_add_module(llm
            SOURCES ${THEMIS_LLM_SOURCES}
            DEPENDENCIES themis_base themis_storage
        )
    endif()
    
    if(THEMIS_MODULE_GEO)
        themis_add_module(geo
            SOURCES ${THEMIS_GEO_SOURCES}
            DEPENDENCIES themis_base themis_storage
        )
    endif()
    
    if(THEMIS_MODULE_GRAPH)
        themis_add_module(graph
            SOURCES ${THEMIS_GRAPH_SOURCES}
            DEPENDENCIES themis_base themis_storage
        )
    endif()
    
    if(THEMIS_MODULE_CONTENT)
        themis_add_module(content
            SOURCES ${THEMIS_CONTENT_SOURCES}
            DEPENDENCIES themis_base themis_storage
        )
    endif()
    
    # Create convenience variable for all modules to link against
    set(THEMIS_ALL_MODULES
        themis_base
        themis_storage
        themis_query
        themis_security
        themis_network
        PARENT_SCOPE
    )
    
    if(THEMIS_MODULE_SHARDING)
        list(APPEND THEMIS_ALL_MODULES themis_sharding)
    endif()
    
    if(THEMIS_MODULE_TIMESERIES)
        list(APPEND THEMIS_ALL_MODULES themis_timeseries)
    endif()
    
    if(THEMIS_MODULE_LLM)
        list(APPEND THEMIS_ALL_MODULES themis_llm)
    endif()
    
    if(THEMIS_MODULE_GEO)
        list(APPEND THEMIS_ALL_MODULES themis_geo)
    endif()
    
    if(THEMIS_MODULE_GRAPH)
        list(APPEND THEMIS_ALL_MODULES themis_graph)
    endif()
    
    if(THEMIS_MODULE_CONTENT)
        list(APPEND THEMIS_ALL_MODULES themis_content)
    endif()
    
    set(THEMIS_ALL_MODULES ${THEMIS_ALL_MODULES} PARENT_SCOPE)
endfunction()
