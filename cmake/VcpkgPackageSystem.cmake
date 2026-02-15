# ThemisDB Multi-Platform Package System - CMake Module
# Builds vcpkg packages for all platforms using only CMake

include_guard(GLOBAL)

# ============================================================================
# Configuration
# ============================================================================

set(THEMIS_PACKAGE_STORE "${CMAKE_SOURCE_DIR}/vcpkg_packages" 
    CACHE PATH "Directory for pre-built vcpkg packages")

set(THEMIS_VCPKG_ROOT "${CMAKE_SOURCE_DIR}/vcpkg" 
    CACHE PATH "vcpkg installation directory")

# Platform detection
if(WIN32)
    set(THEMIS_PLATFORM "x64-windows")
    set(THEMIS_VCPKG_EXECUTABLE "${THEMIS_VCPKG_ROOT}/vcpkg.exe")
elseif(UNIX AND NOT APPLE)
    set(THEMIS_PLATFORM "x64-linux")
    set(THEMIS_VCPKG_EXECUTABLE "${THEMIS_VCPKG_ROOT}/vcpkg")
else()
    message(WARNING "Unsupported platform for package pre-building")
    return()
endif()

# Build configuration
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(THEMIS_BUILD_CONFIG "debug")
else()
    set(THEMIS_BUILD_CONFIG "release")
endif()

# Package directories
set(THEMIS_PACKAGE_DIR_WINDOWS_DEBUG "${THEMIS_PACKAGE_STORE}/x64-windows/debug")
set(THEMIS_PACKAGE_DIR_WINDOWS_RELEASE "${THEMIS_PACKAGE_STORE}/x64-windows/release")
set(THEMIS_PACKAGE_DIR_LINUX_DEBUG "${THEMIS_PACKAGE_STORE}/x64-linux/debug")
set(THEMIS_PACKAGE_DIR_LINUX_RELEASE "${THEMIS_PACKAGE_STORE}/x64-linux/release")

# ============================================================================
# Helper Functions
# ============================================================================

function(themis_vcpkg_bootstrap)
    # Bootstrap vcpkg if not already done
    if(WIN32)
        set(_bootstrap_script "${THEMIS_VCPKG_ROOT}/bootstrap-vcpkg.bat")
    else()
        set(_bootstrap_script "${THEMIS_VCPKG_ROOT}/bootstrap-vcpkg.sh")
    endif()
    
    if(NOT EXISTS "${THEMIS_VCPKG_EXECUTABLE}")
        message(STATUS "Bootstrapping vcpkg...")
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E chdir "${THEMIS_VCPKG_ROOT}" "${_bootstrap_script}"
            RESULT_VARIABLE _result
            OUTPUT_VARIABLE _output
            ERROR_VARIABLE _error
        )
        
        if(NOT _result EQUAL 0)
            message(FATAL_ERROR "vcpkg bootstrap failed: ${_error}")
        endif()
        
        message(STATUS "vcpkg bootstrapped successfully")
    else()
        message(STATUS "vcpkg already bootstrapped: ${THEMIS_VCPKG_EXECUTABLE}")
    endif()
endfunction()

function(themis_build_vcpkg_packages)
    # Parse arguments
    cmake_parse_arguments(
        ARGS
        ""  # Options
        "TRIPLET;EDITION;OUTPUT_DIR"  # Single-value arguments
        ""  # Multi-value arguments
        ${ARGN}
    )
    
    if(NOT ARGS_TRIPLET)
        message(FATAL_ERROR "TRIPLET argument required")
    endif()
    
    if(NOT ARGS_EDITION)
        set(ARGS_EDITION "COMMUNITY")
    endif()
    
    if(NOT ARGS_OUTPUT_DIR)
        message(FATAL_ERROR "OUTPUT_DIR argument required")
    endif()
    
    # Get manifest file for edition
    string(TOLOWER "${ARGS_EDITION}" _edition_lower)
    set(_manifest_file "${CMAKE_SOURCE_DIR}/docker/vcpkg-${_edition_lower}.json")
    
    if(NOT EXISTS "${_manifest_file}")
        message(FATAL_ERROR "Manifest not found: ${_manifest_file}")
    endif()
    
    # Ensure vcpkg is bootstrapped
    themis_vcpkg_bootstrap()
    
    # Create output directory
    file(MAKE_DIRECTORY "${ARGS_OUTPUT_DIR}")
    
    message(STATUS "Building vcpkg packages:")
    message(STATUS "  Triplet: ${ARGS_TRIPLET}")
    message(STATUS "  Edition: ${ARGS_EDITION}")
    message(STATUS "  Manifest: ${_manifest_file}")
    message(STATUS "  Output: ${ARGS_OUTPUT_DIR}")
    
    # Run vcpkg install
    execute_process(
        COMMAND "${THEMIS_VCPKG_EXECUTABLE}" install
            --triplet=${ARGS_TRIPLET}
            --x-install-root=${ARGS_OUTPUT_DIR}
            --x-manifest-root=${CMAKE_SOURCE_DIR}/docker
            --x-buildtrees-root=${THEMIS_VCPKG_ROOT}/buildtrees
            --x-packages-root=${THEMIS_VCPKG_ROOT}/packages
            --x-downloads-root=${THEMIS_VCPKG_ROOT}/downloads
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        RESULT_VARIABLE _result
        OUTPUT_VARIABLE _output
        ERROR_VARIABLE _error
    )
    
    if(NOT _result EQUAL 0)
        message(FATAL_ERROR "vcpkg install failed: ${_error}")
    endif()
    
    message(STATUS "Packages built successfully: ${ARGS_OUTPUT_DIR}")
