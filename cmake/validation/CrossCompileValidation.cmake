# ThemisDB Cross-Compile Validation
# Validates consistency between toolchain file, triplet, target processor, and sysroot.

if(NOT CMAKE_CROSSCOMPILING)
    message(STATUS "Cross-compile validation: skipped (native build)")
    return()
endif()

message(STATUS "Validating cross-compile toolchain/triplet configuration...")

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE OR CMAKE_TOOLCHAIN_FILE STREQUAL "")
    message(FATAL_ERROR
        "Cross-compile validation failed: CMAKE_TOOLCHAIN_FILE is required when CMAKE_CROSSCOMPILING=TRUE.")
endif()

if(NOT EXISTS "${CMAKE_TOOLCHAIN_FILE}")
    message(FATAL_ERROR
        "Cross-compile validation failed: CMAKE_TOOLCHAIN_FILE does not exist: ${CMAKE_TOOLCHAIN_FILE}")
endif()

if(NOT DEFINED VCPKG_TARGET_TRIPLET OR VCPKG_TARGET_TRIPLET STREQUAL "")
    message(FATAL_ERROR
        "Cross-compile validation failed: VCPKG_TARGET_TRIPLET must be set for cross-compilation.")
endif()

set(_themis_triplet_arch "")
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64|AMD64|x64)$")
    set(_themis_triplet_arch "x64")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64|ARM64)$")
    set(_themis_triplet_arch "arm64")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm|ARM|armv7|armv7l)$")
    set(_themis_triplet_arch "arm")
endif()

if(_themis_triplet_arch STREQUAL "")
    message(FATAL_ERROR
        "Cross-compile validation failed: unsupported CMAKE_SYSTEM_PROCESSOR='${CMAKE_SYSTEM_PROCESSOR}' for triplet validation.")
endif()

set(_themis_triplet_os_pattern "")
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(_themis_triplet_os_pattern "(windows|mingw[^-]*)")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(_themis_triplet_os_pattern "linux")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(_themis_triplet_os_pattern "(osx|darwin)")
else()
    message(FATAL_ERROR
        "Cross-compile validation failed: unsupported CMAKE_SYSTEM_NAME='${CMAKE_SYSTEM_NAME}' for triplet validation.")
endif()

set(_themis_triplet_pattern "^${_themis_triplet_arch}-${_themis_triplet_os_pattern}(-.*)?$")
if(NOT VCPKG_TARGET_TRIPLET MATCHES "${_themis_triplet_pattern}")
    message(FATAL_ERROR
        "Cross-compile validation failed: VCPKG_TARGET_TRIPLET='${VCPKG_TARGET_TRIPLET}' "
        "does not match target processor/system (${CMAKE_SYSTEM_PROCESSOR}/${CMAKE_SYSTEM_NAME}). "
        "Expected pattern: ${_themis_triplet_pattern}")
endif()

set(_themis_requires_sysroot FALSE)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND _themis_triplet_arch MATCHES "^(arm64|arm)$")
    set(_themis_requires_sysroot TRUE)
endif()

if(_themis_requires_sysroot)
    if(NOT DEFINED CMAKE_SYSROOT OR CMAKE_SYSROOT STREQUAL "")
        message(FATAL_ERROR
            "Cross-compile validation failed: CMAKE_SYSROOT is required for ${CMAKE_SYSTEM_NAME}/${CMAKE_SYSTEM_PROCESSOR} targets.")
    endif()
    if(NOT EXISTS "${CMAKE_SYSROOT}")
        message(FATAL_ERROR
            "Cross-compile validation failed: CMAKE_SYSROOT path does not exist: ${CMAKE_SYSROOT}")
    endif()
endif()

message(STATUS
    "Cross-compile validation: OK (toolchain=${CMAKE_TOOLCHAIN_FILE}, "
    "triplet=${VCPKG_TARGET_TRIPLET}, target=${CMAKE_SYSTEM_NAME}/${CMAKE_SYSTEM_PROCESSOR})")
