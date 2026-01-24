# ThemisDB Root CMakeLists.txt - New Modular Architecture
# Clean, minimal orchestration of the build system

cmake_minimum_required(VERSION 3.20)

# Read VERSION first (before project())
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION")
    message(FATAL_ERROR "VERSION file not found at ${CMAKE_CURRENT_SOURCE_DIR}/VERSION")
endif()

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" _ver_raw)
string(STRIP "${_ver_raw}" _ver_raw)

# Parse semantic version
if(NOT _ver_raw MATCHES "^([0-9]+)\\.([0-9]+)\\.([0-9]+)")
    message(FATAL_ERROR "Invalid VERSION format: '${_ver_raw}'. Expected: 1.2.3 or 1.2.3-alpha")
endif()

string(REGEX MATCH "^([0-9]+)\\.([0-9]+)\\.([0-9]+)" _ver_numeric "${_ver_raw}")

# Define project with numeric version (CMake requirement)
project(Themis 
    VERSION ${_ver_numeric}
    DESCRIPTION "ThemisDB - Enterprise Distributed Database"
    LANGUAGES CXX
)

# Store full semantic version
set(THEMIS_VERSION_STRING "${_ver_raw}")

# ============================================================================
# MODULAR CMAKE INCLUDES - REFACTORED BUILD SYSTEM
# ============================================================================
# Build order (critical):
# 1. Version info
# 2. Compiler settings
# 3. Platform detection (arch/OS)
# 4. Architecture optimizations
# 5. Preload targets
# 6. Dependencies
# 7. Edition selection (AFTER platform!)
# 8. Feature configuration (AFTER edition!)
# 9. Validation (platform, edition, features)

# 1. Version management (version info only)
include(cmake/Versions.cmake)

# 2. Compiler setup
include(cmake/CompilerOptions.cmake)

# 3. Platform detection (OS, CPU, triplet)
include(cmake/platforms/PlatformDetection.cmake)

# 4. Architecture optimizations (AVX2, NEON, etc.)
include(cmake/platforms/ArchitectureOptimizations.cmake)

# 5. Preload targets for system packages
include(cmake/PreloadTargets.cmake)

# 6. Dependencies
include(cmake/Dependencies.cmake)

# 7. Edition selection (MINIMAL, COMMUNITY, ENTERPRISE, HYPERSCALER)
include(cmake/editions/EditionDefaults.cmake)

# 8. Feature configuration (LLM, GPU, Network, Security, Tools, Optimizations)
include(cmake/features/FeatureDefaults.cmake)

# 9. Validation layer
include(cmake/validation/PlatformValidation.cmake)
include(cmake/validation/EditionValidation.cmake)
include(cmake/validation/FeatureValidation.cmake)

# Testing - handled in cmake/CMakeLists.txt via add_subdirectory(tests)
# (Moved to cmake/CMakeLists.txt to be closer to target definitions)

