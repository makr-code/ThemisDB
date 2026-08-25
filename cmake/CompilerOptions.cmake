# ThemisDB Compiler Options and C++ Standards

# C++20 Standard (required)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ════════════════════════════════════════════════════════════════════════════
# SECURITY HARDENING OPTION (SEC-CC-4)
# ════════════════════════════════════════════════════════════════════════════
# Compiler/linker security hardening flags are ON by default for all Release
# builds (both MSVC and GCC/Clang).  They protect against memory-safety
# attacks, code injection, and exploitation primitives.
#
# Disable ONLY if a specific target environment does not support the flags.
# Disabling this in production is a security policy violation (SEC-CC-4).
#
# See docs/de/security/COMPILER_SECURITY_HARDENING.md for the full flag list.
# ════════════════════════════════════════════════════════════════════════════
option(THEMIS_DISABLE_SECURITY_HARDENING
    "Disable compiler security hardening flags. NOT recommended for production builds. SEC-CC-4."
    OFF)

if(THEMIS_DISABLE_SECURITY_HARDENING)
    message(WARNING
        "\n========================================================================\n"
        "SECURITY WARNING: THEMIS_DISABLE_SECURITY_HARDENING=ON\n"
        "Compiler security hardening is DISABLED for this build.\n"
        "This configuration is NOT suitable for production deployments.\n"
        "Disabled protections: stack-protector, FORTIFY_SOURCE, PIE/ASLR,\n"
        "                      RELRO, CFG/Control-Flow-Guard.\n"
        "SEC-CC-4 requirement violation — do NOT deploy to production.\n"
        "========================================================================\n")
endif()

