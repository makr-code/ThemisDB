# CMake script to build vcpkg packages
# Called by VcpkgPackageSystem.cmake custom targets

cmake_minimum_required(VERSION 3.20)

# Check required variables
if(NOT DEFINED THEMIS_VCPKG_ROOT)
    message(FATAL_ERROR "THEMIS_VCPKG_ROOT not defined")
endif()

if(NOT DEFINED THEMIS_TRIPLET)
    message(FATAL_ERROR "THEMIS_TRIPLET not defined")
endif()

if(NOT DEFINED THEMIS_EDITION)
    set(THEMIS_EDITION "COMMUNITY")
endif()

if(NOT DEFINED THEMIS_OUTPUT_DIR)
    message(FATAL_ERROR "THEMIS_OUTPUT_DIR not defined")
endif()

# ============================================================================
# Platform-specific vcpkg executable
# ============================================================================

if(WIN32 OR THEMIS_TRIPLET MATCHES "windows")
    set(VCPKG_EXECUTABLE "${THEMIS_VCPKG_ROOT}/vcpkg.exe")
    set(BOOTSTRAP_SCRIPT "${THEMIS_VCPKG_ROOT}/bootstrap-vcpkg.bat")
else()
    set(VCPKG_EXECUTABLE "${THEMIS_VCPKG_ROOT}/vcpkg")
    set(BOOTSTRAP_SCRIPT "${THEMIS_VCPKG_ROOT}/bootstrap-vcpkg.sh")
endif()

# ============================================================================
# Bootstrap vcpkg if needed
# ============================================================================

if(NOT EXISTS "${VCPKG_EXECUTABLE}")
    message(STATUS "Bootstrapping vcpkg for ${THEMIS_TRIPLET}...")
    
    if(THEMIS_TRIPLET MATCHES "linux" AND WIN32)
        # Building Linux packages on Windows - use WSL
        message(STATUS "Using WSL for Linux package build...")
        
        # Convert Windows path to WSL path
        string(REPLACE "\\" "/" _vcpkg_root_unix "${THEMIS_VCPKG_ROOT}")
        string(REGEX REPLACE "^([A-Z]):" "/mnt/\\L\\1" _vcpkg_root_wsl "${_vcpkg_root_unix}")
        
        execute_process(
            COMMAND wsl bash -c "cd ${_vcpkg_root_wsl} && ./bootstrap-vcpkg.sh"
            RESULT_VARIABLE _result
            OUTPUT_VARIABLE _output
            ERROR_VARIABLE _error
        )
    else()
        execute_process(
            COMMAND "${BOOTSTRAP_SCRIPT}"
            WORKING_DIRECTORY "${THEMIS_VCPKG_ROOT}"
            RESULT_VARIABLE _result
            OUTPUT_VARIABLE _output
            ERROR_VARIABLE _error
        )
    endif()
    
    if(NOT _result EQUAL 0)
        message(FATAL_ERROR "vcpkg bootstrap failed: ${_error}")
    endif()
    
    message(STATUS "vcpkg bootstrapped successfully")
endif()

# ============================================================================
# Manifest file selection
# ============================================================================

string(TOLOWER "${THEMIS_EDITION}" _edition_lower)

# Try Docker manifest first, fall back to root
set(MANIFEST_ROOT "${CMAKE_SOURCE_DIR}/docker")
set(MANIFEST_FILE "${MANIFEST_ROOT}/vcpkg-${_edition_lower}.json")

if(NOT EXISTS "${MANIFEST_FILE}")
    set(MANIFEST_ROOT "${CMAKE_SOURCE_DIR}")
    set(MANIFEST_FILE "${CMAKE_SOURCE_DIR}/vcpkg.json")
endif()

if(NOT EXISTS "${MANIFEST_FILE}")
    message(FATAL_ERROR "No vcpkg manifest found for edition ${THEMIS_EDITION}")
endif()

# ============================================================================
# Create output directory
# ============================================================================

file(MAKE_DIRECTORY "${THEMIS_OUTPUT_DIR}")

