# ThemisDB Compiler Options and C++ Standards

# C++20 Standard (required)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ════════════════════════════════════════════════════════════════════════════
# CRITICAL MSVC SETUP - Must come FIRST before any other compiler setup!
# ════════════════════════════════════════════════════════════════════════════
if(MSVC)
    # Setup MSVC toolset directory
    if(DEFINED ENV{VCToolsInstallDir})
        set(_VC_TOOLS_DIR "$ENV{VCToolsInstallDir}")
        string(REGEX REPLACE "\\\\$" "" _VC_TOOLS_DIR "${_VC_TOOLS_DIR}")
    else()
        set(_VC_TOOLS_DIR "C:/Program Files/Microsoft Visual Studio/2022/Professional/VC/Tools/MSVC/14.44.35207")
    endif()
    
    set(_WIN_SDK_VERSION "10.0.22621.0")
    set(_WIN_SDK_ROOT "C:/Program Files (x86)/Windows Kits/10")
    
    # Use include_directories() instead of /I flags - cleaner and no escaping issues
    # DO NOT use SYSTEM - MSVC headers need normal priority to find each other
    include_directories(
        "${_VC_TOOLS_DIR}/include"
        "${_WIN_SDK_ROOT}/Include/${_WIN_SDK_VERSION}/ucrt"
        "${_WIN_SDK_ROOT}/Include/${_WIN_SDK_VERSION}/shared"
        "${_WIN_SDK_ROOT}/Include/${_WIN_SDK_VERSION}/um"
    )
    
    message(STATUS "MSVC Include Paths Added (EARLY):")
    message(STATUS "  - ${_VC_TOOLS_DIR}/include")
    message(STATUS "  - ${_WIN_SDK_ROOT}/Include/${_WIN_SDK_VERSION}/{ucrt,shared,um}")
    
    # Add lib paths for linker
    link_directories(
        "${_VC_TOOLS_DIR}/lib/x64"
        "${_WIN_SDK_ROOT}/Lib/${_WIN_SDK_VERSION}/ucrt/x64"
        "${_WIN_SDK_ROOT}/Lib/${_WIN_SDK_VERSION}/um/x64"
    )
    
    message(STATUS "MSVC Library Paths Added:")
    message(STATUS "  - ${_VC_TOOLS_DIR}/lib/x64")
    message(STATUS "  - ${_WIN_SDK_ROOT}/Lib/${_WIN_SDK_VERSION}/{ucrt,um}/x64")
endif()

# ════════════════════════════════════════════════════════════════════════════
# MSVC CRT Runtime Selection
# ════════════════════════════════════════════════════════════════════════════
if(MSVC)
    # Set policy CMP0091 to use MSVC_RUNTIME_LIBRARY
    if(POLICY CMP0091)
        cmake_policy(SET CMP0091 NEW)
    endif()

    set(_themis_use_static_crt OFF)
    if(DEFINED VCPKG_TARGET_TRIPLET AND VCPKG_TARGET_TRIPLET MATCHES "static")
        set(_themis_use_static_crt ON)
    endif()

    if(_themis_use_static_crt)
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "MSVC Runtime" FORCE)
        message(STATUS "MSVC Runtime: MultiThreaded (MT) / MultiThreadedDebug (MTd) [static triplet]")
    else()
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL" CACHE STRING "MSVC Runtime" FORCE)
        message(STATUS "MSVC Runtime: MultiThreadedDLL (MD) / MultiThreadedDebugDLL (MDd) [shared triplet]")
    endif()
endif()

# Export compile commands for IDE support (VSCode, Clion, etc)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# CUDA support (if enabled)
if(THEMIS_ENABLE_CUDA)
    # Fix for CMake ↔ VS2022 ↔ CUDA 13.1 Registry Lookup Issue
    # Explicitly set CUDA Toolkit paths BEFORE enable_language(CUDA)
    if(MSVC AND NOT CMAKE_CUDA_TOOLKIT_INCLUDE_DIRECTORIES)
        set(_CUDA_TOOLKIT_ROOT "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v13.1")
        if(EXISTS "${_CUDA_TOOLKIT_ROOT}/include/cuda.h")
            set(CMAKE_CUDA_TOOLKIT_INCLUDE_DIRECTORIES 
                "${_CUDA_TOOLKIT_ROOT}/include" CACHE PATH "CUDA Toolkit Include Path" FORCE)
            message(STATUS "CUDA Toolkit Include: ${CMAKE_CUDA_TOOLKIT_INCLUDE_DIRECTORIES}")
        else()
            message(WARNING "CUDA Toolkit not found at ${_CUDA_TOOLKIT_ROOT}")
        endif()
    endif()
    
    enable_language(CUDA)
    set(CMAKE_CUDA_STANDARD 17)
    set(CMAKE_CUDA_STANDARD_REQUIRED ON)
    message(STATUS "CUDA Language Enabled")