# ════════════════════════════════════════════════════════════════════════════
# CRITICAL MSVC SETUP - Must come FIRST before any other compiler setup!
# ════════════════════════════════════════════════════════════════════════════
if(MSVC)
    set(_themis_program_files_x86 "$ENV{ProgramFiles\(x86\)}")
    if(NOT _themis_program_files_x86)
        set(_themis_program_files_x86 "$ENV{ProgramFiles}")
    endif()

    set(_themis_vs_search_roots)
    foreach(_themis_vs_root IN ITEMS "$ENV{ProgramFiles\(x86\)}" "$ENV{ProgramFiles}" "$ENV{ProgramW6432}")
        if(_themis_vs_root)
            list(APPEND _themis_vs_search_roots "${_themis_vs_root}")
        endif()
    endforeach()
    list(REMOVE_DUPLICATES _themis_vs_search_roots)

    # Setup MSVC toolset directory
    if(DEFINED ENV{VCToolsInstallDir})
        set(_VC_TOOLS_DIR "$ENV{VCToolsInstallDir}")
        string(REGEX REPLACE "\\\\$" "" _VC_TOOLS_DIR "${_VC_TOOLS_DIR}")
    else()
        set(_VC_TOOLS_DIR "")
    endif()

    if(NOT _VC_TOOLS_DIR)
        set(_themis_msvc_tool_roots)
        foreach(_themis_vs_root IN LISTS _themis_vs_search_roots)
            file(GLOB _themis_msvc_tool_roots_glob
                "${_themis_vs_root}/Microsoft Visual Studio/*/*/VC/Tools/MSVC/*")
            list(APPEND _themis_msvc_tool_roots ${_themis_msvc_tool_roots_glob})
        endforeach()
        list(REMOVE_DUPLICATES _themis_msvc_tool_roots)
        list(SORT _themis_msvc_tool_roots ORDER DESCENDING)
        list(LENGTH _themis_msvc_tool_roots _themis_msvc_tool_roots_count)
        if(_themis_msvc_tool_roots_count GREATER 0)
            list(GET _themis_msvc_tool_roots 0 _VC_TOOLS_DIR)
        endif()
    endif()
    
    set(_WIN_SDK_VERSION "$ENV{WindowsSDKVersion}")
    if(NOT _WIN_SDK_VERSION)
        set(_WIN_SDK_VERSION "$ENV{WindowsSDKLibVersion}")
    endif()
    set(_WIN_SDK_ROOT "$ENV{WindowsSdkDir}")
    if(_WIN_SDK_ROOT)
        string(REGEX REPLACE "[\\/]$" "" _WIN_SDK_ROOT "${_WIN_SDK_ROOT}")
    endif()

    if(NOT _WIN_SDK_ROOT AND _themis_program_files_x86)
        set(_WIN_SDK_ROOT "${_themis_program_files_x86}/Windows Kits/10")
    endif()

    if(_WIN_SDK_VERSION)
        string(REGEX REPLACE "[\\/]$" "" _WIN_SDK_VERSION "${_WIN_SDK_VERSION}")
    endif()

    if(_WIN_SDK_ROOT AND NOT _WIN_SDK_VERSION)
        file(GLOB _themis_windows_sdk_versions "${_WIN_SDK_ROOT}/Include/*")
        set(_themis_windows_sdk_version_names)
        foreach(_themis_sdk_dir IN LISTS _themis_windows_sdk_versions)
            get_filename_component(_themis_sdk_ver "${_themis_sdk_dir}" NAME)
            if(EXISTS "${_themis_sdk_dir}/ucrt" AND EXISTS "${_themis_sdk_dir}/shared" AND EXISTS "${_themis_sdk_dir}/um")
                list(APPEND _themis_windows_sdk_version_names "${_themis_sdk_ver}")
            endif()
        endforeach()
        list(SORT _themis_windows_sdk_version_names ORDER DESCENDING)
        list(LENGTH _themis_windows_sdk_version_names _themis_windows_sdk_version_count)
        if(_themis_windows_sdk_version_count GREATER 0)
            list(GET _themis_windows_sdk_version_names 0 _WIN_SDK_VERSION)
        endif()
    endif()
    
    if(_VC_TOOLS_DIR AND _WIN_SDK_ROOT AND _WIN_SDK_VERSION)
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
        set(_CUDA_TOOLKIT_ROOT "")

        if(DEFINED CUDAToolkit_ROOT AND EXISTS "${CUDAToolkit_ROOT}/include/cuda.h")
            set(_CUDA_TOOLKIT_ROOT "${CUDAToolkit_ROOT}")
        elseif(DEFINED ENV{CUDA_PATH} AND EXISTS "$ENV{CUDA_PATH}/include/cuda.h")
            set(_CUDA_TOOLKIT_ROOT "$ENV{CUDA_PATH}")
        elseif(DEFINED ENV{CUDA_HOME} AND EXISTS "$ENV{CUDA_HOME}/include/cuda.h")
            set(_CUDA_TOOLKIT_ROOT "$ENV{CUDA_HOME}")
        else()
            set(_themis_cuda_toolkits)
            foreach(_themis_program_files_root IN ITEMS "$ENV{ProgramFiles}" "$ENV{ProgramW6432}")
                if(_themis_program_files_root)
                    file(GLOB _themis_cuda_toolkits_glob "${_themis_program_files_root}/NVIDIA GPU Computing Toolkit/CUDA/v*")
                    list(APPEND _themis_cuda_toolkits ${_themis_cuda_toolkits_glob})
                endif()
            endforeach()
            list(REMOVE_DUPLICATES _themis_cuda_toolkits)
            list(SORT _themis_cuda_toolkits ORDER DESCENDING)
            foreach(_cuda_root IN LISTS _themis_cuda_toolkits)
                if(EXISTS "${_cuda_root}/include/cuda.h")
                    set(_CUDA_TOOLKIT_ROOT "${_cuda_root}")
                    break()
                endif()
            endforeach()
        endif()

        if(_CUDA_TOOLKIT_ROOT AND EXISTS "${_CUDA_TOOLKIT_ROOT}/include/cuda.h" AND EXISTS "${_CUDA_TOOLKIT_ROOT}/bin/nvcc.exe")
            set(CMAKE_CUDA_TOOLKIT_INCLUDE_DIRECTORIES 
                "${_CUDA_TOOLKIT_ROOT}/include" CACHE PATH "CUDA Toolkit Include Path" FORCE)
            message(STATUS "CUDA Toolkit Include: ${CMAKE_CUDA_TOOLKIT_INCLUDE_DIRECTORIES}")
        else()
            message(WARNING "CUDA Toolkit include path could not be resolved automatically")
        endif()
    endif()

    include(CheckLanguage)
    if(CMAKE_CUDA_COMPILER OR (_CUDA_TOOLKIT_ROOT AND EXISTS "${_CUDA_TOOLKIT_ROOT}/bin/nvcc.exe"))
        check_language(CUDA)
        if(CMAKE_CUDA_COMPILER)
            enable_language(CUDA)
            set(CMAKE_CUDA_STANDARD 17)
            set(CMAKE_CUDA_STANDARD_REQUIRED ON)
            message(STATUS "CUDA Language Enabled")
        else()
            message(WARNING "THEMIS_ENABLE_CUDA=ON requested, but no CUDA compiler was detected. Disabling CUDA backend.")
            set(THEMIS_ENABLE_CUDA OFF CACHE BOOL "CUDA disabled: compiler not found" FORCE)
        endif()
    else()
        message(WARNING "THEMIS_ENABLE_CUDA=ON requested, but no CUDA toolkit root was detected. Disabling CUDA backend.")
        set(THEMIS_ENABLE_CUDA OFF CACHE BOOL "CUDA disabled: toolkit not found" FORCE)
    endif()
