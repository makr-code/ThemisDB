# ThemisDB Windows MinGW-w64 Cross-Compile Toolchain
# For cross-compiling from Linux to Windows x64 using MinGW-w64

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Specify the cross compiler
set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)

# Where is the target environment
set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)

# Search for programs in the build host directories
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# Search for libraries and headers in the target directories
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Set vcpkg triplet for Windows x64 MinGW
set(VCPKG_TARGET_TRIPLET "x64-mingw-static" CACHE STRING "vcpkg target triplet")

# Windows-specific definitions
add_compile_definitions(
    _WIN32_WINNT=0x0A00    # Windows 10+
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

# MinGW-specific flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -static-libgcc" CACHE STRING "C flags")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++" CACHE STRING "CXX flags")

message(STATUS "MinGW-w64 cross-compile toolchain loaded (Linux → Windows x64)")
