# Optional compiler cache integration (sccache/ccache) to speed up incremental builds.

option(THEMIS_ENABLE_COMPILER_CACHE
       "Enable compiler cache launcher auto-detection (sccache/ccache)"
       ON)

set(THEMIS_COMPILER_CACHE_PROGRAM ""
    CACHE STRING
    "Optional explicit cache launcher executable (e.g. sccache, ccache, full path)")

if(NOT THEMIS_ENABLE_COMPILER_CACHE)
    # Clear cached launchers so reconfigure with OFF actually disables sccache/ccache.
    unset(CMAKE_C_COMPILER_LAUNCHER CACHE)
    unset(CMAKE_CXX_COMPILER_LAUNCHER CACHE)
    message(STATUS "Compiler cache: disabled (THEMIS_ENABLE_COMPILER_CACHE=OFF)")
    return()
endif()

set(_themis_cache_candidates "")
if(THEMIS_COMPILER_CACHE_PROGRAM)
    list(APPEND _themis_cache_candidates "${THEMIS_COMPILER_CACHE_PROGRAM}")
endif()

# Prefer sccache (works well with MSVC + Ninja). On non-Windows, also try ccache.
if(WIN32)
    list(APPEND _themis_cache_candidates sccache)
else()
    list(APPEND _themis_cache_candidates sccache ccache)
endif()

find_program(THEMIS_COMPILER_CACHE_EXECUTABLE NAMES ${_themis_cache_candidates})

if(THEMIS_COMPILER_CACHE_EXECUTABLE)
    set(CMAKE_C_COMPILER_LAUNCHER "${THEMIS_COMPILER_CACHE_EXECUTABLE}" CACHE STRING "C compiler launcher" FORCE)
    set(CMAKE_CXX_COMPILER_LAUNCHER "${THEMIS_COMPILER_CACHE_EXECUTABLE}" CACHE STRING "CXX compiler launcher" FORCE)
    message(STATUS "Compiler cache: enabled (${THEMIS_COMPILER_CACHE_EXECUTABLE})")
else()
    unset(CMAKE_C_COMPILER_LAUNCHER CACHE)
    unset(CMAKE_CXX_COMPILER_LAUNCHER CACHE)
    message(STATUS "Compiler cache: no launcher found (tried: ${_themis_cache_candidates})")
endif()
