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

# In script mode (-P), CMAKE_SOURCE_DIR points to the current working directory,
# not necessarily the repository root. Resolve the project root from this script.
get_filename_component(THEMIS_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

# Some callers pass quoted -D values (e.g. -DTHEMIS_VCPKG_ROOT=\"C:/...\").
# Strip a single pair of surrounding quotes so path checks remain valid.
foreach(_var IN ITEMS THEMIS_VCPKG_ROOT THEMIS_TRIPLET THEMIS_EDITION THEMIS_OUTPUT_DIR)
    if(DEFINED ${_var})
        set(_sanitized_value "${${_var}}")
        string(REPLACE "\\\"" "\"" _sanitized_value "${_sanitized_value}")
        string(REGEX REPLACE "^\"(.*)\"$" "\\1" _sanitized_value "${_sanitized_value}")
        set(${_var} "${_sanitized_value}")
    endif()
endforeach()

# Normalize filesystem paths for reliable EXISTS checks across generators/shells.
file(TO_CMAKE_PATH "${THEMIS_VCPKG_ROOT}" THEMIS_VCPKG_ROOT)
file(TO_CMAKE_PATH "${THEMIS_OUTPUT_DIR}" THEMIS_OUTPUT_DIR)
if(WIN32)
    string(REGEX REPLACE "^/([A-Za-z]:/)" "\\1" THEMIS_VCPKG_ROOT "${THEMIS_VCPKG_ROOT}")
    string(REGEX REPLACE "^/([A-Za-z]:/)" "\\1" THEMIS_OUTPUT_DIR "${THEMIS_OUTPUT_DIR}")
endif()

function(themis_win_path_to_wsl input_path output_var)
    string(REPLACE "\\" "/" _path "${input_path}")
    if(_path MATCHES "^([A-Za-z]):(.*)")
        string(TOLOWER "${CMAKE_MATCH_1}" _drive)
        set(_path "/mnt/${_drive}${CMAKE_MATCH_2}")
    endif()
    set(${output_var} "${_path}" PARENT_SCOPE)
endfunction()

# ============================================================================
# Platform-specific vcpkg executable
# ============================================================================

if(THEMIS_TRIPLET MATCHES "linux" AND WIN32)
    set(VCPKG_EXECUTABLE "${THEMIS_VCPKG_ROOT}/vcpkg")
    set(BOOTSTRAP_SCRIPT "${THEMIS_VCPKG_ROOT}/bootstrap-vcpkg.sh")
elseif(WIN32 OR THEMIS_TRIPLET MATCHES "windows")
    set(VCPKG_EXECUTABLE "${THEMIS_VCPKG_ROOT}/vcpkg.exe")
    set(BOOTSTRAP_SCRIPT "${THEMIS_VCPKG_ROOT}/bootstrap-vcpkg.bat")
else()
    set(VCPKG_EXECUTABLE "${THEMIS_VCPKG_ROOT}/vcpkg")
    set(BOOTSTRAP_SCRIPT "${THEMIS_VCPKG_ROOT}/bootstrap-vcpkg.sh")
endif()

# ============================================================================
# Bootstrap vcpkg if needed
# ============================================================================

set(_need_bootstrap OFF)
if(THEMIS_TRIPLET MATCHES "linux" AND WIN32)
    # Linux package builds on Windows run entirely in WSL and bootstrap on demand.
    set(_need_bootstrap OFF)
else()
    if(NOT EXISTS "${VCPKG_EXECUTABLE}")
        set(_need_bootstrap ON)
    endif()
endif()

if(_need_bootstrap)
    message(STATUS "Bootstrapping vcpkg for ${THEMIS_TRIPLET}...")
    
    if(THEMIS_TRIPLET MATCHES "linux" AND WIN32)
        # Building Linux packages on Windows - use WSL
        message(STATUS "Using WSL for Linux package build...")
        
        # Convert Windows path to WSL path
        themis_win_path_to_wsl("${THEMIS_VCPKG_ROOT}" _vcpkg_root_wsl)
        
        execute_process(
            COMMAND wsl bash -c "cd ${_vcpkg_root_wsl} && bash ./bootstrap-vcpkg.sh"
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
set(MANIFEST_ROOT "${THEMIS_ROOT_DIR}/docker")
set(MANIFEST_FILE "${MANIFEST_ROOT}/vcpkg-${_edition_lower}.json")

if(NOT EXISTS "${MANIFEST_FILE}")
    set(MANIFEST_ROOT "${THEMIS_ROOT_DIR}")
    set(MANIFEST_FILE "${THEMIS_ROOT_DIR}/vcpkg.json")
endif()

if(NOT EXISTS "${MANIFEST_FILE}")
    message(FATAL_ERROR "No vcpkg manifest found for edition ${THEMIS_EDITION}")
endif()

# vcpkg expects a file named vcpkg.json inside --x-manifest-root.
# Stage the selected edition manifest under that canonical name.
set(MANIFEST_STAGE_ROOT "${THEMIS_OUTPUT_DIR}/.vcpkg-manifest")
file(MAKE_DIRECTORY "${MANIFEST_STAGE_ROOT}")
file(READ "${MANIFEST_FILE}" _manifest_content)
file(WRITE "${MANIFEST_STAGE_ROOT}/vcpkg.json" "${_manifest_content}")

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
message(STATUS "Manifest root: ${MANIFEST_STAGE_ROOT}")
message(STATUS "Output: ${THEMIS_OUTPUT_DIR}")
message(STATUS "==========================================")

# ============================================================================
# Run vcpkg install
# ============================================================================

set(_vcpkg_args
    install
    --vcpkg-root=${THEMIS_VCPKG_ROOT}
    --triplet=${THEMIS_TRIPLET}
    --x-install-root=${THEMIS_OUTPUT_DIR}
    --x-manifest-root=${MANIFEST_STAGE_ROOT}
    --x-buildtrees-root=${THEMIS_VCPKG_ROOT}/buildtrees
    --x-packages-root=${THEMIS_VCPKG_ROOT}/packages
    --x-downloads-root=${THEMIS_VCPKG_ROOT}/downloads
)

if(THEMIS_TRIPLET MATCHES "linux" AND WIN32)
    # Use WSL for Linux builds on Windows
    message(STATUS "Building Linux packages via WSL...")
    
    # Convert paths to WSL format
    themis_win_path_to_wsl("${THEMIS_OUTPUT_DIR}" _output_dir_wsl)
    
    themis_win_path_to_wsl("${MANIFEST_STAGE_ROOT}" _manifest_root_wsl)
    
    themis_win_path_to_wsl("${THEMIS_VCPKG_ROOT}" _vcpkg_root_wsl)
    
    # Build WSL command
    set(_wsl_cmd
        "cd ${_vcpkg_root_wsl} && if [ ! -x ./vcpkg ]; then bash ./bootstrap-vcpkg.sh; fi && ./vcpkg install --triplet=${THEMIS_TRIPLET} --x-install-root=${_output_dir_wsl} --x-manifest-root=${_manifest_root_wsl} --x-buildtrees-root=${_vcpkg_root_wsl}/buildtrees --x-packages-root=${_vcpkg_root_wsl}/packages --x-downloads-root=${_vcpkg_root_wsl}/downloads"
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
        WORKING_DIRECTORY "${THEMIS_ROOT_DIR}"
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
