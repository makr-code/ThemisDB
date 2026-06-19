# MSVC Compiler Pre-validation for ExternalProject Sub-CMake Processes
# This script is loaded with `cmake -C` before project() in all CMake invocations
# It pre-validates MSVC compiler to skip expensive ABI detection tests
# that create deep directory structures and exceed Windows MAX_PATH

if(WIN32 AND MSVC)
    # Pre-mark compiler as working - skips expensive ABI tests
    set(CMAKE_C_COMPILER_WORKS ON CACHE BOOL "C compiler works" FORCE)
    set(CMAKE_CXX_COMPILER_WORKS ON CACHE BOOL "CXX compiler works" FORCE)
    
    # Suppress ABI info detection (another source of deep directories)
    set(CMAKE_C_ABI_COMPILED ON CACHE BOOL "C ABI compiled" FORCE)
    set(CMAKE_CXX_ABI_COMPILED ON CACHE BOOL "CXX ABI compiled" FORCE)
    
    message(STATUS "[MSVC Init] Compiler pre-validated to skip ABI tests")
endif()