endif()

# ════════════════════════════════════════════════════════════════════════════
# General Compiler Options
# ════════════════════════════════════════════════════════════════════════════
if(MSVC)
    # Windows MSVC compiler options (include paths already added above)
    add_compile_options(
        /W4              # Warning level 4
        /WX-             # Don't treat warnings as errors (unless THEMIS_STRICT_BUILD)
        /fp:precise      # Precise floating point
        /Gy              # Enable function-level linking
        /permissive-     # Conformance mode
        /EHsc            # Exception handling (C++ only, not SEH)
        /w14018          # Enable C4018: signed/unsigned mismatch warning
    )
    
    # Also use include_directories for good measure
    include_directories(
        "${_VC_TOOLS_DIR}/include"
        "${_WIN_SDK_ROOT}/Include/${_WIN_SDK_VERSION}/ucrt"
        "${_WIN_SDK_ROOT}/Include/${_WIN_SDK_VERSION}/shared"
        "${_WIN_SDK_ROOT}/Include/${_WIN_SDK_VERSION}/um"
    )
    
    # Add Windows SDK and MSVC runtime library paths
    set(_WIN_SDK_LIB_PATH "${_WIN_SDK_ROOT}/Lib/${_WIN_SDK_VERSION}/um/x64")
    set(_WIN_SDK_UCRT_LIB_PATH "${_WIN_SDK_ROOT}/Lib/${_WIN_SDK_VERSION}/ucrt/x64")
    
    if(EXISTS "${_WIN_SDK_LIB_PATH}")
        link_directories("${_WIN_SDK_LIB_PATH}")
        message(STATUS "Added Windows SDK lib path: ${_WIN_SDK_LIB_PATH}")
    endif()
    
    if(EXISTS "${_WIN_SDK_UCRT_LIB_PATH}")
        link_directories("${_WIN_SDK_UCRT_LIB_PATH}")
        message(STATUS "Added Windows SDK UCRT lib path: ${_WIN_SDK_UCRT_LIB_PATH}")
    endif()
    
    # Add MSVC runtime lib path
    if(_VC_TOOLS_DIR AND EXISTS "${_VC_TOOLS_DIR}/lib/x64")
        link_directories("${_VC_TOOLS_DIR}/lib/x64")
        message(STATUS "Added MSVC lib path: ${_VC_TOOLS_DIR}/lib/x64")
    endif()
    
    # Release-specific options for SIMD optimization
    if(CMAKE_BUILD_TYPE STREQUAL "Release" AND THEMIS_ENABLE_AVX2)
        add_compile_options(/arch:AVX2)
        add_compile_definitions(THEMIS_HAS_AVX2=1)
    endif()
    
    # Treat warnings as errors if requested
    if(THEMIS_STRICT_BUILD)
        add_compile_options(/WX)
    endif()
    
