# ThemisDB Compiler Options and C++ Standards

# C++20 Standard (required)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# CRITICAL: Select MSVC CRT runtime consistently with vcpkg triplet
# - x64-windows-static     -> /MT  (MultiThreaded)
# - other triplets (shared)-> /MD  (MultiThreadedDLL)
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

# Compiler-specific options
if(MSVC)
    # Windows MSVC compiler options
    add_compile_options(
        /W4              # Warning level 4
        /WX-             # Don't treat warnings as errors (unless THEMIS_STRICT_BUILD)
        /fp:precise      # Precise floating point
        /Gy              # Enable function-level linking
        /permissive-     # Conformance mode
        /EHa             # Exception handling (C++ + SEH) - Required for _set_se_translator()
    )
    
    # Fix for missing standard library headers (when VSDevCmd not initialized)
    # Include standard MSVC toolset paths explicitly
    if(DEFINED ENV{VCToolsInstallDir})
        set(_VC_TOOLS_DIR "$ENV{VCToolsInstallDir}")
    else()
        # Fallback: discover MSVC installation
        get_filename_component(_VC_TOOLS_DIR 
            "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\VisualStudio\\SxS\\VC7;17.0]" ABSOLUTE)
    endif()
    
    if(_VC_TOOLS_DIR)
        message(STATUS "MSVC VC Tools Directory: ${_VC_TOOLS_DIR}")
        include_directories("${_VC_TOOLS_DIR}include")
    else()
        message(WARNING "Could not locate MSVC VC Tools directory for standard library includes")
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
    )
    
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
    # Check if IPO/LTO is supported
    include(CheckIPOSupported)
    check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)
    
    if(ipo_supported)
        # Enable IPO/LTO for all targets
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
        
        if(MSVC)
            # MSVC: Enable Link-Time Code Generation (LTCG)
            add_compile_options(/GL)
            add_link_options(/LTCG:INCREMENTAL)  # Incremental LTCG for faster iterative builds
            message(STATUS "LTCG enabled: /GL (compile) + /LTCG:INCREMENTAL (link)")
        else()
            # GCC/Clang: Enable Link-Time Optimization (LTO)
            add_compile_options(-flto)
            add_link_options(-flto)
            message(STATUS "LTO enabled: -flto")
        endif()
    else()
        message(WARNING "IPO/LTO not supported: ${ipo_error}")
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