# Set root directory for use in subdirectories (MUST be before add_subdirectory(cmake))
set(THEMIS_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" CACHE PATH "ThemisDB root directory")

# Benchmarking
if(THEMIS_BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

# CHIMERA Adapters (optional, vendor-neutral)
option(BUILD_CHIMERA_ADAPTERS "Build CHIMERA database adapters" OFF)
if(BUILD_CHIMERA_ADAPTERS)
    add_subdirectory(adapters/chimera)
endif()

# Main build logic
add_subdirectory(cmake)

# ============================================================================
# DOCUMENTATION DATABASE GENERATION
# ============================================================================

# Control whether docs.db is generated during build. Defaults ON for native builds;
# Docker should pass -DTHEMIS_BUILD_DOCS_DB=OFF and provide a prebuilt docs.db.
option(THEMIS_BUILD_DOCS_DB "Generate documentation database during build" ON)

if(THEMIS_BUILD_DOCS_DB)
    # Find Python3 for documentation database generation
    find_package(Python3 COMPONENTS Interpreter)
    
    if(Python3_FOUND)
        # Documentation database generation target
        set(DOCS_DB_OUTPUT "${CMAKE_BINARY_DIR}/data/docs.db")
        set(DOCS_JSON_OUTPUT "${CMAKE_BINARY_DIR}/data/docs_database.json")
        set(DOCS_IMPORTER_CPP "${CMAKE_BINARY_DIR}/data/import_docs_rocksdb.cpp")
        
        # Generate JSON documentation database
        add_custom_command(
            OUTPUT ${DOCS_JSON_OUTPUT}
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/data"
            COMMAND ${Python3_EXECUTABLE}
                    ${CMAKE_SOURCE_DIR}/scripts/generate_docs_database.py
                    --output ${DOCS_JSON_OUTPUT}
            DEPENDS
                ${CMAKE_SOURCE_DIR}/scripts/generate_docs_database.py
                ${CMAKE_SOURCE_DIR}/tools/ingest.py
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMENT "Generating documentation JSON database (1151 documents)..."
            VERBATIM
        )
        
        # Generate C++ RocksDB importer
        add_custom_command(
            OUTPUT ${DOCS_IMPORTER_CPP}
            COMMAND ${Python3_EXECUTABLE}
                    ${CMAKE_SOURCE_DIR}/scripts/generate_docs_rocksdb.py
                    --method cpp
                    --output ${DOCS_DB_OUTPUT}
            DEPENDS
                ${DOCS_JSON_OUTPUT}
                ${CMAKE_SOURCE_DIR}/scripts/generate_docs_rocksdb.py
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMENT "Generating RocksDB importer C++ code..."
            VERBATIM
        )
        
        # Build RocksDB importer executable (if RocksDB is available)
        find_package(RocksDB QUIET)
        if(RocksDB_FOUND)
            add_executable(import_docs_rocksdb ${DOCS_IMPORTER_CPP})
            target_link_libraries(import_docs_rocksdb
                PRIVATE
                    RocksDB::rocksdb
                    nlohmann_json::nlohmann_json
            )
            target_compile_features(import_docs_rocksdb PRIVATE cxx_std_17)
            
            # Import documentation into RocksDB
            add_custom_command(
                OUTPUT ${DOCS_DB_OUTPUT}
                COMMAND import_docs_rocksdb ${DOCS_JSON_OUTPUT} ${DOCS_DB_OUTPUT}
                DEPENDS
                    import_docs_rocksdb
                    ${DOCS_JSON_OUTPUT}
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                COMMENT "Importing documentation into RocksDB (relational, graph, vector, metadata, :document)..."
                VERBATIM
            )
            
            # Create top-level target
            add_custom_target(docs_database
                DEPENDS ${DOCS_DB_OUTPUT}
            )
            
            # Install documentation database
            install(
                DIRECTORY ${DOCS_DB_OUTPUT}
                DESTINATION ${CMAKE_INSTALL_PREFIX}/data
                COMPONENT documentation
                OPTIONAL
            )
            
            message(STATUS "Documentation database generation enabled")
            message(STATUS "  Output: ${DOCS_DB_OUTPUT}")
            message(STATUS "  Size: ~2-3 MB (1151 documents)")
        else()
            message(WARNING "RocksDB not found. Documentation database will be generated as JSON only.")
            message(STATUS "JSON documentation database will be available at: ${DOCS_JSON_OUTPUT}")
        endif()
    else()
        message(WARNING "Python3 not found. Documentation database generation disabled.")
        message(STATUS "Install Python 3 to enable automatic documentation database generation.")
    endif()
else()
    message(STATUS "Documentation database generation skipped (THEMIS_BUILD_DOCS_DB=OFF)")
    # If a prebuilt docs.db exists in source tree, install it with the package
    if(EXISTS "${CMAKE_SOURCE_DIR}/data/docs.db")
        install(FILES "${CMAKE_SOURCE_DIR}/data/docs.db"
            DESTINATION ${CMAKE_INSTALL_PREFIX}/data
            COMPONENT documentation
        )
        message(STATUS "Using prebuilt documentation database at ${CMAKE_SOURCE_DIR}/data/docs.db")
    endif()
endif()

# ============================================================================
# CPACK DISTRIBUTION
# ============================================================================

include(CPack)

set(CPACK_PACKAGE_NAME "themisdb")
set(CPACK_PACKAGE_VENDOR "VCC")
set(CPACK_PACKAGE_VERSION "${THEMIS_VERSION_STRING}")

if(WIN32)
    set(CPACK_GENERATOR "ZIP;WIX")
    set(CPACK_PACKAGE_FILE_NAME "ThemisDB-${THEMIS_VERSION_STRING}-Windows-x64")
else()
    set(CPACK_GENERATOR "TGZ;DEB;RPM")
    set(CPACK_PACKAGE_FILE_NAME "themisdb-${THEMIS_VERSION_STRING}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "librocksdb-dev, libgrpc-dev, libgrpc++-dev, libssl-dev, libboost-all-dev")
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
    set(CPACK_RPM_PACKAGE_REQUIRES "rocksdb-devel, grpc-devel, openssl-devel, boost-devel")
endif()

message(STATUS "==========================================")
message(STATUS "ThemisDB ${THEMIS_VERSION} (${THEMIS_VERSION_STRING})")
message(STATUS "Edition: ${THEMIS_EDITION}")
message(STATUS "==========================================")