endif()

# ════════════════════════════════════════════════════════════════════════════
# General Compiler Options
# ════════════════════════════════════════════════════════════════════════════
if(MSVC)
    # Windows MSVC compiler options (include paths already added above)
    set(_themis_msvc_warning_flags
        /W4              # Warning level 4
        /fp:precise      # Precise floating point
        /Gy              # Enable function-level linking
        /permissive-     # Conformance mode
        /EHsc            # Exception handling (C++ only, not SEH)
        /w14018          # Enable C4018: signed/unsigned mismatch warning
    )
    if(THEMIS_STRICT_BUILD)
        list(APPEND _themis_msvc_warning_flags /WX)
    else()
        list(APPEND _themis_msvc_warning_flags /WX-)
    endif()
    add_compile_options(${_themis_msvc_warning_flags})
    
    # Also use include_directories for good measure
    if(_VC_TOOLS_DIR AND _WIN_SDK_ROOT AND _WIN_SDK_VERSION)
        include_directories(
            "${_VC_TOOLS_DIR}/include"
            "${_WIN_SDK_ROOT}/Include/${_WIN_SDK_VERSION}/ucrt"
            "${_WIN_SDK_ROOT}/Include/${_WIN_SDK_VERSION}/shared"
            "${_WIN_SDK_ROOT}/Include/${_WIN_SDK_VERSION}/um"
        )
    else()
        message(WARNING "MSVC toolset/SDK headers could not be fully resolved automatically; standard library headers may be unavailable during compilation.")
    endif()
    
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

    # Seed the MSVC linker environment for try_compile and nested CMake checks.
    # The implicit Windows system libraries are resolved through LIB/LIBPATH,
    # so CMake configure steps must carry these paths even when VsDevCmd was
    # not able to fully populate the shell environment.
    set(_themis_msvc_lib_paths)
    if(_VC_TOOLS_DIR AND EXISTS "${_VC_TOOLS_DIR}/lib/x64")
        list(APPEND _themis_msvc_lib_paths "${_VC_TOOLS_DIR}/lib/x64")
    endif()
    if(EXISTS "${_WIN_SDK_UCRT_LIB_PATH}")
        list(APPEND _themis_msvc_lib_paths "${_WIN_SDK_UCRT_LIB_PATH}")
    endif()
    if(EXISTS "${_WIN_SDK_LIB_PATH}")
        list(APPEND _themis_msvc_lib_paths "${_WIN_SDK_LIB_PATH}")
    endif()
    if(_themis_msvc_lib_paths)
        list(REMOVE_DUPLICATES _themis_msvc_lib_paths)
        string(JOIN ";" _themis_msvc_lib_paths_joined ${_themis_msvc_lib_paths})
        set(ENV{LIB} "${_themis_msvc_lib_paths_joined}")
        set(ENV{LIBPATH} "${_themis_msvc_lib_paths_joined}")
        message(STATUS "MSVC linker environment seeded: ${_themis_msvc_lib_paths_joined}")
    endif()

    set(CMAKE_TRY_COMPILE_PLATFORM_VARIABLES
        CMAKE_LIBRARY_PATH
        CMAKE_INCLUDE_PATH
        CMAKE_PREFIX_PATH
        VCToolsInstallDir
        WindowsSdkDir
        WindowsSDKVersion
        WindowsSDKLibVersion
    )
    
    # Release-specific options for SIMD optimization
    if(CMAKE_BUILD_TYPE STREQUAL "Release" AND THEMIS_ENABLE_AVX2)
        add_compile_options(/arch:AVX2)
        add_compile_definitions(THEMIS_HAS_AVX2=1)
    endif()
    
    # ── MSVC Security Hardening (Release builds) ──────────────────────────────
    # Applied via generator expressions so multi-config generators work correctly.
    #   /GS       – Buffer Security Check (stack canaries)
    #   /sdl      – Additional SDL checks (promotes several warnings to errors)
    #   /guard:cf – Control Flow Guard (CFG) compile-time instrumentation
    # Linker:
    #   /GUARD:CF  – CFG enforcement at link time
    #   /NXCOMPAT  – Data Execution Prevention (DEP / NX bit)
    #   /DYNAMICBASE – Address Space Layout Randomization (ASLR)
    if(NOT THEMIS_DISABLE_SECURITY_HARDENING)
        add_compile_options(
            $<$<CONFIG:Release>:/GS>
            $<$<CONFIG:Release>:/sdl>
            $<$<CONFIG:Release>:/guard:cf>
        )
        add_link_options(
            $<$<CONFIG:Release>:/GUARD:CF>
            $<$<CONFIG:Release>:/NXCOMPAT>
            $<$<CONFIG:Release>:/DYNAMICBASE>
        )
        message(STATUS "Security Hardening (MSVC): /GS /sdl /guard:cf | /GUARD:CF /NXCOMPAT /DYNAMICBASE  [Release]")
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
    # Note: -Werror is applied at target level via target_compile_options()
    # from themis_apply_strict_build_flags()
    # not globally via add_compile_options(), to avoid external dependency build failures
    if(THEMIS_STRICT_BUILD)
        # Define strict compilation flag that targets can opt into
        set(THEMIS_STRICT_COMPILE_OPTIONS -Werror)
        message(STATUS "Strict compilation mode: -Werror will be applied to ThemisDB targets only")
    else()
        set(THEMIS_STRICT_COMPILE_OPTIONS)
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

    # ── GCC/Clang Security Hardening (Release builds) ────────────────────────
    # Flags are applied via generator expressions so both single-config
    # (Ninja, Makefiles) and multi-config (Ninja Multi-Config, Xcode) generators
    # work correctly.  _FORTIFY_SOURCE=3 requires -O1+; Release already adds -O3.
    #
    # For documentation of every flag and platform policy see:
    #   docs/de/security/COMPILER_SECURITY_HARDENING.md
    if(NOT THEMIS_DISABLE_SECURITY_HARDENING)
        include(CheckCXXCompilerFlag)
        include(CheckLinkerFlag)

        # ── Helper: abort if a required compile flag is not supported ──────
        macro(themis_require_compile_flag _flag)
            string(MAKE_C_IDENTIFIER "THEMIS_CXX_FLAG${_flag}" _var)
            check_cxx_compiler_flag("${_flag}" ${_var})
            if(NOT ${_var})
                message(FATAL_ERROR
                    "Security hardening compile flag '${_flag}' is not supported by "
                    "${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}. "
                    "Upgrade your compiler or set THEMIS_DISABLE_SECURITY_HARDENING=ON "
                    "(not recommended for production). SEC-CC-4.")
            endif()
        endmacro()

        # Probe capabilities unconditionally so checks work for all generator types.
        # Stack-Protector: guard against stack-buffer overflows (GCC 4.9+, Clang 3.7+)
        themis_require_compile_flag(-fstack-protector-strong)

        # Stack-Clash Protection: guard against stack–heap collision (GCC 8+, Clang 11+)
        check_cxx_compiler_flag(-fstack-clash-protection _THEMIS_HAVE_STACK_CLASH)

        # Linux full RELRO: probe linker flags at configure time (unconditional)
        if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
            set(_themis_relro_flags "-Wl,-z,relro" "-Wl,-z,now" "-Wl,-z,noexecstack")
            set(_themis_relro_ok TRUE)
            foreach(_lf IN LISTS _themis_relro_flags)
                string(MAKE_C_IDENTIFIER "THEMIS_LD${_lf}" _lv)
                check_linker_flag(CXX "${_lf}" ${_lv})
                if(NOT ${_lv})
                    message(WARNING
                        "Security linker flag '${_lf}' not supported; "
                        "Full RELRO will be unavailable. SEC-CC-4.")
                    set(_themis_relro_ok FALSE)
                endif()
            endforeach()
        endif()

        # Apply compile flags via generator expressions (Release config only).
        # -U_FORTIFY_SOURCE before -D_FORTIFY_SOURCE=3 ensures our level takes
        # precedence even on distributions that inject _FORTIFY_SOURCE via the
        # compiler spec (GCC 12+, Ubuntu 22.04+, RHEL 9+) and avoids a duplicate-
        # define diagnostic under -Werror.  Level 3 supersedes level 2 and is
        # available since GCC 12 / glibc 2.35.
        add_compile_options(
            $<$<CONFIG:Release>:-fstack-protector-strong>
            $<$<CONFIG:Release>:-U_FORTIFY_SOURCE>
            $<$<CONFIG:Release>:-D_FORTIFY_SOURCE=3>
        )
        if(_THEMIS_HAVE_STACK_CLASH)
            add_compile_options($<$<CONFIG:Release>:-fstack-clash-protection>)
        endif()

        # PIE (Position Independent Executable): required for kernel ASLR.
        # CMAKE_POSITION_INDEPENDENT_CODE applies -fPIC for shared libs and
        # -fPIE for executables; CMake adds the -pie linker flag for exe targets.
        set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)

        # Linux full RELRO + non-executable stack (linker flags, Release config only).
        # macOS uses ld64 which does not support ELF-style -z flags; RELRO is Linux-only.
        if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND _themis_relro_ok)
            add_link_options(
                $<$<CONFIG:Release>:-Wl,-z,relro>
                $<$<CONFIG:Release>:-Wl,-z,now>
                $<$<CONFIG:Release>:-Wl,-z,noexecstack>
            )
            message(STATUS "  Linker:  Full RELRO (-z relro -z now -z noexecstack)")
        endif()

        set(_themis_hardening_flags "-fstack-protector-strong -D_FORTIFY_SOURCE=3 -fPIE/-pie")
        if(_THEMIS_HAVE_STACK_CLASH)
            string(APPEND _themis_hardening_flags " -fstack-clash-protection")
        endif()
        message(STATUS "Security Hardening (GCC/Clang): ${_themis_hardening_flags}  [Release]")
        unset(_themis_hardening_flags)
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