message(STATUS "==========================================")
message(STATUS "Building vcpkg packages")
message(STATUS "==========================================")
message(STATUS "Triplet: ${THEMIS_TRIPLET}")
message(STATUS "Edition: ${THEMIS_EDITION}")
message(STATUS "Manifest: ${MANIFEST_FILE}")
message(STATUS "Output: ${THEMIS_OUTPUT_DIR}")
message(STATUS "==========================================")

# ============================================================================
# Run vcpkg install
# ============================================================================

set(_vcpkg_args
    install
    --triplet=${THEMIS_TRIPLET}
    --x-install-root=${THEMIS_OUTPUT_DIR}
    --x-manifest-root=${MANIFEST_ROOT}
    --x-buildtrees-root=${THEMIS_VCPKG_ROOT}/buildtrees
    --x-packages-root=${THEMIS_VCPKG_ROOT}/packages
    --x-downloads-root=${THEMIS_VCPKG_ROOT}/downloads
)

if(THEMIS_TRIPLET MATCHES "linux" AND WIN32)
    # Use WSL for Linux builds on Windows
    message(STATUS "Building Linux packages via WSL...")
    
    # Convert paths to WSL format
    string(REPLACE "\\" "/" _output_dir_unix "${THEMIS_OUTPUT_DIR}")
    string(REGEX REPLACE "^([A-Z]):" "/mnt/\\L\\1" _output_dir_wsl "${_output_dir_unix}")
    
    string(REPLACE "\\" "/" _manifest_root_unix "${MANIFEST_ROOT}")
    string(REGEX REPLACE "^([A-Z]):" "/mnt/\\L\\1" _manifest_root_wsl "${_manifest_root_unix}")
    
    string(REPLACE "\\" "/" _vcpkg_root_unix "${THEMIS_VCPKG_ROOT}")
    string(REGEX REPLACE "^([A-Z]):" "/mnt/\\L\\1" _vcpkg_root_wsl "${_vcpkg_root_unix}")
    
    # Build WSL command
    set(_wsl_cmd 
        "cd ${_vcpkg_root_wsl} && ./vcpkg install --triplet=${THEMIS_TRIPLET} --x-install-root=${_output_dir_wsl} --x-manifest-root=${_manifest_root_wsl} --x-buildtrees-root=${_vcpkg_root_wsl}/buildtrees --x-packages-root=${_vcpkg_root_wsl}/packages --x-downloads-root=${_vcpkg_root_wsl}/downloads"
    )
    
    execute_process(
        COMMAND wsl bash -c "${_wsl_cmd}"
        RESULT_VARIABLE _result
        OUTPUT_VARIABLE _output
        ERROR_VARIABLE _error
        ECHO_OUTPUT_VARIABLE
        ECHO_ERROR_VARIABLE
    )
else()
    # Native build
    execute_process(
        COMMAND "${VCPKG_EXECUTABLE}" ${_vcpkg_args}
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        RESULT_VARIABLE _result
        OUTPUT_VARIABLE _output
        ERROR_VARIABLE _error
        ECHO_OUTPUT_VARIABLE
        ECHO_ERROR_VARIABLE
    )
endif()

if(NOT _result EQUAL 0)
    message(FATAL_ERROR "vcpkg install failed with exit code ${_result}")
endif()

# ============================================================================
# Success
# ============================================================================

message(STATUS "==========================================")
message(STATUS "Packages built successfully!")
message(STATUS "Output directory: ${THEMIS_OUTPUT_DIR}")
message(STATUS "==========================================")

# Calculate package size
file(GLOB_RECURSE _package_files "${THEMIS_OUTPUT_DIR}/*")
list(LENGTH _package_files _file_count)

set(_total_size 0)
foreach(_file ${_package_files})
    if(IS_DIRECTORY "${_file}")
        continue()
    endif()
    file(SIZE "${_file}" _file_size)
    math(EXPR _total_size "${_total_size} + ${_file_size}")
endforeach()

math(EXPR _size_mb "${_total_size} / 1048576")
message(STATUS "Package statistics:")
message(STATUS "  Files: ${_file_count}")
message(STATUS "  Size: ${_size_mb} MB")
