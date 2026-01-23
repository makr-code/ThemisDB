# ThemisDB ARMv7 Cross-Compile Toolchain
# For cross-compiling from x86_64 to ARMv7 Linux (Raspberry Pi 2/3)

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR armv7l)

# Specify the cross compiler
set(CMAKE_C_COMPILER arm-linux-gnueabihf-gcc)
set(CMAKE_CXX_COMPILER arm-linux-gnueabihf-g++)

# Where is the target environment
set(CMAKE_FIND_ROOT_PATH /usr/arm-linux-gnueabihf)

# Search for programs in the build host directories
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# Search for libraries and headers in the target directories
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Set vcpkg triplet for ARM Linux
set(VCPKG_TARGET_TRIPLET "arm-linux" CACHE STRING "vcpkg target triplet")

# ARMv7-specific optimization flags (with NEON)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=armv7-a -mfpu=neon -mfloat-abi=hard" CACHE STRING "C flags")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=armv7-a -mfpu=neon -mfloat-abi=hard" CACHE STRING "CXX flags")

message(STATUS "ARMv7 cross-compile toolchain loaded (with NEON)")
