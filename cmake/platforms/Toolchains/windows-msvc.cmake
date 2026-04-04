# ThemisDB Windows MSVC Toolchain
# For building with MSVC compiler on Windows

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Use MSVC compiler
set(CMAKE_C_COMPILER cl)
set(CMAKE_CXX_COMPILER cl)

# Set vcpkg triplet for Windows x64
if(NOT DEFINED VCPKG_TARGET_TRIPLET)
    set(VCPKG_TARGET_TRIPLET "x64-windows" CACHE STRING "vcpkg target triplet")
endif()

# Windows-specific definitions
add_compile_definitions(
    _WIN32_WINNT=0x0A00    # Windows 10+
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

message(STATUS "Windows MSVC toolchain loaded")
