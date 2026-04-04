# ThemisDB AMD64 Cross-Compile Toolchain
# For cross-compiling to x86_64 Linux (mainly for testing/CI)

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Specify the cross compiler (usually system default for x86_64)
set(CMAKE_C_COMPILER gcc)
set(CMAKE_CXX_COMPILER g++)

# Set vcpkg triplet for x64 Linux
set(VCPKG_TARGET_TRIPLET "x64-linux" CACHE STRING "vcpkg target triplet")

# x86_64-specific optimization flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=x86-64" CACHE STRING "C flags")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=x86-64" CACHE STRING "CXX flags")

message(STATUS "AMD64/x86_64 cross-compile toolchain loaded")
