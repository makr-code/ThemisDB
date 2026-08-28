# ThemisDB Tools and Development Features
# Tests, benchmarks, build options

# Canonical build-profile default:
# Debug/test-focused configurations include unit tests by default; release
# builds keep tests disabled unless explicitly enabled. This matches the root
# CMake policy and avoids preset drift.
if(NOT DEFINED THEMIS_BUILD_TESTS)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        option(THEMIS_BUILD_TESTS "Build unit tests" ON)
    else()
        option(THEMIS_BUILD_TESTS "Build unit tests" OFF)
    endif()
endif()

# Benchmarks stay opt-in and remain disabled by default in normal dev/release
# configurations.
if(NOT DEFINED THEMIS_BUILD_BENCHMARKS)
    option(THEMIS_BUILD_BENCHMARKS "Build benchmarks" OFF)
endif()

# CHIMERA Suite
if(NOT DEFINED THEMIS_BUILD_CHIMERA)
    option(THEMIS_BUILD_CHIMERA "Build CHIMERA adapters, focused tests, and benchmark integration" OFF)
endif()

# Docker RAID benchmark (special build)
if(NOT DEFINED THEMIS_BUILD_DOCKER_RAID_BENCHMARK)
    option(THEMIS_BUILD_DOCKER_RAID_BENCHMARK "Build Docker RAID benchmark" OFF)
endif()

# Build options
if(NOT DEFINED THEMIS_STRICT_BUILD)
    option(THEMIS_STRICT_BUILD "Treat warnings as errors" OFF)
endif()

if(NOT DEFINED THEMIS_ENABLE_ASAN)
    option(THEMIS_ENABLE_ASAN "Enable AddressSanitizer" OFF)
endif()

if(NOT DEFINED THEMIS_ENABLE_LSAN)
    option(THEMIS_ENABLE_LSAN "Enable LeakSanitizer" OFF)
endif()

if(NOT DEFINED THEMIS_ENABLE_UBSAN)
    option(THEMIS_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)
endif()

if(NOT DEFINED THEMIS_CORE_SHARED)
    option(THEMIS_CORE_SHARED "Build themis_core as shared library" OFF)
endif()

if(NOT DEFINED THEMIS_STATIC_BUILD)
    option(THEMIS_STATIC_BUILD "Build fully static binary" OFF)
endif()

# mimalloc
if(NOT DEFINED THEMIS_ENABLE_MIMALLOC)
    option(THEMIS_ENABLE_MIMALLOC "Enable mimalloc" ON)
endif()

# jemalloc (alternative allocator, Linux/Mac only)
if(NOT DEFINED THEMIS_ENABLE_JEMALLOC)
    option(THEMIS_ENABLE_JEMALLOC "Enable jemalloc as alternative allocator" OFF)
endif()

# Display tools features
message(STATUS "  Development Tools:")
if(THEMIS_BUILD_TESTS)
    message(STATUS "    Unit tests: Enabled")
endif()
if(THEMIS_BUILD_BENCHMARKS)
    message(STATUS "    Benchmarks: Enabled")
endif()
if(THEMIS_BUILD_CHIMERA)
    message(STATUS "    CHIMERA suite: Enabled")
endif()
if(THEMIS_STRICT_BUILD)
    message(STATUS "    Strict build (warnings as errors): Enabled")
endif()
if(THEMIS_ENABLE_ASAN)
    message(STATUS "    AddressSanitizer: Enabled")
endif()
if(THEMIS_ENABLE_LSAN)
    message(STATUS "    LeakSanitizer: Enabled")
endif()
if(THEMIS_ENABLE_UBSAN)
    message(STATUS "    UndefinedBehaviorSanitizer: Enabled")
endif()
if(THEMIS_ENABLE_MIMALLOC)
    message(STATUS "    mimalloc: Enabled")
endif()
if(THEMIS_ENABLE_JEMALLOC)
    message(STATUS "    jemalloc: Enabled")
endif()
