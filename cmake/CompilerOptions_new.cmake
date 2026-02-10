# ThemisDB Compiler Options and C++ Standards

# C++20 Standard (required)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ════════════════════════════════════════════════════════════════════════════
# CRITICAL: MSVC Include Paths - MUST be set FIRST!
# This fixes "fatal error C1083: cannot open include file: 'stddef.h', 'windows.h'"
# ════════════════════════════════════════════════════════════════════════════
if(MSVC)
    # Detect MSVC toolset
    if(DEFINED ENV{VCToolsInstallDir})
        set(_VC_TOOLS_DIR "$ENV{VCToolsInstallDir}")
        string(REGEX REPLACE "\\\\$" "" _VC_TOOLS_DIR "${_VC_TOOLS_DIR}")
    else()
        set(_VC_TOOLS_DIR "C:/Program Files/Microsoft Visual Studio/2022/Professional/VC/Tools/MSVC/14.44.35207")
    endif()
    
    set(_WIN_SDK_VERSION "10.0.22621.0")
    set(_WIN_SDK_ROOT "C:/Program Files (x86)/Windows Kits/10")
    
    # Add include paths VIA add_compile_options FIRST
    # This ensures they're parsed BEFORE any /I flags from include_directories
    add_compile_options(
        /I"${_VC_TOOLS_DIR}/include"
        /I"${_WIN_SDK_ROOT}/Include/${_WIN_SDK_VERSION}/ucrt"
        /I"${_WIN_SDK_ROOT}/Include/${_WIN_SDK_VERSION}/shared"
        /I"${_WIN_SDK_ROOT}/Include/${_WIN_SDK_VERSION}/um"
    )
    
    message(STATUS "✓ MSVC Include Paths (via /I flags):")
    message(STATUS "    VC: ${_VC_TOOLS_DIR}/include")
    message(STATUS "    SDK: ${_WIN_SDK_ROOT}/Include/${_WIN_SDK_VERSION}")
    
    # Also add via include_directories for CMake's built-in search
    include_directories(
        "${_VC_TOOLS_DIR}/include"
        "${_WIN_SDK_ROOT}/Include/${_WIN_SDK_VERSION}/ucrt"
        "${_WIN_SDK_ROOT}/Include/${_WIN_SDK_VERSION}/shared"
        "${_WIN_SDK_ROOT}/Include/${_WIN_SDK_VERSION}/um"
    )
    
    # Setup linker paths
    link_directories(
        "${_VC_TOOLS_DIR}/lib/x64"
        "${_WIN_SDK_ROOT}/Lib/${_WIN_SDK_VERSION}/ucrt/x64"
        "${_WIN_SDK_ROOT}/Lib/${_WIN_SDK_VERSION}/um/x64"
    )
    
    message(STATUS "✓ MSVC Library Paths:")
    message(STATUS "    VC: ${_VC_TOOLS_DIR}/lib/x64")
    message(STATUS "    SDK: ${_WIN_SDK_ROOT}/Lib/${_WIN_SDK_VERSION}")
endif()

# ════════════════════════════════════════════════════════════════════════════
# MSVC CRT Runtime Selection
# ════════════════════════════════════════════════════════════════════════════
if(MSVC)
    if(POLICY CMP0091)
        cmake_policy(SET CMP0091 NEW)
    endif()

    set(_themis_use_static_crt OFF)
    if(DEFINED VCPKG_TARGET_TRIPLET AND VCPKG_TARGET_TRIPLET MATCHES "static")
        set(_themis_use_static_crt ON)
    endif()

    if(_themis_use_static_crt)
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "MSVC Runtime" FORCE)
        message(STATUS "MSVC Runtime: MT/MTd (static)")
    else()
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL" CACHE STRING "MSVC Runtime" FORCE)
        message(STATUS "MSVC Runtime: MD/MDd (dynamic)")
    endif()
endif()

# Export compile commands for IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# CUDA support (if enabled)
if(THEMIS_ENABLE_CUDA)
    if(MSVC AND NOT CMAKE_CUDA_TOOLKIT_INCLUDE_DIRECTORIES)
        set(_CUDA_TOOLKIT_ROOT "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v13.1")
        if(EXISTS "${_CUDA_TOOLKIT_ROOT}/include/cuda.h")
            set(CMAKE_CUDA_TOOLKIT_INCLUDE_DIRECTORIES 
                "${_CUDA_TOOLKIT_ROOT}/include" CACHE PATH "CUDA Toolkit Include Path" FORCE)
            message(STATUS "CUDA Toolkit: ${CMAKE_CUDA_TOOLKIT_INCLUDE_DIRECTORIES}")
        else()
            message(WARNING "CUDA Toolkit not found at ${_CUDA_TOOLKIT_ROOT}")
        endif()
    endif()
    
    enable_language(CUDA)
    set(CMAKE_CUDA_STANDARD 17)
    set(CMAKE_CUDA_STANDARD_REQUIRED ON)
    message(STATUS "CUDA enabled")
endif()

# ════════════════════════════════════════════════════════════════════════════
# Compiler-Specific Options
# ════════════════════════════════════════════════════════════════════════════
if(MSVC)
    # MSVC compiler warnings and optimizations
    add_compile_options(
        /W4              # Warning level 4
        /WX-             # Don't treat warnings as errors by default
        /fp:precise      # Precise floating point
        /Gy              # Function-level linking
        /permissive-     # Conformance mode
        /EHsc            # C++ exception handling only (not SEH)
    )
    
    # AVX2 optimization for Release builds
    if(CMAKE_BUILD_TYPE STREQUAL "Release" AND THEMIS_ENABLE_AVX2)
        add_compile_options(/arch:AVX2)
        add_compile_definitions(THEMIS_HAS_AVX2=1)
    endif()
    
    # Strict warnings if requested
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
    
    # AVX2 optimization for Release builds
    if(CMAKE_BUILD_TYPE STREQUAL "Release" AND THEMIS_ENABLE_AVX2)
        add_compile_options(-mavx2 -mfma)
        add_compile_definitions(THEMIS_HAS_AVX2=1)
    elseif(THEMIS_QNAP_BUILD)
        add_compile_options(-march=x86-64)
        add_compile_definitions(THEMIS_BASELINE_X64=1)
    endif()
    
    # Strict warnings if requested
    if(THEMIS_STRICT_BUILD)
        add_compile_options(-Werror)
    endif()
    
    # AddressSanitizer for debugging
    if(THEMIS_ENABLE_ASAN)
        add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address)
        message(STATUS "AddressSanitizer enabled")
    endif()
endif()

message(STATUS "Compiler options configured")
