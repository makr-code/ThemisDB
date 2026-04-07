# ThemisDB Root CMakeLists.txt - New Modular Architecture
# Clean, minimal orchestration of the build system

cmake_minimum_required(VERSION 3.20)

# Optional dependency bootstrap during configure (OFF by default for reproducible builds)
set(THEMIS_AUTO_BOOTSTRAP_DEPS OFF CACHE BOOL "Automatically bootstrap missing third-party dependencies (vcpkg, submodules) during CMake configure")

# CRITICAL: Set CMAKE_PROJECT_INCLUDE before any project() call
# This ensures MSVC includes are applied to ALL subprojects (including llama.cpp)
# Must be set unconditionally (before MSVC is defined by project())
set(CMAKE_PROJECT_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/msvc_includes_fix.cmake")

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

# Preflight for externally provided toolchain file
# Common failure: CMAKE_TOOLCHAIN_FILE points to missing repo-local vcpkg path.
if(DEFINED CMAKE_TOOLCHAIN_FILE AND NOT EXISTS "${CMAKE_TOOLCHAIN_FILE}")
    if(THEMIS_AUTO_BOOTSTRAP_DEPS)
        find_program(THEMIS_GIT_EXECUTABLE git)
        if(NOT THEMIS_GIT_EXECUTABLE)
            message(FATAL_ERROR
                "CMAKE_TOOLCHAIN_FILE is missing: ${CMAKE_TOOLCHAIN_FILE}\n"
                "Auto-bootstrap is enabled but 'git' was not found in PATH.")
        endif()

        set(_themis_vcpkg_dir "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg")
        set(_themis_vcpkg_toolchain "${_themis_vcpkg_dir}/scripts/buildsystems/vcpkg.cmake")

        if(NOT EXISTS "${_themis_vcpkg_toolchain}")
            message(STATUS "Auto-bootstrap: cloning vcpkg into ${_themis_vcpkg_dir}")
            execute_process(
                COMMAND ${THEMIS_GIT_EXECUTABLE} clone https://github.com/microsoft/vcpkg.git "${_themis_vcpkg_dir}"
                RESULT_VARIABLE _themis_vcpkg_clone_result
                OUTPUT_QUIET
                ERROR_VARIABLE _themis_vcpkg_clone_error
            )
            if(NOT _themis_vcpkg_clone_result EQUAL 0)
                message(FATAL_ERROR
                    "Failed to auto-bootstrap vcpkg.\n"
                    "git clone error: ${_themis_vcpkg_clone_error}\n"
                    "Run manually: powershell -ExecutionPolicy Bypass -File scripts/setup-third-party.ps1")
            endif()
        endif()

        set(CMAKE_TOOLCHAIN_FILE "${_themis_vcpkg_toolchain}" CACHE STRING "vcpkg toolchain file" FORCE)
        message(STATUS "Auto-bootstrap: using toolchain ${CMAKE_TOOLCHAIN_FILE}")
    else()
        message(FATAL_ERROR
            "CMAKE_TOOLCHAIN_FILE points to a missing file:\n"
            "  ${CMAKE_TOOLCHAIN_FILE}\n\n"
            "Fix options:\n"
            "  1) Bootstrap dependencies: powershell -ExecutionPolicy Bypass -File scripts/setup-third-party.ps1\n"
            "  2) Reconfigure with -DTHEMIS_AUTO_BOOTSTRAP_DEPS=ON\n"
            "  3) Set CMAKE_TOOLCHAIN_FILE to a valid vcpkg toolchain path")
    endif()
endif()

# Store full semantic version
set(THEMIS_VERSION_STRING "${_ver_raw}")

# Optional submodule initialization during configure
if(THEMIS_AUTO_BOOTSTRAP_DEPS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git")
    find_program(THEMIS_GIT_EXECUTABLE git)
    if(THEMIS_GIT_EXECUTABLE)
        set(_themis_missing_submodules "")
        foreach(_themis_submodule IN ITEMS "ffmpeg" "llama.cpp" "whisper.cpp" "stable-diffusion.cpp")
            if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${_themis_submodule}/.git" AND
               NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${_themis_submodule}/CMakeLists.txt")
                list(APPEND _themis_missing_submodules "${_themis_submodule}")
            endif()
        endforeach()

        if(_themis_missing_submodules)
            message(STATUS "Auto-bootstrap: initializing submodules: ${_themis_missing_submodules}")
            execute_process(
                COMMAND ${THEMIS_GIT_EXECUTABLE} -C "${CMAKE_CURRENT_SOURCE_DIR}" submodule update --init --recursive ${_themis_missing_submodules}
                RESULT_VARIABLE _themis_submodule_result
                ERROR_VARIABLE _themis_submodule_error
            )
            if(NOT _themis_submodule_result EQUAL 0)
                message(WARNING
                    "Failed to initialize submodules (${_themis_missing_submodules}): ${_themis_submodule_error}\n"
                    "You can run manually: git submodule update --init --recursive ffmpeg llama.cpp whisper.cpp stable-diffusion.cpp")
            endif()
        endif()
    endif()
endif()

# ============================================================================
# MODULAR CMAKE INCLUDES - REFACTORED BUILD SYSTEM
# ============================================================================
# Build order (critical):
# 1. Version info
# 2. Compiler settings
# 3. Platform detection (arch/OS)
# 4. Architecture optimizations
# 5. Preload targets
# 6. Edition selection (BEFORE vcpkg to set feature flags!)
# 7. Feature configuration (BEFORE vcpkg to enable correct features!)
# 8. vcpkg Configuration (AFTER features are set!)
# 9. Dependencies
# 10. Validation (platform, edition, features)

# 1. Version management (version info only)
include(cmake/Versions.cmake)

# 2. Compiler setup
include(cmake/CompilerOptions.cmake)

# 2.5. Compiler cache launcher (sccache/ccache)
include(cmake/CompilerCache.cmake)

# 3. Platform detection (OS, CPU, triplet)
include(cmake/platforms/PlatformDetection.cmake)

# 4. Architecture optimizations (AVX2, NEON, etc.)
include(cmake/platforms/ArchitectureOptimizations.cmake)

# 5. Preload targets for system packages
include(cmake/PreloadTargets.cmake)

# 6. Edition selection (MINIMAL, COMMUNITY, ENTERPRISE, HYPERSCALER)
include(cmake/editions/EditionDefaults.cmake)

# 7. Feature configuration (LLM, GPU, Network, Security, Tools, Optimizations)
include(cmake/features/FeatureDefaults.cmake)

# 7.5. Edition-Feature Matrix Validation (ensures features are compatible with edition)
include(cmake/EditionMatrix.cmake)

# 8. vcpkg Configuration (binary cache, feature dependencies)
include(cmake/VcpkgConfiguration.cmake)

# 9. Dependencies
include(cmake/Dependencies.cmake)

# 10. Validation layer
include(cmake/validation/PlatformValidation.cmake)
include(cmake/validation/EditionValidation.cmake)
include(cmake/validation/FeatureValidation.cmake)

# Testing - handled in cmake/CMakeLists.txt via add_subdirectory(tests)
# (Moved to cmake/CMakeLists.txt to be closer to target definitions)

# ============================================================================
# PERFORMANCE MEASUREMENT FEATURE FLAGS
# ============================================================================

option(THEMIS_ENABLE_CYCLE_METRICS 
       "Enable CPU cycle measurement (low overhead: ~0.05%)" 
       OFF)

option(THEMIS_ENABLE_DETAILED_METRICS 
       "Enable detailed per-function metrics (overhead: ~5-10%)" 
       OFF)

option(THEMIS_ENABLE_GPU_CYCLE_METRICS 
       "Enable GPU cycle measurement via CUDA events (overhead: ~10-20%)" 
       OFF)

option(THEMIS_ENABLE_PMU_COUNTERS 
       "Enable hardware performance counters via perf (overhead: ~10-50%)" 
       OFF)

option(THEMIS_ENABLE_IO_URING
       "Enable io_uring zero-copy I/O path for network performance (Linux ≥ 5.1 only)"
       OFF)

option(THEMIS_ENABLE_METRICS_EXPORT 
       "Enable Prometheus/CHIMERA export (adds async overhead)" 
       OFF)

option(THEMIS_BENCHMARK_MODE 
       "Enable all performance measurement features (FOR BENCHMARKING ONLY!)" 
       OFF)

# ============================================================================
# THREAD SANITIZER SUPPORT
# ============================================================================

option(THEMIS_ENABLE_TSAN 
       "Enable ThreadSanitizer for detecting data races (overhead: ~5-15x)" 
       OFF)

if(THEMIS_ENABLE_TSAN)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        message(FATAL_ERROR "ThreadSanitizer requires Clang or GCC compiler")
    endif()
    
    set(TSAN_FLAGS "-fsanitize=thread -g -O1 -fno-omit-frame-pointer")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TSAN_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${TSAN_FLAGS}")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${TSAN_FLAGS}")
    
    # Set TSan suppressions file if it exists
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tsan_suppressions.txt")
        set(ENV{TSAN_OPTIONS} "suppressions=${CMAKE_CURRENT_SOURCE_DIR}/tsan_suppressions.txt")
        message(STATUS "ThreadSanitizer suppressions: ${CMAKE_CURRENT_SOURCE_DIR}/tsan_suppressions.txt")
    endif()
    
    message(WARNING "==========================================")
    message(WARNING "ThreadSanitizer (TSan) is ENABLED")
    message(WARNING "Expected slowdown: 5-15x")
    message(WARNING "Memory overhead: 5-10x")
    message(WARNING "FOR TESTING ONLY - NOT FOR PRODUCTION")
    message(WARNING "==========================================")
endif()

# When THEMIS_BENCHMARK_MODE is ON, enable all measurement features
if(THEMIS_BENCHMARK_MODE)
    set(THEMIS_ENABLE_CYCLE_METRICS ON CACHE BOOL "" FORCE)
    set(THEMIS_ENABLE_DETAILED_METRICS ON CACHE BOOL "" FORCE)
    set(THEMIS_ENABLE_GPU_CYCLE_METRICS ON CACHE BOOL "" FORCE)
    set(THEMIS_ENABLE_PMU_COUNTERS ON CACHE BOOL "" FORCE)
    set(THEMIS_ENABLE_METRICS_EXPORT ON CACHE BOOL "" FORCE)
    
    message(WARNING "==========================================")
    message(WARNING "THEMIS_BENCHMARK_MODE is ENABLED")
    message(WARNING "All performance measurement features are ON")
    message(WARNING "Expected overhead: 5-50%")
    message(WARNING "FOR BENCHMARKING ONLY - NOT FOR PRODUCTION")
    message(WARNING "==========================================")
endif()

# Set compile definitions based on options
if(THEMIS_ENABLE_CYCLE_METRICS)
    add_compile_definitions(THEMIS_ENABLE_CYCLE_METRICS)
    message(STATUS "Cycle metrics: ENABLED")
endif()

if(THEMIS_ENABLE_DETAILED_METRICS)
    add_compile_definitions(THEMIS_ENABLE_DETAILED_METRICS)
    message(STATUS "Detailed metrics: ENABLED")
endif()

if(THEMIS_ENABLE_GPU_CYCLE_METRICS)
    add_compile_definitions(THEMIS_ENABLE_GPU_CYCLE_METRICS)
    message(STATUS "GPU cycle metrics: ENABLED")
endif()

if(THEMIS_ENABLE_PMU_COUNTERS)
    add_compile_definitions(THEMIS_ENABLE_PMU_COUNTERS)
    message(STATUS "PMU counters: ENABLED")
endif()

if(THEMIS_ENABLE_IO_URING)
    add_compile_definitions(THEMIS_ENABLE_IO_URING)
    message(STATUS "io_uring zero-copy I/O: ENABLED")
endif()

if(THEMIS_ENABLE_METRICS_EXPORT)
    add_compile_definitions(THEMIS_ENABLE_METRICS_EXPORT)
    message(STATUS "Metrics export: ENABLED")
endif()

# Set root directory for use in subdirectories (MUST be before add_subdirectory(cmake))
set(THEMIS_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" CACHE PATH "ThemisDB root directory")


# CHIMERA Adapters (optional, vendor-neutral)
option(BUILD_CHIMERA_ADAPTERS "Build CHIMERA database adapters" OFF)
if(BUILD_CHIMERA_ADAPTERS)
    add_subdirectory(adapters/chimera)
endif()

# Enable CTest infrastructure at root level so `ctest --preset` can find tests
# registered inside add_subdirectory(cmake). Without this, CTestTestfile.cmake
# is only generated in build-dir/cmake/, not in the root binary dir.
enable_testing()

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
# LEGAL TRAINING DATA INGESTION (HuggingFace legal-bert-de)
# ============================================================================

# Control whether legal training data is ingested during build. Defaults OFF.
# This is an optional feature that downloads and imports HuggingFace legal-bert-de dataset.
option(THEMIS_BUILD_LEGAL_TRAINING_DATA "Generate legal training data database during build" OFF)

if(THEMIS_BUILD_LEGAL_TRAINING_DATA)
    # Find Python3 for legal training data ingestion
    find_package(Python3 COMPONENTS Interpreter)
    
    if(Python3_FOUND)
        # Legal training data paths
        set(LEGAL_JSON_OUTPUT "${CMAKE_BINARY_DIR}/data/legal_training_data.json")
        set(LEGAL_DB_OUTPUT "${CMAKE_BINARY_DIR}/data/legal_training.db")
        set(LEGAL_DB_STAMP "${CMAKE_BINARY_DIR}/data/legal_training.db.stamp")
        set(LEGAL_IMPORTER_CPP "${CMAKE_BINARY_DIR}/data/import_legal_rocksdb.cpp")
        
        # Step 1: Download and ingest HuggingFace dataset to JSON
        add_custom_command(
            OUTPUT ${LEGAL_JSON_OUTPUT}
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/data"
            COMMAND ${Python3_EXECUTABLE}
                    ${CMAKE_SOURCE_DIR}/scripts/ingest_legal_training_data.py
                    --output ${LEGAL_JSON_OUTPUT}
                    --max-samples 10000
            DEPENDS
                ${CMAKE_SOURCE_DIR}/scripts/ingest_legal_training_data.py
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMENT "Ingesting HuggingFace legal-bert-de dataset (10000 samples)..."
            VERBATIM
        )
        
        # Step 2: Generate C++ RocksDB importer
        add_custom_command(
            OUTPUT ${LEGAL_IMPORTER_CPP}
            COMMAND ${Python3_EXECUTABLE}
                    ${CMAKE_SOURCE_DIR}/scripts/generate_legal_rocksdb.py
                    --method cpp
                    --output ${LEGAL_DB_OUTPUT}
            DEPENDS
                ${LEGAL_JSON_OUTPUT}
                ${CMAKE_SOURCE_DIR}/scripts/generate_legal_rocksdb.py
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMENT "Generating RocksDB importer for legal training data..."
            VERBATIM
        )
        
        # Step 3: Build and run RocksDB importer (if RocksDB is available)
        find_package(RocksDB QUIET)
        if(RocksDB_FOUND)
            # Build importer executable
            add_executable(import_legal_rocksdb ${LEGAL_IMPORTER_CPP})
            target_link_libraries(import_legal_rocksdb
                PRIVATE
                    RocksDB::rocksdb
                    nlohmann_json::nlohmann_json
            )
            target_compile_features(import_legal_rocksdb PRIVATE cxx_std_17)
            
            # Step 4: Import legal training data into RocksDB
            # Use a stamp file because RocksDB creates a directory, which CMake cannot track properly
            add_custom_command(
                OUTPUT ${LEGAL_DB_STAMP}
                COMMAND import_legal_rocksdb ${LEGAL_JSON_OUTPUT} ${LEGAL_DB_OUTPUT}
                COMMAND ${CMAKE_COMMAND} -E touch ${LEGAL_DB_STAMP}
                DEPENDS
                    import_legal_rocksdb
                    ${LEGAL_JSON_OUTPUT}
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                COMMENT "Importing legal training data into RocksDB..."
                VERBATIM
            )
            
            # Step 5: Create top-level target
            add_custom_target(legal_training_data
                DEPENDS ${LEGAL_DB_STAMP}
            )
            
            # Step 6: Installation (optional component)
            install(
                DIRECTORY ${LEGAL_DB_OUTPUT}
                DESTINATION ${CMAKE_INSTALL_PREFIX}/data
                COMPONENT legal_training
                OPTIONAL
            )
            
            message(STATUS "Legal training data ingestion enabled")
            message(STATUS "  Output: ${LEGAL_DB_OUTPUT}")
            message(STATUS "  Source: HuggingFace joelito/legal_mc_de")
            message(STATUS "  Max samples: 10000")
        else()
            message(WARNING "RocksDB not found. Legal training data will be JSON only.")
            message(STATUS "JSON legal training data will be available at: ${LEGAL_JSON_OUTPUT}")
            
            # Create a target for JSON-only output
            add_custom_target(legal_training_data
                DEPENDS ${LEGAL_JSON_OUTPUT}
            )
        endif()
    else()
        message(WARNING "Python3 not found. Legal training data ingestion disabled.")
        message(STATUS "Install Python 3 to enable automatic legal training data ingestion.")
    endif()
else()
    message(STATUS "Legal training data ingestion skipped (THEMIS_BUILD_LEGAL_TRAINING_DATA=OFF)")
    message(STATUS "To enable: cmake -DTHEMIS_BUILD_LEGAL_TRAINING_DATA=ON")
endif()

# ============================================================================
# MULTI-PLATFORM PACKAGE SYSTEM (CMake-Only)
# ============================================================================

# Enable multi-platform vcpkg package building targets
option(THEMIS_ENABLE_PACKAGE_SYSTEM "Enable multi-platform package building targets" ON)
if(THEMIS_ENABLE_PACKAGE_SYSTEM AND EXISTS "${CMAKE_SOURCE_DIR}/cmake/VcpkgPackageSystem.cmake")
    include(cmake/VcpkgPackageSystem.cmake)
    message(STATUS "Multi-platform package system: ENABLED")
else()
    message(STATUS "Multi-platform package system: DISABLED")
endif()

# ============================================================================
# DOCKER BUILD SYSTEM (CMake-Only)
# ============================================================================

# Enable Docker build targets
option(THEMIS_ENABLE_DOCKER_SYSTEM "Enable Docker build targets" ON)
if(THEMIS_ENABLE_DOCKER_SYSTEM AND EXISTS "${CMAKE_SOURCE_DIR}/cmake/DockerBuildSystem.cmake")
    include(cmake/DockerBuildSystem.cmake)
    message(STATUS "Docker build system: ENABLED")
else()
    message(STATUS "Docker build system: DISABLED")
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
