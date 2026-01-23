# ThemisDB Architecture-Specific Optimizations
# Applies -march flags based on architecture and platform
# Requires: PlatformDetection.cmake to be included first

if(NOT THEMIS_PLATFORM_DETECTED)
    message(FATAL_ERROR "ArchitectureOptimizations.cmake requires PlatformDetection.cmake to be included first")
endif()

# Skip architecture optimizations if cross-compiling (handled by toolchain file)
if(CMAKE_CROSSCOMPILING)
    message(STATUS "Cross-compiling: Architecture optimizations handled by toolchain file")
    return()
endif()

# Apply architecture-specific optimizations only in Release mode
if(NOT CMAKE_BUILD_TYPE STREQUAL "Release")
    message(STATUS "Architecture optimizations: Skipped (only applied in Release mode)")
    return()
endif()

message(STATUS "Applying architecture optimizations for ${THEMIS_TARGET_ARCH}...")

# x86_64 Optimizations
if(THEMIS_TARGET_ARCH STREQUAL "x86_64")
    if(THEMIS_QNAP_BUILD OR THEMIS_TARGET_PLATFORM STREQUAL "qnap")
        # QNAP Celeron N5095: SSE4.2 only (no AVX/FMA3)
        if(MSVC)
            # MSVC doesn't have -march, use /arch for specific instruction sets
            # SSE4.2 is baseline for x64, no flag needed
            add_compile_definitions(THEMIS_BASELINE_X64=1)
        else()
            add_compile_options(-march=x86-64 -msse4.2)
            add_compile_definitions(THEMIS_BASELINE_X64=1)
        endif()
        message(STATUS "  x86_64 optimization: SSE4.2 only (QNAP Celeron compatible)")
    elseif(THEMIS_ENABLE_AVX2)
        # Enable AVX2 + FMA for modern x86_64 CPUs
        if(MSVC)
            add_compile_options(/arch:AVX2)
        else()
            add_compile_options(-march=native)
        endif()
        add_compile_definitions(THEMIS_HAS_AVX2=1)
        message(STATUS "  x86_64 optimization: AVX2 + FMA (native)")
    else()
        # Baseline x86_64 without AVX2
        if(NOT MSVC)
            add_compile_options(-march=x86-64)
        endif()
        message(STATUS "  x86_64 optimization: Baseline (AVX2 disabled)")
    endif()
    
# ARM64 Optimizations
elseif(THEMIS_TARGET_ARCH STREQUAL "aarch64")
    if(NOT MSVC)
        # ARM64: NEON is standard on ARMv8-A
        add_compile_options(-march=armv8-a)
        add_compile_definitions(THEMIS_HAS_NEON=1)
        message(STATUS "  ARM64 optimization: ARMv8-A with NEON")
    endif()
    
# ARMv7 Optimizations
elseif(THEMIS_TARGET_ARCH STREQUAL "armv7")
    if(NOT MSVC)
        # ARMv7: NEON available on Raspberry Pi 2/3+, but optional
        option(THEMIS_ENABLE_ARM_NEON "Enable ARM NEON on ARMv7" ON)
        if(THEMIS_ENABLE_ARM_NEON)
            add_compile_options(-march=armv7-a -mfpu=neon -mfloat-abi=hard)
            add_compile_definitions(THEMIS_HAS_NEON=1)
            message(STATUS "  ARMv7 optimization: ARMv7-A with NEON")
        else()
            add_compile_options(-march=armv7-a)
            message(STATUS "  ARMv7 optimization: ARMv7-A without NEON")
        endif()
    endif()
    
# Unknown architecture
else()
    message(STATUS "  Architecture optimization: Skipped (unknown architecture: ${THEMIS_TARGET_ARCH})")
endif()
