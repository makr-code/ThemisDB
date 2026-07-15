# Cross-compile toolchain for ARM64 Linux (aarch64)
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

# Compiler
set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)

# vcpkg triplet
set(VCPKG_TARGET_TRIPLET arm64-linux CACHE STRING "vcpkg triplet")

# Search paths
set(CMAKE_FIND_ROOT_PATH /usr/aarch64-linux-gnu)
if(NOT DEFINED CMAKE_SYSROOT)
    set(CMAKE_SYSROOT /usr/aarch64-linux-gnu)
endif()
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Architecture flags
set(CMAKE_C_FLAGS_INIT "-march=armv8-a")
set(CMAKE_CXX_FLAGS_INIT "-march=armv8-a")

message(STATUS "ARM64 cross-compile toolchain loaded")
