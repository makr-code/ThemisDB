# Cross-compile toolchain for ARMv7 Linux (32-bit)
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR armv7l)

# Compiler
set(CMAKE_C_COMPILER arm-linux-gnueabihf-gcc)
set(CMAKE_CXX_COMPILER arm-linux-gnueabihf-g++)

# vcpkg triplet
set(VCPKG_TARGET_TRIPLET arm-linux CACHE STRING "vcpkg triplet")

# Search paths
set(CMAKE_FIND_ROOT_PATH /usr/arm-linux-gnueabihf)
if(NOT DEFINED CMAKE_SYSROOT)
    set(CMAKE_SYSROOT /usr/arm-linux-gnueabihf)
endif()
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Architecture flags (hard float ABI required for gnueabihf)
set(CMAKE_C_FLAGS_INIT "-march=armv7-a -mfpu=neon -mfloat-abi=hard")
set(CMAKE_CXX_FLAGS_INIT "-march=armv7-a -mfpu=neon -mfloat-abi=hard")

message(STATUS "ARMv7 cross-compile toolchain loaded (with NEON)")
