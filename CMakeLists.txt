# ThemisDB Root CMakeLists.txt - New Modular Architecture
# Clean, minimal orchestration of the build system

cmake_minimum_required(VERSION 3.20)

# Optional dependency bootstrap during configure (OFF by default for reproducible builds)
set(THEMIS_AUTO_BOOTSTRAP_DEPS OFF CACHE BOOL "Automatically bootstrap missing third-party dependencies (vcpkg, submodules) during CMake configure")

# CRITICAL: Set CMAKE_PROJECT_INCLUDE before any project() call
# This ensures MSVC includes are applied to ALL subprojects (including llama.cpp)
# Must be set unconditionally (before MSVC is defined by project())
set(CMAKE_PROJECT_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/msvc_includes_fix.cmake")

# Initialize selected Visual Studio developer environment (optional)
# This runs VsDevCmd.bat and applies LIB/INCLUDE/LIBPATH/WindowsSdkDir/VCToolsInstallDir
# into the CMake process so compiler detection and configure steps use the selected
# MSVC toolset without external wrapper scripts. Control via -DTHEMIS_MSVC_SELECTION=vs2022
include(cmake/DetectVsDevCmd.cmake OPTIONAL)

# Windows/MSVC fresh-configure guard:
# some local shells lack complete SDK linker paths during CMake try-compile
# even though the actual project build can succeed with the repository's
# configured environment. Pre-validating the compiler avoids those fragile
# early link probes in fresh configure runs.
if(WIN32)
    set(CMAKE_C_COMPILER_WORKS ON CACHE BOOL "C compiler pre-validated" FORCE)
    set(CMAKE_CXX_COMPILER_WORKS ON CACHE BOOL "CXX compiler pre-validated" FORCE)
endif()

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

# Treat uncomponentized install() rules as runtime payload by default.
# This prevents third-party runtime DLLs from leaking into a separate
# 'Unspecified' package component and colliding with the explicit runtime
# package during WiX/MSI generation.
set(CMAKE_INSTALL_DEFAULT_COMPONENT_NAME runtime)

option(ENABLE_FUZZING "Build standalone AFL++/libFuzzer harness targets" OFF)
if(ENABLE_FUZZING)
    message(STATUS "Configuring standalone fuzz harness build")
    set(THEMIS_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" CACHE PATH "ThemisDB root directory")
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
    add_subdirectory(fuzz)
    return()
endif()

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

# Default parallelism used by build wrappers and as a hint for developers.
# This cache entry can be inspected by tools and users; wrappers default to this
# value when no explicit `-j` is provided.

# Optional submodule initialization during configure
if(THEMIS_AUTO_BOOTSTRAP_DEPS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git")
    find_program(THEMIS_GIT_EXECUTABLE git)
    if(THEMIS_GIT_EXECUTABLE)
        set(_themis_missing_submodules "")
        foreach(_themis_submodule IN ITEMS "vcpkg" "ffmpeg" "llama.cpp" "whisper.cpp" "stable-diffusion.cpp")
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
                    "You can run manually: git submodule update --init --recursive vcpkg ffmpeg llama.cpp whisper.cpp stable-diffusion.cpp")
            endif()
        endif()
    endif()
endif()

# Note: wrapper-based build logging was removed; rely on CMake presets and
# the build system's own `--parallel` / `jobs` settings instead.

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

# Core helpers (sets global C++ standard, test helper)
include(cmake/helpers.cmake OPTIONAL)

# Emit deprecation/warning checks (e.g. add_definitions() usage)
include(cmake/deprecated_checks.cmake OPTIONAL)

# 3. Platform detection (OS, CPU, triplet)
include(cmake/platforms/PlatformDetection.cmake)
include(cmake/validation/CrossCompileValidation.cmake)

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

# Fallback: ensure vcpkg "installed/<triplet>/include" is on the global include path
# This helps in environments where per-package imported targets do not propagate
# an umbrella include directory to all targets (diagnostic / compatibility aid).
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/installed/${VCPKG_TARGET_TRIPLET}/include")
    include_directories(BEFORE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/installed/${VCPKG_TARGET_TRIPLET}/include")
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/installed/x64-windows/include")
    include_directories(BEFORE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/installed/x64-windows/include")
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed/x64-windows/include")
    include_directories(BEFORE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed/x64-windows/include")
endif()

# 9. Dependencies
include(cmake/Dependencies.cmake)

# 10. Validation layer
include(cmake/validation/PlatformValidation.cmake)
include(cmake/validation/EditionValidation.cmake)
include(cmake/validation/FeatureValidation.cmake)

# Redirect runtime/library/archive outputs to separate folders on Windows to
# avoid parallel linker/file-lock collisions in shared `bin/` during large
# parallel builds. Tests and runtime loaders should prefer the `bin_out` path
# added below; PATH modifications in tests are updated accordingly.
if(WIN32)
    set(_themis_bin_out "${CMAKE_BINARY_DIR}/bin")
    set(_themis_lib_out "${CMAKE_BINARY_DIR}/lib")
    file(MAKE_DIRECTORY "${_themis_bin_out}")
    file(MAKE_DIRECTORY "${_themis_lib_out}")

    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${_themis_bin_out})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${_themis_bin_out})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${_themis_lib_out})

    foreach(_cfg IN ITEMS Debug Release RelWithDebInfo MinSizeRel)
        string(TOUPPER "${_cfg}" _ucfg)
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${_ucfg} ${_themis_bin_out})
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${_ucfg} ${_themis_bin_out})
        set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${_ucfg} ${_themis_lib_out})
    endforeach()