else()
    # GCC/Clang compiler options
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Wno-unused-parameter
        -Wno-deprecated-declarations
        -Wsign-compare   # Enable signed/unsigned comparison warnings
    )

    # ── Release optimisation flags ────────────────────────────────────────────
    # Active by default.  Opt-out:
    #   -DTHEMIS_DISABLE_O3=ON          → use compiler default (-O2)
    #   -DTHEMIS_DISABLE_FAST_MATH=ON   → strict IEEE 754 (no -ffast-math)
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        # -fno-omit-frame-pointer is always added in Release so profilers
        # (perf, VTune) can produce accurate call graphs.
        add_compile_options(-fno-omit-frame-pointer)

        if(NOT THEMIS_DISABLE_O3)
            add_compile_options(-O3)
            message(STATUS "  Release: -O3 enabled (set THEMIS_DISABLE_O3=ON to use -O2)")
        endif()

        if(NOT THEMIS_DISABLE_FAST_MATH)
            add_compile_options(-ffast-math)
            message(STATUS "  Release: -ffast-math enabled (set THEMIS_DISABLE_FAST_MATH=ON for strict IEEE 754)")
        endif()

        add_compile_options(-funroll-loops)
    endif()

    # Release-specific options for SIMD optimization
    if(CMAKE_BUILD_TYPE STREQUAL "Release" AND THEMIS_ENABLE_AVX2)
        add_compile_options(-mavx2 -mfma)
        add_compile_definitions(THEMIS_HAS_AVX2=1)
    elseif(THEMIS_QNAP_BUILD)
        # QNAP: baseline x86-64 without AVX
        add_compile_options(-march=x86-64)
        add_compile_definitions(THEMIS_BASELINE_X64=1)
    endif()
    
    # Treat warnings as errors if requested
    if(THEMIS_STRICT_BUILD)
        add_compile_options(-Werror)
    endif()
    
    # AddressSanitizer support for debugging
    if(THEMIS_ENABLE_ASAN)
        add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address)
        message(STATUS "AddressSanitizer enabled for debugging")
    endif()
    
    # LeakSanitizer support (can be combined with AddressSanitizer)
    if(THEMIS_ENABLE_LSAN)
        add_compile_options(-fsanitize=leak -fno-omit-frame-pointer)
        add_link_options(-fsanitize=leak)
        message(STATUS "LeakSanitizer enabled for debugging")
    endif()
    
    # UndefinedBehaviorSanitizer support for detecting alignment issues
    # Use separate flag to avoid conflicts with other sanitizers
    if(THEMIS_ENABLE_UBSAN)
        add_compile_options(-fsanitize=undefined -fno-omit-frame-pointer)
        add_link_options(-fsanitize=undefined)
        message(STATUS "UndefinedBehaviorSanitizer enabled for alignment checking")
    endif()
endif()

# ============================================================================
# ARM/AARCH64 STRICT ALIGNMENT REQUIREMENTS
# ============================================================================
# ARM platforms strictly require aligned memory access for performance and correctness
# Unaligned access can cause SIGBUS crashes or severe performance degradation

if(THEMIS_TARGET_ARCH MATCHES "^(aarch64|armv7)$")
    if(NOT MSVC)
        # Enable strict alignment warnings/errors for ARM platforms
        add_compile_options(-Werror=cast-align)
        add_compile_definitions(THEMIS_STRICT_ALIGNMENT=1)
        message(STATUS "ARM Strict Alignment: Enabled (-Werror=cast-align)")
    endif()
elseif(CMAKE_SYSTEM_NAME STREQUAL "Android")
    if(NOT MSVC)
        # Android also uses ARM processors which require strict alignment
        add_compile_options(-Werror=cast-align)
        add_compile_definitions(THEMIS_STRICT_ALIGNMENT=1)
        message(STATUS "Android/ARM Strict Alignment: Enabled (-Werror=cast-align)")
    endif()
endif()

# ============================================================================
# LINK-TIME CODE GENERATION (LTCG) / INTERPROCEDURAL OPTIMIZATION (IPO)
# ============================================================================
# Enable Link-Time Code Generation for Release builds to improve performance
# This allows the compiler to optimize across translation units

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    # On Windows, IPO probing links a test binary and requires initialized MSVC
    # LIB paths. Skip probing when LIB is unavailable (e.g. plain shells).
    if(WIN32 AND "$ENV{LIB}" STREQUAL "")
        message(WARNING "IPO/LTO skipped on Windows: LIB environment is not initialized")
    else()
        # Check if IPO/LTO is supported
        include(CheckIPOSupported)
        check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)

        if(ipo_supported)
            # Enable IPO/LTO for all targets
            set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

            # Let CMake handle compiler/linker-specific IPO flags per target.
            # This avoids global /GL injection that conflicts with targets which
            # must disable IPO (e.g. WINDOWS_EXPORT_ALL_SYMBOLS def generation).
            message(STATUS "IPO/LTO enabled via CMAKE_INTERPROCEDURAL_OPTIMIZATION")
        else()
            message(WARNING "IPO/LTO not supported: ${ipo_error}")
        endif()
    endif()
else()
    message(STATUS "IPO/LTO skipped (only enabled in Release mode)")
endif()

# Platform-specific handling
if(WIN32)
    add_compile_definitions(
        _WIN32_WINNT=0x0A00    # Windows 10+
        WIN32_LEAN_AND_MEAN
        NOMINMAX               # Prevent min/max macro conflicts
    )
endif()

message(STATUS "C++ Standard: C++${CMAKE_CXX_STANDARD}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