endfunction()

# ============================================================================
# Custom Targets for Package Building
# ============================================================================

# Target: Build all packages
add_custom_target(build-all-packages
    COMMENT "Building vcpkg packages for all platforms"
)

# Windows Debug
add_custom_target(build-packages-windows-debug
    COMMAND ${CMAKE_COMMAND} 
        -DTHEMIS_VCPKG_ROOT="${THEMIS_VCPKG_ROOT}"
        -DTHEMIS_TRIPLET=x64-windows
        -DTHEMIS_EDITION=${THEMIS_EDITION}
        -DTHEMIS_OUTPUT_DIR="${THEMIS_PACKAGE_DIR_WINDOWS_DEBUG}"
        -P "${CMAKE_CURRENT_LIST_DIR}/VcpkgPackageBuild.cmake"
    COMMENT "Building Windows Debug packages"
    VERBATIM
)
add_dependencies(build-all-packages build-packages-windows-debug)

# Windows Release
add_custom_target(build-packages-windows-release
    COMMAND ${CMAKE_COMMAND} 
        -DTHEMIS_VCPKG_ROOT="${THEMIS_VCPKG_ROOT}"
        -DTHEMIS_TRIPLET=x64-windows
        -DTHEMIS_EDITION=${THEMIS_EDITION}
        -DTHEMIS_OUTPUT_DIR="${THEMIS_PACKAGE_DIR_WINDOWS_RELEASE}"
        -P "${CMAKE_CURRENT_LIST_DIR}/VcpkgPackageBuild.cmake"
    COMMENT "Building Windows Release packages"
    VERBATIM
)
add_dependencies(build-all-packages build-packages-windows-release)

# Linux Debug (requires WSL on Windows or native Linux)
add_custom_target(build-packages-linux-debug
    COMMAND ${CMAKE_COMMAND} 
        -DTHEMIS_VCPKG_ROOT="${THEMIS_VCPKG_ROOT}"
        -DTHEMIS_TRIPLET=x64-linux
        -DTHEMIS_EDITION=${THEMIS_EDITION}
        -DTHEMIS_OUTPUT_DIR="${THEMIS_PACKAGE_DIR_LINUX_DEBUG}"
        -P "${CMAKE_CURRENT_LIST_DIR}/VcpkgPackageBuild.cmake"
    COMMENT "Building Linux Debug packages"
    VERBATIM
)
add_dependencies(build-all-packages build-packages-linux-debug)

# Linux Release
add_custom_target(build-packages-linux-release
    COMMAND ${CMAKE_COMMAND} 
        -DTHEMIS_VCPKG_ROOT="${THEMIS_VCPKG_ROOT}"
        -DTHEMIS_TRIPLET=x64-linux
        -DTHEMIS_EDITION=${THEMIS_EDITION}
        -DTHEMIS_OUTPUT_DIR="${THEMIS_PACKAGE_DIR_LINUX_RELEASE}"
        -P "${CMAKE_CURRENT_LIST_DIR}/VcpkgPackageBuild.cmake"
    COMMENT "Building Linux Release packages"
    VERBATIM
)
add_dependencies(build-all-packages build-packages-linux-release)

# ============================================================================
# Status Output
# ============================================================================

message(STATUS "==========================================")
message(STATUS "Multi-Platform Package System")
message(STATUS "==========================================")
message(STATUS "Package store: ${THEMIS_PACKAGE_STORE}")
message(STATUS "Current platform: ${THEMIS_PLATFORM}")
message(STATUS "Build configuration: ${THEMIS_BUILD_CONFIG}")
message(STATUS "")
message(STATUS "Available targets:")
message(STATUS "  build-all-packages         - Build packages for all platforms")
message(STATUS "  build-packages-windows-debug   - Build Windows Debug packages")
message(STATUS "  build-packages-windows-release - Build Windows Release packages")
message(STATUS "  build-packages-linux-debug     - Build Linux Debug packages")
message(STATUS "  build-packages-linux-release   - Build Linux Release packages")
message(STATUS "==========================================")
