# ThemisDB Platform Detection
# Detects: OS, CPU architecture, special platforms (QNAP, Raspberry Pi, Docker)
# Sets: THEMIS_TARGET_ARCH, THEMIS_TARGET_ARCH_TRIPLET, THEMIS_TARGET_PLATFORM

# Detect target architecture
set(THEMIS_TARGET_ARCH "unknown")
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64|AMD64|x64)$")
    set(THEMIS_TARGET_ARCH "x86_64")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64|ARM64)$")
    set(THEMIS_TARGET_ARCH "aarch64")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm|ARM|armv7l)$")
    set(THEMIS_TARGET_ARCH "armv7")
endif()

# Detect target platform
set(THEMIS_TARGET_PLATFORM "generic")
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(THEMIS_TARGET_PLATFORM "linux")
    # Check for Docker environment
    if(EXISTS "/.dockerenv")
        set(THEMIS_TARGET_PLATFORM "docker")
    endif()
    # Check for Raspberry Pi
    if(EXISTS "/proc/device-tree/model")
        file(READ "/proc/device-tree/model" _device_model)
        if(_device_model MATCHES "Raspberry Pi")
            set(THEMIS_TARGET_PLATFORM "raspberry-pi")
        endif()
    endif()
    # QNAP detection (user-defined via option)
    if(THEMIS_QNAP_BUILD)
        set(THEMIS_TARGET_PLATFORM "qnap")
    endif()
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(THEMIS_TARGET_PLATFORM "windows")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(THEMIS_TARGET_PLATFORM "macos")
endif()

# Map to vcpkg triplets
set(THEMIS_TARGET_ARCH_TRIPLET "unknown")
if(THEMIS_TARGET_ARCH STREQUAL "x86_64")
    if(THEMIS_TARGET_PLATFORM STREQUAL "windows")
        set(THEMIS_TARGET_ARCH_TRIPLET "x64-windows")
    else()
        set(THEMIS_TARGET_ARCH_TRIPLET "x64-linux")
    endif()
elseif(THEMIS_TARGET_ARCH STREQUAL "aarch64")
    if(THEMIS_TARGET_PLATFORM STREQUAL "windows")
        set(THEMIS_TARGET_ARCH_TRIPLET "arm64-windows")
    else()
        set(THEMIS_TARGET_ARCH_TRIPLET "arm64-linux")
    endif()
elseif(THEMIS_TARGET_ARCH STREQUAL "armv7")
    set(THEMIS_TARGET_ARCH_TRIPLET "arm-linux")
endif()

# Detect cross-compilation
if(CMAKE_CROSSCOMPILING)
    message(STATUS "Cross-compiling detected")
    message(STATUS "  Host system: ${CMAKE_HOST_SYSTEM_NAME} ${CMAKE_HOST_SYSTEM_PROCESSOR}")
    message(STATUS "  Target system: ${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_PROCESSOR}")
    if(NOT CMAKE_TOOLCHAIN_FILE)
        message(WARNING "Cross-compiling without CMAKE_TOOLCHAIN_FILE")
        message(WARNING "Consider using: -DCMAKE_TOOLCHAIN_FILE=cmake/platforms/Toolchains/<triplet>.cmake")
    endif()
endif()

# Set global flag for platform detection completed
set(THEMIS_PLATFORM_DETECTED TRUE CACHE INTERNAL "Platform detection completed")

# Display detected platform information
message(STATUS "==========================================")
message(STATUS "Platform Detection:")
message(STATUS "  Target Architecture: ${THEMIS_TARGET_ARCH}")
message(STATUS "  Target Platform: ${THEMIS_TARGET_PLATFORM}")
message(STATUS "  Target Triplet: ${THEMIS_TARGET_ARCH_TRIPLET}")
message(STATUS "  System: ${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_VERSION}")
message(STATUS "  Processor: ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "==========================================")