endif()

# Testing - handled in cmake/CMakeLists.txt via add_subdirectory(tests)
# (Moved to cmake/CMakeLists.txt to be closer to target definitions)

find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_Interpreter_FOUND)
    add_custom_target(license-compliance
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/license-compliance"
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/scripts/check_vcpkg_licenses.py"
                --manifest "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg.json"
                --policy "${CMAKE_CURRENT_SOURCE_DIR}/.license-policy.json"
                --output-dir "${CMAKE_BINARY_DIR}/license-compliance"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMENT "Checking vcpkg dependency licenses against .license-policy.json"
    )
else()
    message(STATUS "Python3 interpreter not found; license-compliance target is unavailable")
endif()

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
# CLOUD SDK INTEGRATION
# ============================================================================

option(THEMIS_WITH_S3_SDK 
       "Enable AWS S3 cloud backup provider (requires aws-sdk-cpp)" 
       OFF)

option(THEMIS_WITH_AZURE_SDK 
       "Enable Azure Blob Storage cloud backup provider (requires azure-storage-blobs-cpp)" 
       OFF)

option(THEMIS_WITH_GCS_SDK 
       "Enable Google Cloud Storage cloud backup provider (requires google-cloud-cpp)" 
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

# ── Wave 3 optional dependencies (Kategorie C Prio 2, Q1 2027) ───────────────

# THEMIS_HAS_GGML — real ggml_new_tensor_1d + ggml_type_register
option(THEMIS_HAS_GGML
       "Enable real ggml tensor allocation via ggml_new_tensor_1d() (requires ggml)"
       OFF)
if(THEMIS_HAS_GGML)
    find_package(ggml QUIET)
    if(ggml_FOUND)
        add_compile_definitions(THEMIS_HAS_GGML)
        message(STATUS "THEMIS_HAS_GGML: ON (ggml found via find_package)")
    else()
        message(WARNING "THEMIS_HAS_GGML requested but ggml not found; falling back to FakeTensor")
    endif()
endif()

# THEMIS_HAS_IO_URING — async readahead via liburing in ggml_tensor_bridge
option(THEMIS_HAS_IO_URING
       "Enable io_uring async readahead in GgmlTensorBridge::prefetch() (requires liburing)"
       OFF)
if(THEMIS_HAS_IO_URING)
    find_library(LIBURING_LIB uring)
    find_path(LIBURING_INCLUDE liburing.h)
    if(LIBURING_LIB AND LIBURING_INCLUDE)
        add_compile_definitions(THEMIS_HAS_IO_URING)
        set(THEMIS_IO_URING_LIB "${LIBURING_LIB}" CACHE INTERNAL "liburing path for target-scoped linking")
        set(THEMIS_IO_URING_INCLUDE "${LIBURING_INCLUDE}" CACHE INTERNAL "liburing include dir for target-scoped includes")
        message(STATUS "THEMIS_HAS_IO_URING: ON (liburing found at ${LIBURING_LIB})")
    else()
        message(WARNING "THEMIS_HAS_IO_URING requested but liburing not found; prefetch is a no-op")
    endif()
endif()

# THEMIS_HAS_YAML_CPP — full YAML spec compliance in KnowledgeBase::loadRulesFromYaml()
option(THEMIS_HAS_YAML_CPP
       "Enable yaml-cpp for full YAML spec compliance in KnowledgeBase (requires yaml-cpp)"
       OFF)
if(THEMIS_HAS_YAML_CPP)
    find_package(yaml-cpp QUIET)
    if(yaml-cpp_FOUND)
        add_compile_definitions(THEMIS_HAS_YAML_CPP)
        link_libraries(yaml-cpp)
        message(STATUS "THEMIS_HAS_YAML_CPP: ON (yaml-cpp found)")
    else()
        message(WARNING "THEMIS_HAS_YAML_CPP requested but yaml-cpp not found; inline parser is used")
    endif()
endif()

# THEMIS_HAS_NLI — real ONNX Runtime inference in NLIFaithfulnessVerifier
option(THEMIS_HAS_NLI
       "Enable real ONNX Runtime NLI inference in NLIFaithfulnessVerifier (requires onnxruntime)"
       OFF)
if(THEMIS_HAS_NLI)
    find_package(onnxruntime QUIET)
    if(onnxruntime_FOUND OR TARGET onnxruntime)
        add_compile_definitions(THEMIS_HAS_NLI)
        message(STATUS "THEMIS_HAS_NLI: ON (onnxruntime found)")
    else()
        message(WARNING "THEMIS_HAS_NLI requested but onnxruntime not found; heuristic fallback is used")
    endif()
endif()

# ── End Wave 3 optional dependencies ─────────────────────────────────────────

if(THEMIS_ENABLE_METRICS_EXPORT)
    add_compile_definitions(THEMIS_ENABLE_METRICS_EXPORT)
    message(STATUS "Metrics export: ENABLED")
endif()

# Set root directory for use in subdirectories (MUST be before add_subdirectory(cmake))
set(THEMIS_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" CACHE PATH "ThemisDB root directory")


# CHIMERA Suite (optional, vendor-neutral)
option(THEMIS_BUILD_CHIMERA "Build CHIMERA adapters, focused tests, and benchmark integration" OFF)
option(BUILD_CHIMERA_ADAPTERS "Deprecated alias for THEMIS_BUILD_CHIMERA" OFF)
if(BUILD_CHIMERA_ADAPTERS)
    message(DEPRECATION "BUILD_CHIMERA_ADAPTERS is deprecated; use THEMIS_BUILD_CHIMERA instead.")
    if(NOT THEMIS_BUILD_CHIMERA)
        set(THEMIS_BUILD_CHIMERA ON CACHE BOOL "Build CHIMERA adapters, focused tests, and benchmark integration" FORCE)
    endif()
endif()

# Module Externalization Options (Wave-2+ targets)
option(THEMIS_EXTERNALIZE_GEO_PLUGIN
       "Build geospatial module as external plugin (default: OFF = integrated mode)" OFF)
if(THEMIS_EXTERNALIZE_GEO_PLUGIN)
    message(STATUS "Geo module: externalized plugin mode (THEMIS_EXTERNALIZE_GEO_PLUGIN=ON)")
    add_subdirectory(plugins/themisdb_geo)
else()
    message(STATUS "Geo module: integrated mode (default, THEMIS_EXTERNALIZE_GEO_PLUGIN=OFF)")
endif()

# Enable CTest infrastructure at root level so `ctest --preset` can find tests
# registered inside add_subdirectory(cmake). Without this, CTestTestfile.cmake
# is only generated in build-dir/cmake/, not in the root binary dir.
enable_testing()

# Main build logic
add_subdirectory(cmake)

# Include CPack configuration if present (provides CPack packaging config and generators)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CPackConfig.cmake")
    include(cmake/CPackConfig.cmake)
    message(STATUS "CPack configuration included from cmake/CPackConfig.cmake")
else()
    message(STATUS "No cmake/CPackConfig.cmake found — packaging disabled until added")
endif()

# ============================================================================
# DOCUMENTATION DATABASE GENERATION
# ============================================================================

# Control docs.db handling for local and CI/CD builds.
# Modes:
#   AUTO     - local: prefer cache, otherwise build; CI: prefer prebuilt from source
#   BUILD    - always build docs.db from sources
#   PREBUILT - use prebuilt docs.db from THEMIS_DOCS_DB_PREBUILT_PATH
#   OFF      - disable docs.db packaging entirely
set(THEMIS_DOCS_DB_MODE "AUTO" CACHE STRING "docs.db mode: AUTO, BUILD, PREBUILT, OFF")
set_property(CACHE THEMIS_DOCS_DB_MODE PROPERTY STRINGS AUTO BUILD PREBUILT OFF)
set(THEMIS_DOCS_DB_PREBUILT_PATH "${CMAKE_SOURCE_DIR}/data/docs.db" CACHE PATH "Prebuilt docs.db directory path")
set(THEMIS_DOCS_DB_CACHE_PATH "${CMAKE_BINARY_DIR}/cache/docs.db" CACHE PATH "Local docs.db cache directory path")

# Backward-compatible flag support
option(THEMIS_BUILD_DOCS_DB "Legacy docs.db switch (ON=BUILD, OFF=PREBUILT)" ON)
if(NOT DEFINED CACHE{THEMIS_DOCS_DB_MODE})
    if(THEMIS_BUILD_DOCS_DB)
        set(THEMIS_DOCS_DB_MODE "BUILD" CACHE STRING "docs.db mode" FORCE)
    else()
        set(THEMIS_DOCS_DB_MODE "PREBUILT" CACHE STRING "docs.db mode" FORCE)
    endif()
endif()

set(_themis_docs_db_mode "${THEMIS_DOCS_DB_MODE}")
if(_themis_docs_db_mode STREQUAL "AUTO")
    if(EXISTS "${THEMIS_DOCS_DB_CACHE_PATH}")
        set(_themis_docs_db_mode "PREBUILT")
        set(_themis_docs_db_selected_prebuilt "${THEMIS_DOCS_DB_CACHE_PATH}")
        message(STATUS "docs.db mode AUTO -> PREBUILT (using local cache: ${THEMIS_DOCS_DB_CACHE_PATH})")
    elseif((DEFINED ENV{CI} OR DEFINED ENV{GITHUB_ACTIONS}) AND EXISTS "${THEMIS_DOCS_DB_PREBUILT_PATH}")
        set(_themis_docs_db_mode "PREBUILT")
        set(_themis_docs_db_selected_prebuilt "${THEMIS_DOCS_DB_PREBUILT_PATH}")
        message(STATUS "docs.db mode AUTO -> PREBUILT (CI with source prebuilt docs.db)")
    else()
        set(_themis_docs_db_mode "BUILD")
        message(STATUS "docs.db mode AUTO -> BUILD")
    endif()
endif()

if(_themis_docs_db_mode STREQUAL "BUILD")
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
                COMMAND ${CMAKE_COMMAND} -E remove_directory "${THEMIS_DOCS_DB_CACHE_PATH}"
                COMMAND ${CMAKE_COMMAND} -E copy_directory ${DOCS_DB_OUTPUT} "${THEMIS_DOCS_DB_CACHE_PATH}"
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
                DESTINATION data
                COMPONENT documentation
                OPTIONAL
            )
            
            message(STATUS "Documentation database generation enabled")
            message(STATUS "  Output: ${DOCS_DB_OUTPUT}")
            message(STATUS "  Size: ~2-3 MB (1151 documents)")
            message(STATUS "  Cache: ${THEMIS_DOCS_DB_CACHE_PATH}")
        else()
            message(WARNING "RocksDB not found. Documentation database will be generated as JSON only.")
            message(STATUS "JSON documentation database will be available at: ${DOCS_JSON_OUTPUT}")
        endif()
    else()
        message(WARNING "Python3 not found. Documentation database generation disabled.")
        message(STATUS "Install Python 3 to enable automatic documentation database generation.")
    endif()
elseif(_themis_docs_db_mode STREQUAL "PREBUILT")
    if(NOT DEFINED _themis_docs_db_selected_prebuilt)
        set(_themis_docs_db_selected_prebuilt "${THEMIS_DOCS_DB_PREBUILT_PATH}")
    endif()

    if(EXISTS "${_themis_docs_db_selected_prebuilt}")
        install(DIRECTORY "${_themis_docs_db_selected_prebuilt}"
            DESTINATION data
            COMPONENT documentation
            OPTIONAL
        )
        message(STATUS "Using prebuilt documentation database at ${_themis_docs_db_selected_prebuilt}")
    else()
        message(WARNING "docs.db mode PREBUILT selected, but path not found: ${_themis_docs_db_selected_prebuilt}")
    endif()
elseif(_themis_docs_db_mode STREQUAL "OFF")
    message(STATUS "Documentation database packaging disabled (THEMIS_DOCS_DB_MODE=OFF)")
else()
    message(FATAL_ERROR "Invalid THEMIS_DOCS_DB_MODE='${THEMIS_DOCS_DB_MODE}'. Allowed: AUTO, BUILD, PREBUILT, OFF")
endif()

if(DEFINED THEMIS_RELEASE_ROOT_DOC_FILES)
    foreach(_themis_doc_file IN LISTS THEMIS_RELEASE_ROOT_DOC_FILES)
        if(EXISTS "${CMAKE_SOURCE_DIR}/${_themis_doc_file}")
            install(FILES "${CMAKE_SOURCE_DIR}/${_themis_doc_file}"
                DESTINATION .
                COMPONENT documentation
                OPTIONAL
            )
        endif()
    endforeach()
endif()

if(DEFINED THEMIS_RELEASE_LAYOUT_DOC)
    if(EXISTS "${CMAKE_SOURCE_DIR}/${THEMIS_RELEASE_LAYOUT_DOC}")
        install(FILES "${CMAKE_SOURCE_DIR}/${THEMIS_RELEASE_LAYOUT_DOC}"
            DESTINATION docs
            COMPONENT documentation
            OPTIONAL
        )
    endif()
endif()

if(DEFINED THEMIS_RELEASE_SETUP_SCRIPT)
    if(EXISTS "${CMAKE_SOURCE_DIR}/${THEMIS_RELEASE_SETUP_SCRIPT}")
        install(FILES "${CMAKE_SOURCE_DIR}/${THEMIS_RELEASE_SETUP_SCRIPT}"
            DESTINATION bin
            COMPONENT tools
            OPTIONAL
        )
    endif()
endif()

# ============================================================================
# OPTIONAL LLM MODEL PACKAGE CONTENT
# ============================================================================

# Models for release packages:
# - Gemma stays local for develop workflows
# - tinyllama is the deploy baseline via Ollama/local model directory
set(THEMIS_MODELS_MODE "SKIP" CACHE STRING "models packaging mode: AUTO, PACKAGE, SKIP")
set_property(CACHE THEMIS_MODELS_MODE PROPERTY STRINGS AUTO PACKAGE SKIP)
set(THEMIS_MODELS_SOURCE_DIR "${CMAKE_SOURCE_DIR}/models" CACHE PATH "Directory containing packaged LLM model files")
option(THEMIS_PACKAGE_INCLUDE_TINYLLAMA "Include tinyllama artifacts in packaged models component" ON)
option(THEMIS_PACKAGE_INCLUDE_GEMMA "Include Gemma artifacts in packaged models component" OFF)

set(_themis_models_mode "${THEMIS_MODELS_MODE}")
if(_themis_models_mode STREQUAL "AUTO")
    if(EXISTS "${THEMIS_MODELS_SOURCE_DIR}")
        set(_themis_models_mode "PACKAGE")
        message(STATUS "models mode AUTO -> PACKAGE")
    else()
        message(FATAL_ERROR "models mode AUTO resolved to PACKAGE, but directory not found: ${THEMIS_MODELS_SOURCE_DIR}")
    endif()
endif()

if(_themis_models_mode STREQUAL "PACKAGE")
    if(EXISTS "${THEMIS_MODELS_SOURCE_DIR}")
        if(THEMIS_PACKAGE_INCLUDE_TINYLLAMA)
            install(DIRECTORY "${THEMIS_MODELS_SOURCE_DIR}/"
                DESTINATION models
                COMPONENT models
                OPTIONAL
                FILES_MATCHING
                PATTERN "*tinyllama*"
                PATTERN "*TinyLlama*"
                PATTERN "*gemma*" EXCLUDE
                PATTERN "*Gemma*" EXCLUDE
            )
        endif()

        if(THEMIS_PACKAGE_INCLUDE_GEMMA)
            install(DIRECTORY "${THEMIS_MODELS_SOURCE_DIR}/"
                DESTINATION models
                COMPONENT models
                OPTIONAL
                FILES_MATCHING
                PATTERN "*gemma*"
                PATTERN "*Gemma*"
            )
        endif()

        message(STATUS "Including packaged runtime models from: ${THEMIS_MODELS_SOURCE_DIR}")
        message(STATUS "  - tinyllama in package: ${THEMIS_PACKAGE_INCLUDE_TINYLLAMA}")
        message(STATUS "  - gemma in package: ${THEMIS_PACKAGE_INCLUDE_GEMMA}")
    else()
        message(FATAL_ERROR "models mode PACKAGE selected, but directory not found: ${THEMIS_MODELS_SOURCE_DIR}")
    endif()
elseif(_themis_models_mode STREQUAL "SKIP")
    message(STATUS "Runtime model packaging disabled (THEMIS_MODELS_MODE=${_themis_models_mode})")
else()
    message(FATAL_ERROR "Invalid THEMIS_MODELS_MODE='${THEMIS_MODELS_MODE}'. Allowed: AUTO, PACKAGE, SKIP")
endif()

# Local develop deployment helper: pull tinyllama via Ollama and export/copy into models/.
if(WIN32)
    add_custom_target(deploy-ollama-tinyllama
        COMMAND "${CMAKE_COMMAND}" -E echo "Deploying tinyllama:1.1b via Ollama to ${THEMIS_MODELS_SOURCE_DIR}"
        COMMAND pwsh -NoProfile -ExecutionPolicy Bypass
            -File "${CMAKE_SOURCE_DIR}/scripts/download-ollama-models-fixed.ps1"
            -ModelNames tinyllama:1.1b
            -OutputDir "${THEMIS_MODELS_SOURCE_DIR}"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMENT "Download and deploy tinyllama model for local develop workflow"
        VERBATIM
    )
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
                DESTINATION data
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
# All CPACK_* variables must be set BEFORE include(CPack) so that
# the generated CPackConfig.cmake / CPackSourceConfig.cmake picks them up.
#
# Deployment folder structure (defined in cmake/CMakeLists.txt install() rules):
#   bin/         — all runtime binaries (themis_server, themisctl, themis-export, themis-model, …)
#   lib/         — shared/static libraries (themis_core, *.so / *.dll / *.a)
#   include/     — public SDK headers
#   data/        — optional: docs.db, legal training data
#   models/      — optional: LLM runtime models
#   bin/benchmarks/ — optional: benchmark binaries (THEMIS_BUILD_BENCHMARKS)
#
# Build-time binaries land in <build-dir>/bin/ via RUNTIME_OUTPUT_DIRECTORY
# (search for "themis_server RUNTIME_OUTPUT_DIRECTORY" in cmake/CMakeLists.txt).
# GNUInstallDirs is included below so cmake/CMakeLists.txt install() rules and
# this CPack block use the same standard variables (CMAKE_INSTALL_BINDIR = bin).
# ============================================================================

set(_themis_runtime_layout_file "${CMAKE_SOURCE_DIR}/cmake/packaging/RuntimeLayout.cmake")
if(EXISTS "${_themis_runtime_layout_file}")
    include("${_themis_runtime_layout_file}")
endif()

include(GNUInstallDirs)

# Linux DEB/RPM packages follow the /usr prefix convention so installed files
# land in /usr/bin, /usr/lib, /usr/include — not /usr/local/*.
# ZIP/TGZ archives use the default (no prefix, relative paths).
if(NOT WIN32 AND NOT APPLE)
    set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")
endif()

# --- Common metadata ---
set(CPACK_PACKAGE_NAME        "themisdb")
set(CPACK_PACKAGE_VENDOR      "ThemisDB Group GmbH")
set(CPACK_PACKAGE_VERSION     "${THEMIS_VERSION_STRING}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "ThemisDB - Enterprise Distributed Database")
set(CPACK_PACKAGE_DESCRIPTION
    "ThemisDB is an enterprise-grade distributed database with built-in AI/ML support, graph traversal, time-series analytics, and a hybrid relational/document/vector storage engine.")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/makr-code/ThemisDB")
set(CPACK_PACKAGE_CONTACT      "ThemisDB Maintainers <themisdb@vcc.example.com>")

# CPack WIX requires a license file extension like .txt/.rtf.
set(_themis_cpack_license_txt "${CMAKE_BINARY_DIR}/CPACK-LICENSE.txt")
configure_file("${CMAKE_SOURCE_DIR}/LICENSE" "${_themis_cpack_license_txt}" COPYONLY)
set(CPACK_RESOURCE_FILE_LICENSE "${_themis_cpack_license_txt}")

# Source archive (works on all platforms)
set(CPACK_SOURCE_GENERATOR "TGZ;ZIP")
set(CPACK_SOURCE_IGNORE_FILES
    "\\.git/"
    "\\.github/"
    "build/"
    "vcpkg/"
    "\\.vs/"
    "node_modules/"
    "\.gitmodules"
)

if(WIN32)
    # ---- Windows: ZIP + WiX/MSI (+ NSIS as optional fallback installer) ----
    # WIX requires WiX Toolset v3 (candle.exe/light.exe) on PATH.
    # NSIS requires NSIS installed (makensis on PATH) — add it only when available.
    find_program(_NSIS_MAKE makensis)
    if(_NSIS_MAKE)
        set(CPACK_GENERATOR "ZIP;WIX;NSIS")
    else()
        set(CPACK_GENERATOR "ZIP;WIX")
    endif()
    set(CPACK_PACKAGE_FILE_NAME "ThemisDB-${THEMIS_VERSION_STRING}-Windows-x64")
    # WiX Product/@Version must be numeric (x.y.z.w); prerelease labels are invalid here.
    set(CPACK_PACKAGE_VERSION "${_ver_numeric}")

    # WiX / MSI specific
    # A stable upgrade GUID ensures "upgrade in place" behaviour across versions.
    # Do NOT change this GUID once the product has been released.
    set(CPACK_WIX_UPGRADE_GUID     "E2A3B4C5-D6E7-4F80-91A2-B3C4D5E6F7A8")
    set(CPACK_WIX_PRODUCT_GUID     "*")   # auto-generated per build is fine for ProductCode
    set(CPACK_WIX_PRODUCT_NAME     "ThemisDB ${THEMIS_VERSION_STRING}")
    set(CPACK_WIX_PROGRAM_MENU_FOLDER "ThemisDB")
    set(CPACK_WIX_PROPERTY_ARPURLINFOABOUT  "https://github.com/makr-code/ThemisDB")
    set(CPACK_WIX_PROPERTY_ARPHELPLINK      "https://github.com/makr-code/ThemisDB/issues")
    set(CPACK_WIX_PROPERTY_ARPCONTACT       "ThemisDB Maintainers")

elseif(APPLE)
    # ---- macOS: TGZ + ZIP ----
    set(CPACK_GENERATOR "TGZ;ZIP")
    set(CPACK_PACKAGE_FILE_NAME "themisdb-${THEMIS_VERSION_STRING}-macOS-${CMAKE_SYSTEM_PROCESSOR}")

else()
    # ---- Linux: TGZ + DEB + RPM ----
    set(CPACK_GENERATOR "TGZ;DEB;RPM")
    set(CPACK_PACKAGE_FILE_NAME
        "themisdb-${THEMIS_VERSION_STRING}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")

    # DEB metadata
    set(CPACK_DEBIAN_PACKAGE_NAME        "themisdb")
    set(CPACK_DEBIAN_PACKAGE_VERSION     "${THEMIS_VERSION_STRING}")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER  "ThemisDB Maintainers <themisdb@vcc.example.com>")
    set(CPACK_DEBIAN_PACKAGE_DESCRIPTION
        "ThemisDB Enterprise Distributed Database\n Enterprise-grade distributed database with AI/ML support, graph traversal, time-series analytics, and a hybrid relational/document/vector storage engine.")
    set(CPACK_DEBIAN_PACKAGE_SECTION     "database")
    set(CPACK_DEBIAN_PACKAGE_PRIORITY    "optional")
    set(CPACK_DEBIAN_PACKAGE_HOMEPAGE    "https://github.com/makr-code/ThemisDB")
    # Let CPack derive the file name following Debian conventions (name_version_arch.deb)
    set(CPACK_DEBIAN_FILE_NAME           "DEB-DEFAULT")
    # Runtime library dependencies (not -dev packages — those are for compilation only).
    # dpkg-shlibdeps (SHLIBDEPS ON below) will auto-resolve most shared-library deps;
    # these explicit entries act as a conservative fallback for static/mixed builds.
    set(CPACK_DEBIAN_PACKAGE_DEPENDS
        "librocksdb8.9 | librocksdb-dev, libgrpc++1.51 | libgrpc++-dev, libssl3, libboost-filesystem1.83.0 | libboost-filesystem-dev")
    # SHLIBDEPS resolves shared-library deps automatically when dpkg-shlibdeps is available.
    # Falls back gracefully when the tool is absent (local builds without full dpkg stack).
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS   ON)
    if(EXISTS "${CMAKE_SOURCE_DIR}/debian/themisdb.postinst" AND EXISTS "${CMAKE_SOURCE_DIR}/debian/themisdb.postrm")
        set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
            "${CMAKE_SOURCE_DIR}/debian/themisdb.postinst;${CMAKE_SOURCE_DIR}/debian/themisdb.postrm")
    endif()

    # RPM metadata
    set(CPACK_RPM_PACKAGE_NAME       "themisdb")
    set(CPACK_RPM_PACKAGE_VERSION    "${THEMIS_VERSION_STRING}")
    set(CPACK_RPM_PACKAGE_LICENSE    "Proprietary")
    set(CPACK_RPM_PACKAGE_GROUP      "Applications/Databases")
    set(CPACK_RPM_PACKAGE_URL        "https://github.com/makr-code/ThemisDB")
    set(CPACK_RPM_PACKAGE_REQUIRES
        "rocksdb, grpc, openssl-libs, boost-filesystem")
endif()

# include(CPack) must come AFTER all CPACK_* variables are set.
include(CPack)

message(STATUS "==========================================")
message(STATUS "ThemisDB ${THEMIS_VERSION} (${THEMIS_VERSION_STRING})")
message(STATUS "Edition: ${THEMIS_EDITION}")
message(STATUS "==========================================")
