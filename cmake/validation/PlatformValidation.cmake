# ThemisDB Platform Validation
# Validates platform-specific requirements and constraints

if(NOT THEMIS_PLATFORM_DETECTED)
    message(FATAL_ERROR "PlatformValidation.cmake requires PlatformDetection.cmake to be included first")
endif()

message(STATUS "Validating platform requirements...")

# ARMv7 performance warnings
if(THEMIS_TARGET_ARCH STREQUAL "armv7")
    if(THEMIS_TARGET_PLATFORM STREQUAL "raspberry-pi")
        message(STATUS "  Building for Raspberry Pi (ARMv7)")
        message(WARNING "  Note: Compilation on Raspberry Pi may take several hours.")
        message(WARNING "  Recommended: Cross-compile from x86_64 using:")
        message(WARNING "    cmake -DCMAKE_TOOLCHAIN_FILE=cmake/platforms/Toolchains/armv7-linux-gnueabihf.cmake")
    endif()
    
    if(THEMIS_ENABLE_GPU)
        message(WARNING "  GPU acceleration on ARMv7 is limited. Consider using CPU-only build.")
    endif()
    
    if(THEMIS_ENABLE_DISTRIBUTED_TRAINING)
        message(WARNING "  Distributed training on ARMv7 is not recommended due to limited memory and CPU.")
    endif()
endif()

# QNAP platform warnings
if(THEMIS_TARGET_PLATFORM STREQUAL "qnap")
    message(STATUS "  Building for QNAP NAS platform")
    
    if(THEMIS_ENABLE_AVX2)
        message(WARNING "  AVX2 is disabled for QNAP build (Celeron N5095 limitation)")
    endif()
    
    if(THEMIS_ENABLE_GPU)
        message(WARNING "  GPU acceleration on QNAP may be limited. Verify GPU support on your model.")
    endif()
endif()

# Docker platform warnings
if(THEMIS_TARGET_PLATFORM STREQUAL "docker")
    message(STATUS "  Building for Docker environment")
    
    if(THEMIS_ENABLE_HUGE_PAGES)
        message(WARNING "  Huge pages may require Docker privileged mode or --cap-add=IPC_LOCK")
    endif()
endif()

# Cross-compilation warnings
if(CMAKE_CROSSCOMPILING)
    message(STATUS "  Cross-compiling from ${CMAKE_HOST_SYSTEM_PROCESSOR} to ${CMAKE_SYSTEM_PROCESSOR}")
    
    if(THEMIS_BUILD_TESTS)
        message(WARNING "  Building tests in cross-compile mode. Tests cannot be run on host system.")
    endif()
    
    if(THEMIS_BUILD_BENCHMARKS)
        message(WARNING "  Building benchmarks in cross-compile mode. Benchmarks cannot be run on host system.")
    endif()
endif()

# Windows-specific validation
if(THEMIS_TARGET_PLATFORM STREQUAL "windows")
    message(STATUS "  Building for Windows platform")
    
    if(THEMIS_ENABLE_HUGE_PAGES)
        message(STATUS "  Huge pages: auto-disabled (Linux-specific, not supported on Windows)")
        set(THEMIS_ENABLE_HUGE_PAGES OFF CACHE BOOL "Huge pages not supported on Windows" FORCE)
    endif()
endif()

# macOS-specific validation
if(THEMIS_TARGET_PLATFORM STREQUAL "macos")
    message(STATUS "  Building for macOS platform")
    
    if(THEMIS_ENABLE_CUDA)
        message(WARNING "  CUDA is not supported on macOS. Consider using Metal or disable GPU.")
    endif()
endif()

message(STATUS "Platform validation: OK")
