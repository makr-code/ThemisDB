cmake_minimum_required(VERSION 3.20)

# LOAD MSVC COMPILER INIT (MUST be before any project() or subdirectory)
include("${CMAKE_CURRENT_SOURCE_DIR}/msvc-compiler-init.cmake" OPTIONAL RESULT_VARIABLE _INIT_LOADED)
if(NOT _INIT_LOADED)
    message(WARNING "msvc-compiler-init.cmake not found - compiler ABI tests may fail")
endif()

# ============================================================================
# WINDOWS COMPILER VALIDATION FIX (PRIORITY 0 - Before all else)
# ============================================================================
# MSVC compiler ABI detection tests create deep TryCompile-* directories
# that can exceed Windows MAX_PATH in nested ExternalProject contexts.
# Solution: Pre-validate compiler to skip these expensive tests in ALL sub-processes
# MUST be set as CACHE FORCE before any project() or sub-include that triggers compiler checks
if(WIN32 AND MSVC)
    set(CMAKE_C_COMPILER_WORKS ON CACHE BOOL "C compiler pre-validated" FORCE)
    set(CMAKE_CXX_COMPILER_WORKS ON CACHE BOOL "CXX compiler pre-validated" FORCE)
endif()

# ============================================================================
# WINDOWS MAX_PATH FLATTENING SOLUTION
# ============================================================================
# Problem: ExternalProject (llama.cpp) + Sub-CMake (Vulkan Shader Generator)
# create deeply nested paths that exceed Windows MAX_PATH (260 chars).
# Example:
#   C:\Projects\ThemisDB\build-x\
#   └─ _deps/llama_cpp-build/ggml/src/ggml-vulkan/vulkan-shaders-gen-prefix/src/vulkan-shaders-gen-build/CMakeFiles/CMakeScratch/TryCompile-xxx/
# 
# Solution: Flatten ExternalProject directories + enable Long Paths on Windows
# ============================================================================

# 0. WINDOWS SDK LIBRARY PATH FIX FOR EXTERNALPROJECT SUB-CMAKE PROCESSES
# When ExternalProject spawns isolated sub-CMake processes (e.g., Vulkan Shader Generator),
# they lose access to Windows SDK library directories, causing LNK1104 errors.
# Solution: Set CMAKE_LIBRARY_PATH globally to ensure libraries are found in all sub-processes
if(WIN32 AND MSVC AND NOT CMAKE_LIBRARY_PATH)
    # Find Windows SDK and MSVC library directories
    set(_WIN_SDK_VERSIONS "10.0.26100.0" "10.0.22621.0" "10.0.19041.0")
    set(_CMAKE_LIB_PATH_LIST "")
    set(_themis_program_files_x86 "$ENV{ProgramFiles\(x86\)}")
    if(NOT _themis_program_files_x86)
        set(_themis_program_files_x86 "$ENV{ProgramFiles}")
    endif()
    
    # Add MSVC library paths
    set(_MSVC_LIB_PATHS)
    if(DEFINED ENV{VCToolsInstallDir} AND EXISTS "$ENV{VCToolsInstallDir}/lib/x64")
        list(APPEND _MSVC_LIB_PATHS "$ENV{VCToolsInstallDir}/lib/x64")
    endif()
    if(_themis_program_files_x86)
        file(GLOB _themis_globbed_msvc_lib_paths
            "${_themis_program_files_x86}/Microsoft Visual Studio/*/*/VC/Tools/MSVC/*/lib/x64")
        list(APPEND _MSVC_LIB_PATHS ${_themis_globbed_msvc_lib_paths})
    endif()
    list(REMOVE_DUPLICATES _MSVC_LIB_PATHS)
    if(_MSVC_LIB_PATHS)
        list(APPEND _CMAKE_LIB_PATH_LIST ${_MSVC_LIB_PATHS})
    endif()
    
    # Add Windows SDK library paths
    set(_themis_windows_sdk_dir "$ENV{WindowsSdkDir}")
    set(_themis_windows_sdk_lib_version "$ENV{WindowsSDKLibVersion}")
    if(NOT _themis_windows_sdk_lib_version)
        set(_themis_windows_sdk_lib_version "$ENV{WindowsSDKVersion}")
    endif()
    if(_themis_windows_sdk_dir AND _themis_windows_sdk_lib_version)
        set(_SDK_UM_PATH "${_themis_windows_sdk_dir}/Lib/${_themis_windows_sdk_lib_version}/um/x64")
        set(_SDK_UCRT_PATH "${_themis_windows_sdk_dir}/Lib/${_themis_windows_sdk_lib_version}/ucrt/x64")
        if(EXISTS "${_SDK_UM_PATH}")
            list(APPEND _CMAKE_LIB_PATH_LIST "${_SDK_UM_PATH}")
        endif()
        if(EXISTS "${_SDK_UCRT_PATH}")
            list(APPEND _CMAKE_LIB_PATH_LIST "${_SDK_UCRT_PATH}")
        endif()
    endif()
    foreach(_VER ${_WIN_SDK_VERSIONS})
        if(NOT _themis_program_files_x86)
            break()
        endif()
        set(_SDK_UM_PATH "${_themis_program_files_x86}/Windows Kits/10/Lib/${_VER}/um/x64")
        set(_SDK_UCRT_PATH "${_themis_program_files_x86}/Windows Kits/10/Lib/${_VER}/ucrt/x64")
        if(EXISTS "${_SDK_UM_PATH}")
            list(APPEND _CMAKE_LIB_PATH_LIST "${_SDK_UM_PATH}")
        endif()
        if(EXISTS "${_SDK_UCRT_PATH}")
            list(APPEND _CMAKE_LIB_PATH_LIST "${_SDK_UCRT_PATH}")
        endif()
    endforeach()
    
    # Set CMAKE_LIBRARY_PATH as global cache variable (visible to all sub-CMake processes)
    if(_CMAKE_LIB_PATH_LIST)
        string(REPLACE ";" "\\;" _CMAKE_LIB_PATH_STR "${_CMAKE_LIB_PATH_LIST}")
        set(CMAKE_LIBRARY_PATH "${_CMAKE_LIB_PATH_STR}" CACHE STRING "Windows SDK and MSVC library paths" FORCE)
        message(STATUS "[Windows SDK Fix] CMAKE_LIBRARY_PATH configured for sub-CMake processes")
    endif()
endif()

# 1. FLATTEN EXTERNAL PROJECT DIRECTORIES
# Instead of nested _deps/llama_cpp-build/..., use flat directory in ThemisDB root
# This keeps ExternalProject builds persistent across clean builds and minimizes path depth
set(CMAKE_EXTERNALS_BASE_DIR "${THEMIS_ROOT_DIR}/_e")
file(MAKE_DIRECTORY "${CMAKE_EXTERNALS_BASE_DIR}")
message(STATUS "ExternalProject base directory: ${CMAKE_EXTERNALS_BASE_DIR}")

# 2. WINDOWS EXTENDED PATHS (LongPathsEnabled on Windows 10+)
# Allows paths > 260 chars if explicitly enabled
if(WIN32)
    # Enable long paths in Windows registry (requires admin or Group Policy)
    # This is informational; actual setting is system-level, not CMake-level
    message(STATUS "Windows detected: Long path support recommended. To enable:")
    message(STATUS "  PowerShell (Admin): Set-ItemProperty -Path 'HKLM:\\SYSTEM\\CurrentControlSet\\Control\\FileSystem' -Name 'LongPathsEnabled' -Value 1")
    message(STATUS "  Or: Group Policy Editor > Computer Config > Admin Temp > System > Filesystem > Enable Win32 Long Paths")
    
    # Increase CMAKE_OBJECT_PATH_MAX to accommodate longer build paths
    set(CMAKE_OBJECT_PATH_MAX 512 CACHE STRING "Max path length for object files (default 250, set to 512 for long paths)")
endif()

# 3. MSVC-SPECIFIC: Reduce compiler test directory depth
if(MSVC)
    # Skip ABI detection for sub-projects to avoid deep CMakeScratch directories
    set(CMAKE_C_COMPILER_WORKS ON CACHE BOOL "C compiler pre-validated" FORCE)
    set(CMAKE_CXX_COMPILER_WORKS ON CACHE BOOL "CXX compiler pre-validated" FORCE)
    
    # Use config-style package discovery to avoid deep library searches
    set(CMAKE_FIND_PACKAGE_PREFER_CONFIG ON CACHE BOOL "Prefer -config.cmake over Find modules" FORCE)
endif()

# 4. EXTERNAL PROJECT PATH FLATTENING FUNCTION
# Defines helper to redirect ExternalProject to flat directories instead of nesting
function(add_external_project_flat PROJECT_NAME)
    # This function ensures external projects go to CMAKE_EXTERNALS_BASE_DIR
    # Usage: add_external_project_flat(llama_cpp ...)
    # Creates: _exts/llama_cpp/ instead of _deps/llama_cpp-build/...
    set(${PROJECT_NAME}_BINARY_DIR "${CMAKE_EXTERNALS_BASE_DIR}/${PROJECT_NAME}" PARENT_SCOPE)
endfunction()

# When building directly from the cmake/ subdirectory (e.g. cmake -S cmake),
# THEMIS_ROOT_DIR is not yet set.  Default it to the parent of this file.
if(NOT THEMIS_ROOT_DIR)
    get_filename_component(THEMIS_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
endif()

# VERSION ist Single-Source-of-Truth: zwingend aus Datei lesen
if(NOT EXISTS "${THEMIS_ROOT_DIR}/VERSION")
    message(FATAL_ERROR "VERSION file not found at project root. Please create and set semantic version (e.g., 1.2.3).")
endif()
file(READ "${THEMIS_ROOT_DIR}/VERSION" _ver)
string(STRIP "${_ver}" _ver)
# Accept semantic versions with optional pre-release/build suffixes, but sanitize for CMake's project(VERSION)
if(NOT _ver MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+(.*)?$")
    message(FATAL_ERROR "Invalid VERSION content: '${_ver}'. Expected semantic version like 1.2.3")
endif()
# Extract numeric MAJOR.MINOR.PATCH for CMake's version field
string(REGEX MATCH "^[0-9]+\\.[0-9]+\\.[0-9]+" _ver_numeric "${_ver}")
if(NOT _ver_numeric)
    message(FATAL_ERROR "Failed to parse numeric version from '${_ver}'")
endif()

project(Themis VERSION ${_ver_numeric} LANGUAGES CXX)

# Treat uncomponentized install() rules as runtime payload by default.
# This keeps third-party DLLs in the runtime component instead of a parallel
# 'Unspecified' component, which otherwise causes duplicate WiX components.
set(CMAKE_INSTALL_DEFAULT_COMPONENT_NAME runtime)

# Make project version available to all translation units
if(NOT THEMIS_VERSION_COMPILE_DEF_DEFINED)
    add_compile_definitions(THEMIS_VERSION_STRING="${_ver}")
    set(THEMIS_VERSION_COMPILE_DEF_DEFINED ON)
endif()

# GPU backend auto-discovery (no preset branching required).
# Semantics: THEMIS_ENABLE_GPU=ON means "probe all supported frameworks and
# enable what is available", with CPU fallback always present at runtime.
if(NOT DEFINED THEMIS_ENABLE_GPU)
    set(THEMIS_ENABLE_GPU ON CACHE BOOL "Enable GPU acceleration for vector search (default ON)")
endif()
option(THEMIS_GPU_AUTO_DETECT "Auto-detect CUDA/HIP/Vulkan backends when THEMIS_ENABLE_GPU=ON" ON)

if(NOT DEFINED THEMIS_ENABLE_CUDA)
    set(THEMIS_ENABLE_CUDA OFF CACHE BOOL "Enable NVIDIA CUDA acceleration" FORCE)
endif()
if(NOT DEFINED THEMIS_ENABLE_HIP)
    set(THEMIS_ENABLE_HIP OFF CACHE BOOL "Enable AMD HIP acceleration" FORCE)
endif()
if(NOT DEFINED THEMIS_ENABLE_VULKAN)
    set(THEMIS_ENABLE_VULKAN ON CACHE BOOL "Enable Vulkan Compute acceleration" FORCE)
endif()

if(THEMIS_ENABLE_GPU AND THEMIS_GPU_AUTO_DETECT)
    message(STATUS "GPU backend auto-detect: probing CUDA/HIP/Vulkan")
    set(_themis_detected_gpu_backends)

    # Vulkan (cross-vendor)
    find_package(Vulkan QUIET)
    if(Vulkan_FOUND)
        set(THEMIS_ENABLE_VULKAN ON CACHE BOOL "Enable Vulkan Compute acceleration" FORCE)
        list(APPEND _themis_detected_gpu_backends "Vulkan")
    else()
        set(THEMIS_ENABLE_VULKAN OFF CACHE BOOL "Enable Vulkan Compute acceleration" FORCE)
    endif()

    # HIP (AMD ROCm)
    find_program(_THEMIS_HIPCC_EXE NAMES hipcc HINTS ENV ROCM_PATH ENV HIP_PATH PATH_SUFFIXES bin)
    if(_THEMIS_HIPCC_EXE)
        set(THEMIS_ENABLE_HIP ON CACHE BOOL "Enable AMD HIP acceleration" FORCE)
        list(APPEND _themis_detected_gpu_backends "HIP")
    else()
        set(THEMIS_ENABLE_HIP OFF CACHE BOOL "Enable AMD HIP acceleration" FORCE)
    endif()

    # CUDA (NVIDIA)
    if(NOT DEFINED CMAKE_CUDA_COMPILER OR CMAKE_CUDA_COMPILER STREQUAL "")
        set(_themis_cuda_nvcc_candidates)

        if(DEFINED ENV{CUDACXX} AND EXISTS "$ENV{CUDACXX}")
            list(APPEND _themis_cuda_nvcc_candidates "$ENV{CUDACXX}")
        endif()
        if(DEFINED ENV{CUDA_PATH} AND EXISTS "$ENV{CUDA_PATH}/bin/nvcc.exe")
            list(APPEND _themis_cuda_nvcc_candidates "$ENV{CUDA_PATH}/bin/nvcc.exe")
        endif()
        if(DEFINED ENV{CUDA_HOME} AND EXISTS "$ENV{CUDA_HOME}/bin/nvcc.exe")
            list(APPEND _themis_cuda_nvcc_candidates "$ENV{CUDA_HOME}/bin/nvcc.exe")
        endif()

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
            if(EXISTS "${_cuda_root}/bin/nvcc.exe")
                list(APPEND _themis_cuda_nvcc_candidates "${_cuda_root}/bin/nvcc.exe")
            endif()
        endforeach()

        list(LENGTH _themis_cuda_nvcc_candidates _themis_cuda_nvcc_count)
        if(_themis_cuda_nvcc_count GREATER 0)
            list(GET _themis_cuda_nvcc_candidates 0 _themis_cuda_nvcc)
            set(CMAKE_CUDA_COMPILER "${_themis_cuda_nvcc}" CACHE FILEPATH "CUDA compiler" FORCE)
            get_filename_component(_themis_cuda_bin "${_themis_cuda_nvcc}" DIRECTORY)
            get_filename_component(_themis_cuda_root "${_themis_cuda_bin}" DIRECTORY)
            set(CUDAToolkit_ROOT "${_themis_cuda_root}" CACHE PATH "CUDA toolkit root" FORCE)
        endif()
    endif()

    include(CheckLanguage)
    if(CMAKE_CUDA_COMPILER OR _themis_cuda_nvcc_count GREATER 0)
        check_language(CUDA)
        if(CMAKE_CUDA_COMPILER)
            enable_language(CUDA)
            set(CMAKE_CUDA_STANDARD 17)
            set(CMAKE_CUDA_STANDARD_REQUIRED ON)
            set(THEMIS_ENABLE_CUDA ON CACHE BOOL "Enable NVIDIA CUDA acceleration" FORCE)
            list(APPEND _themis_detected_gpu_backends "CUDA")
        else()
            set(THEMIS_ENABLE_CUDA OFF CACHE BOOL "Enable NVIDIA CUDA acceleration" FORCE)
        endif()
    else()
        set(THEMIS_ENABLE_CUDA OFF CACHE BOOL "Enable NVIDIA CUDA acceleration" FORCE)
        message(STATUS "CUDA toolkit not found; skipping CUDA language probe")
    endif()

    if(_themis_detected_gpu_backends)
        string(REPLACE ";" ", " _themis_detected_gpu_backends_msg "${_themis_detected_gpu_backends}")
        message(STATUS "GPU backend auto-detect: enabled backends = ${_themis_detected_gpu_backends_msg}")
    else()
        message(WARNING "GPU backend auto-detect: no CUDA/HIP/Vulkan toolchain found; runtime will use CPU fallback")
    endif()
endif()

# Build provenance: channel, Git SHA, timestamp, Ed25519 signature
# Generates ${CMAKE_BINARY_DIR}/include/updates/build_info.h
include(${CMAKE_CURRENT_SOURCE_DIR}/BuildInfo.cmake)

# Include modular build configuration (post-v1.3.0 feature)
# See docs/architecture/MODULARIZATION_PLAN.md for details
include(${CMAKE_CURRENT_SOURCE_DIR}/ModularBuild.cmake)

# C++20 Standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Keep all runtime artifacts (EXE/DLL) in a single bin/ directory to avoid PATH/env mismatches.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin_out")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin_out")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin_out")

# Runtime library for static linking on Windows
if(MSVC AND VCPKG_TARGET_TRIPLET MATCHES "static")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "" FORCE)
endif()

# CUDA detection deduplication: (removed - identical code already above in GPU auto-detect section)

# Export compile commands for IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# vcpkg integration
if(DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "Vcpkg toolchain file")
endif()
# Prefer CONFIG packages (vcpkg) over FindXXX modules
set(CMAKE_FIND_PACKAGE_PREFER_CONFIG ON)

# Some imported dependency graphs still emit bare library names (e.g. lz4.lib,
# snappy.lib) for selected focused test targets. Ensure the vcpkg lib dir is in
# the global linker search path on Windows so those names resolve reliably.
if(WIN32 AND DEFINED VCPKG_TARGET_TRIPLET AND NOT "${VCPKG_TARGET_TRIPLET}" STREQUAL "")
    set(_themis_vcpkg_lib_dir "${THEMIS_ROOT_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/lib")
    if(EXISTS "${_themis_vcpkg_lib_dir}")
        link_directories("${_themis_vcpkg_lib_dir}")
    endif()
endif()

# Dependencies
find_package(OpenSSL CONFIG)
if(NOT OpenSSL_FOUND)
    find_package(OpenSSL)
endif()
if(NOT OpenSSL_FOUND)
    message(WARNING "OpenSSL not found; using interface fallback targets and disabling OpenSSL-backed TSA features")
    add_library(OpenSSL::SSL INTERFACE IMPORTED)
    add_library(OpenSSL::Crypto INTERFACE IMPORTED)
    set(THEMIS_USE_OPENSSL_TSA OFF CACHE BOOL "Enable RFC 3161 timestamping via OpenSSL TS_* API + libcurl" FORCE)
endif()

# zlib (required for SAML HTTP-Redirect binding DEFLATE compression)
find_package(ZLIB REQUIRED)

# Make CURL optional: some environments (CI/dev) may not have dev package installed.
# If found, create CURL::libcurl target; otherwise provide a fallback and continue.
find_package(CURL CONFIG)
if(NOT CURL_FOUND)
    find_package(CURL)
endif()
if(NOT CURL_FOUND)
    message(WARNING "CURL package not found; building without libcurl. Some features may be disabled.")
endif()

# Office document support (DOCX/XLSX/PPTX)
# Find libzip (needed for both Office and Archive processing)
# Skip libzip to avoid broken CONFIG targets
# find_package(libzip CONFIG)
# Commented out due to broken system libzip CMake config
# if(NOT libzip_FOUND)
#     find_package(libzip)
# endif()
set(libzip_FOUND OFF)

if(libzip_FOUND)
    message(STATUS "libzip found - Archive processing available")
else()
    message(WARNING "libzip not found - Install with: vcpkg install libzip")
    message(WARNING "Archive processing will not be available")
endif()

# pugixml is used by SAML/XML parsing and Office processing.
find_package(pugixml CONFIG)
if(NOT pugixml_FOUND)
    find_package(pugixml)
endif()

# Office support requires both libzip and pugixml
if(THEMIS_ENABLE_OFFICE)
    if(libzip_FOUND AND pugixml_FOUND)
        message(STATUS "Office support enabled - libzip and pugixml found")
        add_compile_definitions(THEMIS_ENABLE_OFFICE)
    else()
        if(NOT pugixml_FOUND)
            message(WARNING "pugixml not found - Install with: vcpkg install pugixml")
        endif()
        message(WARNING "Disabling Office support due to missing dependencies")
        set(THEMIS_ENABLE_OFFICE OFF CACHE BOOL "Office support disabled - missing dependencies" FORCE)
    endif()
endif()

# PDF support requires poppler-cpp
if(THEMIS_ENABLE_PDF)
    find_package(Poppler CONFIG COMPONENTS cpp QUIET)
    if(NOT Poppler_FOUND)
        find_package(Poppler COMPONENTS cpp QUIET)
    endif()

    if(Poppler_FOUND)
        message(STATUS "PDF support enabled - poppler-cpp found")
        add_compile_definitions(THEMIS_ENABLE_PDF)
    else()
        message(WARNING "poppler-cpp not found - Install with: vcpkg install poppler")
        message(WARNING "Disabling PDF layout preservation due to missing dependencies")
        set(THEMIS_ENABLE_PDF OFF CACHE BOOL "PDF support disabled - missing poppler-cpp" FORCE)
    endif()
endif()

# OCR support requires libtesseract + leptonica
if(THEMIS_ENABLE_OCR)
    find_package(Tesseract CONFIG QUIET)
    if(NOT Tesseract_FOUND)
        find_package(Tesseract QUIET)
    endif()

    if(Tesseract_FOUND)
        message(STATUS "OCR support enabled - libtesseract found")
        add_compile_definitions(THEMIS_ENABLE_OCR)
    else()
        message(WARNING "libtesseract not found - Install with: vcpkg install tesseract")
        message(WARNING "Disabling OCR support due to missing dependencies")
        set(THEMIS_ENABLE_OCR OFF CACHE BOOL "OCR support disabled - missing libtesseract" FORCE)
    endif()
endif()

# FFmpeg support for video processing (optional)
# Priority: 1) Local Submodule, 2) vcpkg, 3) System
find_package(FFmpeg CONFIG QUIET)
if(NOT FFmpeg_FOUND)
    find_package(FFmpeg QUIET)
endif()

# If not found, try building from local submodule
if(NOT FFmpeg_FOUND AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../ffmpeg/CMakeLists.txt")
    message(STATUS "Found FFmpeg submodule - building from source")
    set(FFMPEG_ENABLE_SMALL ON CACHE BOOL "Build small FFmpeg" FORCE)
    set(FFMPEG_ENABLE_OPTIMIZED_BUILD ON CACHE BOOL "Build optimized FFmpeg" FORCE)
    # Note: FFmpeg might not have CMakeLists directly; requires configure + make wrapper
    # For now, mark as found if directory exists
    set(FFmpeg_FOUND TRUE)
    set(FFMPEG_SUBMODULE_BUILD ON)
    message(STATUS "FFmpeg submodule found at: ${CMAKE_CURRENT_SOURCE_DIR}/../ffmpeg")
endif()

if(FFmpeg_FOUND)
    if(FFMPEG_SUBMODULE_BUILD)
        message(STATUS "FFmpeg will be built from submodule")
    else()
        message(STATUS "Found FFmpeg: ${FFmpeg_DIR}")
    endif()
    set(FFMPEG_FOUND TRUE)
else()
    message(STATUS "FFmpeg not found - attempting to build from submodule or install with: vcpkg install ffmpeg")
    set(FFMPEG_FOUND FALSE)
endif()

# Build options
option(THEMIS_BUILD_TESTS "Build unit tests" ON)
option(THEMIS_BUILD_BENCHMARKS "Build performance benchmarks" OFF)
option(THEMIS_BUILD_CHIMERA "Build CHIMERA adapters, focused tests, and benchmark integration" OFF)

# GPU and LLM defaults (can be overridden via -D flags)
# GPU acceleration includes FAISS, OpenBLAS, LAPACK via vcpkg
if(NOT DEFINED THEMIS_ENABLE_GPU)
    set(THEMIS_ENABLE_GPU ON CACHE BOOL "Enable GPU acceleration for vector search (default ON)" )
endif()
if(NOT DEFINED THEMIS_ENABLE_LLM)
    set(THEMIS_ENABLE_LLM ON CACHE BOOL "Enable LLM plugin support with llama.cpp (default ON)" )
endif()

option(THEMIS_ENABLE_TRACING "Enable OpenTelemetry distributed tracing" ON)
option(THEMIS_ENABLE_ASAN "Enable AddressSanitizer for debugging" OFF)
option(THEMIS_STRICT_BUILD "Treat warnings as errors" OFF)
option(THEMIS_ENABLE_AVX2 "Enable AVX2/FMA for SIMD kernels in Release builds (MSVC)" ON)
option(THEMIS_QNAP_BUILD "Build for QNAP NAS with baseline x86-64 (no AVX)" OFF)
option(THEMIS_ENABLE_HSM_REAL "Enable real PKCS#11 HSM provider (fallback to stub if OFF)" OFF)
option(THEMIS_STATIC_BUILD "Build fully static binary for maximum portability (e.g., QNAP)" OFF)
option(THEMIS_CORE_SHARED "Build themis_core as a shared library (DLL/.so) instead of static" OFF)
option(THEMIS_ENABLE_CONTENT_PROCESSORS "Include content processors (audio/image/video/geo/cad) in core" OFF)

# For QNAP or fully static builds, force static core library
if(THEMIS_QNAP_BUILD OR THEMIS_STATIC_BUILD)
    set(THEMIS_CORE_SHARED OFF CACHE BOOL "Force static core for QNAP/static builds" FORCE)
    # FORCE STATIC BUILD FOR WINDOWS (LNK1189 Export Limit Workaround - v1.3.0+)
    set(THEMIS_CORE_SHARED OFF CACHE BOOL "Force static core to avoid Windows DLL export limit (LNK1189)" FORCE)
endif()

# Note: THEMIS_ENABLE_LLM is now set above as FORCE ON (Core Feature)
# Community Edition: LLM is a core feature (llama.cpp Embedding, Similarity, Inference)
# Enterprise/Hyperscaler: Advanced LLM features (Fine-Tuning, Model Management) available as add-on module

# Voice Assistant Support (v1.4.0+) - Enterprise Feature
# Requires: Whisper.cpp (STT), Piper TTS (TTS), llama.cpp (LLM)
option(THEMIS_ENABLE_VOICE_ASSISTANT "Enable Voice Assistant with STT/TTS/LLM (v1.4.0+)" OFF)
option(THEMIS_ENABLE_WHISPER "Enable Whisper.cpp for Speech-to-Text" OFF)
option(THEMIS_ENABLE_PIPER_TTS "Enable Piper TTS for Text-to-Speech" OFF)
option(THEMIS_ENABLE_RNNOISE "Enable RNNoise deep-learning noise suppression" OFF)

# Hardware acceleration backend options
option(THEMIS_ENABLE_CUDA "Enable NVIDIA CUDA acceleration" OFF)
option(THEMIS_ENABLE_HIP "Enable AMD HIP acceleration" OFF)
option(THEMIS_ENABLE_NCCL "Enable NCCL for NVIDIA multi-GPU (v2.5+)" OFF)
option(THEMIS_ENABLE_RCCL "Enable RCCL for AMD multi-GPU (v2.5+)" OFF)

# cuVS/RAFT GPU-accelerated ANN (Approximate Nearest Neighbor) search.
# Requires THEMIS_ENABLE_CUDA=ON and the cuVS/RAFT CMake package to be
# installed (e.g. via conda: `conda install -c rapidsai cuvs`).
# When OFF the CPU brute-force fallback in GPUQueryAccelerator::annSearch()
# is used unconditionally.
if(THEMIS_ENABLE_CUDA)
    option(THEMIS_ENABLE_CUVS "Enable cuVS/RAFT GPU-accelerated ANN search (requires CUDA)" OFF)
else()
    set(THEMIS_ENABLE_CUVS OFF CACHE BOOL
        "cuVS/RAFT ANN search (requires THEMIS_ENABLE_CUDA=ON)" FORCE)
endif()

# Geospatial Processing (P2 - v1.4.0+)
# Enables GDAL integration for Shapefile and GeoTIFF support
option(THEMIS_ENABLE_GDAL "Enable GDAL integration for geospatial data processing" OFF)

# Geospatial CUDA kernel dispatch (v2.1.0+)
# Enables real CUDA kernels for batch point-in-polygon and Haversine distance in
# the geo GPU backend.  Requires THEMIS_ENABLE_CUDA=ON and CMAKE_CUDA_COMPILER.
# Default OFF so CPU-only builds are unaffected.
if(THEMIS_ENABLE_CUDA AND CMAKE_CUDA_COMPILER)
    option(THEMIS_GEO_CUDA "Enable CUDA kernel dispatch in geo GPU backend" ON)
else()
    set(THEMIS_GEO_CUDA OFF CACHE BOOL
        "CUDA geo kernels (requires THEMIS_ENABLE_CUDA=ON)" FORCE)
endif()

# Geospatial HIP kernel dispatch (ROCm/AMD hardware)
# Enables real HIP kernels for batch point-in-polygon and Haversine distance in
# the geo GPU backend.  Requires THEMIS_ENABLE_HIP=ON.
# THEMIS_GEO_CUDA takes precedence when both are available.
# Default OFF so CPU-only and CUDA-only builds are unaffected.
if(THEMIS_ENABLE_HIP AND NOT THEMIS_GEO_CUDA)
    option(THEMIS_GEO_HIP "Enable HIP kernel dispatch in geo GPU backend (AMD ROCm)" ON)
else()
    set(THEMIS_GEO_HIP OFF CACHE BOOL
        "HIP geo kernels (requires THEMIS_ENABLE_HIP=ON and THEMIS_GEO_CUDA=OFF)" FORCE)
endif()

# PDF Document Processing (Q2 2026)
# Enables poppler-cpp integration for layout-preserving PDF text extraction
option(THEMIS_ENABLE_PDF "Enable PDF text extraction with layout preservation (requires poppler-cpp)" OFF)

# Office Document Processing (P2 - v1.4.0+)
# Enables libzip and pugixml integration for DOCX/XLSX/PPTX support
option(THEMIS_ENABLE_OFFICE "Enable Office document support (DOCX/XLSX/PPTX) with libzip+pugixml" ON)

# OCR Processing (Phase 3 - Advanced Format Support)
# Enables Tesseract OCR for text extraction from image content (JPEG, PNG, TIFF)
option(THEMIS_ENABLE_OCR "Enable Tesseract OCR for image text extraction (requires libtesseract+leptonica)" OFF)

# Performance Optimization Options (Research-based, see docs/de/research/)
# Phase 1: Quick Wins - Low effort, high impact optimizations (from PR #157)
# NOTE: These are automatically enabled when building tests/benchmarks for accurate performance measurement
option(THEMIS_ENABLE_MIMALLOC "Use mimalloc allocator for +10-20% performance" OFF)
option(THEMIS_ENABLE_JEMALLOC "Use jemalloc allocator as alternative (best fragmentation resistance, Linux/Mac only)" OFF)
option(THEMIS_ENABLE_HUGE_PAGES "Enable huge pages support for +15-30% memory performance" OFF)
option(THEMIS_ENABLE_RCU_INDEX "Use RCU for read-heavy index paths (+200-500% read performance)" OFF)
option(THEMIS_ENABLE_LIRS_CACHE "Use LIRS cache replacement policy (+30-40% hit rate)" OFF)

# Auto-enable Phase 1 performance features for tests/benchmarks
if(THEMIS_BUILD_TESTS OR THEMIS_BUILD_BENCHMARKS)
    if(NOT DEFINED THEMIS_PERF_AUTO_ENABLE)
        set(THEMIS_PERF_AUTO_ENABLE ON CACHE BOOL "Auto-enable performance features for tests/benchmarks")
    endif()
    
    if(THEMIS_PERF_AUTO_ENABLE)
        message(STATUS "Auto-enabling Phase 1 performance optimizations for tests/benchmarks")
        message(STATUS "  (Disable with: cmake -DTHEMIS_PERF_AUTO_ENABLE=OFF)")
        
        # Only auto-enable if not explicitly set by user
        if(NOT THEMIS_ENABLE_MIMALLOC)
            set(THEMIS_ENABLE_MIMALLOC ON CACHE BOOL "Auto-enabled for tests/benchmarks" FORCE)
            message(STATUS "  → THEMIS_ENABLE_MIMALLOC=ON (auto)")
        endif()
        
        if(NOT THEMIS_ENABLE_RCU_INDEX)
            set(THEMIS_ENABLE_RCU_INDEX ON CACHE BOOL "Auto-enabled for tests/benchmarks" FORCE)
            message(STATUS "  → THEMIS_ENABLE_RCU_INDEX=ON (auto)")
        endif()
        
        if(NOT THEMIS_ENABLE_LIRS_CACHE)
            set(THEMIS_ENABLE_LIRS_CACHE ON CACHE BOOL "Auto-enabled for tests/benchmarks" FORCE)
            message(STATUS "  → THEMIS_ENABLE_LIRS_CACHE=ON (auto)")
        endif()
        
        # Huge Pages nur auf Linux auto-enablen
        if(NOT WIN32 AND NOT THEMIS_ENABLE_HUGE_PAGES)
            set(THEMIS_ENABLE_HUGE_PAGES ON CACHE BOOL "Auto-enabled for tests/benchmarks (Linux only)" FORCE)
            message(STATUS "  → THEMIS_ENABLE_HUGE_PAGES=ON (auto, Linux)")
        endif()
    endif()
endif()

# Phase 2: Medium-Term - Higher impact, medium effort optimizations (from PR #156/#157 research)
# Medium-term optimizations (3-6 months effort, +100-200% overall performance)
option(THEMIS_ENABLE_WISCKEY "Enable WiscKey key/value separation (+40-60% writes)" OFF)
option(THEMIS_ENABLE_DOSTOEVSKY "Enable Dostoevsky adaptive LSM (+25-35% mixed)" OFF)
option(THEMIS_ENABLE_CICADA "Enable Cicada optimistic CC (+100-150% transactions)" OFF)
option(THEMIS_ENABLE_LIGRA "Enable Ligra graph processing (+200-300% graph)" OFF)
option(THEMIS_ENABLE_RABITQ "Enable RaBitQ vector quantization (16x memory reduction)" OFF)

# Phase 3: Long-Term - Very high impact, high effort optimizations (from PR #156/#157 research)
# Long-term optimizations (6-12 months effort, +200-500% specialized workloads)
option(THEMIS_ENABLE_DISKANN "Enable DiskANN billion-scale vector search (+300-400%)" OFF)
option(THEMIS_ENABLE_BWTREE "Enable Bw-Tree lock-free index (+100-200%)" OFF)
option(THEMIS_ENABLE_SPLINTERDB "Enable SplinterDB concurrent compaction (-70% P99)" OFF)
option(THEMIS_ENABLE_GUNROCK "Enable Gunrock GPU graph analytics (+1000-3000%)" OFF)
option(THEMIS_ENABLE_BAO "Enable Bao ML query optimizer (+30-70%)" OFF)
option(THEMIS_ENABLE_PER_QUERY_COST_MODEL "Enable per-query cost model integration with query optimizer (+10-30% plan accuracy)" OFF)

# Phase 4: ML & CI Integration - Very high impact, very high effort (Issue #2424)
# Long-term optimizations (6-12 months effort, +50-200% PMem-heavy workloads)
option(THEMIS_ENABLE_PMEM "Enable Persistent Memory (Optane) aware storage layout (+50-200%)" OFF)
option(THEMIS_ENABLE_PMU_COUNTERS "Enable hardware performance counter (PMU) integration for cache-miss analysis" OFF)
option(THEMIS_ENABLE_IO_URING "Enable io_uring zero-copy I/O path for network performance (Linux kernel ≥ 5.1)" OFF)

# Build options
option(THEMIS_BUILD_BENCHMARKS "Build performance benchmarks (requires Google Benchmark)" ON)

# Performance Optimization Dependencies
if(THEMIS_ENABLE_MIMALLOC)
    message(STATUS "Enabling mimalloc allocator optimization (+10-20% performance)")
    find_package(mimalloc CONFIG)
    if(NOT mimalloc_FOUND)
        message(WARNING "mimalloc not found via vcpkg. Install with: vcpkg install mimalloc")
        message(WARNING "Building without mimalloc. Performance optimization disabled.")
        set(THEMIS_ENABLE_MIMALLOC OFF CACHE BOOL "mimalloc not available" FORCE)
    else()
        message(STATUS "Found mimalloc: ${mimalloc_DIR}")
        add_compile_definitions(THEMIS_ENABLE_MIMALLOC)
        
        # On Windows: exclude mimalloc-redirect.dll to prevent DLL-injection crash
        # Use only mimalloc.dll (static override is safer)
        if(WIN32)
            message(STATUS "Windows: Disabling mimalloc-redirect DLL to prevent loader crash")
            # Tell mimalloc to use static override instead of DLL redirection
            add_compile_definitions(MI_DISABLE_REDIRECT=1)
        endif()
    endif()
endif()

if(THEMIS_ENABLE_JEMALLOC)
    if(WIN32)
        message(WARNING "jemalloc is not supported on Windows. THEMIS_ENABLE_JEMALLOC will be ignored.")
        set(THEMIS_ENABLE_JEMALLOC OFF CACHE BOOL "jemalloc not supported on Windows" FORCE)
    else()
        message(STATUS "Enabling jemalloc allocator (best fragmentation resistance for RocksDB-heavy workloads)")
        if(THEMIS_ENABLE_MIMALLOC)
            message(WARNING "Both THEMIS_ENABLE_JEMALLOC and THEMIS_ENABLE_MIMALLOC are ON. jemalloc takes priority.")
            set(THEMIS_ENABLE_MIMALLOC OFF CACHE BOOL "Disabled: jemalloc selected" FORCE)
        endif()
        find_package(jemalloc CONFIG QUIET)
        if(NOT jemalloc_FOUND)
            # Try pkg-config fallback
            find_package(PkgConfig QUIET)
            if(PkgConfig_FOUND)
                pkg_check_modules(jemalloc QUIET jemalloc)
            endif()
        endif()
        if(NOT jemalloc_FOUND AND NOT jemalloc_LIBRARIES)
            message(WARNING "jemalloc not found via vcpkg or pkg-config. Install with: vcpkg install jemalloc OR apt install libjemalloc-dev")
            message(WARNING "Building without jemalloc. Performance optimization disabled.")
            set(THEMIS_ENABLE_JEMALLOC OFF CACHE BOOL "jemalloc not available" FORCE)
        else()
            message(STATUS "Found jemalloc")
            add_compile_definitions(THEMIS_ENABLE_JEMALLOC)
        endif()
    endif()
endif()


if(THEMIS_ENABLE_HUGE_PAGES)
    message(STATUS "Enabling huge pages optimization (+15-30% memory performance)")
    
    # Check platform support
    if(UNIX)
        message(STATUS "Huge pages: Linux/Unix support enabled")
        message(STATUS "Configure with: echo 1024 | sudo tee /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages")
        add_compile_definitions(THEMIS_USE_HUGE_PAGES)
    elseif(WIN32)
        message(STATUS "Huge pages: Windows large pages support enabled")
        message(STATUS "Configure via: secpol.msc -> Lock pages in memory")
        add_compile_definitions(THEMIS_USE_HUGE_PAGES)
    else()
        message(WARNING "Huge pages not supported on this platform")
        set(THEMIS_ENABLE_HUGE_PAGES OFF CACHE BOOL "Platform not supported" FORCE)
    endif()
endif()

if(THEMIS_ENABLE_RCU_INDEX)
    message(STATUS "Enabling RCU index optimization (+200-500% read performance)")
    message(STATUS "RCU: Read-Copy-Update for lock-free reads in read-heavy workloads")
    add_compile_definitions(THEMIS_USE_RCU_INDEX)
    
    # RCU requires C++17 or later (already set to C++20)
    # No additional dependencies needed
endif()

if(THEMIS_ENABLE_LIRS_CACHE)
    message(STATUS "Enabling LIRS cache optimization (+30-40% cache hit rate)")
    message(STATUS "LIRS: Low Inter-reference Recency Set for scan-resistant caching")
    add_compile_definitions(THEMIS_USE_LIRS_CACHE)
    
    # LIRS requires C++17 or later (already set to C++20)
    # No additional dependencies needed
endif()

# Static build configuration for QNAP/older systems
if(THEMIS_STATIC_BUILD)
    message(STATUS "Configuring static build for maximum portability")
    
    # Static linking flags
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static-libgcc -static-libstdc++")
        
        # For fully static builds, link everything statically
        # Note: This requires static versions of all libraries
        # set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
    endif()
    
    # Prefer static libraries on Linux
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
else()
    # Windows: Use dynamic linking (exe + DLLs)
    set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
endif()
option(THEMIS_ENABLE_ZLUDA "Enable AMD ZLUDA (CUDA compatibility on AMD)" OFF)
option(THEMIS_ENABLE_ROCM "Enable AMD ROCm acceleration" OFF)
option(THEMIS_ENABLE_DIRECTX "Enable DirectX 12 Compute Shaders (Windows)" OFF)
option(THEMIS_ENABLE_VULKAN "Enable Vulkan Compute acceleration" ON)
option(THEMIS_ENABLE_OPENGL "Enable OpenGL Compute Shaders" OFF)
option(THEMIS_ENABLE_METAL "Enable Apple Metal acceleration" OFF)
option(THEMIS_ENABLE_ONEAPI "Enable Intel OneAPI/SYCL acceleration" OFF)
option(THEMIS_ENABLE_OPENCL "Enable OpenCL acceleration" OFF)
option(THEMIS_ENABLE_WEBGPU "Enable WebGPU acceleration (experimental)" OFF)

if(THEMIS_ENABLE_OPENCL)
    find_package(OpenCL REQUIRED)
endif()

# Lokal aktivierbare Code-Qualit�t (clang-tidy) ohne CI-Kosten.
option(ENABLE_LOCAL_CLANG_TIDY "Enable local clang-tidy analysis (off by default)" OFF)


# v1.1.0 Build Variants (Q1 2026)
option(THEMIS_ENABLE_OLAP_VARIANT "Build with DuckDB for OLAP workloads" OFF)
option(THEMIS_EMBEDDED "Build embedded/lightweight variant (reduced dependencies)" OFF)
option(THEMIS_VLLM_COLOCATION "Optimize for vLLM co-location (AI/ML workloads)" OFF)

# Network Protocol Support Options
# Security: All protocols are opt-in (OFF by default) except HTTP/1.1 and protocols already in production
# Note: HTTP/1.1 is always enabled as the base protocol and cannot be disabled
option(THEMIS_ENABLE_HTTP2 "Enable HTTP/2 protocol support" OFF)
option(THEMIS_ENABLE_HTTP3 "Enable HTTP/3 (QUIC) protocol support" OFF)
option(THEMIS_ENABLE_GRPC "Enable gRPC protocol support (inter-shard communication)" ON)  # Already implemented in v1.3.0
option(THEMIS_ENABLE_WEBSOCKET "Enable WebSocket protocol support" OFF)
option(THEMIS_ENABLE_GRAPHQL "Enable GraphQL API support" ON)  # Already implemented
option(THEMIS_ENABLE_SSE "Enable Server-Sent Events (SSE) support" ON)  # Already implemented
option(THEMIS_ENABLE_MCP "Enable Model Context Protocol (MCP) support for LLM integration" OFF)
option(THEMIS_ENABLE_MQTT "Enable MQTT protocol support (IoT)" OFF)
option(THEMIS_ENABLE_POSTGRES_WIRE "Enable PostgreSQL Wire Protocol support" OFF)
option(THEMIS_ENABLE_KAFKA "Enable Kafka-compatible CDC producer (requires librdkafka)" OFF)
option(THEMIS_ENABLE_SERVICE_MESH "Enable Istio/Envoy service mesh sidecar proxy mode (xDS v3 REST client)" OFF)

set(THEMIS_SERVER_PROFILE "standard" CACHE STRING "Server activation profile: minimal, standard, enterprise")
set_property(CACHE THEMIS_SERVER_PROFILE PROPERTY STRINGS minimal standard enterprise)
string(TOLOWER "${THEMIS_SERVER_PROFILE}" _THEMIS_SERVER_PROFILE_NORMALIZED)
if(NOT _THEMIS_SERVER_PROFILE_NORMALIZED MATCHES "^(minimal|standard|enterprise)$")
    message(FATAL_ERROR "THEMIS_SERVER_PROFILE must be one of: minimal, standard, enterprise (got: ${THEMIS_SERVER_PROFILE})")
endif()
set(THEMIS_SERVER_PROFILE "${_THEMIS_SERVER_PROFILE_NORMALIZED}" CACHE STRING "Server activation profile: minimal, standard, enterprise" FORCE)
add_compile_definitions(THEMIS_SERVER_PROFILE_DEFAULT="${THEMIS_SERVER_PROFILE}")
message(STATUS "Server activation profile default: ${THEMIS_SERVER_PROFILE}")

# vLLM Co-Location: Automatically enable CUDA (Kernbestand, not Enterprise!)
if(THEMIS_VLLM_COLOCATION)
    set(THEMIS_ENABLE_CUDA ON CACHE BOOL "Force CUDA for vLLM co-location" FORCE)
    message(STATUS "vLLM Co-Location enabled: CUDA forced ON (Kernbestand)")
endif()

# Embedded variant: Disable heavy dependencies
if(THEMIS_EMBEDDED)
    set(THEMIS_ENABLE_TRACING OFF CACHE BOOL "Disable OpenTelemetry for embedded" FORCE)
    set(THEMIS_USE_MIMALLOC OFF CACHE BOOL "Disable mimalloc for embedded (minimal deps)" FORCE)
    message(STATUS "Embedded variant: Lightweight build (TBB/Arrow/OpenTelemetry/CUDA disabled)")
endif()

# Geo feature tiering options (Core vs. Enterprise add-ins)
option(THEMIS_GEO "Enable Geo core feature scaffolding" ON)
option(THEMIS_GEO_SIMD "Enable SIMD kernels for Geo (enterprise add-in)" OFF)
option(THEMIS_GEO_GPU "Enable GPU backend for Geo (enterprise add-in)" OFF)
option(THEMIS_GEO_H3 "Enable H3 integration (enterprise add-in)" OFF)
option(THEMIS_GEO_GEOS_PLUGIN "Enable GEOS prepared geometries plugin (enterprise add-in)" OFF)
option(THEMIS_ENTERPRISE "Enable enterprise capabilities bundle" OFF)

# Optional public submodule externalization for formerly integrated modules.
# When OFF (default), geo/timeseries remain compiled from the monorepo core sources.
option(THEMIS_EXTERNALIZE_GEO_PLUGIN "Build Geo from optional plugins/themisdb_geo submodule instead of integrated src/geo sources" OFF)
option(THEMIS_EXTERNALIZE_TIMESERIES_PLUGIN "Build TimeSeries from optional plugins/themisdb_timeseries submodule instead of integrated src/timeseries sources" OFF)
set(THEMIS_GEO_PLUGIN_SUBMODULE_DIR "${THEMIS_ROOT_DIR}/plugins/themisdb_geo" CACHE PATH "Path to optional public Geo plugin submodule")
set(THEMIS_TIMESERIES_PLUGIN_SUBMODULE_DIR "${THEMIS_ROOT_DIR}/plugins/themisdb_timeseries" CACHE PATH "Path to optional public TimeSeries plugin submodule")

set(THEMIS_USE_EXTERNAL_GEO_PLUGIN OFF)
if(THEMIS_EXTERNALIZE_GEO_PLUGIN)
    if(NOT THEMIS_PLUGIN_GEO)
        message(STATUS "Geo externalization requested but THEMIS_PLUGIN_GEO=OFF; integrated Geo sources stay active")
    elseif(EXISTS "${THEMIS_GEO_PLUGIN_SUBMODULE_DIR}/CMakeLists.txt")
        set(THEMIS_USE_EXTERNAL_GEO_PLUGIN ON)
        message(STATUS "Geo externalization enabled: using optional submodule at ${THEMIS_GEO_PLUGIN_SUBMODULE_DIR}")
    else()
        message(WARNING "Geo externalization requested, but optional submodule CMakeLists is missing: ${THEMIS_GEO_PLUGIN_SUBMODULE_DIR}/CMakeLists.txt")
    endif()
endif()

set(THEMIS_USE_EXTERNAL_TIMESERIES_PLUGIN OFF)
if(THEMIS_EXTERNALIZE_TIMESERIES_PLUGIN)
    if(NOT THEMIS_PLUGIN_TIMESERIES)
        message(STATUS "TimeSeries externalization requested but THEMIS_PLUGIN_TIMESERIES=OFF; integrated TimeSeries sources stay active")
    elseif(EXISTS "${THEMIS_TIMESERIES_PLUGIN_SUBMODULE_DIR}/CMakeLists.txt")
        set(THEMIS_USE_EXTERNAL_TIMESERIES_PLUGIN ON)
        message(STATUS "TimeSeries externalization enabled: using optional submodule at ${THEMIS_TIMESERIES_PLUGIN_SUBMODULE_DIR}")
    else()
        message(WARNING "TimeSeries externalization requested, but optional submodule CMakeLists is missing: ${THEMIS_TIMESERIES_PLUGIN_SUBMODULE_DIR}/CMakeLists.txt")
    endif()
endif()

# Multi-Edition Support (v1.3.5+)
# THEMIS_EDITION: One of MINIMAL, COMMUNITY, ENTERPRISE, or HYPERSCALER (compile-time selection)
# This setting determines feature availability, licensing limits, and hardware constraints
# MINIMAL: Core database only (embedded/IoT), COMMUNITY: Full features (default), ENTERPRISE: Scaling+HA, HYPERSCALER: Unlimited
set(THEMIS_EDITION "COMMUNITY" CACHE STRING "Edition: MINIMAL, COMMUNITY, ENTERPRISE, or HYPERSCALER")
set_property(CACHE THEMIS_EDITION PROPERTY STRINGS MINIMAL COMMUNITY ENTERPRISE HYPERSCALER)

# Validate edition selection
if(NOT THEMIS_EDITION MATCHES "^(MINIMAL|COMMUNITY|ENTERPRISE|HYPERSCALER)$")
    message(FATAL_ERROR "THEMIS_EDITION must be MINIMAL, COMMUNITY, ENTERPRISE, or HYPERSCALER (got: ${THEMIS_EDITION})")
endif()

# Edition feature configuration (compile-time feature flags)
# These determine which functionality is available at build time

# GPU Memory Limits: Edition-specific VRAM constraints
# MINIMAL: No GPU, no LLM, no sharding - only core database features
# COMMUNITY: 24 GB (typical gaming GPU, consumer hardware)
# ENTERPRISE: 256 GB (data center grade A100/H100)
# HYPERSCALER: Unlimited (custom, OEM deployments)
if(THEMIS_EDITION STREQUAL "MINIMAL")
    # MINIMAL Edition: Core database features only
    # No LLM, no GPU acceleration, no sharding, no replication, no advanced analytics
    # Smallest footprint, fastest build time, ideal for embedded/edge deployments
    set(THEMIS_GPU_MAX_VRAM_GB 0 CACHE INTERNAL "No GPU support in Minimal Edition")
    set(THEMIS_SHARDING_MAX_NODES 1 CACHE INTERNAL "Single-node only in Minimal Edition")
    set(THEMIS_ENABLE_ENTERPRISE_PLUGINS OFF CACHE INTERNAL "Enterprise plugins disabled in Minimal")
    set(THEMIS_ENABLE_MULTI_MASTER OFF CACHE INTERNAL "Multi-master replication disabled in Minimal")
    set(THEMIS_ENABLE_FIELD_ENCRYPTION OFF CACHE INTERNAL "Field-level encryption disabled in Minimal")
    set(THEMIS_ENABLE_RBAC OFF CACHE INTERNAL "RBAC disabled in Minimal")
    set(THEMIS_ENABLE_HSM OFF CACHE INTERNAL "HSM disabled in Minimal")
    set(THEMIS_ENABLE_LLM OFF CACHE INTERNAL "LLM disabled in Minimal")
    set(THEMIS_ENABLE_GPU OFF CACHE INTERNAL "GPU disabled in Minimal")
    set(THEMIS_ENABLE_CUDA OFF CACHE INTERNAL "CUDA disabled in Minimal")
    set(THEMIS_ENABLE_HIP OFF CACHE INTERNAL "HIP disabled in Minimal")
    set(THEMIS_ENABLE_VULKAN OFF CACHE INTERNAL "Vulkan disabled in Minimal")
    set(THEMIS_ENABLE_CONTENT_PROCESSORS OFF CACHE INTERNAL "Content processors disabled in Minimal")
    set(THEMIS_ENABLE_TRACING OFF CACHE INTERNAL "OpenTelemetry tracing disabled in Minimal")
    set(THEMIS_ENABLE_VOICE_ASSISTANT OFF CACHE INTERNAL "Voice assistant disabled in Minimal")
    set(THEMIS_ENABLE_WHISPER OFF CACHE INTERNAL "Whisper disabled in Minimal")
    set(THEMIS_ENABLE_PIPER_TTS OFF CACHE INTERNAL "Piper TTS disabled in Minimal")
    set(THEMIS_ENABLE_RNNOISE OFF CACHE INTERNAL "RNNoise disabled in Minimal")
    set(THEMIS_ENABLE_GRPC OFF CACHE INTERNAL "gRPC disabled in Minimal")
    set(THEMIS_ENABLE_HTTP2 OFF CACHE INTERNAL "HTTP/2 disabled in Minimal")
    set(THEMIS_ENABLE_HTTP3 OFF CACHE INTERNAL "HTTP/3 disabled in Minimal")
    set(THEMIS_ENABLE_WEBSOCKET OFF CACHE INTERNAL "WebSocket disabled in Minimal")
    set(THEMIS_ENABLE_MQTT OFF CACHE INTERNAL "MQTT disabled in Minimal")
    set(THEMIS_ENABLE_POSTGRES_WIRE OFF CACHE INTERNAL "PostgreSQL Wire disabled in Minimal")
    set(THEMIS_ENABLE_MCP OFF CACHE INTERNAL "MCP disabled in Minimal")
    set(THEMIS_BUILD_TESTS OFF CACHE INTERNAL "Tests disabled in Minimal for faster builds")
    set(THEMIS_BUILD_BENCHMARKS OFF CACHE INTERNAL "Benchmarks disabled in Minimal")
    message(STATUS "Edition: MINIMAL - Core database only, no GPU, no LLM, no sharding, no advanced features")
    message(STATUS "  Features: ACID transactions, multi-model storage, secondary indexes, basic queries")
    message(STATUS "  Protocols: HTTP/1.1, REST API only")
    message(STATUS "  Build time: ~5-10 minutes (vs ~30-40 for full build)")
    
elseif(THEMIS_EDITION STREQUAL "COMMUNITY")
    set(THEMIS_GPU_MAX_VRAM_GB 8 CACHE INTERNAL "Max GPU VRAM for Community Edition")
    set(THEMIS_SHARDING_MAX_NODES 5 CACHE INTERNAL "Max sharding nodes for Community Edition")
    set(THEMIS_ENABLE_ENTERPRISE_PLUGINS OFF CACHE INTERNAL "Enterprise plugins disabled in Community")
    set(THEMIS_ENABLE_MULTI_MASTER OFF CACHE INTERNAL "Multi-master replication disabled in Community")
    set(THEMIS_ENABLE_FIELD_ENCRYPTION OFF CACHE INTERNAL "Field-level encryption disabled in Community")
    set(THEMIS_ENABLE_RBAC OFF CACHE INTERNAL "RBAC disabled in Community")
    set(THEMIS_ENABLE_HSM OFF CACHE INTERNAL "HSM disabled in Community")
    
    # GPU acceleration: Vulkan as default (cross-platform, multi-vendor)
    # Vulkan works on NVIDIA, AMD, Intel GPUs across Windows/Linux/macOS/Android
    # CUDA is vendor-specific (NVIDIA only) and must be explicitly enabled
    if(NOT DEFINED THEMIS_ENABLE_VULKAN)
        set(THEMIS_ENABLE_VULKAN ON CACHE BOOL "Enable Vulkan GPU acceleration (default ON, cross-platform)")
    endif()
    
    # CUDA is OFF by default (NVIDIA-specific, use -DTHEMIS_ENABLE_CUDA=ON to enable)
    # When enabled alongside Vulkan, CUDA takes priority for NVIDIA GPUs
    if(NOT DEFINED THEMIS_ENABLE_CUDA)
        set(THEMIS_ENABLE_CUDA OFF CACHE BOOL "Enable CUDA GPU acceleration (default OFF, NVIDIA-specific)")
    endif()
    
    # LLM support enabled by default in Community Edition (v1.3.0+)
    if(NOT DEFINED THEMIS_ENABLE_LLM)
        set(THEMIS_ENABLE_LLM ON CACHE BOOL "Enable LLM support in Community Edition (default ON)")
    endif()
    
    message(STATUS "Edition: COMMUNITY - GPU limited to ${THEMIS_GPU_MAX_VRAM_GB}GB, up to ${THEMIS_SHARDING_MAX_NODES} nodes")
    message(STATUS "  GPU Backend: Vulkan=${THEMIS_ENABLE_VULKAN} (cross-platform), CUDA=${THEMIS_ENABLE_CUDA} (NVIDIA-only)")
    message(STATUS "  LLM Support: ${THEMIS_ENABLE_LLM}")
    if(THEMIS_ENABLE_VULKAN)
        message(STATUS "  Vulkan will provide GPU acceleration on NVIDIA/AMD/Intel GPUs with CPU fallback")
    endif()

elseif(THEMIS_EDITION STREQUAL "ENTERPRISE")
    set(THEMIS_GPU_MAX_VRAM_GB 256 CACHE INTERNAL "Max GPU VRAM for Enterprise Edition")
    set(THEMIS_SHARDING_MAX_NODES 100 CACHE INTERNAL "Max sharding nodes for Enterprise Edition")
    set(THEMIS_ENABLE_ENTERPRISE_PLUGINS ON CACHE INTERNAL "Enterprise plugins enabled")
    set(THEMIS_ENABLE_MULTI_MASTER ON CACHE INTERNAL "Multi-master replication enabled")
    set(THEMIS_ENABLE_FIELD_ENCRYPTION ON CACHE INTERNAL "Field-level encryption enabled")
    set(THEMIS_ENABLE_RBAC ON CACHE INTERNAL "RBAC enabled")
    set(THEMIS_ENABLE_HSM ON CACHE INTERNAL "HSM enabled")
    
    # GPU acceleration: Vulkan as default (cross-platform, multi-vendor)
    if(NOT DEFINED THEMIS_ENABLE_VULKAN)
        set(THEMIS_ENABLE_VULKAN ON CACHE BOOL "Enable Vulkan GPU acceleration (default ON, cross-platform)")
    endif()
    
    # CUDA OFF by default (enable with -DTHEMIS_ENABLE_CUDA=ON for NVIDIA-specific optimizations)
    if(NOT DEFINED THEMIS_ENABLE_CUDA)
        set(THEMIS_ENABLE_CUDA OFF CACHE BOOL "Enable CUDA GPU acceleration (default OFF, NVIDIA-specific)")
    endif()
    
    # LLM support enabled by default
    if(NOT DEFINED THEMIS_ENABLE_LLM)
        set(THEMIS_ENABLE_LLM ON CACHE BOOL "Enable LLM support in Enterprise Edition (default ON)")
    endif()
    
    message(STATUS "Edition: ENTERPRISE - GPU up to ${THEMIS_GPU_MAX_VRAM_GB}GB, up to ${THEMIS_SHARDING_MAX_NODES} nodes")
    message(STATUS "  GPU Backend: Vulkan=${THEMIS_ENABLE_VULKAN} (cross-platform), CUDA=${THEMIS_ENABLE_CUDA} (NVIDIA-only)")
    message(STATUS "  LLM Support: ${THEMIS_ENABLE_LLM}")

elseif(THEMIS_EDITION STREQUAL "HYPERSCALER")
    set(THEMIS_GPU_MAX_VRAM_GB 999999 CACHE INTERNAL "Max GPU VRAM for Hyperscaler Edition (unlimited)")
    set(THEMIS_SHARDING_MAX_NODES 999999 CACHE INTERNAL "Max sharding nodes for Hyperscaler Edition (unlimited)")
    set(THEMIS_ENABLE_ENTERPRISE_PLUGINS ON CACHE INTERNAL "Enterprise plugins enabled")
    set(THEMIS_ENABLE_MULTI_MASTER ON CACHE INTERNAL "Multi-master replication enabled")
    set(THEMIS_ENABLE_FIELD_ENCRYPTION ON CACHE INTERNAL "Field-level encryption enabled")
    set(THEMIS_ENABLE_RBAC ON CACHE INTERNAL "RBAC enabled")
    set(THEMIS_ENABLE_HSM ON CACHE INTERNAL "HSM enabled")
    
    # GPU acceleration: Vulkan as default (cross-platform, multi-vendor)
    if(NOT DEFINED THEMIS_ENABLE_VULKAN)
        set(THEMIS_ENABLE_VULKAN ON CACHE BOOL "Enable Vulkan GPU acceleration (default ON, cross-platform)")
    endif()
    
    # CUDA OFF by default (enable with -DTHEMIS_ENABLE_CUDA=ON for NVIDIA-specific optimizations)
    if(NOT DEFINED THEMIS_ENABLE_CUDA)
        set(THEMIS_ENABLE_CUDA OFF CACHE BOOL "Enable CUDA GPU acceleration (default OFF, NVIDIA-specific)")
    endif()
    
    # LLM support enabled by default
    if(NOT DEFINED THEMIS_ENABLE_LLM)
        set(THEMIS_ENABLE_LLM ON CACHE BOOL "Enable LLM support in Hyperscaler Edition (default ON)")
    endif()
    
    message(STATUS "Edition: HYPERSCALER - Unlimited GPU/VRAM and nodes")
    message(STATUS "  GPU Backend: Vulkan=${THEMIS_ENABLE_VULKAN} (cross-platform), CUDA=${THEMIS_ENABLE_CUDA} (NVIDIA-only)")
    message(STATUS "  LLM Support: ${THEMIS_ENABLE_LLM}")
    set(THEMIS_ENABLE_ENTERPRISE_PLUGINS ON CACHE INTERNAL "Enterprise plugins enabled")
    set(THEMIS_ENABLE_MULTI_MASTER ON CACHE INTERNAL "Multi-master replication enabled")
    set(THEMIS_ENABLE_FIELD_ENCRYPTION ON CACHE INTERNAL "Field-level encryption enabled")
    set(THEMIS_ENABLE_RBAC ON CACHE INTERNAL "RBAC enabled")
    set(THEMIS_ENABLE_HSM ON CACHE INTERNAL "HSM enabled")
    message(STATUS "Edition: HYPERSCALER - Unlimited GPU VRAM, unlimited nodes, full custom capabilities")

endif()

# ============================================================================
# LICENSE ENFORCEMENT (v1.4.0+)
# ============================================================================
# Enforce license requirements for ENTERPRISE and HYPERSCALER editions

# ENTERPRISE Edition: License REQUIRED for Release builds only
if(THEMIS_EDITION STREQUAL "ENTERPRISE")
    # Check CMAKE_BUILD_TYPE - if not set, default to Release for safety
    if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
    endif()
    
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        # Release builds: License is MANDATORY
        if(NOT THEMIS_LICENSE_FILE OR THEMIS_LICENSE_FILE STREQUAL "")
            message(FATAL_ERROR 
                "ENTERPRISE Edition Release builds require an embedded license file.\n"
                "Usage: cmake -B build -S . \\\n"
                "  -DTHEMIS_EDITION=ENTERPRISE \\\n"
                "  -DCMAKE_BUILD_TYPE=Release \\\n"
                "  -DTHEMIS_LICENSE_FILE=/path/to/license.json\n"
                "\n"
                "For development/testing, use Debug build:\n"
                "  cmake -B build -S . -DTHEMIS_EDITION=ENTERPRISE -DCMAKE_BUILD_TYPE=Debug\n")
        endif()
        message(STATUS "ENTERPRISE Edition (Release): License requirement ENFORCED")
    else()
        # Debug/Development builds: License is OPTIONAL
        message(STATUS "ENTERPRISE Edition (${CMAKE_BUILD_TYPE}): License requirement OPTIONAL (dev build)")
    endif()
endif()

# HYPERSCALER Edition: License ALWAYS MANDATORY (no exceptions)
if(THEMIS_EDITION STREQUAL "HYPERSCALER")
    if(NOT THEMIS_BUILD_CHANNEL STREQUAL "official")
        message(FATAL_ERROR
            "HYPERSCALER Edition requires official owner-signed builds.\n"
            "Set -DTHEMIS_BUILD_CHANNEL=official and provide a valid THEMIS_BUILD_SIG.")
    endif()

    if(NOT THEMIS_BUILD_SIG OR THEMIS_BUILD_SIG STREQUAL "")
        message(FATAL_ERROR
            "HYPERSCALER Edition requires THEMIS_BUILD_SIG.\n"
            "Only owner-issued signing flows are supported.")
    endif()

    if(NOT THEMIS_LICENSE_FILE OR THEMIS_LICENSE_FILE STREQUAL "")
        message(FATAL_ERROR 
            "HYPERSCALER Edition requires an embedded license file (mandatory for all builds).\n"
            "This edition is only available with volume licensing.\n"
            "\n"
            "Usage: cmake -B build -S . \\\n"
            "  -DTHEMIS_EDITION=HYPERSCALER \\\n"
            "  -DTHEMIS_LICENSE_FILE=/path/to/hyperscaler-license.json\n"
            "\n"
            "For licensing inquiries, contact: licensing@themisdb.io\n")
    endif()

    set(_themis_hyperscaler_private_root "${THEMIS_ROOT_DIR}/plugins/private")
    set(_themis_hyperscaler_required_modules
        "${_themis_hyperscaler_private_root}/themisdb-hyperscaler-core/CMakeLists.txt"
        "${_themis_hyperscaler_private_root}/themisdb_plugin_signer/README.md")
    foreach(_themis_hyperscaler_required_file IN LISTS _themis_hyperscaler_required_modules)
        if(NOT EXISTS "${_themis_hyperscaler_required_file}")
            message(FATAL_ERROR
                "HYPERSCALER Edition requires private owner-controlled modules.\n"
                "Missing required private artifact: ${_themis_hyperscaler_required_file}\n"
                "Initialize the private hyperscaler submodules before configuring.")
        endif()
    endforeach()

    message(STATUS "HYPERSCALER Edition: License requirement ENFORCED (mandatory)")
endif()

# Compile-time edition identifier for runtime checks and telemetry
add_compile_definitions(
    "THEMIS_EDITION_STRING=\"${THEMIS_EDITION}\""
    "THEMIS_SHARDING_MAX_NODES=${THEMIS_SHARDING_MAX_NODES}"
)

if(NOT THEMIS_GPU_LIMIT_COMPILE_DEF_DEFINED)
    add_compile_definitions("THEMIS_GPU_MAX_VRAM_GB=${THEMIS_GPU_MAX_VRAM_GB}")
    set(THEMIS_GPU_LIMIT_COMPILE_DEF_DEFINED ON)
endif()

# ============================================================================
# BUILD REPRODUCIBILITY – Git commit metadata (v1.7.0+)
# ============================================================================
# Capture the current HEAD commit, branch, dirty flag, and build environment
# so that any binary can be traced back to its exact source revision.

find_package(Git QUIET)

set(_THEMIS_GIT_COMMIT      "unknown")
set(_THEMIS_GIT_COMMIT_DATE "unknown")
set(_THEMIS_GIT_BRANCH      "unknown")
set(_THEMIS_GIT_DIRTY       0)

if(Git_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} log -1 --format=%H
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        OUTPUT_VARIABLE _THEMIS_GIT_COMMIT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    execute_process(
        COMMAND ${GIT_EXECUTABLE} log -1 --format=%ci
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        OUTPUT_VARIABLE _THEMIS_GIT_COMMIT_DATE
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        OUTPUT_VARIABLE _THEMIS_GIT_BRANCH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    execute_process(
        COMMAND ${GIT_EXECUTABLE} status --porcelain
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        OUTPUT_VARIABLE _git_dirty_output
            RESULT_VARIABLE _git_dirty_result
            TIMEOUT 20
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(_git_dirty_result EQUAL 0 AND _git_dirty_output)
        set(_THEMIS_GIT_DIRTY 1)
    endif()
endif()

# Build host / user (best-effort)
cmake_host_system_information(RESULT _THEMIS_BUILD_HOST QUERY HOSTNAME)
if(DEFINED ENV{USER})
    set(_THEMIS_BUILD_USER "$ENV{USER}")
elseif(DEFINED ENV{USERNAME})
    set(_THEMIS_BUILD_USER "$ENV{USERNAME}")
else()
    set(_THEMIS_BUILD_USER "unknown")
endif()

message(STATUS "Build reproducibility: commit=${_THEMIS_GIT_COMMIT} branch=${_THEMIS_GIT_BRANCH} dirty=${_THEMIS_GIT_DIRTY}")

add_compile_definitions(
    "THEMIS_GIT_COMMIT=\"${_THEMIS_GIT_COMMIT}\""
    "THEMIS_GIT_COMMIT_DATE=\"${_THEMIS_GIT_COMMIT_DATE}\""
    "THEMIS_GIT_BRANCH=\"${_THEMIS_GIT_BRANCH}\""
    "THEMIS_GIT_DIRTY=${_THEMIS_GIT_DIRTY}"
    "THEMIS_BUILD_HOST=\"${_THEMIS_BUILD_HOST}\""
    "THEMIS_BUILD_USER=\"${_THEMIS_BUILD_USER}\""
)


# Allows embedding company/license data into the binary at build time.
# This supports both offline deployments and CI/CD pipelines.
# 
# Default Behavior:
#   If no license file is specified, automatically uses the community license
#   from config/license_community_default.json
#
# Usage:
#   cmake -B build -S .  # Uses community license by default
#   cmake -B build -S . -DTHEMIS_LICENSE_FILE=path/to/license.json  # Custom license
#
# The license file should be a JSON file with the following structure:
# {
#   "organization_name": "Company Name",
#   "organization_id": "optional-org-id",
#   "contact_email": "contact@company.com",
#   "license_key": "THEMIS-XXX-XXXXXXXXXXXX",
#   "edition": "ENTERPRISE",
#   "issued_date": "2026-01-01",
#   "expiry_date": "2027-12-31",
#   "max_nodes": 100,
#   "max_cores": -1,
#   "max_storage_tb": -1,
#   "build_id": "optional-build-id",
#   "signature": "required-rsa-signature-for-hyperscaler"
# }

set(THEMIS_LICENSE_FILE "" CACHE FILEPATH "Path to license JSON file for embedding")

# Auto-detect default license file if none specified
if(NOT THEMIS_LICENSE_FILE OR THEMIS_LICENSE_FILE STREQUAL "")
    set(DEFAULT_LICENSE_FILE "${THEMIS_ROOT_DIR}/config/license_community_default.json")
    if(EXISTS "${DEFAULT_LICENSE_FILE}")
        set(THEMIS_LICENSE_FILE "${DEFAULT_LICENSE_FILE}")
        message(STATUS "No license file specified, using default community license: ${THEMIS_LICENSE_FILE}")
    endif()
endif()

# Default license values (empty/unset if no license file provided)
set(THEMIS_LICENSE_ORG_NAME "")
set(THEMIS_LICENSE_ORG_ID "")
set(THEMIS_LICENSE_CONTACT_EMAIL "")
set(THEMIS_LICENSE_KEY "")
set(THEMIS_LICENSE_EDITION "")
set(THEMIS_LICENSE_ISSUED_DATE "")
set(THEMIS_LICENSE_EXPIRY_DATE "")
set(THEMIS_LICENSE_MAX_NODES -1)
set(THEMIS_LICENSE_MAX_CORES -1)
set(THEMIS_LICENSE_MAX_STORAGE_TB -1)
set(THEMIS_LICENSE_BUILD_ID "")
set(THEMIS_LICENSE_BUILD_TIMESTAMP "")
set(THEMIS_LICENSE_SIGNATURE "")

# If license file is provided or auto-detected, read and parse it
if(THEMIS_LICENSE_FILE AND EXISTS "${THEMIS_LICENSE_FILE}")
    message(STATUS "Embedding license data from: ${THEMIS_LICENSE_FILE}")
    
    # Read the license file
    file(READ "${THEMIS_LICENSE_FILE}" LICENSE_JSON_CONTENT)
    
    # Parse JSON using CMake's built-in JSON support (CMake 3.19+)
    # Note: We need to extract values using string operations since CMake's JSON support is limited
    
    # Helper function to extract JSON string value
    function(extract_json_string VAR_NAME JSON_KEY)
        string(REGEX MATCH "\"${JSON_KEY}\"[ \t\n]*:[ \t\n]*\"([^\"]*)\"" _match "${LICENSE_JSON_CONTENT}")
        if(_match)
            set(${VAR_NAME} "${CMAKE_MATCH_1}" PARENT_SCOPE)
        endif()
    endfunction()
    
    # Helper function to extract JSON integer value
    function(extract_json_int VAR_NAME JSON_KEY)
        string(REGEX MATCH "\"${JSON_KEY}\"[ \t\n]*:[ \t\n]*(-?[0-9]+)" _match "${LICENSE_JSON_CONTENT}")
        if(_match)
            set(${VAR_NAME} "${CMAKE_MATCH_1}" PARENT_SCOPE)
        endif()
    endfunction()
    
    # Extract all license fields
    extract_json_string(THEMIS_LICENSE_ORG_NAME "organization_name")
    extract_json_string(THEMIS_LICENSE_ORG_ID "organization_id")
    extract_json_string(THEMIS_LICENSE_CONTACT_EMAIL "contact_email")
    extract_json_string(THEMIS_LICENSE_KEY "license_key")
    extract_json_string(THEMIS_LICENSE_EDITION "edition")
    extract_json_string(THEMIS_LICENSE_ISSUED_DATE "issued_date")
    extract_json_string(THEMIS_LICENSE_EXPIRY_DATE "expiry_date")
    extract_json_int(THEMIS_LICENSE_MAX_NODES "max_nodes")
    extract_json_int(THEMIS_LICENSE_MAX_CORES "max_cores")
    extract_json_int(THEMIS_LICENSE_MAX_STORAGE_TB "max_storage_tb")
    extract_json_string(THEMIS_LICENSE_BUILD_ID "build_id")
    extract_json_string(THEMIS_LICENSE_SIGNATURE "signature")
    
    # Set build timestamp to current time
    string(TIMESTAMP THEMIS_LICENSE_BUILD_TIMESTAMP "%Y-%m-%d %H:%M:%S UTC" UTC)
    
    # Display embedded license information
    message(STATUS "  Organization:     ${THEMIS_LICENSE_ORG_NAME}")
    message(STATUS "  License Key:      ${THEMIS_LICENSE_KEY}")
    message(STATUS "  Edition:          ${THEMIS_LICENSE_EDITION}")
    message(STATUS "  Expiry Date:      ${THEMIS_LICENSE_EXPIRY_DATE}")
    message(STATUS "  Max Nodes:        ${THEMIS_LICENSE_MAX_NODES}")
    message(STATUS "  Build Timestamp:  ${THEMIS_LICENSE_BUILD_TIMESTAMP}")
    
    # Validate that license edition matches build edition
    if(THEMIS_LICENSE_EDITION AND NOT THEMIS_LICENSE_EDITION STREQUAL THEMIS_EDITION)
        # For ENTERPRISE and HYPERSCALER, edition mismatch is a FATAL error
        if(THEMIS_EDITION STREQUAL "ENTERPRISE" OR THEMIS_EDITION STREQUAL "HYPERSCALER")
            message(FATAL_ERROR 
                "License edition mismatch: License is for '${THEMIS_LICENSE_EDITION}' but build is '${THEMIS_EDITION}'.\n"
                "You must use a license file that matches your build edition.\n"
                "License file: ${THEMIS_LICENSE_FILE}")
        else()
            # For MINIMAL/COMMUNITY, just warn
            message(WARNING "License edition (${THEMIS_LICENSE_EDITION}) does not match build edition (${THEMIS_EDITION})")
        endif()
    endif()

    if(THEMIS_EDITION STREQUAL "HYPERSCALER" AND
       (NOT THEMIS_LICENSE_SIGNATURE OR THEMIS_LICENSE_SIGNATURE STREQUAL ""))
        message(FATAL_ERROR
            "HYPERSCALER Edition requires a signed license (signature field must be present). "
            "Unsigned licenses are rejected.")
    endif()
else()
    if(THEMIS_LICENSE_FILE)
        message(WARNING "License file specified but not found: ${THEMIS_LICENSE_FILE}")
    endif()
    message(STATUS "No license data will be embedded in this build")
endif()

# Add license data as compile definitions
add_compile_definitions(
    "THEMIS_LICENSE_ORG_NAME=\"${THEMIS_LICENSE_ORG_NAME}\""
    "THEMIS_LICENSE_ORG_ID=\"${THEMIS_LICENSE_ORG_ID}\""
    "THEMIS_LICENSE_CONTACT_EMAIL=\"${THEMIS_LICENSE_CONTACT_EMAIL}\""
    "THEMIS_LICENSE_KEY=\"${THEMIS_LICENSE_KEY}\""
    "THEMIS_LICENSE_EDITION=\"${THEMIS_LICENSE_EDITION}\""
    "THEMIS_LICENSE_ISSUED_DATE=\"${THEMIS_LICENSE_ISSUED_DATE}\""
    "THEMIS_LICENSE_EXPIRY_DATE=\"${THEMIS_LICENSE_EXPIRY_DATE}\""
    "THEMIS_LICENSE_MAX_NODES=${THEMIS_LICENSE_MAX_NODES}"
    "THEMIS_LICENSE_MAX_CORES=${THEMIS_LICENSE_MAX_CORES}"
    "THEMIS_LICENSE_MAX_STORAGE_TB=${THEMIS_LICENSE_MAX_STORAGE_TB}"
    "THEMIS_LICENSE_BUILD_ID=\"${THEMIS_LICENSE_BUILD_ID}\""
    "THEMIS_LICENSE_BUILD_TIMESTAMP=\"${THEMIS_LICENSE_BUILD_TIMESTAMP}\""
    "THEMIS_LICENSE_SIGNATURE=\"${THEMIS_LICENSE_SIGNATURE}\""
)

# MSVC: sanitize any accidentally injected '/WX:NO' options (invalid for cl.exe)
if(MSVC)
    get_property(_allTargets DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
    foreach(_t IN LISTS _allTargets)
        foreach(_prop IN ITEMS COMPILE_OPTIONS INTERFACE_COMPILE_OPTIONS)
            get_target_property(_opts ${_t} ${_prop})
            if(_opts)
                list(TRANSFORM _opts REPLACE "/WX:NO" "")
                list(TRANSFORM _opts REPLACE "/WX:No" "")
                list(TRANSFORM _opts REPLACE "/WX:0"  "")
                set_target_properties(${_t} PROPERTIES ${_prop} "${_opts}")
            endif()
        endforeach()
    endforeach()
endif()

# Note: Architecture detection and optimization moved to cmake/platforms/
# See cmake/platforms/PlatformDetection.cmake and ArchitectureOptimizations.cmake

# Compiler warnings and optimizations
if(MSVC)
    # Replace /Zi (shared PDB) with /Z7 (embedded debug info in .obj) to eliminate
    # PDB write contention (C1041) in parallel Ninja builds.
    foreach(_lang C CXX)
        foreach(_cfg DEBUG RELWITHDEBINFO)
            string(REPLACE "/Zi" "/Z7"
                CMAKE_${_lang}_FLAGS_${_cfg} "${CMAKE_${_lang}_FLAGS_${_cfg}}")
            set(CMAKE_${_lang}_FLAGS_${_cfg} "${CMAKE_${_lang}_FLAGS_${_cfg}}"
                CACHE STRING "" FORCE)
        endforeach()
    endforeach()
    add_compile_options(/W4 /utf-8 /wd4701 /bigobj)
    # /FS removed: /Z7 renders it unnecessary (no shared PDB file at compile time)
    # Suppress unreachable code warnings from external headers (e.g., nlohmann/json)
    add_compile_options(/wd4702)
    # Suppress AddressSanitizer warnings treated as errors (C5072)
    add_compile_options(/wd5072)
    # Suppress OpenSSL 3.0 deprecation warnings globally (C4996)
    add_compile_options(/wd4996)
    if(THEMIS_STRICT_BUILD)
        add_compile_options(/WX)
    endif()
    add_compile_definitions(NOMINMAX _CRT_SECURE_NO_WARNINGS _WIN32_WINNT=0x0A00)
    # Apply Release-specific settings for single- and multi-config generators
    add_compile_options($<$<CONFIG:Release>:/O2>)
    # Disable iterator debugging in Release to match vcpkg RocksDB
    add_compile_definitions($<$<CONFIG:Release>:_ITERATOR_DEBUG_LEVEL=0>)
    # Note: AVX2 optimization moved to cmake/platforms/ArchitectureOptimizations.cmake
    # Disable LTCG to avoid file locking issues on Windows with Ninja
    # Globally disable interprocedural optimization (LTCG/IPO) to prevent /GL flag conflicts
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION FALSE)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE FALSE)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO FALSE)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_MINSIZEREL FALSE)
    # Disable /GL (IPO) globally in Release for all targets to prevent compiler conflicts
    add_compile_options($<$<CONFIG:Release>:/GL->)
    add_compile_options($<$<CONFIG:RelWithDebInfo>:/GL->)
    add_compile_options($<$<CONFIG:MinSizeRel>:/GL->)
    add_link_options($<$<CONFIG:Release>:/INCREMENTAL:NO>)
    add_link_options($<$<CONFIG:Release>:/LTCG:OFF>)
    add_link_options($<$<CONFIG:RelWithDebInfo>:/LTCG:OFF>)
    add_link_options($<$<CONFIG:MinSizeRel>:/LTCG:OFF>)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
    # Note: -Werror is applied at target level via themis_apply_strict_build_flags()
    # not globally via add_compile_options(), to avoid external dependency build failures
    
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(-O3)
        # Note: Architecture-specific optimizations moved to cmake/platforms/ArchitectureOptimizations.cmake
    endif()
endif()

# AddressSanitizer support
if(THEMIS_ENABLE_ASAN)
    if(MSVC)
        add_compile_options(/fsanitize=address)
    else()
        add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address)
    endif()
endif()

# LeakSanitizer support
if(THEMIS_ENABLE_LSAN)
    if(NOT MSVC)
        add_compile_options(-fsanitize=leak -fno-omit-frame-pointer)
        add_link_options(-fsanitize=leak)
    else()
        message(WARNING "LeakSanitizer not supported on MSVC")
    endif()
endif()

# Optionale lokale clang-tidy Aktivierung (nicht in CI standardm��ig).
if(ENABLE_LOCAL_CLANG_TIDY)
    find_program(CLANG_TIDY_EXE NAMES clang-tidy)
    if(CLANG_TIDY_EXE)
        message(STATUS "clang-tidy aktiviert: ${CLANG_TIDY_EXE}")
        # Keine automatischen Fixes hier; Entwickler nutzen Skript f�r -fix.
        set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE}")
    else()
        message(WARNING "ENABLE_LOCAL_CLANG_TIDY=ON aber clang-tidy nicht gefunden.")
    endif()
endif()

# CMake helper function to safely link required targets
# This function checks if a target exists before attempting to link it
# and provides a clear error message if the target is missing.
# Usage: link_required(target_name <PRIVATE|PUBLIC|INTERFACE> dependency_target [additional_targets...])
# Example: link_required(themis_core PRIVATE SomePackage::component)
# Note: This helper is provided for future use. Current code uses explicit target
# checks before target_link_libraries to maintain minimal changes to existing code.
function(link_required target_name visibility dependency_target)
    if(NOT TARGET ${dependency_target})
        message(FATAL_ERROR "Required CMake target '${dependency_target}' not found for '${target_name}'. "
                            "Ensure the corresponding package was found successfully and the target is available.")
    endif()
    # Check additional targets in ARGN if any
    foreach(extra_target IN LISTS ARGN)
        if(NOT TARGET ${extra_target})
            message(FATAL_ERROR "Required CMake target '${extra_target}' not found for '${target_name}'. "
                                "Ensure the corresponding package was found successfully and the target is available.")
        endif()
    endforeach()
    target_link_libraries(${target_name} ${visibility} ${dependency_target} ${ARGN})
endfunction()

# Find required packages
# RocksDB requires zstd::zstd target, so we need to ensure it exists before finding RocksDB
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(ZSTD_PKG QUIET libzstd)
    if(ZSTD_PKG_FOUND AND NOT TARGET zstd::zstd)
        add_library(zstd::zstd INTERFACE IMPORTED)
        set_target_properties(zstd::zstd PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${ZSTD_PKG_INCLUDE_DIRS}"
            INTERFACE_LINK_LIBRARIES "${ZSTD_PKG_LIBRARIES}")
        message(STATUS "Created zstd::zstd target from pkg-config")
    endif()
endif()

find_package(RocksDB CONFIG)

# Select appropriate RocksDB target based on build type
# Note: vcpkg's rocksdb-shared only exports the C API, so linking the C++ wrapper
# requires the static library even when themis_core is built as a DLL.
set(THEMIS_ROCKSDB_TARGET "")
if(TARGET RocksDB::rocksdb)
    set(THEMIS_ROCKSDB_TARGET "RocksDB::rocksdb")
    message(STATUS "RocksDB: Using static library (rocksdb.lib) to satisfy C++ API usage")
elseif(TARGET RocksDB::rocksdb-shared)
    set(THEMIS_ROCKSDB_TARGET "RocksDB::rocksdb-shared")
    message(STATUS "RocksDB: Using shared C API library (rocksdb-shared.dll)")
endif()

# Fallback: locate system RocksDB if vcpkg didn't provide it
if(NOT TARGET RocksDB::rocksdb AND NOT TARGET RocksDB::rocksdb-shared)
    find_library(ROCKSDB_LIB NAMES rocksdb)
    find_path(ROCKSDB_INCLUDE_DIR rocksdb/db.h)
    if(ROCKSDB_LIB AND ROCKSDB_INCLUDE_DIR)
        add_library(RocksDB::rocksdb SHARED IMPORTED)
        set_target_properties(RocksDB::rocksdb PROPERTIES
            IMPORTED_LOCATION "${ROCKSDB_LIB}"
            INTERFACE_INCLUDE_DIRECTORIES "${ROCKSDB_INCLUDE_DIR}")
        set(THEMIS_ROCKSDB_TARGET "RocksDB::rocksdb")
        message(STATUS "Using system RocksDB library at ${ROCKSDB_LIB} and include ${ROCKSDB_INCLUDE_DIR}")
    else()
        message(FATAL_ERROR "RocksDB not found. Install via vcpkg or system package (librocksdb-dev).")
    endif()
endif()

# Core packages via vcpkg (or system where available)
find_package(simdjson CONFIG QUIET)
if(NOT simdjson_FOUND)
    message(WARNING "simdjson not found - some features may be disabled")
endif()

find_package(TBB CONFIG QUIET)
if(NOT TBB_FOUND)
    message(WARNING "TBB not found - will use fallback threading")
endif()

find_package(Arrow CONFIG QUIET)

# fmt: Try CONFIG first, fall back to MODULE
find_package(fmt CONFIG QUIET)
if(NOT fmt_FOUND)
    find_package(fmt MODULE QUIET)
endif()
if(fmt_FOUND)
    message(STATUS "fmt found")
else()
    if(THEMIS_ALLOW_MISSING_ROCKSDB)
        message(WARNING
            "fmt not found. Continuing configure because THEMIS_ALLOW_MISSING_ROCKSDB=ON, "
            "but fmt-dependent targets will still require fmt before building."
        )
        if(NOT TARGET fmt::fmt)
            add_library(fmt::fmt INTERFACE IMPORTED)
        endif()
    else()
        message(FATAL_ERROR 
            "fmt not found. Install via:\n"
            "  - vcpkg: vcpkg install fmt\n"
            "  - Debian/Ubuntu: sudo apt-get install libfmt-dev\n"
            "  - Fedora/RHEL: sudo dnf install fmt-devel\n"
            "  - macOS: brew install fmt"
        )
    endif()
endif()

# spdlog: Try CONFIG first, fall back to MODULE
find_package(spdlog CONFIG QUIET)
if(NOT spdlog_FOUND)
    find_package(spdlog MODULE QUIET)
endif()
if(spdlog_FOUND)
    message(STATUS "spdlog found")
else()
    if(THEMIS_ALLOW_MISSING_ROCKSDB)
        message(WARNING
            "spdlog not found. Continuing configure because THEMIS_ALLOW_MISSING_ROCKSDB=ON, "
            "but spdlog-dependent targets will still require spdlog before building."
        )
        if(NOT TARGET spdlog::spdlog)
            add_library(spdlog::spdlog INTERFACE IMPORTED)
        endif()
    else()
        message(FATAL_ERROR 
            "spdlog not found. Install via:\n"
            "  - vcpkg: vcpkg install spdlog\n"
            "  - Debian/Ubuntu: sudo apt-get install libspdlog-dev\n"
            "  - Fedora/RHEL: sudo dnf install spdlog-devel\n"
            "  - macOS: brew install spdlog"
        )
    endif()
endif()

# nlohmann_json: Try CONFIG first, fall back to MODULE
find_package(nlohmann_json CONFIG QUIET)
if(NOT nlohmann_json_FOUND)
    find_package(nlohmann_json MODULE QUIET)
endif()
if(nlohmann_json_FOUND)
    message(STATUS "nlohmann_json found")
else()
    if(EXISTS "${THEMIS_ROOT_DIR}/include/nlohmann/json.hpp")
        message(WARNING
            "nlohmann_json not found; using local fallback header at include/nlohmann/json.hpp"
        )
        if(NOT TARGET nlohmann_json::nlohmann_json)
            add_library(nlohmann_json::nlohmann_json INTERFACE IMPORTED)
            set_target_properties(nlohmann_json::nlohmann_json PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${THEMIS_ROOT_DIR}/include"
            )
        endif()
    else()
        message(FATAL_ERROR 
            "nlohmann_json not found. Install via:\n"
            "  - vcpkg: vcpkg install nlohmann-json\n"
            "  - Debian/Ubuntu: sudo apt-get install nlohmann-json3-dev\n"
            "  - Fedora/RHEL: sudo dnf install nlohmann-json-devel\n"
            "  - macOS: brew install nlohmann-json"
        )
    endif()
endif()

find_package(Boost QUIET COMPONENTS system)
if(NOT Boost_FOUND)
    message(WARNING "Boost not found - some features may be disabled")
endif()

# yaml-cpp might be found via PkgConfig or CONFIG
find_package(yaml-cpp CONFIG QUIET)
if(yaml-cpp_FOUND)
    message(STATUS "yaml-cpp found via CONFIG")
    if(NOT TARGET yaml-cpp::yaml-cpp AND TARGET yaml-cpp)
        # Some versions export as 'yaml-cpp' instead of 'yaml-cpp::yaml-cpp'
        add_library(yaml-cpp::yaml-cpp ALIAS yaml-cpp)
    endif()
else()
    # Try pkg-config fallback
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(YAMLCPP QUIET yaml-cpp)
        if(YAMLCPP_FOUND)
            message(STATUS "yaml-cpp found via PkgConfig")
            add_library(yaml-cpp::yaml-cpp INTERFACE IMPORTED)
            set_target_properties(yaml-cpp::yaml-cpp PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${YAMLCPP_INCLUDE_DIRS}"
                INTERFACE_LINK_LIBRARIES "${YAMLCPP_LIBRARIES}")
        else()
            message(WARNING "yaml-cpp not found - some configuration features may be limited")
        endif()
    else()
        message(WARNING "yaml-cpp not found - some configuration features may be limited")
    endif()
endif()

if(NOT TARGET yaml-cpp::yaml-cpp)
    message(STATUS "yaml-cpp optional dependency not available - continuing without it")
endif()

# Define optional YAML target for linking
set(THEMIS_YAML_TARGET "")
if(TARGET yaml-cpp::yaml-cpp)
    set(THEMIS_YAML_TARGET yaml-cpp::yaml-cpp)
endif()

# Defensive check: Ensure Boost::system target exists after find_package
if(Boost_FOUND)
    if(NOT TARGET Boost::system)
        message(FATAL_ERROR "Required CMake target 'Boost::system' not found. "
                            "Ensure Boost was found and the 'system' component is available.")
    endif()
else()
    message(WARNING "Boost not available - network features may be disabled")
endif()
# Optional ZSTD for content compression wrappers
# Note: ZSTD packages may expose different target names depending on the build system
# This defensive logic checks for multiple possible target names
find_package(zstd CONFIG)
if(zstd_FOUND)
    set(THEMIS_ZSTD_TARGET zstd::libzstd_shared)
    if(NOT TARGET zstd::libzstd_shared AND TARGET zstd::libzstd)
        set(THEMIS_ZSTD_TARGET zstd::libzstd)
    endif()
elseif(NOT zstd_FOUND)
    find_package(ZSTD CONFIG)
    if(ZSTD_FOUND)
        set(THEMIS_ZSTD_TARGET ZSTD::ZSTD)
    endif()
endif()

# Optional LZ4 for connection-level frame compression
find_package(lz4 CONFIG QUIET)
if(lz4_FOUND)
    if(TARGET LZ4::lz4_shared)
        set(THEMIS_LZ4_TARGET LZ4::lz4_shared)
    elseif(TARGET LZ4::lz4_static)
        set(THEMIS_LZ4_TARGET LZ4::lz4_static)
    elseif(TARGET lz4::lz4)
        set(THEMIS_LZ4_TARGET lz4::lz4)
    endif()
    message(STATUS "LZ4 found via CONFIG")
else()
    # Fallback: pkg-config
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(LZ4_PKG QUIET liblz4)
        if(LZ4_PKG_FOUND AND NOT TARGET LZ4::lz4_shared)
            add_library(LZ4::lz4_shared INTERFACE IMPORTED)
            set_target_properties(LZ4::lz4_shared PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${LZ4_PKG_INCLUDE_DIRS}"
                INTERFACE_LINK_LIBRARIES "${LZ4_PKG_LIBRARIES}")
            set(THEMIS_LZ4_TARGET LZ4::lz4_shared)
            message(STATUS "LZ4 found via pkg-config")
        endif()
    endif()
endif()

# Snappy compression – required for Prometheus remote-write endpoint compatibility.
# Snappy is a transitive dependency of RocksDB (vcpkg feature "snappy"), so it is
# typically available automatically.  We discover it here so we can link it
# explicitly against themis_core and surface a clear warning when absent.
find_package(Snappy CONFIG QUIET)
if(Snappy_FOUND OR TARGET Snappy::snappy)
    set(THEMIS_SNAPPY_TARGET Snappy::snappy)
    message(STATUS "Snappy found – Prometheus remote-write snappy decompression enabled")
else()
    # Fallback: system library via pkg-config
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(SNAPPY_PKG QUIET snappy)
        if(SNAPPY_PKG_FOUND AND NOT TARGET Snappy::snappy)
            add_library(Snappy::snappy INTERFACE IMPORTED)
            set_target_properties(Snappy::snappy PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${SNAPPY_PKG_INCLUDE_DIRS}"
                INTERFACE_LINK_LIBRARIES "${SNAPPY_PKG_LIBRARIES}")
            set(THEMIS_SNAPPY_TARGET Snappy::snappy)
            message(STATUS "Snappy found via pkg-config")
        endif()
    endif()
    if(NOT THEMIS_SNAPPY_TARGET)
        message(WARNING "Snappy not found – Prometheus remote-write snappy decompression will be unavailable. Install with: vcpkg install snappy")
    endif()
endif()

# Windows/MSVC hardening: some discovered lz4/snappy targets resolve to bare
# linker names (lz4.lib/snappy.lib) without a matching LIBPATH in focused test
# targets, which can trigger sporadic LNK1181. Prefer absolute vcpkg paths when
# available.
if(WIN32 AND DEFINED VCPKG_TARGET_TRIPLET AND NOT "${VCPKG_TARGET_TRIPLET}" STREQUAL "")
    set(_themis_vcpkg_lib_dir "${THEMIS_ROOT_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/lib")

    if(EXISTS "${_themis_vcpkg_lib_dir}/lz4.lib")
        set(THEMIS_LZ4_TARGET "${_themis_vcpkg_lib_dir}/lz4.lib")
    endif()

    if(EXISTS "${_themis_vcpkg_lib_dir}/snappy.lib")
        set(THEMIS_SNAPPY_TARGET "${_themis_vcpkg_lib_dir}/snappy.lib")
    endif()
endif()

# Optional: HNSW (if not using Faiss)
find_package(hnswlib CONFIG)

# Optional: Faiss for GPU support (graceful degradation if not found)
if(THEMIS_ENABLE_GPU)
    # Hint CMake's FindBLAS towards OpenBLAS on Windows to avoid vendor auto-detection issues
    set(BLA_VENDOR OpenBLAS CACHE STRING "Preferred BLAS vendor for Windows" FORCE)

    # Try to locate OpenBLAS/LAPACK provided by vcpkg (CONFIG mode)
    find_package(OpenBLAS CONFIG QUIET)
    if(OpenBLAS_FOUND)
        message(STATUS "OpenBLAS found - BLAS support enabled")
    else()
        message(WARNING "OpenBLAS not found - install with: vcpkg install openblas")
    endif()

    find_package(LAPACK CONFIG QUIET)
    if(LAPACK_FOUND)
        message(STATUS "LAPACK gefunden")
    endif()

    # FAISS (CONFIG mode via vcpkg). If missing, gracefully disable GPU.
    find_package(faiss CONFIG)
    if(NOT faiss_FOUND)
        message(WARNING "Faiss not found - GPU support will be automatically disabled.")
        set(THEMIS_ENABLE_GPU OFF CACHE BOOL "Enable GPU acceleration for vector search" FORCE)
    endif()
endif()

# Optional: OpenTelemetry for distributed tracing
if(THEMIS_ENABLE_TRACING)
    find_package(opentelemetry-cpp CONFIG REQUIRED)
endif()

# v1.1.0: mimalloc integration (20-40% memory boost, drop-in replacement)
if(THEMIS_USE_MIMALLOC)
    find_package(mimalloc CONFIG)
    if(mimalloc_FOUND)
        message(STATUS "mimalloc found - enabling for 20-40% memory boost")
    else()
        message(WARNING "mimalloc not found - falling back to system allocator. Install with: vcpkg install mimalloc")
        set(THEMIS_USE_MIMALLOC OFF CACHE BOOL "mimalloc not available" FORCE)
    endif()
endif()

# HTTP/2 Protocol Support
if(THEMIS_ENABLE_HTTP2)
    find_package(nghttp2 CONFIG)
    if(nghttp2_FOUND)
        message(STATUS "nghttp2 found - HTTP/2 protocol support enabled")
    else()
        message(WARNING "nghttp2 not found - disabling HTTP/2 support. Install with: vcpkg install nghttp2")
        set(THEMIS_ENABLE_HTTP2 OFF CACHE BOOL "nghttp2 not available" FORCE)
    endif()
endif()

# HTTP/3 Protocol Support (requires both nghttp3 and ngtcp2)
if(THEMIS_ENABLE_HTTP3)
    find_package(nghttp3 CONFIG)
    find_package(ngtcp2 CONFIG)
    if(nghttp3_FOUND AND ngtcp2_FOUND)
        message(STATUS "nghttp3 and ngtcp2 found - HTTP/3 (QUIC) protocol support enabled")
    else()
        if(NOT nghttp3_FOUND)
            message(WARNING "nghttp3 not found - Install with: vcpkg install nghttp3")
        endif()
        if(NOT ngtcp2_FOUND)
            message(WARNING "ngtcp2 not found - Install with: vcpkg install ngtcp2[openssl]")
        endif()
        message(WARNING "Disabling HTTP/3 support due to missing dependencies")
        set(THEMIS_ENABLE_HTTP3 OFF CACHE BOOL "nghttp3/ngtcp2 not available" FORCE)
    endif()
endif()

# ═══════════════════════════════════════════════════════════
# GDAL Integration (P2 - v1.4.0+)
# ═══════════════════════════════════════════════════════════
if(THEMIS_ENABLE_GDAL)
    message(STATUS "GDAL support enabled - integrating GDAL for geospatial data processing")
    
    # Find GDAL package
    find_package(GDAL CONFIG QUIET)
    if(GDAL_FOUND)
        message(STATUS "GDAL found via CONFIG: ${GDAL_VERSION}")
    else()
        # Fallback to MODULE mode
        find_package(GDAL MODULE)
        if(GDAL_FOUND)
            message(STATUS "GDAL found via MODULE: ${GDAL_VERSION}")
            # Create imported target if not already created
            if(NOT TARGET GDAL::GDAL)
                add_library(GDAL::GDAL UNKNOWN IMPORTED)
                set_target_properties(GDAL::GDAL PROPERTIES
                    IMPORTED_LOCATION "${GDAL_LIBRARY}"
                    INTERFACE_INCLUDE_DIRECTORIES "${GDAL_INCLUDE_DIR}")
            endif()
        else()
            message(WARNING "GDAL not found - Install with: vcpkg install gdal")
            message(WARNING "Disabling GDAL support due to missing dependency")
            set(THEMIS_ENABLE_GDAL OFF CACHE BOOL "GDAL not available" FORCE)
        endif()
    endif()
    
    if(GDAL_FOUND)
        message(STATUS "  GDAL features: Shapefile, GeoTIFF, GeoPackage support enabled")
    endif()
endif()

# Include directories
include_directories(
    ${THEMIS_ROOT_DIR}/include
    ${THEMIS_ROOT_DIR}/src
)

# Copy runtime config files into the build directory so tests and local runs
# can find `config/*` when executed from ${CMAKE_BINARY_DIR} (common when
# running tests via CTest/gtest_discover_tests). This is safe and cheap.
if(EXISTS "${THEMIS_ROOT_DIR}/config")
    file(COPY "${THEMIS_ROOT_DIR}/config" DESTINATION "${CMAKE_BINARY_DIR}")
endif()

# -----------------------------------------------------------
# llama.cpp Integration (v1.3.0+)
# -----------------------------------------------------------
if(THEMIS_ENABLE_LLM)
    message(STATUS "LLM support enabled - integrating llama.cpp")

    # Prefer local static build of llama.cpp to avoid shared DLL issues
    option(THEMIS_FORCE_LLAMA_SUBDIR "Prefer local static llama.cpp even if a target exists" OFF)

    # Check if llama was already found via find_package (e.g. from installed version)
    if(TARGET llama AND NOT THEMIS_FORCE_LLAMA_SUBDIR)
        message(STATUS "llama.cpp already available as target (from find_package) - skipping add_subdirectory")
    else()
        # Prefer local, non-committed root clone at ${THEMIS_ROOT_DIR}/llama.cpp
        # Fallback to legacy path external/llama.cpp if present
        set(LLAMA_SRC_DIR "${THEMIS_ROOT_DIR}/llama.cpp")
        if(NOT EXISTS "${LLAMA_SRC_DIR}/CMakeLists.txt")
            if(EXISTS "${THEMIS_ROOT_DIR}/external/llama.cpp/CMakeLists.txt")
                set(LLAMA_SRC_DIR "${THEMIS_ROOT_DIR}/external/llama.cpp")
                message(WARNING "Using legacy llama.cpp at external/llama.cpp. Prefer root llama.cpp/ (ignored by Git/Docker).")
            else()
                message(FATAL_ERROR
                    "llama.cpp source not found!\n"
                    "Clone locally (ignored by Git/Docker):\n"
                    "  git clone https://github.com/ggerganov/llama.cpp.git llama.cpp\n"
                    "Or run: bash scripts/setup-llamacpp.sh")
            endif()
        endif()

        # Configure llama.cpp build options
        set(LLAMA_BUILD_TESTS OFF CACHE BOOL "Disable llama.cpp tests" FORCE)
        set(LLAMA_BUILD_EXAMPLES OFF CACHE BOOL "Disable llama.cpp examples" FORCE)
        set(LLAMA_BUILD_SERVER OFF CACHE BOOL "Disable llama.cpp server" FORCE)
        set(LLAMA_STATIC ON CACHE BOOL "Build llama.cpp as static library" FORCE)
    
    # GPU acceleration options
    if(THEMIS_ENABLE_CUDA)
        message(STATUS "  Enabling CUDA support in llama.cpp")
        set(LLAMA_CUDA ON CACHE BOOL "Enable CUDA in llama.cpp" FORCE)
        set(LLAMA_CUDA_F16 ON CACHE BOOL "Enable FP16 in CUDA" FORCE)
    endif()
    
    if(THEMIS_ENABLE_METAL)
        message(STATUS "  Enabling Metal support in llama.cpp")
        set(LLAMA_METAL ON CACHE BOOL "Enable Metal in llama.cpp" FORCE)
    endif()
    
    if(THEMIS_ENABLE_VULKAN)
        message(STATUS "  Enabling Vulkan support in llama.cpp")
        set(LLAMA_VULKAN ON CACHE BOOL "Enable Vulkan in llama.cpp" FORCE)
    endif()
    
    if(THEMIS_ENABLE_HIP OR THEMIS_ENABLE_ROCM)
        message(STATUS "  Enabling HIP/ROCm support in llama.cpp")
        set(LLAMA_HIPBLAS ON CACHE BOOL "Enable hipBLAS in llama.cpp" FORCE)
    endif()
    
    # Add llama.cpp subdirectory (resolved via LLAMA_SRC_DIR)
    # When LLAMA_SRC_DIR is outside the current source dir (cmake/), CMake requires an explicit binary dir.
    # Guard against double-add since Dependencies.cmake may have already added it
    if(NOT TARGET llama)
        # Use flat directory structure to avoid deep nesting (Windows MAX_PATH issue)
        # Instead of: ${CMAKE_BINARY_DIR}/llama_cpp_build/ggml/src/ggml-vulkan/vulkan-shaders-gen-prefix/src/...
        # Use: _exts/llama_cpp/
        if(THEMIS_FORCE_LLAMA_SUBDIR)
            set(LLAMA_BIN_DIR "${CMAKE_EXTERNALS_BASE_DIR}/llama_cpp_forced")
        else()
            set(LLAMA_BIN_DIR "${CMAKE_EXTERNALS_BASE_DIR}/llama_cpp")
        endif()
        
        message(STATUS "llama.cpp build dir: ${LLAMA_BIN_DIR}")
        file(MAKE_DIRECTORY "${LLAMA_BIN_DIR}")
        
        # WINDOWS FIX: Pass Windows SDK paths to sub-CMake processes (e.g., Vulkan Shader Generator)
        # Sub-processes lose Windows SDK library paths, causing LNK1104 (kernel32.lib not found)
        # Solution: Pass CMAKE_LIBRARY_PATH with Windows SDK directories
        if(WIN32 AND MSVC)
            # Query Windows SDK and MSVC library paths from environment
            set(WIN_SDK_PATHS)
            if(DEFINED ENV{WindowsSdkDir} AND EXISTS "$ENV{WindowsSdkDir}/Lib")
                file(GLOB WIN_SDK_PATHS "$ENV{WindowsSdkDir}/Lib/*")
            endif()
            set(MSVC_LIB_PATHS)
            if(DEFINED ENV{VCToolsInstallDir} AND EXISTS "$ENV{VCToolsInstallDir}/lib/x64")
                list(APPEND MSVC_LIB_PATHS "$ENV{VCToolsInstallDir}/lib/x64")
            elseif(_themis_program_files_x86)
                file(GLOB MSVC_LIB_PATHS
                    "${_themis_program_files_x86}/Microsoft Visual Studio/*/*/VC/Tools/MSVC/*/lib/x64")
            endif()
            
            if(WIN_SDK_PATHS OR MSVC_LIB_PATHS)
                # Build library search path for sub-CMake
                set(_SUBPROJECT_LIB_PATH "")
                foreach(_path ${WIN_SDK_PATHS} ${MSVC_LIB_PATHS})
                    if(_SUBPROJECT_LIB_PATH)
                        string(APPEND _SUBPROJECT_LIB_PATH ";${_path}")
                    else()
                        set(_SUBPROJECT_LIB_PATH "${_path}")
                    endif()
                endforeach()
                if(_SUBPROJECT_LIB_PATH)
                    set(CMAKE_LIBRARY_PATH "${_SUBPROJECT_LIB_PATH};${CMAKE_LIBRARY_PATH}" CACHE STRING "Windows SDK and MSVC library paths for sub-CMake projects" FORCE)
                    set(ENV{LIB} "${_SUBPROJECT_LIB_PATH};$ENV{LIB}")
                    set(ENV{LIBPATH} "${_SUBPROJECT_LIB_PATH};$ENV{LIBPATH}")
                endif()
                message(STATUS "Sub-CMake CMAKE_LIBRARY_PATH set: ${_SUBPROJECT_LIB_PATH}")
            endif()
        endif()
        
        add_subdirectory(${LLAMA_SRC_DIR} ${LLAMA_BIN_DIR} EXCLUDE_FROM_ALL)
    endif()
    
    # MSVC fix: char8_t string literals in llama-chat.cpp require /Zc:char8_t-
    if(MSVC AND TARGET llama)
        target_compile_options(llama PRIVATE /Zc:char8_t-)
    endif()
    
    # Define LLM enabled for ThemisDB code
    add_compile_definitions(THEMIS_LLM_ENABLED)
    
    message(STATUS "  llama.cpp configured successfully")
    endif() # end of else block for TARGET llama check
endif()

# Helper: normalize source entries that start with '/' to be rooted at the project source dir
function(_themis_normalize_sources list_var)
    set(_normalized "")
    foreach(_entry IN LISTS ${list_var})
        if(_entry MATCHES "^/")
            list(APPEND _normalized "${THEMIS_ROOT_DIR}${_entry}")
        else()
            list(APPEND _normalized "${_entry}")
        endif()
    endforeach()
    set(${list_var} "${_normalized}" PARENT_SCOPE)
endfunction()

function(_themis_mark_public_optional_source result_var relative_src_path)
    if(EXISTS "${THEMIS_ROOT_DIR}/${relative_src_path}")
        set(${result_var} ON PARENT_SCOPE)
    else()
        message(STATUS "Optional public source missing (treated as private/externalized): ${relative_src_path}")
        set(${result_var} OFF PARENT_SCOPE)
    endif()
endfunction()

_themis_mark_public_optional_source(THEMIS_HAS_PUBLIC_MYSQL_IMPORTER "src/importers/mysql_importer.cpp")
_themis_mark_public_optional_source(THEMIS_HAS_PUBLIC_MONGO_IMPORTER "src/importers/mongo_importer.cpp")
_themis_mark_public_optional_source(THEMIS_HAS_PUBLIC_KAFKA_IMPORTER "src/importers/kafka_importer.cpp")
_themis_mark_public_optional_source(THEMIS_HAS_PUBLIC_S3_IMPORTER "src/importers/s3_importer.cpp")
_themis_mark_public_optional_source(THEMIS_HAS_PUBLIC_BLOB_BACKEND_S3 "src/storage/blob_backend_s3.cpp")
_themis_mark_public_optional_source(THEMIS_HAS_PUBLIC_BLOB_BACKEND_AZURE "src/storage/blob_backend_azure.cpp")

# Core library sources
set(THEMIS_CORE_SOURCES
    ../src/storage/rocksdb_wrapper.cpp
    ../src/storage/base_entity.cpp
    ../src/storage/key_schema.cpp
    ../src/storage/backup_manager.cpp
    ../src/storage/database_connection_manager.cpp
    ../src/storage/disk_space_monitor.cpp
    ../src/storage/nlp_metadata_extractor.cpp
    ../src/storage/columnar_format.cpp
    ../src/storage/simd_filter.cpp
    ../src/storage/storage_parquet_exporter.cpp
    ../src/storage/tensor_train_decomposer.cpp
    ../src/storage/tensor_network_storage_engine.cpp
    ../src/storage/tt_quantizer.cpp
    ../src/storage/tensor_router.cpp
    ../src/tensor/tensor_mmap_bridge.cpp
    ../src/tensor/hiss_structural_search.cpp
    ../src/tensor/tensor_index_manager.cpp
    ../src/tensor/tensor_fingerprint_graph.cpp
    ../src/tensor/tensor_error_handling.cpp
    ../src/tensor/tensor_ingestion_bridge.cpp
    ../src/tensor/tensor_core_bridge.cpp
    # Distributed Tensor Retrieval (EPIC 3)
    ../src/tensor/tensor_summary_types.cpp
    ../src/tensor/compression_strategy.cpp
    ../src/distributed_tensor/src/distributed_planner.cc
    ../src/storage/batch_write_optimizer.cpp
    ../src/storage/wal_storage.cpp
    ../src/storage/transaction_retry_manager.cpp
    ../src/storage/compaction_manager.cpp
    ../src/storage/adaptive_compaction.cpp
    ../src/storage/schema_dead_weight_detector.cpp
    ../src/storage/storage_layout_advisor.cpp
    ../src/storage/merge_operators.cpp
    ../src/storage/wom_tree.cpp
    ../src/storage/compressed_storage.cpp
    ../src/storage/compression_strategy.cpp
    ../src/storage/gpu_compression.cpp
    ../src/storage/index_maintenance.cpp
    ../src/storage/index_analyzer.cpp
    ../src/storage/storage_engine.cpp
    # Phase 3: Error Handling & Edge Cases
    ../src/storage/storage_error_diagnostics.cpp
    ../src/storage/storage_recovery_fault_handler.cpp
    ../src/storage/storage_pressure_manager.cpp
    ../src/storage/blob_backend_filesystem.cpp
    $<$<BOOL:${THEMIS_HAS_PUBLIC_BLOB_BACKEND_S3}>:../src/storage/blob_backend_s3.cpp>
    $<$<BOOL:${THEMIS_HAS_PUBLIC_BLOB_BACKEND_AZURE}>:../src/storage/blob_backend_azure.cpp>
    ../src/storage/blob_backend_webdav.cpp
    ../src/storage/blob_backend_gcs.cpp
    # nvme_manager.cpp is registered via cmake/StorageEnhancements.cmake
    ../src/storage/streaming_ingest_manager.cpp
    ../src/storage/encrypted_blob_backend.cpp
    ../src/storage/mvcc_chain_pruner.cpp

    ../src/metadata/schema_manager.cpp
    ../src/metadata/statistics_collector.cpp
    ../src/metadata/information_schema.cpp
    ../src/metadata/schema_constraints.cpp
    ../src/metadata/schema_version_manager.cpp
    ../src/metadata/schema_audit_log.cpp
    ../src/metadata/schema_consistency_checker.cpp
    ../src/metadata/index_recommender.cpp
    ../src/metadata/column_lineage.cpp
    ../src/metadata/catalog_exporter.cpp
    ../src/metadata/distributed_catalog.cpp
    ../src/metadata/er_diagram_exporter.cpp
    ../src/server/schema_api_handler.cpp
    
    ../src/index/secondary_index.cpp
    ../src/index/inverted_index.cpp
    ../src/index/temporal_graph.cpp
    ../src/index/property_graph.cpp
    ../src/index/edge_types.cpp
    ../src/index/process_graph.cpp
    ../src/index/gnn_embeddings.cpp
    ../src/index/graph_analytics.cpp
    ../src/index/graph_auto_buffer.cpp
    ../src/index/vector_index.cpp
    ../src/index/ann_index.cpp
    ../src/index/advanced_vector_index.cpp
    ../src/index/approximate_radius_search.cpp
    ../src/index/hnsw_layer_optimizer.cpp
    ../src/index/hnsw_parameter_tuner.cpp
    ../src/index/hnsw_production_defaults.cpp
    ../src/index/cuda_hnsw_graph_traversal.cpp
    ../src/index/product_quantizer.cpp
    ../src/index/binary_quantizer.cpp
    ../src/index/learned_quantizer.cpp
    ../src/index/residual_quantizer.cpp
    ../src/index/adaptive_index.cpp
    ../src/index/workload_replay.cpp
    ../src/index/index_manager.cpp
    ../src/index/rotary_embeddings.cpp
    ../src/index/rotary_embeddings_gpu_cpu.cpp
    ../src/index/learnable_rope.cpp
    ../src/index/lora_rope.cpp
    ../src/index/multi_vector_search.cpp
    ../src/index/distributed_vector_index.cpp
    ../src/index/tiered_index_manager.cpp
    ../src/index/partition_manager.cpp
    ../src/index/vector_index_manager_safety.cpp
    ../src/index/vector_auto_buffer.cpp
    ../src/index/gpu_memory_oversubscription.cpp
    ../src/index/index_compression.cpp
    $<$<BOOL:${THEMIS_ENABLE_GPU}>:../src/index/gpu_vector_index.cpp>
    $<$<BOOL:${THEMIS_ENABLE_GPU}>:../src/index/multi_gpu_vector_index.cpp>
    $<$<BOOL:${THEMIS_ENABLE_VULKAN}>:../src/index/gpu_vector_index_vulkan.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CUDA}>:../src/index/rotary_embeddings_cuda.cu>
    $<$<BOOL:${THEMIS_ENABLE_HIP}>:../src/index/rotary_embeddings_hip.cpp>
    
    ../src/security/mock_key_provider.cpp
    ../src/security/vault_key_provider.cpp
    ../src/security/key_cache.cpp
    ../src/security/keyprovider_signing.cpp
    ../src/security/hsm_signing.cpp
    ../src/security/vault_signing_provider.cpp
    ../src/security/xxe_safe_xml_parser.cpp
    ../src/security/field_encryption.cpp
    ../src/security/encrypted_field.cpp
    ../src/security/malware_scanner.cpp
    ../src/security/usb_admin_authenticator.cpp
    ../src/security/usb_volume_hardening.cpp
    ../src/security/aql_injection_detector.cpp
    ../src/security/intent_classifier.cpp
    ../src/security/ai_operation_guard.cpp
    ../src/security/ai_snapshot_cleanup.cpp
    ../src/core/security_initialization.cpp
    ../src/performance/cycle_metrics.cpp
    ../src/performance/numa_topology.cpp
    ../src/performance/workload_predictor.cpp
    ../src/performance/async_metrics_exporter.cpp
    ../src/performance/prometheus_exporter.cpp
    ../src/performance/prometheus_exporter.cpp
    ../src/performance/async_metrics_exporter.cpp
    
    ../src/config/config_path_resolver.cpp
    ../src/config/config_metrics_exporter.cpp
    ../src/config/config_schema_validator.cpp
    ../src/config/config_file_watcher.cpp
    ../src/config/config_audit_log.cpp
    ../src/config/config_encrypted_store.cpp
    
    ../src/transaction/transaction_manager.cpp
    ../src/transaction/saga.cpp
    ../src/transaction/distributed_saga.cpp
    ../src/transaction/distributed_transaction_manager.cpp
    ../src/transaction/saga_orchestrator.cpp
    ../src/transaction/compensation_log.cpp
    ../src/transaction/saga_plugin_bridge.cpp
    ../src/transaction/snapshot_manager.cpp
    ../src/transaction/merge_engine.cpp
    ../src/transaction/branch_manager.cpp
    ../src/transaction/lock_manager.cpp
    ../src/transaction/crash_recovery_manager.cpp
    ../src/transaction/global_transaction_manager.cpp
    ../src/transaction/deadlock_predictor.cpp
    ../src/transaction/transaction_semantic_advisor.cpp
    
    ../src/query/query_engine.cpp
    ../src/query/query_optimizer.cpp
    ../src/query/adaptive_optimizer.cpp
    ../src/query/adaptive_join.cpp
    ../src/query/runtime_reoptimizer.cpp
    ../src/graph/graph_query_optimizer.cpp
    ../src/graph/graph_query_rewriter.cpp
    ../src/graph/explain_plan.cpp
    ../src/graph/ontology_manager.cpp
    ../src/graph/knowledge_graph_reasoner.cpp
    ../src/graph/path_constraints.cpp
    ../src/graph/gpu_traversal.cpp
    $<$<BOOL:${THEMIS_ENABLE_CUDA}>:../src/graph/gpu_traversal_cuda.cu>
    ../src/graph/parallel_traversal.cpp
    ../src/graph/distributed_graph.cpp
    ../src/graph/scheduled_edge_refresh.cpp
    ../src/graph/graph_query_cache.cpp
    ../src/graph/adjacency_list.cpp
    ../src/query/optimizer_cost_model.cpp
    ../src/query/optimizer_cost_model_enhancements.cpp
    ../src/query/geospatial_cost_model.cpp
    ../src/query/geospatial_optimizer_hints.cpp
    ../src/query/geospatial_index_selector.cpp
    ../src/query/geospatial_query_rewrite.cpp
    ../src/query/aql_parser.cpp
    ../src/query/aql_parser_json.cpp
    ../src/query/aql_translator.cpp
    ../src/query/aql_runner.cpp
    ../src/query/aql_safety_validator.cpp
    ../src/query/result_type_annotation.cpp
    ../src/query/query_plan_visualizer.cpp
    ../src/query/sql_parser.cpp
    ../src/query/sparql_parser.cpp
    ../src/query/cypher_parser.cpp
    ../src/query/gremlin_parser.cpp
    ../src/query/let_evaluator.cpp
    ../src/query/window_evaluator.cpp
    ../src/query/cte_subquery.cpp
    ../src/query/cte_cache.cpp
    ../src/query/materialized_cte.cpp
    ../src/query/result_stream.cpp
    ../src/query/query_cache.cpp
    ../src/query/synopsis_store.cpp
    ../src/query/incremental_agg.cpp
    ../src/query/cq_watermark.cpp
    ../src/query/continuous_query_engine.cpp
    ../src/query/continuous_query_planner.cpp
    ../src/query/query_rewrite_rule.cpp
    ../src/query/query_profiler.cpp

    # Vectorized Execution Engine – column-store style batch processing (Issue #2434)
    ../src/query/vectorized_execution.cpp
    ../src/query/parallel_executor.cpp

    ../src/query/statistical_aggregator.cpp
    ../src/query/approximate_aggregator.cpp
    ../src/query/semantic_cache.cpp
    ../src/query/cross_cluster_federation.cpp
    ../src/query/workload_cache_strategy.cpp
    ../src/query/query_cache_manager.cpp
    ../src/query/query_canceller.cpp
    ../src/query/query_federation.cpp
    ../src/query/query_federation_timeout.cpp
    ../src/query/query_federation_memory.cpp
    ../src/query/federation_resilience.cpp
    ../src/query/plan_cache.cpp
    ../src/query/query_executor.cpp
    ../src/query/query_compiler.cpp
    ../src/query/functions/function_registry.cpp
    ../src/api/graphql.cpp
    ../src/query/functions/ethics_functions.cpp
    ../src/query/functions/fulltext_functions.cpp
    ../src/query/functions/lora_functions.cpp
    ../src/query/functions/tensor_functions.cpp
    ../src/query/tensor_contraction_engine.cpp
    ../src/query/functions/process_mining_functions.cpp
    ../src/query/functions/udf_registry.cpp
    ../src/prompt_engineering/prompt_manager.cpp
    ../src/prompt_engineering/prompt_engineering_metrics.cpp
    ../src/prompt_engineering/feedback_collector.cpp
    ../src/prompt_engineering/prompt_evaluator.cpp
    ../src/prompt_engineering/meta_prompt_generator.cpp
    ../src/prompt_engineering/prompt_optimizer.cpp
    ../src/prompt_engineering/prompt_performance_tracker.cpp
    ../src/prompt_engineering/prompt_version_control.cpp
    ../src/prompt_engineering/self_improvement_orchestrator.cpp
    ../src/prompt_engineering/prompt_engineering_integration.cpp
    ../src/prompt_engineering/prompt_injection_detector.cpp
    ../src/prompt_engineering/chain_of_thought.cpp
    ../src/prompt_engineering/rag_prompt_builder.cpp
    ../src/prompt_engineering/system_prompt_manager.cpp
    ../src/prompt_engineering/context_window_manager.cpp
    ../src/prompt_engineering/reflection_tuner.cpp
    ../src/prompt_engineering/llm_reflection_adapter.cpp
    ../src/prompt_engineering/cot_tracer.cpp
    ../src/prompt_engineering/prompt_regression_runner.cpp
    ../src/prompt_engineering/prompt_ab_experiment.cpp
    ../src/prompt_engineering/prompt_library_io.cpp
    ../src/prompt_engineering/rag_context_budget_manager.cpp
    ../src/prompt_engineering/prompt_quality_evaluator.cpp
    ../src/prompt_engineering/tree_of_thoughts.cpp
    ../src/prompt_engineering/protegi_optimizer.cpp
    ../src/prompt_engineering/dspy_module.cpp
    ../src/prompt_engineering/structured_output.cpp
    ../src/prompt_engineering/markdown_utils.cpp
    ../src/prompt_engineering/prompt_compressor.cpp
    ../src/prompt_engineering/adversarial_prompt_tester.cpp
    
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/http_server.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/transaction_api_handler.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/distributed_txn_api_handler.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/auth_middleware.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/request_validation_middleware.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/api_auth_config.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/api_security_audit.cpp>
    $<$<BOOL:${THEMIS_ENABLE_SSE}>:../src/server/sse_connection_manager.cpp>
    ../src/server/cache_api_handler.cpp
    ../src/server/workload_fingerprint_engine.cpp
    ../src/server/cache_admin_api_handler.cpp
    ../src/server/admin_api_handler.cpp
    ../src/server/vector_api_handler.cpp
    ../src/server/rope_api_handler.cpp
    ../src/server/spatial_api_handler.cpp
    ../src/server/openapi_route_registry.cpp
    ../src/server/monitoring_api_handler.cpp
    ../src/server/query_api_handler.cpp
    ../src/server/udf_api_handler.cpp
    ../src/server/policy_api_handler.cpp
    ../src/server/policy_manager_api_handler.cpp
    ../src/server/policy_versioning_api_handler.cpp
    ../src/server/policy_template_api_handler.cpp
    ../src/server/compliance_reporting_api_handler.cpp
    ../src/server/policy_validation_api_handler.cpp
    ../src/server/review_scheduling_api_handler.cpp
    ../src/server/prompt_api_handler.cpp
    ../src/server/prompt_engineering_api_handler.cpp
    ../src/server/bpmn_api_handler.cpp
    ../src/server/branch_api_handler.cpp
    ../src/server/graph_api_handler.cpp
    ../src/server/index_api_handler.cpp
    ../src/server/timeseries_api_handler.cpp
    ../src/server/entity_api_handler.cpp
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/server/content_api_handler.cpp>
    ../src/server/changefeed_api_handler.cpp
    ../src/server/chunked_response_writer.cpp
    ../src/server/wal_api_handler.cpp
    ../src/server/replication_topology_api_handler.cpp
    ../src/server/audit_api_handler.cpp
    ../src/server/pki_api_handler.cpp
    ../src/server/saga_api_handler.cpp
    ../src/server/pii_api_handler.cpp
    ../src/server/retention_api_handler.cpp
    ../src/server/keys_api_handler.cpp
    ../src/server/api_key_mgmt_handler.cpp
    ../src/server/session_api_handler.cpp
    ../src/server/saml_auth_provider.cpp
    ../src/server/oauth2_provider.cpp
    ../src/server/classification_api_handler.cpp
    ../src/server/reports_api_handler.cpp
    ../src/server/ranger_adapter.cpp
    ../src/server/policy_engine.cpp
    ../src/server/opa_adapter.cpp
    ../src/server/auth_middleware.cpp
    $<$<BOOL:${THEMIS_ENABLE_GRPC}>:../src/server/wal_grpc_service.cpp>
    $<$<BOOL:${THEMIS_ENABLE_GRPC}>:../src/server/pitr_grpc_service.cpp>
    
    # LLM integration (server/API/grpc) is added conditionally below
    ../src/server/rate_limiter.cpp
    ../src/server/rate_limiter_v2.cpp
    ../src/server/adaptive_rate_limiter.cpp
    ../src/server/cost_based_rate_limiter.cpp
    ../src/server/rate_limiting_middleware.cpp
    ../src/server/load_shedder.cpp
    ../src/server/api_version.cpp
    ../src/server/response_transformer.cpp
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/api_gateway.cpp>
    ../src/server/update_api_handler.cpp
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/maintenance_api_handler.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/geo_topology_api_handler.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/mvcc_api_handler.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/serverless_function_api_handler.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/async_job_api_handler.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/maintenance/database_maintenance_orchestrator.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/maintenance/maintenance_schedule_store.cpp>
    ../src/server/hot_reload_api_handler.cpp
    ../src/server/export_api_handler.cpp
    ../src/server/continuous_query_api_handler.cpp
    ../src/server/shard_repair_api_handler.cpp
    ../src/server/hsm_provider_global.cpp
    $<$<AND:$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>,$<BOOL:${THEMIS_ENABLE_LLM}>>:../src/server/feedback_api_handler.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/snapshot_api_handler.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/pitr_api_handler.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/cdn_cache_middleware.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/import_api_handler.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/import_wizard_builder.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/profiling_api_handler.cpp>
    $<$<AND:$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>,$<BOOL:${THEMIS_ENABLE_LLM}>>:../src/server/ethics_api_handler.cpp>
    $<$<AND:$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>,$<BOOL:${THEMIS_ENABLE_VOICE_ASSISTANT}>>:../src/server/voice_api_handler.cpp>
    $<$<BOOL:${THEMIS_ENABLE_GRPC}>:../src/server/prompt_engineering_grpc_service.cpp>
    $<$<BOOL:${THEMIS_ENABLE_GRPC}>:../src/server/themis_core_grpc_service.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/cdc/cdc_admin.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/task_scheduler_api_handler.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/scheduler/task_scheduler.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/scheduler/task_audit_event.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/scheduler/task_audit_manager.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/scheduler/task_result_store.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/scheduler/task_anomaly_detector.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/scheduler/hybrid_retention_manager.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/scheduler/event_trigger.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/scheduler/external_scheduler_adapter.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/scheduler/distributed_task_coordinator.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP2}>:../src/server/http2_session.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP3}>:../src/server/http3_session.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP3}>:../src/server/http3_datagram.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP3}>:../src/server/http3_production_config.cpp>
    $<$<BOOL:${THEMIS_ENABLE_WEBSOCKET}>:../src/server/websocket_session.cpp>
    $<$<BOOL:${THEMIS_ENABLE_MQTT}>:../src/server/mqtt_session.cpp>
    $<$<BOOL:${THEMIS_ENABLE_POSTGRES_WIRE}>:../src/server/postgres_session.cpp>
    $<$<BOOL:${THEMIS_ENABLE_MCP}>:../src/server/mcp_server.cpp>
    $<$<BOOL:${THEMIS_ENABLE_GRPC}>:../src/server/grpc_web_proxy_handler.cpp>
    $<$<BOOL:${THEMIS_ENABLE_SERVICE_MESH}>:../src/server/service_mesh_api_handler.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP_SERVER}>:../src/server/wasm_handler_registry.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP3}>:../src/server/http3_datagram.cpp>
    $<$<BOOL:${THEMIS_ENABLE_GRPC}>:../src/server/prompt_engineering_grpc_service.cpp>
    $<$<BOOL:${THEMIS_ENABLE_GRPC}>:../src/server/themis_core_grpc_service.cpp>
    $<$<BOOL:${THEMIS_ENABLE_VOICE_ASSISTANT}>:../src/server/voice_api_handler.cpp>
    # Voice assistant implementation
    $<$<BOOL:${THEMIS_ENABLE_LLM}>:../src/voice/voice_assistant.cpp>
    $<$<BOOL:${THEMIS_ENABLE_LLM}>:../src/voice/voice_assistant_llm.cpp>
    $<$<BOOL:${THEMIS_ENABLE_LLM}>:../src/voice/audio_preprocessing.cpp>
    $<$<BOOL:${THEMIS_ENABLE_LLM}>:../src/voice/emotion_analyzer.cpp>
    $<$<BOOL:${THEMIS_ENABLE_LLM}>:../src/voice/voice_accessibility.cpp>
    $<$<BOOL:${THEMIS_ENABLE_LLM}>:../src/voice/voice_audio_storage.cpp>
    $<$<BOOL:${THEMIS_ENABLE_LLM}>:../src/voice/voice_authenticator.cpp>
    $<$<BOOL:${THEMIS_ENABLE_LLM}>:../src/voice/voice_batch_processor.cpp>
    $<$<BOOL:${THEMIS_ENABLE_LLM}>:../src/voice/voice_browser_streaming.cpp>
    $<$<BOOL:${THEMIS_ENABLE_LLM}>:../src/voice/voice_error_handler.cpp>
    $<$<BOOL:${THEMIS_ENABLE_LLM}>:../src/voice/voice_intent_detector.cpp>
    $<$<BOOL:${THEMIS_ENABLE_LLM}>:../src/voice/voice_macro_manager.cpp>
    $<$<BOOL:${THEMIS_ENABLE_LLM}>:../src/voice/voice_meeting_support.cpp>
    $<$<BOOL:${THEMIS_ENABLE_LLM}>:../src/voice/voice_model_cache.cpp>
    $<$<BOOL:${THEMIS_ENABLE_LLM}>:../src/voice/voice_security.cpp>
    $<$<BOOL:${THEMIS_ENABLE_LLM}>:../src/voice/voice_session_manager.cpp>
    $<$<BOOL:${THEMIS_ENABLE_LLM}>:../src/voice/voice_telephony.cpp>
    $<$<BOOL:${THEMIS_ENABLE_LLM}>:../src/voice/voice_tts_customizer.cpp>
    $<$<BOOL:${THEMIS_ENABLE_LLM}>:../src/voice/wake_word_detector.cpp>
    ../src/server/cdn_cache_middleware.cpp
    ../src/server/ethics_api_handler.cpp
    ../src/server/import_api_handler.cpp
    ../src/server/import_wizard_builder.cpp
    ../src/server/profiling_api_handler.cpp
    
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/content_type.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/content_manager.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/text_processor.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/html_processor.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/markdown_processor.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/video_processor.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/audio_processor.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/stt_processor.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/image_processor.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/pdf_processor.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/ocr_processor.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/geo_processor.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/cad_processor.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/mock_clip_processor.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/mime_detector.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/content_policy.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/content_errors.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/content_validator.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/content_metrics.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/content_logger.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/content_security.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/abuse_detector.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/content_fs.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/version_manager.cpp>
    $<$<AND:$<BOOL:${THEMIS_ENABLE_CONTENT}>,$<BOOL:${THEMIS_ENABLE_OFFICE}>>:../src/content/office_processor.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/archive_processor.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/async_ingestion_worker.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/ingestion_plugin.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/deduplication_checker.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/language_detector.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/embedding_pipeline.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/content_manager_embedding.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/tts_processor.cpp>
    $<$<AND:$<BOOL:${THEMIS_ENABLE_CONTENT}>,$<BOOL:${THEMIS_ENABLE_LLM}>>:../src/content/content_manager_llm.cpp>
    
    # Content ↔ Toolbox adapter layer (IFormatExtractor implementations)
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/adapters/pdf_extractor_adapter.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/adapters/image_extractor_adapter.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/adapters/archive_extractor_adapter.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/adapters/text_extractor_adapter.cpp>
    $<$<AND:$<BOOL:${THEMIS_ENABLE_CONTENT}>,$<BOOL:${THEMIS_ENABLE_OFFICE}>>:../src/content/adapters/office_extractor_adapter.cpp>
    $<$<AND:$<BOOL:${THEMIS_ENABLE_CONTENT}>,$<BOOL:${THEMIS_ENABLE_VOICE_ASSISTANT}>>:../src/content/adapters/audio_extractor_adapter.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/adapters/format_extractor_factory.cpp>
    
    # Content Pipeline (GAP-005) - Enhanced with streaming and multi-modal support
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/pipeline/zstd_compression.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/pipeline/content_chunker.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/pipeline/bulk_upload_interface.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/pipeline/async_bulk_uploader.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/content/pipeline/multimodal_chunker.cpp>
    
    ../src/cache/semantic_cache.cpp
    ../src/cache/adaptive_query_cache.cpp
    ../src/cache/cache_manager.cpp
    ../src/cache/embedding_cache.cpp
    ../src/cache/bounded_lru_cache.cpp
    ../src/cache/warmup.cpp
    ../src/cache/cache_hit_rate_slo_monitor.cpp
    ../src/cache/predictive_prefetcher.cpp
    ../src/cache/cache_replication.cpp
    ../src/cache/cache_replication_coordinator.cpp
    $<$<BOOL:${THEMIS_ENABLE_GRPC}>:../src/cache/grpc_remote_cache_peer.cpp>
    ../src/cache/distributed_cache_coordinator.cpp
    # ../src/cache/redis_cache_coordinator.cpp   # Legacy hiredis impl; conflicts with distributed_cache_coordinator
    ../src/cache/cache_eviction_policy.cpp
    ../src/cache/eviction_listener.cpp
    ../src/cache/eviction.cpp

    # Access Model module — cache↔storage tier coordination
    ../src/access_model/access_coordinator.cpp
    ../src/access_model/age_based_policy.cpp
    ../src/access_model/access_metrics.cpp
    ../src/access_model/promotion_demotion.cpp
    ../src/access_model/access_model_logging.cpp
    ../src/access_model/access_model_trace.cpp

    # Ingestion module – unified data intake layer (filesystem, REST, HuggingFace,
    # Kafka, S3/GCS/Azure, JDBC, web-crawler, CDC, distributed coordinator)
    ../src/ingestion/ingestion_manager.cpp
    ../src/ingestion/filesystem_ingester.cpp
    ../src/ingestion/api_connector.cpp
    ../src/ingestion/huggingface_connector.cpp
    ../src/ingestion/kafka_connector.cpp
    ../src/ingestion/object_storage_connector.cpp
    # s3_connector.cpp: dedicated S3-optimised connector (incremental mode,
    # concurrent downloads, flat-file delegation); uses #ifdef THEMIS_ENABLE_S3
    # internally – always compiles without the AWS SDK.
    ../src/ingestion/s3_connector.cpp
    ../src/ingestion/database_connector.cpp
    ../src/ingestion/web_crawler_connector.cpp
    ../src/ingestion/ingestion_coordinator.cpp
    # cdc_connector.cpp uses #ifdef THEMIS_ENABLE_CDC_STREAM internally; the file
    # always compiles and returns CONNECTOR_NOT_SUPPORTED when the flag is OFF.
    ../src/ingestion/cdc_connector.cpp
    # Legal ingestion pipeline: deontic extraction, semantic validation, reference validation
    ../src/ingestion/deontic_extractor.cpp
    ../src/ingestion/semantic_validator.cpp
    ../src/ingestion/agentic_reference_validator.cpp
    ../src/ingestion/ingestion_quality_judge.cpp
    ../src/ingestion/llm_adapter.cpp
    # Ingestion v2.0 workflow engine + entity assembler + sinks
    ../src/ingestion/workflow_engine.cpp
    ../src/ingestion/entity_assembler.cpp
    ../src/ingestion/ingestion_sinks.cpp
    ../src/ingestion/legal_domain.cpp
    # Ingestion v2.0 builtin steps
    ../src/ingestion/steps/parse_text_step.cpp
    ../src/ingestion/steps/chunk_text_step.cpp
    ../src/ingestion/steps/legal_metadata_step.cpp
    ../src/ingestion/steps/deontic_step.cpp
    ../src/ingestion/steps/base_entity_assembler_step.cpp
    ../src/ingestion/steps/ner_step.cpp
    ../src/ingestion/steps/llm_extract_step.cpp
    ../src/ingestion/steps/format_parse_step.cpp
    # Ingestion v1.4.0 builtin steps
    ../src/ingestion/steps/decompress_step.cpp
    ../src/ingestion/steps/legal_reference_step.cpp
    ../src/ingestion/steps/chunk_embed_step.cpp
    ../src/ingestion/steps/chunk_tt_decompose_step.cpp
    ../src/ingestion/steps/tensor_core_bridge_step.cpp

    # Toolbox module — system-wide access point for ingestion infrastructure
    ../src/toolbox/ingestion_toolbox.cpp
    ../src/toolbox/toolbox_builder.cpp
    ../src/toolbox/toolbox_registry.cpp
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/toolbox/content_toolbox_bridge.cpp>

    # LLM core components are added conditionally below
    ../src/cdc/changefeed.cpp
    ../src/cdc/changefeed_buffer.cpp
    ../src/cdc/consumer_group.cpp
    ../src/cdc/cross_collection_stream.cpp
    ../src/cdc/cdc_materialized_view.cpp
    ../src/cdc/tenant_buffer_manager.cpp
    $<$<BOOL:${THEMIS_ENABLE_WEBSOCKET}>:../src/cdc/ws_transport.cpp>
    $<$<BOOL:${THEMIS_ENABLE_KAFKA}>:../src/cdc/kafka_cdc_producer.cpp>
    ../src/cdc/delivery_tracker.cpp
    ../src/cdc/dead_letter_queue.cpp
    ../src/cdc/outbox.cpp
    
    # Replication
    ../src/cdc/cdc_ws_handler.cpp
    ../src/replication/replication_manager.cpp
    ../src/replication/observability.cpp
    ../src/replication/conflict_resolution.cpp
    ../src/replication/event_stream.cpp
    ../src/replication/policy.cpp
    ../src/replication/replication_slot.cpp
    ../src/replication/raft_v2.cpp
    ../src/replication/schema_cdc.cpp
    ../src/replication/multi_tier_replication.cpp
    ../src/replication/logical_replication.cpp
    
    # Temporal versioning and query engine
    ../src/temporal/temporal_conflict_resolver.cpp
    ../src/temporal/system_versioned_table.cpp
    ../src/temporal/temporal_query_engine.cpp
    ../src/temporal/bi_temporal.cpp
    ../src/temporal/temporal_index.cpp
    ../src/temporal/temporal_aggregator.cpp
    ../src/temporal/snapshot_manager.cpp
    ../src/temporal/retention_manager.cpp
    ../src/temporal/bitemporal_join.cpp
    ../src/temporal/interval_tree_index.cpp
    ../src/temporal/temporal_compressor.cpp
    ../src/temporal/temporal_cdc.cpp
    ../src/temporal/temporal_migrator.cpp
    ../src/temporal/temporal_cold_store.cpp
    ../src/temporal/temporal_tier_manager.cpp
    
    ../src/timeseries/timeseries.cpp
    ../src/timeseries/timeseries_metrics.cpp
    ../src/timeseries/tsstore.cpp
    ../src/timeseries/gorilla.cpp
    ../src/timeseries/gorilla_simd.cpp
    ../src/timeseries/retention.cpp
    ../src/timeseries/continuous_agg.cpp
    ../src/timeseries/aggregate_scheduler.cpp
    ../src/timeseries/aggregate_scheduler_helper.cpp
    ../src/timeseries/query_optimizer.cpp
    ../src/timeseries/prometheus_remote_write.cpp
    ../src/timeseries/ts_auto_buffer.cpp
    ../src/timeseries/hypertable.cpp
    ../src/timeseries/aggregates.cpp
    ../src/timeseries/downsampling.cpp
    ../src/timeseries/ts_auto_buffer_adaptive.cpp
    ../src/timeseries/adaptive_flush_controller.cpp
    ../src/timeseries/encrypted_chunk_store.cpp
    ../src/timeseries/ts_encrypted_key_rotation.cpp
    ../src/timeseries/compression_selector.cpp
    ../src/timeseries/anomaly_detection.cpp
    ../src/timeseries/gap_fill.cpp
    ../src/timeseries/ts_stream_cursor.cpp
    
    ../src/utils/serialization.cpp
    ../src/utils/logger.cpp
    ../src/utils/error_contracts.cpp
    ../src/utils/cursor.cpp
    ../src/utils/tracing.cpp
    
    # Cross-cutting concerns abstraction layer
    ../src/core/concerns/i_logger.cpp
    ../src/core/concerns/concerns_context.cpp
    ../src/core/concerns/context_propagation.cpp
    ../src/core/concerns/adapter_registry.cpp
    ../src/core/concerns/adapter_signing.cpp
    ../src/core/adapters/otel_tracer.cpp
    ../src/core/concerns/prometheus_metrics.cpp
    ../src/core/concerns/lockfree_metrics.cpp
    ../src/core/concerns/redis_cache.cpp
    ../src/core/concerns/zero_copy_logger.cpp
    ../src/utils/zstd_codec.cpp
    ../src/utils/lz4_codec.cpp
    ../src/utils/pki_client.cpp
    ../src/utils/input_validator.cpp
    ../src/utils/audit_logger.cpp
    ../src/utils/pii_detection_engine.cpp
    ../src/utils/regex_detection_engine.cpp
    ../src/utils/ner_detection_engine.cpp
    ../src/utils/pii_detector.cpp
    ../src/utils/retention_manager.cpp
    ../src/utils/hkdf_helper.cpp
    ../src/utils/hkdf_cache.cpp
    ../src/utils/saga_logger.cpp
    ../src/utils/lek_manager.cpp
    ../src/utils/stemmer.cpp
    ../src/utils/stopwords.cpp
    ../src/utils/pii_pseudonymizer.cpp
    ../src/utils/normalizer.cpp
    ../src/utils/simd_distance.cpp
    ../src/utils/update_checker.cpp
    $<$<BOOL:${BOOST_FOUND}>:../src/utils/http_client_pool.cpp>
    $<$<BOOL:${THEMIS_ENABLE_GRPC}>:../src/utils/grpc_channel_pool.cpp>
    ../src/utils/thread_pool_manager.cpp
    ../src/themis/build_info.cpp
    ../src/themis/license_info.cpp
    ../src/utils/runtime_license_gate.cpp
    ../src/utils/error_registry.cpp
    ../src/utils/memory/pool_allocator.cpp
    ../src/utils/checksum_utils.cpp           # SHA256/MD5 checksum utilities for model verification
    ../src/utils/bloom_filter.cpp             # Bloom filter for membership testing
    ../src/utils/consistent_hash.cpp          # Consistent hash ring utilities
    ../src/utils/pii_stream_scanner.cpp       # Streaming PII detection and redaction
    ../src/utils/sampled_logger.cpp           # Sampling-based logger to avoid log flooding
    ../src/utils/timestamp_utils.cpp          # Timestamp formatting and conversion utilities
    ../src/utils/rate_limiter.cpp             # Token-bucket rate limiter (utils namespace)
    ../src/utils/utils_adapters.cpp           # Concrete adapters for utils abstract interfaces
    
    # Updates (hot-reload)
    ../src/updates/release_manifest.cpp
    ../src/updates/manifest_database.cpp
    ../src/updates/hot_reload_engine.cpp
    ../src/updates/updates_config.cpp
    ../src/updates/update_state_machine.cpp
    ../src/updates/canary_rollout.cpp
    ../src/updates/delta_update_engine.cpp
    ../src/updates/parallel_downloader.cpp
    ../src/updates/dependency_resolver.cpp
    ../src/updates/schema_migration_tester.cpp
    ../src/updates/in_place_schema_migrator.cpp
    ../src/updates/schema_migration.cpp
    ../src/updates/update_history_logger.cpp
    ../src/updates/notification_webhook.cpp
    ../src/updates/blue_green_deployment.cpp
    ../src/updates/coordinated_update_manager.cpp
    ../src/updates/cluster_update_manager.cpp
    ../src/updates/preflight_health_check.cpp
    ../src/updates/tenant_update_scheduler.cpp
    ../src/updates/hardware_telemetry.cpp
    ../src/updates/build_verifier.cpp
    
    # Geo utilities
    ../src/utils/geo/ewkb.cpp
    
    # Spatial index
    ../src/index/spatial_index.cpp
    
    # Hybrid search (BM25 + Vector) and v1.5.0 search components
    ../src/search/hybrid_search.cpp
    ../src/search/query_expander.cpp
    ../src/search/fuzzy_matcher.cpp
    ../src/search/faceted_search.cpp
    ../src/search/search_analytics.cpp
    ../src/search/autocomplete.cpp
    ../src/search/learning_to_rank.cpp
    ../src/search/multi_modal_search.cpp
    ../src/search/llm_query_rewriter.cpp
    ../src/search/llm_reranker.cpp
    ../src/search/llm_reranker_factory_stub.cpp
    ../src/search/personalized_ranker.cpp
    ../src/search/multi_field_search.cpp
    ../src/search/cross_lingual_search.cpp
    ../src/search/neural_sparse_retrieval.cpp
    ../src/search/search_highlighter.cpp
    ../src/search/negative_keyword_filter.cpp
    ../src/search/distributed_hybrid_search.cpp
    ../src/search/layered_retrieval_orchestrator.cpp
    ../src/security/pki_key_provider.cpp
    ../src/security/cms_signing.cpp
    ../src/security/rbac.cpp
    ../src/security/access_control_manager.cpp
    ../src/security/zero_trust_policy_enforcer.cpp
    ../src/security/access_control.cpp
    ../src/security/user_registration_plugin.cpp
    ../src/security/arrow_user_registration_plugin.cpp
    ../src/security/webdav_user_registration_plugin.cpp
    ../src/security/embedded_user_registration_plugin.cpp
    ../src/security/hsm_provider.cpp
    ../src/security/hsm_key_provider_adapter.cpp
    ../src/security/timestamp_authority.cpp
    ../src/security/tsa_api.cpp
    ../src/security/vram_secure_clear.cpp
    ../src/security/vcc_pki_client.cpp
    ../src/security/pii_redaction_policy.cpp
    ../src/security/row_level_security.cpp
    ../src/security/query_masking_policy.cpp
    ../src/storage/security_signature.cpp
    ../src/storage/security_signature_manager.cpp
    ../src/security/binary_manifest.cpp
    ../src/security/manifest_signer.cpp
    ../src/security/confidential_computing.cpp
    ../src/security/fips_crypto_mode.cpp
    ../src/security/post_quantum_crypto.cpp
    ../src/security/secret_manager.cpp
    ../src/security/security_evidence_collector.cpp
    ../src/auth/jwt_validator.cpp
    ../src/auth/jwks_validator.cpp
    ../src/auth/gssapi_authenticator.cpp
    ../src/auth/ldap_authenticator.cpp
    ../src/auth/ldap_connection_pool.cpp
    ../src/auth/mtls_authenticator.cpp
    ../src/auth/saml_authenticator.cpp
    ../src/auth/oauth_device_flow.cpp
    ../src/auth/oauth_pkce_flow.cpp
    ../src/auth/oidc_provider.cpp
    ../src/auth/federated_identity_manager.cpp
    ../src/auth/rate_limiter_backend.cpp
    ../src/auth/api_key_authenticator.cpp
    ../src/auth/session_manager.cpp
    ../src/auth/token_blacklist.cpp
    ../src/auth/redis_token_blacklist.cpp
    ../src/auth/rocksdb_token_blacklist.cpp
    ../src/auth/zero_trust_auth_verifier.cpp
    ../src/auth/auth_audit_logger.cpp
    ../src/auth/mfa_authenticator.cpp
    ../src/auth/password_policy.cpp
    ../src/auth/webauthn_authenticator.cpp
    ../src/auth/passkey_authenticator.cpp
    ../src/auth/auth_metrics.cpp
    ../src/auth/auth_error.cpp
    ../src/auth/jwks_security.cpp
    ../src/auth/kerberos_security.cpp
    ../src/auth/totp_replay_cache.cpp
    ../src/auth/totp_secret_encryption.cpp
    ../src/auth/jwt_key_rotation_manager.cpp
    ../src/auth/principal_validator.cpp
    ../src/auth/http_auth_async.cpp
    ../src/auth/distributed_token_blacklist.cpp
    ../src/governance/policy_engine.cpp
    ../src/governance/opa_adapter.cpp
    ../src/governance/policy_manager.cpp
    ../src/governance/cross_tenant_policy_inheritance.cpp
    ../src/governance/policy_template.cpp
    ../src/governance/policy_coordinator.cpp
    ../src/governance/compliance_reporting.cpp
    ../src/governance/policy_validation.cpp
    ../src/governance/policy_version_history.cpp
    ../src/governance/policy_manager_versioned.cpp
    ../src/governance/compliance_reporter.cpp
    ../src/governance/policy_validator.cpp
    ../src/governance/review_scheduler.cpp
    ../src/governance/policy_review.cpp
    ../src/governance/policy_file_watcher.cpp
    ../src/governance/data_lineage.cpp
    ../src/governance/governance_diagnostics.cpp
    ../src/governance/ccpa_rules.cpp
    ../src/governance/soc2_controls.cpp
    ../src/governance/model_governance.cpp
    ../src/governance/pci_dss_rules.cpp
    ../src/governance/data_masker.cpp
    ../src/governance/cross_border_transfer.cpp
    # Geo scaffolding (headers + stubs; safe no-ops until used)
    ../src/geo/cpu_backend.cpp
    ../src/geo/gpu_backend_stub.cpp
    ../src/geo/boost_cpu_exact_backend.cpp
    ../src/geo/geo_rtree.cpp
    ../src/geo/geo_clustering.cpp
    ../src/geo/spatial_join.cpp
    ../src/geo/raster.cpp
    ../src/geo/temporal_spatial_query.cpp
    ../src/geo/tile_server.cpp
    ../src/geo/device_detector.cpp
    ../src/geo/gpu_backend_production.cpp
    ../src/geo/geo_json_geometry.cpp
    ../src/geo/spatial_join_filter.cpp
    ../src/geo/rtree_cursor.cpp
    ../src/geo/temporal_spatial_query_builder.cpp
    ../src/geo/raster_query_interface.cpp
    ../src/api/geo_index_hooks.cpp
    ../src/api/tracing_middleware.cpp
    ../src/api/otlp_exporter.cpp
    ../src/api/federation_admin_handler.cpp
    
    # Sharding (horizontal scaling)
    ../src/utils/boost_throw_exception.cpp
    ../src/sharding/urn.cpp
    ../src/sharding/consistent_hash.cpp
    ../src/sharding/shard_topology.cpp
    ../src/sharding/metadata_shard.cpp
    ../src/sharding/urn_resolver.cpp
    ../src/sharding/pki_shard_certificate.cpp
    ../src/sharding/mtls_client.cpp
    ../src/sharding/mtls_connection_pool.cpp
    ../src/sharding/mtls_connection_factory.cpp
    ../src/sharding/signed_request.cpp
    ../src/sharding/remote_executor.cpp
    ../src/sharding/shard_rpc_client.cpp
    ../src/utils/file_utils.cpp
    ../src/utils/capability_auto_generator.cpp
    ../src/utils/self_awareness.cpp
    ../src/sharding/shard_router.cpp
    ../src/sharding/capability_matcher.cpp
    ../src/sharding/adaptive_shard_router.cpp
    ../src/sharding/rebalance_operation.cpp
    ../src/sharding/data_migrator.cpp
    ../src/sharding/hardware_migration_manager.cpp
    ../src/sharding/shard_load_detector.cpp
    ../src/sharding/auto_rebalancer.cpp
    ../src/sharding/prometheus_metrics.cpp
    ../src/sharding/metrics_registry.cpp
    ../src/sharding/health_check.cpp
    ../src/sharding/admin_api.cpp
    ../src/sharding/cloud_agent.cpp
    ../src/sharding/cloud_backup.cpp
    ../src/sharding/cloud_sdk_integration_stubs.cpp
    ../src/sharding/orphan_detector.cpp
    ../src/sharding/circuit_breaker.cpp
    ../src/sharding/gossip_protocol.cpp
    ../src/sharding/gossip_config_manager.cpp
    ../src/sharding/distributed_coordinator.cpp
    ../src/sharding/shard_resource_manager.cpp
    ../src/sharding/locality_aware_router.cpp
    ../src/sharding/raft_configuration.cpp
    ../src/sharding/raft_log.cpp
    ../src/sharding/raft_state.cpp
    ../src/sharding/raft_wal_integration.cpp
    ../src/sharding/raft_consensus.cpp
    ../src/sharding/raft_shard_manager.cpp
    ../src/sharding/quorum_manager.cpp
    ../src/sharding/partition_detector.cpp
    ../src/sharding/replica_consistency.cpp
    ../src/sharding/stream_protocol.cpp
    ../src/sharding/wal_applier.cpp
    ../src/sharding/wal_manager.cpp
    ../src/sharding/wal_shipper.cpp
    ../src/sharding/replication_coordinator.cpp
    ../src/sharding/replica_topology.cpp
    ../src/sharding/multi_primary_coordinator.cpp
    ../src/sharding/health_monitor.cpp
    ../src/sharding/shard_durability.cpp
    ../src/sharding/truetime.cpp
    ../src/sharding/secure_transport_client.cpp
    ../src/sharding/distributed_time_coordinator.cpp
    ../src/sharding/distributed_transaction.cpp
    ../src/sharding/two_phase_commit_participant.cpp
    ../src/sharding/two_phase_commit_coordinator.cpp
    ../src/sharding/consensus_factory.cpp
    ../src/sharding/raft_consensus_adapter.cpp
    ../src/sharding/gossip_consensus_adapter.cpp
    ../src/sharding/metadata_wal.cpp
    ../src/sharding/metadata_snapshot.cpp
    ../src/sharding/paxos_consensus.cpp
    ../src/sharding/paxos_wal.cpp
    ../src/sharding/paxos_snapshot.cpp
    ../src/sharding/paxos_state_persistence.cpp
    ../src/sharding/cross_shard_transaction.cpp
    ../src/sharding/cross_shard_fk_validator.cpp
    ../src/sharding/cross_shard_ssi_manager.cpp
    ../src/sharding/transaction_wal.cpp
    ../src/sharding/transaction_snapshot.cpp
    ../src/sharding/redundancy_strategy.cpp
    ../src/sharding/hot_spare_manager.cpp
    ../src/sharding/predictive_detector.cpp
    ../src/sharding/shard_repair_engine.cpp
    ../src/sharding/shard_rpc_server.cpp
    ../src/sharding/cloud_backup.cpp
    ../src/sharding/orphan_detector.cpp
    ../src/sharding/gpu_erasure_coder.cpp
    $<$<BOOL:${THEMIS_ENABLE_CUDA}>:../src/sharding/gpu_erasure_coder.cu>
    $<$<BOOL:${THEMIS_ENABLE_OPENCL}>:../src/sharding/gpu_erasure_coder_opencl.cpp>
    ../src/storage/blob_redundancy_manager.cpp
    ../src/storage/erasure_coding_backend.cpp
    ../src/server/sharding_metrics_handler.cpp

    # Enhanced sharding features
    ../src/sharding/admin_operations.cpp
    ../src/sharding/operational_metrics.cpp
    ../src/sharding/sharding_manager_edition.cpp
    ../src/sharding/slo_monitor.cpp
    ../src/sharding/lora_artifact_distribution.cpp

    # Hardware acceleration (core abstraction layer - always included)
    ../src/acceleration/compute_backend.cpp
    ../src/acceleration/backend_registry.cpp
    ../src/acceleration/cpu_backend.cpp
    ../src/acceleration/multi_gpu_backend.cpp
    ../src/acceleration/tensor_core_matmul.cpp
    ../src/acceleration/plugin_loader.cpp
    ../src/acceleration/plugin_security.cpp
    ../src/acceleration/geo_acceleration_bridge.cpp
    ../src/acceleration/device_manager.cpp
    ../src/acceleration/vllm_resource_manager.cpp
    # PERF-D3: Parallel batch insertion + SIMD distance pipeline
    ../src/acceleration/vec_knn.cpp
    # shader_integrity.cpp uses only OpenSSL (SHA-256) — no GPU/graphics dependency.
    # Compiled unconditionally so ShaderIntegrityVerifier is always available.
    ../src/acceleration/shader_integrity.cpp
    # kernel_registry.cpp — central kernel/shader dispatch catalog (issue #5382).
    # No GPU runtime dependency; compiled unconditionally.
    ../src/acceleration/kernel_registry.cpp
    # AI Hardware Dispatcher: universal AI-hardware abstraction layer
    # (Apple ANE, Intel NPU, Qualcomm QNN, ARM Ethos, NNAPI, ONNX Runtime, GPU fallback)
    ../src/acceleration/ai_hardware_dispatcher.cpp
    # Plugin manager (core plugin orchestration)
    ../src/plugins/plugin_manager.cpp
    ../src/plugins/plugin_hot_plug_monitor.cpp
    ../src/plugins/plugin_registry.cpp
    ../src/plugins/plugin_metrics.cpp
    ../src/plugins/plugin_health_monitor.cpp
    ../src/plugins/signed_plugin_repository.cpp
    ../src/importers/huggingface_ingestion_plugin.cpp
    ../src/importers/wikipedia_plugin.cpp
    ../src/importers/huggingface_ingest_plugin.cpp
    # Project collaboration and audit
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/projects/collaboration_manager.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/projects/in_memory_project_audit_log.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/projects/project_diff.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/projects/project_lifecycle.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/projects/project_metrics.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/projects/project_template.cpp>
    $<$<BOOL:${THEMIS_ENABLE_CONTENT}>:../src/projects/project_versioning.cpp>
    ../src/plugins/oci_registry_client.cpp
    ../src/plugins/rpc_service_registry.cpp
    
    # Multi-tenancy support
    ../src/server/tenant_manager.cpp
    
    # Network
    ../src/network/wire_protocol_server.cpp
    ../src/network/wire_protocol_helpers.cpp
    ../src/network/wire_protocol_connection_pool.cpp
    ../src/network/wire_protocol_v2.cpp
    ../src/network/wire_protocol_performance.cpp
    ../src/network/wire_protocol_zero_copy.cpp
    ../src/network/wire_protocol_batch.cpp
    ../src/network/wire_protocol.cpp
    ../src/network/io_uring_batcher.cpp
    ../src/network/connection_compression.cpp
    ../src/network/socket_timeout_manager.cpp
    ../src/network/adaptive_circuit_breaker.cpp
    ../src/network/qos_manager.cpp
    ../src/network/udp_fast_path.cpp
    ../src/network/wire_retry_policy.cpp
    ../src/network/multipath_tcp.cpp
    ../src/network/bbr_congestion_control.cpp
    ../src/network/network_observability.cpp
    $<$<BOOL:${THEMIS_ENABLE_WEBSOCKET}>:../src/network/wire_protocol_server_ws.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP3}>:../src/network/quic_transport.cpp>
    $<$<BOOL:${THEMIS_ENABLE_HTTP3}>:../src/network/quic_server.cpp>
    $<$<BOOL:${THEMIS_ENABLE_GRPC}>:../src/network/grpc_transport.cpp>
    ../src/network/geo_topology_router.cpp
    $<$<BOOL:${THEMIS_ENABLE_SERVICE_MESH}>:../src/network/service_mesh.cpp>
    $<$<BOOL:${THEMIS_ENABLE_SERVICE_MESH}>:../src/network/envoy_xds.cpp>

    # Themis core module (Phase 3: Wire Protocol – themis::wire namespace)
    ../src/themis/wire_protocol_server.cpp
    ../src/themis/module_dependency_resolver.cpp
    ../src/themis/edition_manager.cpp
    ../src/themis/module_hash_verifier.cpp
    ../src/themis/module_signature_verifier.cpp
    
    # Observability (GAP-008: Alertmanager integration + continuous profiling)
    ../src/observability/metrics_collector.cpp
    ../src/observability/field_diagnostics_collector.cpp
    ../src/observability/alertmanager.cpp
    ../src/observability/continuous_profiler.cpp
    ../src/observability/ebpf_tracer.cpp
    ../src/observability/distributed_flame_graph.cpp
    ../src/observability/slo_reporter.cpp
    ../src/observability/metric_anomaly_detector.cpp
    ../src/observability/query_profiler.cpp
    ../src/observability/storage_profiler.cpp
    ../src/observability/performance_analyzer.cpp
    # Observability: standalone span management and structured log aggregation (OBS-MISSING-001)
    ../src/observability/tracer.cpp
    ../src/observability/log_aggregator.cpp
    # Observability: rule-based alerting engine with configurable notification channels
    ../src/observability/alerting_engine.cpp
    # Observability: Prometheus advanced — rate calculation, histogram aggregation, cardinality
    ../src/observability/metric_aggregator.cpp
    # Observability: real-time metric streaming via WebSocket / SSE (v1.6.0, Issue #82)
    ../src/observability/metrics_stream_server.cpp
    # Observability: custom metric types — summary, exponential histogram, cardinality, TWA, rate (v1.6.0)
    ../src/observability/advanced_metrics.cpp
    # Observability: OpenTelemetry Full Integration (v1.6.0)
    ../src/observability/opentelemetry_tracer.cpp
    # Observability: ML-based anomaly detection on metrics time-series (v1.7.0, Issue #2097)
    ../src/observability/ml_anomaly_detector.cpp
    # Observability: automated root cause identification for performance issues (v1.7.0, Issue #84)
    ../src/observability/root_cause_analyzer.cpp
    # Observability: post-generation lineage queryability (GAP-4.1, 2026-06-18)
    ../src/observability/provenance_store.cpp
    # Observability: per-tenant metric namespacing (v1.8.0)
    ../src/observability/tenant_metrics_namespace.cpp
    # RPC service implementation
    ../src/server/rpc/rpc_service_impl.cpp
    # Observability: structured log search API (v1.8.0)
    ../src/observability/log_search_engine.cpp

    # GraphQL API (conditional)
    $<$<BOOL:${THEMIS_ENABLE_GRAPHQL}>:../src/api/graphql.cpp>
    $<$<BOOL:${THEMIS_ENABLE_GRAPHQL}>:../src/server/graphql_api_handler.cpp>

    # WebSocket change-stream handler (conditional)
    $<$<BOOL:${THEMIS_ENABLE_WEBSOCKET}>:../src/api/ws_handler.cpp>
    # GraphQL over WebSocket (graphql-transport-ws) handler (conditional)
    $<$<BOOL:${THEMIS_ENABLE_WEBSOCKET}>:../src/api/graphql_ws_handler.cpp>

    # gRPC API server alongside REST (conditional)
    $<$<BOOL:${THEMIS_ENABLE_GRPC}>:../src/api/grpc_server.cpp>
    $<$<BOOL:${THEMIS_ENABLE_GRPC}>:../src/api/themisdb_grpc_service.cpp>
    
    # Exporters and Importers
    ../src/exporters/jsonl_llm_exporter.cpp
    ../src/exporters/format_template.cpp
    ../src/exporters/parquet_exporter.cpp
    ../src/exporters/exporter_metrics.cpp
    ../src/exporters/pii_detector.cpp
    ../src/exporters/stream_writer.cpp
    ../src/exporters/streaming_exporter.cpp
    ../src/exporters/aql_predicate_filter.cpp
    ../src/exporters/arrow_ipc_exporter.cpp
    ../src/exporters/incremental_exporter.cpp
    ../src/exporters/export_encryption.cpp
    ../src/exporters/data_augmentation.cpp
    ../src/exporters/huggingface_exporter.cpp
    ../src/exporters/huggingface_exporter.cpp
    ../src/exporters/data_augmentation.cpp
    ../src/exporters/export_format_registry.cpp
    ../src/exporters/join_exporter.cpp
    ../src/exporters/huggingface_hub_client.cpp
    ../src/importers/postgres_importer.cpp
    $<$<BOOL:${THEMIS_HAS_PUBLIC_MYSQL_IMPORTER}>:../src/importers/mysql_importer.cpp>
    $<$<BOOL:${THEMIS_HAS_PUBLIC_MONGO_IMPORTER}>:../src/importers/mongo_importer.cpp>
    # Core importers required by focused tests
    ../src/importers/elasticsearch_importer.cpp
    ../src/importers/redis_importer.cpp
    ../src/importers/debezium_cdc_importer.cpp
    ../src/importers/sqlite_importer.cpp
    ../src/importers/flatfile_importer.cpp
    ../src/importers/schema_validator.cpp
    ../src/importers/conflict_resolver.cpp
    $<$<BOOL:${THEMIS_HAS_PUBLIC_KAFKA_IMPORTER}>:../src/importers/kafka_importer.cpp>
    ../src/importers/oracle_importer.cpp
    ../src/importers/gui_import_wizard.cpp
    $<$<AND:$<BOOL:${THEMIS_ENABLE_S3}>,$<BOOL:${THEMIS_HAS_PUBLIC_S3_IMPORTER}>>:../src/importers/s3_importer.cpp>
    ../src/importers/schema_inference.cpp
    ../src/importers/column_importance.cpp
    ../src/importers/crdt_importer.cpp
    ../src/importers/postgres_cdc.cpp
    ../src/importers/data_quality.cpp
    ../src/importers/audit_trail.cpp
    ../src/importers/adaptive_import.cpp
    ../src/importers/polyglot_mapper.cpp
    ../src/importers/temporal_support.cpp
    ../src/importers/blockchain_integrity.cpp
    ../src/importers/federated_learning.cpp
    ../src/importers/graphql_federation.cpp
    ../src/importers/deterministic_matcher.cpp
    ../src/importers/semantic_matcher.cpp
    ../src/importers/entity_linker.cpp
    ../src/importers/canonical_resolver.cpp
    ../src/importers/mdm_engine.cpp
    ../src/importers/mdm_audit_trail.cpp
    ../src/importers/mdm_metrics.cpp
    ../src/importers/postgres_importer_mdm.cpp
    ../src/importers/wikipedia_pipeline.cpp
    ../src/importers/wikipedia_dump_reader.cpp
    ../src/importers/wikipedia_xml_parser.cpp
    ../src/importers/wikipedia_transform.cpp
    ../src/importers/wikipedia_project_graph.cpp
    ../src/importers/wikipedia_project_vector.cpp
    ../src/importers/wikipedia_project_timeseries.cpp
    ../src/importers/wikipedia_project_process.cpp
    ../src/importers/wikipedia_checkpoint.cpp
    ../src/importers/wikipedia_validator.cpp
    ../src/aql/llm_metrics_collector.cpp
    ../src/aql/aql_syntax_highlighter.cpp

    # Process Modeling Module (BPMN/EPK/VCC-VPB + Graph-RAG for Verwaltungsvorgänge)
    ../src/process/process_model_manager.cpp
    ../src/process/bpmn_serializer.cpp
    ../src/process/epk_serializer.cpp
    ../src/process/epk_aris_xml_importer.cpp
    ../src/process/llm_process_descriptor.cpp
    ../src/process/vcc_vpb_importer.cpp
    ../src/process/process_linker.cpp
    ../src/process/process_graph_rag.cpp
    ../src/process/process_community_detector.cpp
    ../src/process/process_light_retriever.cpp
    ../src/process/object_centric_tracer.cpp
    ../src/process/fim_importer.cpp
    ../src/process/cmmn_serializer.cpp
    ../src/process/dmn_evaluator.cpp
    ../src/process/process_diagnostics.cpp
    ../src/process/serializer_hardening.cpp
    ../src/process/process_agentic_rag.cpp
    ../src/process/ocel_exporter.cpp
    ../src/process/federation_replica_manager.cpp
    ../src/process/process_conflict_resolver.cpp

    # OLAP Analytics
    ../src/analytics/olap.cpp
    ../src/gpu/gpu_safe_raii.cpp
    ../src/gpu/kernel_timeout_enforcer.cpp
    ../src/gpu/query_accelerator.cpp
    $<$<BOOL:${THEMIS_ENABLE_CUDA}>:../src/gpu/cuda_operations.cpp>$<$<NOT:$<BOOL:${THEMIS_ENABLE_CUDA}>>:../src/gpu/cuda_operations_stub.cpp>
    ../src/analytics/process_mining.cpp
    ../src/analytics/nlp_text_analyzer.cpp
    ../src/analytics/arrow_export.cpp
    ../src/analytics/analytics_export.cpp
    ../src/analytics/incremental_view.cpp

    # CEP Engine – NFA pattern matching, EPL parser, window+aggregation pipeline (Phase 2)
    ../src/analytics/cep_engine.cpp

    # Streaming Aggregation Windows (Phase 2)
    ../src/analytics/streaming_window.cpp

    # Columnar Execution Engine with Vectorized Operator Pipeline (Phase 3)
    ../src/analytics/columnar_execution.cpp

    # LLVM-JIT compilation for hot aggregation paths (Phase 3, Issue #1482)
    ../src/analytics/jit_aggregation.cpp

    # Predictive Analytics & Time-Series Forecasting (Phase 3)
    ../src/analytics/forecasting.cpp

    # ML Serving Integration – ONNX Runtime + TF Serving (Issue #1476)
    ../src/analytics/ml_serving.cpp
    # Model Serving and Online Inference Pipeline (Issue #1477)
    ../src/analytics/model_serving.cpp

    # Arrow Flight RPC support for remote analytics (Issue #1472)
    ../src/analytics/arrow_flight.cpp

    # Process Pattern Matcher – conformance checking and behavioral similarity (Phase 1)
    ../src/analytics/process_pattern_matcher.cpp

    # Real-Time Anomaly Detection – Z-Score, IQR, Isolation Forest, LOF, Ensemble (Phase 3)
    ../src/analytics/anomaly_detection.cpp

    # AutoML – automated model selection, hyperparameter search, SHAP explanations (Phase 3)
    ../src/analytics/automl.cpp

    # Distributed Analytics Sharding – scatter-gather coordinator for distributed OLAP (Phase 3)
    ../src/analytics/distributed_analytics.cpp

    # GROUP BY aggregation engine with SafeIterator remediation (Sprint 7 Batch C)
    ../src/analytics/aggregation.cpp

    # Time-series data store with SafeIterator remediation (Sprint 7 Batch C)
    ../src/analytics/time_series.cpp

    # Expert System Engine – forward/backward chaining, KnowledgeBase, proof traces (Phase 4)
    ../src/analytics/knowledge_base.cpp
    ../src/analytics/expert_system_engine.cpp

    # LoRA Pattern Classifier – adapter-based CEP event classification with AutoML fallback (Phase 5)
    ../src/analytics/lora_pattern_classifier.cpp

    # Git-like MVCC Features
    # Phase 1: Named Snapshots
    ../src/transaction/snapshot_manager.cpp
    ../src/storage/pitr_manager.cpp
    
    # Phase 2: Diff Engine
    ../src/analytics/diff_engine.cpp
    
    # Phase 5: Three-Way Merge
    ../src/transaction/merge_engine.cpp
)

if(THEMIS_USE_EXTERNAL_TIMESERIES_PLUGIN)
    list(REMOVE_ITEM THEMIS_CORE_SOURCES
        ../src/timeseries/timeseries.cpp
        ../src/timeseries/timeseries_metrics.cpp
        ../src/timeseries/tsstore.cpp
        ../src/timeseries/gorilla.cpp
        ../src/timeseries/gorilla_simd.cpp
        ../src/timeseries/retention.cpp
        ../src/timeseries/continuous_agg.cpp
        ../src/timeseries/aggregate_scheduler.cpp
        ../src/timeseries/aggregate_scheduler_helper.cpp
        ../src/timeseries/query_optimizer.cpp
        ../src/timeseries/prometheus_remote_write.cpp
        ../src/timeseries/ts_auto_buffer.cpp
        ../src/timeseries/hypertable.cpp
        ../src/timeseries/aggregates.cpp
        ../src/timeseries/downsampling.cpp
        ../src/timeseries/ts_auto_buffer_adaptive.cpp
        ../src/timeseries/adaptive_flush_controller.cpp
        ../src/timeseries/encrypted_chunk_store.cpp
        ../src/timeseries/ts_encrypted_key_rotation.cpp
        ../src/timeseries/compression_selector.cpp
        ../src/timeseries/anomaly_detection.cpp
        ../src/timeseries/gap_fill.cpp
        ../src/timeseries/ts_stream_cursor.cpp
    )
    add_compile_definitions(THEMIS_USE_EXTERNAL_TIMESERIES_PLUGIN)
    message(STATUS "TimeSeries externalization active: integrated src/timeseries sources removed from themis_core")
endif()

if(THEMIS_USE_EXTERNAL_GEO_PLUGIN)
    list(REMOVE_ITEM THEMIS_CORE_SOURCES
        ../src/geo/cpu_backend.cpp
        ../src/geo/gpu_backend_stub.cpp
        ../src/geo/boost_cpu_exact_backend.cpp
        ../src/geo/geo_rtree.cpp
        ../src/geo/geo_clustering.cpp
        ../src/geo/spatial_join.cpp
        ../src/geo/raster.cpp
        ../src/geo/temporal_spatial_query.cpp
        ../src/geo/tile_server.cpp
        ../src/geo/device_detector.cpp
        ../src/geo/gpu_backend_production.cpp
        ../src/geo/geo_json_geometry.cpp
        ../src/geo/spatial_join_filter.cpp
        ../src/geo/rtree_cursor.cpp
        ../src/geo/temporal_spatial_query_builder.cpp
        ../src/geo/raster_query_interface.cpp
        ../src/geo/gpu_backend_cuda.cu
        ../src/geo/gpu_backend_hip.cpp
        ../src/geo/gpu_kernel_dispatcher_cpu.cpp
    )
    add_compile_definitions(THEMIS_USE_EXTERNAL_GEO_PLUGIN)
    message(STATUS "Geo externalization active: integrated src/geo sources removed from themis_core")
endif()

# Geo CUDA/HIP kernel dispatch — add the right implementation based on THEMIS_GEO_CUDA / THEMIS_GEO_HIP.
if(THEMIS_USE_EXTERNAL_GEO_PLUGIN)
    message(STATUS "Geo externalization active: GPU dispatcher is expected from external plugin")
elseif(THEMIS_GEO_CUDA)
    list(APPEND THEMIS_CORE_SOURCES ../src/geo/gpu_backend_cuda.cu)
elseif(THEMIS_GEO_HIP)
    list(APPEND THEMIS_CORE_SOURCES ../src/geo/gpu_backend_hip.cpp)
else()
    list(APPEND THEMIS_CORE_SOURCES ../src/geo/gpu_kernel_dispatcher_cpu.cpp)
endif()

# HTTP Server API handlers (only if HTTP server enabled)
if(THEMIS_ENABLE_HTTP_SERVER)
    list(APPEND THEMIS_CORE_SOURCES
        ../src/server/diff_api_handler.cpp
        ../src/server/merge_api_handler.cpp
        ../src/server/pitr_api_handler.cpp
    )
endif()

# Phase 2 Performance Optimizations (PR #156/#157)
# Conditionally add Phase 2 optimization sources when enabled
list(APPEND THEMIS_CORE_SOURCES
    ../src/performance/phase2_feature_flags.cpp
)

if(THEMIS_ENABLE_WISCKEY)
    message(STATUS "Including WiscKey key/value separation (THEMIS_ENABLE_WISCKEY=ON)")
    list(APPEND THEMIS_CORE_SOURCES ../src/performance/wisckey.cpp)
endif()

if(THEMIS_ENABLE_DOSTOEVSKY)
    message(STATUS "Including Dostoevsky adaptive LSM (THEMIS_ENABLE_DOSTOEVSKY=ON)")
    list(APPEND THEMIS_CORE_SOURCES ../src/performance/dostoevsky.cpp)
endif()

if(THEMIS_ENABLE_CICADA)
    message(STATUS "Including Cicada optimistic CC (THEMIS_ENABLE_CICADA=ON)")
    list(APPEND THEMIS_CORE_SOURCES ../src/performance/cicada.cpp)
endif()

if(THEMIS_ENABLE_LIGRA)
    message(STATUS "Including Ligra graph processing (THEMIS_ENABLE_LIGRA=ON)")
    list(APPEND THEMIS_CORE_SOURCES ../src/performance/ligra.cpp)
endif()

if(THEMIS_ENABLE_RABITQ)
    message(STATUS "Including RaBitQ vector quantization (THEMIS_ENABLE_RABITQ=ON)")
    list(APPEND THEMIS_CORE_SOURCES ../src/performance/rabitq.cpp)
endif()

# Phase 3 Performance Optimizations (PR #156/#157)
# Conditionally add Phase 3 optimization sources when enabled
list(APPEND THEMIS_CORE_SOURCES
    ../src/performance/phase3/feature_flags.cpp
    # per_query_cost_model is always compiled because query_optimizer.cpp
    # unconditionally links against it (attachPerQueryCostModel, executeOptimizedKeysWithCost, etc.)
    ../src/performance/phase3/per_query_cost_model.cpp
    # memory_pressure is always compiled: uses only platform OS memory APIs
    # (Linux: /proc/meminfo, macOS: sysctl/host_statistics64, Windows: GlobalMemoryStatusEx).
    ../src/performance/phase3/memory_pressure.cpp
    # adaptive_batch_tuner is always compiled: pure CPU, no GPU required
    ../src/performance/phase3/adaptive_batch_tuner.cpp
)

if(THEMIS_ENABLE_DISKANN)
    message(STATUS "Including DiskANN billion-scale vector search (THEMIS_ENABLE_DISKANN=ON)")
    list(APPEND THEMIS_CORE_SOURCES ../src/performance/phase3/diskann.cpp)
endif()

if(THEMIS_ENABLE_BWTREE)
    message(STATUS "Including Bw-Tree lock-free index (THEMIS_ENABLE_BWTREE=ON)")
    list(APPEND THEMIS_CORE_SOURCES ../src/performance/phase3/bwtree.cpp)
endif()

if(THEMIS_ENABLE_SPLINTERDB)
    message(STATUS "Including SplinterDB concurrent compaction (THEMIS_ENABLE_SPLINTERDB=ON)")
    list(APPEND THEMIS_CORE_SOURCES ../src/performance/phase3/splinterdb.cpp)
endif()

if(THEMIS_ENABLE_GUNROCK)
    message(STATUS "Including Gunrock GPU graph analytics (THEMIS_ENABLE_GUNROCK=ON)")
    list(APPEND THEMIS_CORE_SOURCES ../src/performance/phase3/gunrock.cpp)
endif()

# Keep Bao implementation available in all builds because core/network/tests
# may reference BaoOptimizer symbols even when THEMIS_ENABLE_BAO is OFF.
list(APPEND THEMIS_CORE_SOURCES ../src/performance/phase3/bao.cpp)
if(THEMIS_ENABLE_BAO)
    message(STATUS "Including Bao ML query optimizer (THEMIS_ENABLE_BAO=ON)")
endif()

if(THEMIS_ENABLE_PER_QUERY_COST_MODEL)
    message(STATUS "Per-query cost model integration enabled (THEMIS_ENABLE_PER_QUERY_COST_MODEL=ON)")
endif()

# Phase 4 Performance Optimizations (Issue #2424)
# Conditionally add Phase 4 optimization sources when enabled
list(APPEND THEMIS_CORE_SOURCES
    ../src/performance/phase4/feature_flags.cpp
    ../src/performance/phase4/pmu_counters.cpp
    ../src/performance/numa_memory_manager.cpp
    ../src/performance/advanced_cache_manager.cpp
    ../src/performance/workload_adaptive_optimizer.cpp
)

if(THEMIS_ENABLE_PMEM)
    message(STATUS "Including Persistent Memory aware storage layout (THEMIS_ENABLE_PMEM=ON)")
    list(APPEND THEMIS_CORE_SOURCES ../src/performance/phase4/pmem_storage.cpp)
endif()

if(THEMIS_ENABLE_PMU_COUNTERS)
    message(STATUS "Hardware PMU counter integration enabled (THEMIS_ENABLE_PMU_COUNTERS=ON)")
    # pmu_counters.cpp is already compiled unconditionally above (provides graceful stub
    # fallbacks when perf_event_open is unavailable).  This flag only adds the compile
    # definition that activates the real PMU paths inside pmu_counters.cpp.
    add_compile_definitions(THEMIS_ENABLE_PMU_COUNTERS)
endif()

if(THEMIS_ENABLE_IO_URING)
    message(STATUS "Including io_uring zero-copy I/O path (THEMIS_ENABLE_IO_URING=ON)")
    list(APPEND THEMIS_CORE_SOURCES ../src/performance/phase4/io_uring_zero_copy.cpp)
    add_compile_definitions(THEMIS_ENABLE_IO_URING)
endif()

# ============================================================================
# MODULAR SOURCE FILES (organized by feature/component)
# ============================================================================
include(${CMAKE_CURRENT_SOURCE_DIR}/AccelerationBackends.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/ContentProcessors.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/LLMIntegration.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/LegalTraining.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/DistributedTraining.cmake)
if(THEMIS_BUILD_CHIMERA)
    list(APPEND THEMIS_CORE_SOURCES
        ${CMAKE_SOURCE_DIR}/src/performance/chimera_exporter.cpp
    )
    include(${CMAKE_CURRENT_SOURCE_DIR}/ChimeraAdapters.cmake)
endif()
include(${CMAKE_CURRENT_SOURCE_DIR}/VoiceAssistant.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/BlobStorage.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/BufferManagement.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/ErrorHealthServices.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/IndexQueryEnhancements.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/EditionFeatures.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/StorageEnhancements.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/RPCServices.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/MiscellaneousFeatures.cmake)

# Conditionally add LLM sources (v1.3.0+) only when enabled
if(THEMIS_ENABLE_LLM)
    message(STATUS "Including LLM server and core sources (THEMIS_ENABLE_LLM=ON)")
    list(APPEND THEMIS_CORE_SOURCES
        # Server/API (HTTP + optional gRPC)
    ../src/server/llm_api_handler.cpp
    # AQL handlers (LLM-enabled path)
    ../src/aql/llm_aql_handler.cpp
    ../src/aql/aql_confidence_scorer.cpp
    ../src/aql/aql_query_builder.cpp
    ../src/aql/aql_query_validator.cpp
    ../src/aql/aql_optimizer_advisor.cpp
    ../src/aql/aql_query_template_library.cpp
    ../src/aql/aql_fewshot_example_library.cpp
    ../src/aql/aql_conversation_context.cpp
    ../src/aql/aql_schema_provider.cpp
    ../src/aql/aql_lora_finetuner.cpp
    ../src/aql/aql_migration_assistant.cpp
    ../src/aql/aql_autocomplete.cpp
    ../src/aql/aql_agent.cpp
    ../src/aql/aql_query_diff_explainer.cpp
    ../src/aql/aql_model_router.cpp
    ../src/aql/aql_rollback_suggester.cpp
    ../src/aql/aql_ingestion_bridge.cpp
    ../src/aql/llm_aql_embedding_bridge.cpp

        # Optional: gRPC service and generated proto headers (guarded by gRPC)
        # Only add when gRPC is available, otherwise skip to prevent missing headers
    )

    find_package(gRPC CONFIG QUIET)
    if(gRPC_FOUND)
        message(STATUS "gRPC found - enabling LLM gRPC service")
            # TEMP: gRPC proto generation not yet configured - comment out until ready
            # list(APPEND THEMIS_CORE_SOURCES
            #     ../src/server/llm_grpc_service.cpp
            #     /proto/llm_service.proto
            # )
        # Note: Proto generation should be configured in a separate section when needed
    else()
        message(STATUS "gRPC not found - disabling LLM gRPC service and proto generation")
    endif()
        # TEMP: Disable llm_grpc_service until proto generation is configured
        # list(REMOVE_ITEM THEMIS_CORE_SOURCES ../src/server/llm_grpc_service.cpp)

    # LLM core components
    list(APPEND THEMIS_CORE_SOURCES
    ../src/llm/llm_interaction_store.cpp
    ../src/llm/prompt_manager.cpp
    ../src/llm/block_table.cpp
    ../src/llm/paged_kv_cache.cpp
    ../src/llm/adapter_load_balancer.cpp
    ../src/llm/lora_framework/resource_profiler.cpp
    ../src/llm/lora_framework/model_compatibility.cpp
    ../src/llm/lora_framework/embedding_provider.cpp
    ../src/llm/llm_model_audit_logger.cpp
    ../src/llm/llama_lora_adapter.cpp
    ../src/llm/llama_grammar_adapter.cpp
    ../src/llm/aql_train_parser.cpp
    # GPU VRAM Allocation (vLLM-inspired)
    ../src/llm/adaptive_vram_allocator.cpp
    ../src/llm/active_vram_allocator.cpp
    ../src/llm/multi_gpu_memory_coordinator.cpp
    ../src/llm/paged_kv_cache_manager.cpp
    ../src/llm/mixed_precision_inference.cpp
    # Multi-GPU Communication Backends (NCCL/RCCL - stub implementations when not available)
    ../src/llm/lora_framework/nccl_backend.cpp
    ../src/llm/lora_framework/rccl_backend.cpp
    # SSM-hybrid Phase 2: Episodic memory and RocksDB persistence
    ../src/aql/llm_extractive_compressor.cpp
    )

    # GPU LoRA/VRAM implementation (GPU-only)
    if(THEMIS_ENABLE_GPU)
        list(APPEND THEMIS_CORE_SOURCES
            ../src/llm/lora_framework/gpu_tensor.cpp
            ../src/llm/lora_framework/vram_allocator.cpp
            ../src/llm/lora_framework/gpu_lora_layers.cpp
            ../src/llm/lora_framework/paged_memory_manager.cpp
            ../src/llm/lora_framework/custom_allreduce.cpp
            ../src/llm/lora_framework/multi_gpu.cpp
            ../src/llm/lora_framework/multi_gpu_trainer.cpp
            ../src/llm/lora_framework/multi_gpu_lora_layer.cpp
        )
    endif()
    
    # RAG enhancement modules (Phase 1: Knowledge Gap Detector)
    list(APPEND THEMIS_CORE_SOURCES
    ../src/rag/knowledge_gap_detector.cpp
    # Phase 2: Streaming Retrieval & Re-Ranking
    ../src/rag/streaming_retriever.cpp
    ../src/rag/reranker.cpp
    # Phase 2: Hallucination Rate Tracking Dashboard
    ../src/rag/hallucination_dashboard.cpp
    # Phase 3: Citation Highlighting
    ../src/rag/citation_highlighter.cpp
    # Phase 3: Configurable chunk size and overlap for document splitting
    ../src/rag/document_splitter.cpp
    # Phase 3: Hybrid Retrieval (BM25 + vector, configurable RRF weights)
    ../src/rag/hybrid_retriever.cpp
    # Lightweight LoRA-based re-ranking for retrieval pipelines
    ../src/rag/lora_enhanced_retriever.cpp
    # Continuous Learning Orchestrator (Phase 2)
    ../src/rag/learning_metrics.cpp
    ../src/rag/bayesian_optimizer.cpp
    ../src/rag/ab_testing_framework.cpp
    ../src/rag/continuous_learning_orchestrator.cpp
    # IMPL-A1: Database-domain auto-labeler for LoRA training (S-2)
    ../src/training/database_domain_auto_labeler.cpp
    # AdaLoRA ↔ TT-format bridge — zero-copy adapter lifecycle management (STUB #271)
    ../src/training/adalora_tt_bridge.cpp
    # Phase 4: Agentic RAG with iterative retrieval loops
    ../src/rag/agentic_rag.cpp
    # Delegate evaluator and relay simulator used by AgenticRAG relay paths
    ../src/rag/delegate_evaluator.cpp
    # Phase 4: Multi-modal RAG (image + text retrieval)
    ../src/rag/multimodal_rag.cpp
    # Phase 5: Distributed evaluation and security
    ../src/rag/distributed_rag_evaluator.cpp
    ../src/rag/prompt_injection_detector.cpp
    # Phase 5: Evaluation pipeline (evaluation report, cache, calibration, batch evaluator, bias detection)
    ../src/rag/evaluation_report_exporter.cpp
    ../src/rag/evaluation_cache.cpp
    ../src/rag/calibration_manager.cpp
    ../src/rag/batch_evaluator.cpp
    ../src/rag/bias_detector.cpp
    # Phase 1-3: Core evaluator building blocks (no direct LLM header dependency)
    ../src/rag/rag_context_assembler.cpp
    ../src/rag/continuous_learning_client.cpp
    ../src/rag/http_metrics_client.cpp
    ../src/rag/judge_config.cpp
    ../src/rag/knowledge_graph_retriever.cpp
    ../src/rag/onnx_model_loader.cpp
    ../src/rag/prompt_templates.cpp
    ../src/rag/response_parser.cpp
    # Phase 7: Multi-hop reasoning and adaptive retrieval
    ../src/rag/multi_hop_reasoner.cpp
    ../src/rag/adaptive_retrieval.cpp
    # RAGIngestionBridge — toolbox/RAG integration layer
    ../src/rag/rag_ingestion_bridge.cpp
    # ExplainabilityReasonBuilder — Layer-9 causal explanation (IMPL-B9)
    ../src/rag/explainability_reason_builder.cpp
    # DK-1: Distributed Knowledge Layer 11 — all four sub-components (S-1/S-9)
    ../src/distributed_knowledge/adapter_capability_announcement.cpp
    ../src/distributed_knowledge/lora_federation_coordinator.cpp
    ../src/distributed_knowledge/federated_rag_merger.cpp
    ../src/distributed_knowledge/cross_shard_feedback_sync.cpp
    )
endif()  # Close if(THEMIS_ENABLE_LLM)

# Ensure minimal LLM components are available when referenced from http_server.
# These have lightweight dependencies and avoid undefined references if LLM is disabled.
if(NOT THEMIS_ENABLE_LLM)
    list(APPEND THEMIS_CORE_SOURCES
    ../src/llm/llm_interaction_store.cpp
    ../src/llm/prompt_manager.cpp
    ../src/llm/embedded_llm_stub.cpp
    ../src/llm/ethical_guidelines_manager.cpp
    # Citation highlighting has no LLM dependency; always include so non-LLM
    # builds can use and test the CitationHighlighter class.
    ../src/rag/citation_highlighter.cpp
    )
endif()

_themis_normalize_sources(THEMIS_CORE_SOURCES)

# ============================================================================
# Wire Protocol V1 Protobuf Generation
# (required by src/themis/wire_protocol_server.cpp)
# ============================================================================
find_package(Protobuf CONFIG QUIET)
if(NOT Protobuf_FOUND)
    find_package(Protobuf MODULE QUIET)
endif()

if(Protobuf_FOUND)
    set(WIRE_V1_PROTO ${THEMIS_ROOT_DIR}/src/network/themis_wire_v1.proto)
    set(WIRE_V1_PROTO_GEN_DIR ${CMAKE_BINARY_DIR}/proto_generated)
    file(MAKE_DIRECTORY ${WIRE_V1_PROTO_GEN_DIR})

    set(WIRE_V1_PROTO_SRC ${WIRE_V1_PROTO_GEN_DIR}/themis_wire_v1.pb.cc)
    set(WIRE_V1_PROTO_HDR ${WIRE_V1_PROTO_GEN_DIR}/themis_wire_v1.pb.h)

    add_custom_command(
        OUTPUT  ${WIRE_V1_PROTO_SRC} ${WIRE_V1_PROTO_HDR}
        COMMAND $<TARGET_FILE:protobuf::protoc>
        ARGS    --cpp_out ${WIRE_V1_PROTO_GEN_DIR}
                -I ${THEMIS_ROOT_DIR}/src/network
                ${WIRE_V1_PROTO}
        DEPENDS ${WIRE_V1_PROTO} protobuf::protoc
        COMMENT "Generating protobuf C++ sources for themis_wire_v1.proto"
        VERBATIM
    )

    # Generated protobuf code may legitimately use size_t->int paths from protoc output.
    # Limit suppression strictly to generated translation units.
    if(MSVC)
        set_source_files_properties(${WIRE_V1_PROTO_SRC}
            PROPERTIES COMPILE_OPTIONS "/wd4267")
    endif()

    list(APPEND THEMIS_CORE_SOURCES ${WIRE_V1_PROTO_SRC})
    set(THEMIS_WIRE_V1_PROTO_AVAILABLE ON)
    message(STATUS "themis_wire_v1.proto: protobuf generation configured")
else()
    message(STATUS "Protobuf not found – themis_wire_v1.pb.h will not be generated; "
                   "wire_protocol_server (themis::wire) compiles without proto handlers")
endif()

# ============================================================================
# BUILD MODE SELECTION: Modular vs Monolithic
# ============================================================================
# Check if modular build is enabled (v1.4.0+ feature)
if(THEMIS_BUILD_MODULAR)
    message(STATUS "==============================================")
    message(STATUS "MODULAR BUILD MODE ENABLED")
    message(STATUS "Building ThemisDB as separate module libraries")
    message(STATUS "==============================================")
    
    # Generate shard RPC protobuf stubs BEFORE modular build.
    # Protobuf messages are always required; gRPC service stubs are optional.
    find_package(Protobuf CONFIG)
    if(Protobuf_FOUND)
        set(SHARD_PROTO ${THEMIS_ROOT_DIR}/proto/sharding/shard_rpc.proto)
        set(SHARD_PROTO_GEN_DIR ${CMAKE_BINARY_DIR}/proto_generated)
        file(MAKE_DIRECTORY ${SHARD_PROTO_GEN_DIR})

        set(SHARD_PROTO_SRCS
            ${SHARD_PROTO_GEN_DIR}/shard_rpc.pb.cc
        )
        set(SHARD_PROTO_HDRS
            ${SHARD_PROTO_GEN_DIR}/shard_rpc.pb.h
        )

        if(THEMIS_ENABLE_GRPC)
            find_package(gRPC CONFIG)
            if(gRPC_FOUND)
                list(APPEND SHARD_PROTO_SRCS ${SHARD_PROTO_GEN_DIR}/shard_rpc.grpc.pb.cc)
                list(APPEND SHARD_PROTO_HDRS ${SHARD_PROTO_GEN_DIR}/shard_rpc.grpc.pb.h)
                add_custom_command(
                    OUTPUT ${SHARD_PROTO_SRCS} ${SHARD_PROTO_HDRS}
                    COMMAND $<TARGET_FILE:protobuf::protoc>
                    ARGS --cpp_out ${SHARD_PROTO_GEN_DIR}
                         --grpc_out ${SHARD_PROTO_GEN_DIR}
                         --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>
                         -I ${THEMIS_ROOT_DIR}/proto/sharding
                         ${SHARD_PROTO}
                    DEPENDS ${SHARD_PROTO} protobuf::protoc gRPC::grpc_cpp_plugin
                    COMMENT "Generating protobuf and gRPC sources for shard_rpc.proto"
                    VERBATIM
                )
            else()
                message(WARNING "gRPC not found - shard RPC service stubs disabled; generating protobuf messages only")
                add_custom_command(
                    OUTPUT ${SHARD_PROTO_SRCS} ${SHARD_PROTO_HDRS}
                    COMMAND $<TARGET_FILE:protobuf::protoc>
                    ARGS --cpp_out ${SHARD_PROTO_GEN_DIR}
                         -I ${THEMIS_ROOT_DIR}/proto/sharding
                         ${SHARD_PROTO}
                    DEPENDS ${SHARD_PROTO} protobuf::protoc
                    COMMENT "Generating protobuf sources for shard_rpc.proto"
                    VERBATIM
                )
            endif()
        else()
            add_custom_command(
                OUTPUT ${SHARD_PROTO_SRCS} ${SHARD_PROTO_HDRS}
                COMMAND $<TARGET_FILE:protobuf::protoc>
                ARGS --cpp_out ${SHARD_PROTO_GEN_DIR}
                     -I ${THEMIS_ROOT_DIR}/proto/sharding
                     ${SHARD_PROTO}
                DEPENDS ${SHARD_PROTO} protobuf::protoc
                COMMENT "Generating protobuf sources for shard_rpc.proto"
                VERBATIM
            )
        endif()

        add_library(themis_shard_proto STATIC ${SHARD_PROTO_SRCS} ${SHARD_PROTO_HDRS})
        target_include_directories(themis_shard_proto PUBLIC ${SHARD_PROTO_GEN_DIR})
        target_link_libraries(themis_shard_proto PUBLIC protobuf::libprotobuf)
        if(THEMIS_ENABLE_GRPC AND gRPC_FOUND)
            target_link_libraries(themis_shard_proto PUBLIC gRPC::grpc++)
        endif()

        # Suppress warnings in generated protobuf files
        if(MSVC)
            target_compile_options(themis_shard_proto PRIVATE /wd4996 /wd4018 /wd4244 /wd4267)
        else()
            target_compile_options(themis_shard_proto PRIVATE -Wno-unused-parameter -Wno-deprecated-declarations -Wno-conversion)
        endif()

        message(STATUS "themis_shard_proto shared library created for inter-shard communication (protobuf messages always, gRPC optional)")

        # Generate ThemisDB API protobuf/grpc stubs for modular network module
        if(THEMIS_ENABLE_GRPC)
            find_package(gRPC CONFIG)
            if(gRPC_FOUND)
                set(THEMISDB_API_PROTO ${THEMIS_ROOT_DIR}/proto/themisdb.proto)
                set(THEMISDB_API_PROTO_GEN_DIR ${CMAKE_BINARY_DIR}/proto_generated)
                file(MAKE_DIRECTORY ${THEMISDB_API_PROTO_GEN_DIR})

                set(THEMISDB_API_PROTO_SRCS
                    ${THEMISDB_API_PROTO_GEN_DIR}/themisdb.pb.cc
                    ${THEMISDB_API_PROTO_GEN_DIR}/themisdb.grpc.pb.cc
                )
                set(THEMISDB_API_PROTO_HDRS
                    ${THEMISDB_API_PROTO_GEN_DIR}/themisdb.pb.h
                    ${THEMISDB_API_PROTO_GEN_DIR}/themisdb.grpc.pb.h
                )

                add_custom_command(
                    OUTPUT ${THEMISDB_API_PROTO_SRCS} ${THEMISDB_API_PROTO_HDRS}
                    COMMAND $<TARGET_FILE:protobuf::protoc>
                    ARGS --cpp_out ${THEMISDB_API_PROTO_GEN_DIR}
                         --grpc_out ${THEMISDB_API_PROTO_GEN_DIR}
                         --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>
                         -I ${THEMIS_ROOT_DIR}/proto
                         ${THEMISDB_API_PROTO}
                    DEPENDS ${THEMISDB_API_PROTO} protobuf::protoc gRPC::grpc_cpp_plugin
                    COMMENT "Generating protobuf and gRPC sources for themisdb.proto (modular build)"
                    VERBATIM
                )
                set(THEMIS_API_PROTO_AVAILABLE ON)
                message(STATUS "themisdb API proto generation configured for modular build")
            else()
                message(WARNING "gRPC not found - themisdb API proto stubs will not be generated")
            endif()
        endif()

    endif()

    # Call the modular build function to create all module targets
    themis_build_modular()
    
    # Wire Protocol V1 protobuf compile flag and link for the network module
    # (mirrors the monolithic build setup below; required by src/themis/wire_protocol_server.cpp)
    if(THEMIS_WIRE_V1_PROTO_AVAILABLE AND TARGET themis_network)
        target_sources(themis_network PRIVATE ${WIRE_V1_PROTO_SRC})
        target_compile_definitions(themis_network PUBLIC THEMIS_WIRE_V1_PROTO_AVAILABLE)
        target_link_libraries(themis_network PRIVATE protobuf::libprotobuf)
        target_include_directories(themis_network PRIVATE ${CMAKE_BINARY_DIR}/proto_generated)
        # Proto-generated files use #pragma init_seg(lib) and file-scope const arrays;
        # they must not be merged into a Unity batch (causes C2356/C2374/C2086).
        if(MSVC)
            set_source_files_properties(${WIRE_V1_PROTO_SRC}
                PROPERTIES
                    SKIP_UNITY_BUILD_INCLUSION ON
                    COMPILE_OPTIONS "/wd4267")
        endif()
        message(STATUS "THEMIS_WIRE_V1_PROTO_AVAILABLE: enabled for themis_network (themis::wire protobuf handlers active)")
    endif()

    if(THEMIS_API_PROTO_AVAILABLE AND TARGET themis_network)
        target_sources(themis_network PRIVATE ${THEMISDB_API_PROTO_SRCS})
        # gRPC::grpc_reflection may not be exported in vcpkg; use gRPC::grpc++ only
        target_link_libraries(themis_network PRIVATE gRPC::grpc++ protobuf::libprotobuf)
        target_include_directories(themis_network PRIVATE ${CMAKE_BINARY_DIR}/proto_generated)
        # Proto-generated files must not be in a Unity batch (C2356/C2374/C2086).
        if(MSVC)
            set_source_files_properties(${THEMISDB_API_PROTO_SRCS}
                PROPERTIES
                    SKIP_UNITY_BUILD_INCLUSION ON
                    COMPILE_OPTIONS "/wd4267")
        endif()
        message(STATUS "THEMIS_API_PROTO_AVAILABLE: enabled for themis_network (ThemisDB gRPC API handlers active)")
    endif()

    # Create themis_core as an interface library that links all modules
    # This maintains compatibility with existing code that links to themis_core
    add_library(themis_core INTERFACE)
    target_link_libraries(themis_core INTERFACE ${THEMIS_ALL_MODULES})
    foreach(_themis_mod IN ITEMS
        themis_base
        themis_storage
        themis_query
        themis_security
        themis_transaction
        themis_network
        themis_sharding
        themis_timeseries
        themis_llm
        themis_geo
        themis_graph
        themis_content
    )
        if(TARGET ${_themis_mod})
            target_link_libraries(themis_core INTERFACE ${_themis_mod})
        endif()
    endforeach()

    # HTTP/3 / QUIC linkage for modular build.
    # QUIC code uses helpers from ngtcp2_crypto_ossl (e.g. ngtcp2_crypto_encrypt_cb)
    # that are not provided by ngtcp2::ngtcp2 alone.
    if(THEMIS_ENABLE_HTTP3 AND nghttp3_FOUND AND ngtcp2_FOUND)
        if(TARGET themis_network)
            target_link_libraries(themis_network PUBLIC nghttp3::nghttp3 ngtcp2::ngtcp2)
            if(TARGET ngtcp2::ngtcp2_crypto_ossl)
                target_link_libraries(themis_network PUBLIC ngtcp2::ngtcp2_crypto_ossl)
            endif()
            target_compile_definitions(themis_network PUBLIC THEMIS_ENABLE_HTTP3)
        endif()
    endif()

    message(STATUS "Created themis_core interface library linking: ${THEMIS_ALL_MODULES}")
else()
    # Traditional monolithic build (default for v1.3.x and earlier)
    message(STATUS "Building ThemisDB as monolithic themis_core library")

# Define core library early so subsequent target_* commands are valid
set(_THEMIS_CORE_LIBTYPE STATIC)
if(THEMIS_CORE_SHARED)
    set(_THEMIS_CORE_LIBTYPE SHARED)
endif()
add_library(themis_core ${_THEMIS_CORE_LIBTYPE} ${THEMIS_CORE_SOURCES})

# Enable Unity Build only on MSVC to reduce object file count
# (fixes MSVC LNK1189 with 65535 objects limit).
# On non-MSVC (e.g. Linux/GCC), unity builds can trigger ODR/redefinition
# collisions across large translation units.
if(MSVC)
    # Disable Unity Build on MSVC to avoid include-order conflicts (winsock.h vs winsock2.h)
    # caused by large unity translation units merging sources with different include order.
    set_target_properties(themis_core PROPERTIES
        UNITY_BUILD OFF
    )
else()
    set_target_properties(themis_core PROPERTIES
        UNITY_BUILD OFF
    )
endif()

# Base include directories for core headers
# Needed to resolve includes like "sharding/...", "llm/...", etc.
target_include_directories(themis_core
    PUBLIC
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
        ${CMAKE_BINARY_DIR}/proto_generated
)

if(THEMIS_HAS_IO_URING AND THEMIS_IO_URING_LIB)
    target_link_libraries(themis_core PRIVATE ${THEMIS_IO_URING_LIB})
    if(THEMIS_IO_URING_INCLUDE)
        target_include_directories(themis_core PRIVATE ${THEMIS_IO_URING_INCLUDE})
    endif()
endif()

# Define compile definitions for static builds (THEMIS_BASE_EXPORTS)
# to ensure THEMIS_BASE_API macros resolve to dllexport/empty, not dllimport
if(NOT THEMIS_CORE_SHARED)
    target_compile_definitions(themis_core PRIVATE THEMIS_BASE_EXPORTS)
endif()

# Per-file compile options for heavy AQL parser translation units (MSVC-only)
if(MSVC)
    set_source_files_properties(src/query/aql_parser.cpp PROPERTIES COMPILE_OPTIONS "/O1;/bigobj;/Bt+")
    set_source_files_properties(src/query/aql_parser_json.cpp PROPERTIES COMPILE_OPTIONS "/O1;/bigobj")
    
    # Exclude files with macro/ODR conflicts from Unity Build
    set_source_files_properties(
        ../src/content/audio_processor.cpp
        ../src/content/cad_processor.cpp
        ../src/content/geo_processor.cpp
        ../src/content/image_processor.cpp
        ../src/content/video_processor.cpp
        ../src/observability/alertmanager.cpp
        ../src/plugins/plugin_registry.cpp
        ../src/sharding/hot_spare_manager.cpp
        ../src/sharding/signed_request.cpp
        ../src/server/monitoring_api_handler.cpp
        ../src/server/index_api_handler.cpp
        ../src/governance/policy_validation.cpp
        ../src/governance/policy_validator.cpp
        ../src/governance/compliance_reporting.cpp
        ../src/governance/compliance_reporter.cpp
        ../src/governance/review_scheduler.cpp
        ../src/governance/policy_review.cpp
        ../src/security/pii_redaction_policy.cpp
        ../src/analytics/columnar_execution.cpp
        ../src/analytics/streaming_window.cpp
        ../src/analytics/cep_engine.cpp
        ../src/analytics/incremental_view.cpp
        ../src/observability/continuous_profiler.cpp
        ../src/observability/distributed_flame_graph.cpp
        ../src/importers/mysql_importer.cpp
        ../src/importers/sqlite_importer.cpp
        ../src/importers/flatfile_importer.cpp
        ../src/importers/schema_validator.cpp
        ../src/importers/postgres_importer.cpp
        ../src/importers/mongo_importer.cpp
        ../src/process/process_graph_rag.cpp
        ../src/process/process_community_detector.cpp
        ../src/process/process_light_retriever.cpp
        ../src/process/object_centric_tracer.cpp
        ../src/process/process_model_manager.cpp
        ../src/process/cmmn_serializer.cpp
        ../src/process/fim_importer.cpp
        ../src/cache/distributed_cache_coordinator.cpp
        # ../src/cache/redis_cache_coordinator.cpp   # Legacy hiredis impl; conflicts with distributed_cache_coordinator
        PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON
    )
    set_source_files_properties(
        ../src/server/monitoring_api_handler.cpp
        ../src/server/index_api_handler.cpp
        PROPERTIES COMPILE_OPTIONS "/bigobj;/Zm200"
    )
endif()

# On Windows, export all symbols automatically for shared builds to avoid needing __declspec(dllexport)
if(MSVC AND THEMIS_CORE_SHARED)
    # Auto-export needs normal COFF objects; disable /GL (IPO) so __create_def can read symbols
    set_target_properties(themis_core PROPERTIES
        WINDOWS_EXPORT_ALL_SYMBOLS ON
        INTERPROCEDURAL_OPTIMIZATION FALSE
        VS_GLOBAL_WholeProgramOptimization "false"
    )
    target_compile_options(themis_core PRIVATE
        $<$<CONFIG:Release>:/GL->
        $<$<CONFIG:RelWithDebInfo>:/GL->
        $<$<CONFIG:MinSizeRel>:/GL->
    )
    target_link_options(themis_core PRIVATE
        $<$<CONFIG:Release>:/LTCG:OFF>
        $<$<CONFIG:RelWithDebInfo>:/LTCG:OFF>
        $<$<CONFIG:MinSizeRel>:/LTCG:OFF>
    )
endif()

# Add hardware acceleration backends based on build options
set(THEMIS_ACCELERATION_SOURCES "")

# CUDA backend
if(THEMIS_ENABLE_CUDA)
    find_package(CUDAToolkit REQUIRED)
    
    # Add CUDA source files
    list(APPEND THEMIS_ACCELERATION_SOURCES 
    ../src/acceleration/cuda_backend.cpp
    ../src/acceleration/cuda/ann_kernels.cu
    ../src/acceleration/cuda/vector_kernels.cu
    ../src/acceleration/cuda/tensor_core_matmul.cu
    ../src/acceleration/cuda/tensor_compression_routing_kernels.cu
    ../src/acceleration/cuda/geo_kernels.cu
    ../src/acceleration/cuda/cuda_hnsw_kernels.cu
    ../src/acceleration/cuda/graph_kernels.cu
    )
    
    target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_CUDA)
    target_include_directories(themis_core PRIVATE ${CUDAToolkit_INCLUDE_DIRS})
    target_link_libraries(themis_core PRIVATE CUDA::cudart CUDA::cuda_driver CUDA::cublas)
    
    # Link NCCL if available
    if(NCCL_FOUND)
        target_include_directories(themis_core PRIVATE ${NCCL_INCLUDE_DIRS})
        target_link_libraries(themis_core PRIVATE ${NCCL_LIBRARIES})
        message(STATUS "NCCL libraries linked: ${NCCL_LIBRARIES}")
    endif()
    
    message(STATUS "CUDA acceleration enabled")
    message(STATUS "  CUDA Version: ${CUDAToolkit_VERSION}")
    message(STATUS "  CUDA Include: ${CUDAToolkit_INCLUDE_DIRS}")

    # cuVS/RAFT GPU-accelerated ANN (IVF-Flat index build + search).
    # Activated with -DTHEMIS_ENABLE_CUVS=ON; requires the cuVS CMake package
    # (install via conda: `conda install -c rapidsai cuvs`).
    if(THEMIS_ENABLE_CUVS)
        find_package(cuvs CONFIG REQUIRED)
        target_link_libraries(themis_core PRIVATE cuvs::cuvs)
        target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_CUVS)
        message(STATUS "cuVS/RAFT ANN search enabled (THEMIS_ENABLE_CUVS=ON)")
    endif()
    
    # Propagate geo-CUDA flag to the build system.
    if(THEMIS_GEO_CUDA)
        target_compile_definitions(themis_core PUBLIC THEMIS_GEO_CUDA)
        message(STATUS "Geo CUDA kernel dispatch: ENABLED (THEMIS_GEO_CUDA=ON)")
    endif()
endif()

# Propagate geo-HIP flag to the build system (outside the CUDA block so it
# works for HIP-only builds where THEMIS_ENABLE_CUDA is OFF).
if(THEMIS_GEO_HIP)
    target_compile_definitions(themis_core PUBLIC THEMIS_GEO_HIP)
    message(STATUS "Geo HIP kernel dispatch: ENABLED (THEMIS_GEO_HIP=ON)")
endif()

# Graphics API backends
if(THEMIS_ENABLE_DIRECTX OR THEMIS_ENABLE_VULKAN OR THEMIS_ENABLE_OPENGL)
    list(APPEND THEMIS_ACCELERATION_SOURCES
        ../src/acceleration/graphics_backends.cpp
    )
    
    if(THEMIS_ENABLE_DIRECTX)
        # Add DirectX 12 compute pipeline sources
        list(APPEND THEMIS_ACCELERATION_SOURCES
            ../src/llm/lora_framework/directx_context.cpp
            ../src/llm/lora_framework/directx_buffer.cpp
            ../src/llm/lora_framework/directx_descriptors.cpp
            ../src/llm/lora_framework/directx_shader.cpp
            ../src/llm/lora_framework/directx_pipeline.cpp
            ../src/llm/lora_framework/kernels/directx_kernels.cpp
        )
        
        target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_DIRECTX)
        
        # Link DirectX 12 libraries (Windows only)
        if(WIN32)
            target_link_libraries(themis_core PRIVATE d3d12 dxgi)
            
            # Find DXC shader compiler (part of Windows SDK or DirectX Agility SDK)
            find_program(DXC_COMPILER dxc
                HINTS
                    "$ENV{WindowsSdkDir}bin/${CMAKE_VS_PLATFORM_NAME}"
                    "$ENV{WindowsSdkDir}bin/x64"
                    "$ENV{ProgramFiles\(x86\)}/Windows Kits/10/bin/10.0.22621.0/x64"
                    "$ENV{ProgramFiles\(x86\)}/Windows Kits/10/bin/10.0.19041.0/x64"
            )
            
            if(DXC_COMPILER)
                message(STATUS "DirectX 12 acceleration enabled")
                message(STATUS "  DirectX compute pipeline integrated")
                message(STATUS "  DXC compiler found: ${DXC_COMPILER}")
                
                # Set shader output directory
                set(SHADER_OUTPUT_DIR "${CMAKE_BINARY_DIR}/shaders/lora")
                file(MAKE_DIRECTORY ${SHADER_OUTPUT_DIR})
                
                # Compile HLSL shaders to DXIL bytecode
                set(SHADER_SOURCE_DIR "${THEMIS_ROOT_DIR}/src/acceleration/directx/shaders/lora")
                
                # Matmul shader
                add_custom_command(
                    OUTPUT ${SHADER_OUTPUT_DIR}/matmul.cso
                    COMMAND ${DXC_COMPILER} -T cs_6_0 -E main
                            ${SHADER_SOURCE_DIR}/matmul.hlsl
                            -Fo ${SHADER_OUTPUT_DIR}/matmul.cso
                    DEPENDS ${SHADER_SOURCE_DIR}/matmul.hlsl
                    COMMENT "Compiling matmul.hlsl to DXIL bytecode"
                    VERBATIM
                )
                
                # Elementwise shader
                add_custom_command(
                    OUTPUT ${SHADER_OUTPUT_DIR}/elementwise.cso
                    COMMAND ${DXC_COMPILER} -T cs_6_0 -E main
                            ${SHADER_SOURCE_DIR}/elementwise.hlsl
                            -Fo ${SHADER_OUTPUT_DIR}/elementwise.cso
                    DEPENDS ${SHADER_SOURCE_DIR}/elementwise.hlsl
                    COMMENT "Compiling elementwise.hlsl to DXIL bytecode"
                    VERBATIM
                )
                
                # Gradient shader
                add_custom_command(
                    OUTPUT ${SHADER_OUTPUT_DIR}/gradient.cso
                    COMMAND ${DXC_COMPILER} -T cs_6_0 -E main
                            ${SHADER_SOURCE_DIR}/gradient.hlsl
                            -Fo ${SHADER_OUTPUT_DIR}/gradient.cso
                    DEPENDS ${SHADER_SOURCE_DIR}/gradient.hlsl
                    COMMENT "Compiling gradient.hlsl to DXIL bytecode"
                    VERBATIM
                )
                
                # Custom target to compile all shaders
                add_custom_target(directx_shaders ALL
                    DEPENDS
                        ${SHADER_OUTPUT_DIR}/matmul.cso
                        ${SHADER_OUTPUT_DIR}/elementwise.cso
                        ${SHADER_OUTPUT_DIR}/gradient.cso
                    COMMENT "Building DirectX shaders"
                )
                
                # Install shaders to output directory
                install(FILES
                    ${SHADER_OUTPUT_DIR}/matmul.cso
                    ${SHADER_OUTPUT_DIR}/elementwise.cso
                    ${SHADER_OUTPUT_DIR}/gradient.cso
                    DESTINATION bin/shaders/lora
                    COMPONENT shaders
                )
                
                # Make themis_core depend on shader compilation
                add_dependencies(themis_core directx_shaders)
                
                # Compile ANN/Geospatial HLSL shaders (l2_distance, cosine_distance, etc.)
                if(EXISTS "${CMAKE_SOURCE_DIR}/src/acceleration/directx/shaders/CMakeLists.txt")
                    add_subdirectory(
                        "${CMAKE_SOURCE_DIR}/src/acceleration/directx/shaders"
                        "${CMAKE_BINARY_DIR}/directx_ann_geo_shaders"
                    )
                endif()
                
                message(STATUS "  Shaders: matmul.hlsl, elementwise.hlsl, gradient.hlsl → .cso (LoRA)")
                message(STATUS "  Shaders: l2_distance.hlsl, cosine_distance.hlsl, inner_product_distance.hlsl → .cso (ANN)")
                message(STATUS "  Shaders: haversine_distance.hlsl, point_in_polygon.hlsl → .cso (Geo)")
            else()
                message(WARNING "DXC compiler not found - DirectX shaders will not be compiled")
                message(WARNING "  Install Windows 10 SDK 10.0.19041.0+ or DirectX Agility SDK")
                message(WARNING "  DirectX backend will require pre-compiled shaders or fail at runtime")
            endif()
        else()
            message(WARNING "DirectX is only available on Windows")
        endif()
    endif()
    
    if(THEMIS_ENABLE_VULKAN)
        # Try to find Vulkan SDK
        find_package(Vulkan QUIET)
        if(Vulkan_FOUND)
            target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_VULKAN)
            target_include_directories(themis_core PRIVATE ${Vulkan_INCLUDE_DIRS})
            target_link_libraries(themis_core PRIVATE Vulkan::Vulkan)
            list(APPEND THEMIS_ACCELERATION_SOURCES ../src/acceleration/vulkan_backend_full.cpp)
            if(EXISTS "${CMAKE_SOURCE_DIR}/src/acceleration/vulkan/shaders/CMakeLists.txt")
                add_subdirectory(
                    "${CMAKE_SOURCE_DIR}/src/acceleration/vulkan/shaders"
                    "${CMAKE_BINARY_DIR}/vulkan_shaders")
            endif()
            message(STATUS "Vulkan acceleration enabled")
            message(STATUS "  Vulkan Version: ${Vulkan_VERSION}")
        else()
            message(WARNING "Vulkan SDK not found, Vulkan support disabled")
        endif()
    endif()
    
    if(THEMIS_ENABLE_OPENGL)
        target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_OPENGL)
        # Link dl for dynamic loading of libEGL/libGL at runtime (Linux/Unix)
        if(UNIX AND NOT APPLE)
            target_link_libraries(themis_core PRIVATE dl)
        endif()
        message(STATUS "OpenGL acceleration enabled (EGL/GL 4.3+ compute shaders with CPU fallback)")
    endif()
endif()

# ZLUDA backend (AMD GPUs via CUDA API compatibility layer)
if(THEMIS_ENABLE_ZLUDA)
    list(APPEND THEMIS_ACCELERATION_SOURCES ../src/acceleration/zluda_backend.cpp)
    target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_ZLUDA)
    message(STATUS "ZLUDA acceleration enabled (AMD GPU via CUDA compatibility layer)")
endif()

# Metal backend (Apple Silicon / macOS)
if(THEMIS_ENABLE_METAL AND APPLE)
    list(APPEND THEMIS_ACCELERATION_SOURCES ../src/acceleration/metal_backend.mm)
    target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_METAL)
    target_link_libraries(themis_core PRIVATE "-framework Metal" "-framework Foundation")
    message(STATUS "Metal acceleration enabled (Apple Silicon / macOS)")
endif()

# Vulkan full backend (cross-platform GPU compute)
if(THEMIS_ENABLE_VULKAN)
    list(APPEND THEMIS_ACCELERATION_SOURCES ../src/acceleration/vulkan_backend_full.cpp)
    message(STATUS "Vulkan full backend enabled (cross-platform GPU compute)")
endif()

# Add acceleration sources to core if any were selected
_themis_normalize_sources(THEMIS_ACCELERATION_SOURCES)
if(THEMIS_ACCELERATION_SOURCES)
    target_sources(themis_core PRIVATE ${THEMIS_ACCELERATION_SOURCES})
endif()

# Main library already defined above (before feature-specific target modifications)

if(THEMIS_ENABLE_HSM_REAL)
    target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_HSM_REAL)
    target_sources(themis_core PRIVATE ../src/security/hsm_provider_pkcs11.cpp)
endif()

# Wire Protocol V1 protobuf compile flag and link
if(THEMIS_WIRE_V1_PROTO_AVAILABLE)
    target_compile_definitions(themis_core PUBLIC THEMIS_WIRE_V1_PROTO_AVAILABLE)
    target_link_libraries(themis_core PRIVATE protobuf::libprotobuf)
    message(STATUS "THEMIS_WIRE_V1_PROTO_AVAILABLE: enabled (themis::wire protobuf handlers active)")
endif()

# Geo feature flags and Boost.Geometry support
if(THEMIS_GEO)
    target_compile_definitions(themis_core PUBLIC THEMIS_GEO_ENABLED)
    # Try to find Boost for Boost.Geometry backend
    find_package(Boost QUIET)
    if(Boost_FOUND)
        target_compile_definitions(themis_core PRIVATE THEMIS_GEO_BOOST_BACKEND)
        target_include_directories(themis_core PRIVATE ${Boost_INCLUDE_DIRS})
        message(STATUS "Boost.Geometry enabled for exact geometry checks")
    else()
        message(STATUS "Boost not found - Boost.Geometry backend disabled (will use MBR-only)")
    endif()
endif()

# Defensive selection of Arrow linking target: prefer shared, fall back to static.
if(TARGET Arrow::arrow_shared)
    set(THEMIS_ARROW_TARGET Arrow::arrow_shared)
elseif(TARGET Arrow::arrow_static)
    set(THEMIS_ARROW_TARGET Arrow::arrow_static)
elseif(TARGET arrow_shared)
    set(THEMIS_ARROW_TARGET arrow_shared)
elseif(TARGET arrow_static)
    set(THEMIS_ARROW_TARGET arrow_static)
else()
    set(THEMIS_ARROW_TARGET "")
endif()

# Parquet target (for Arrow parquet writer)
find_package(Parquet CONFIG QUIET)
set(THEMIS_PARQUET_TARGET "")
if(Parquet_FOUND)
    if(TARGET Parquet::parquet_shared)
        set(THEMIS_PARQUET_TARGET Parquet::parquet_shared)
    elseif(TARGET Parquet::parquet_static)
        set(THEMIS_PARQUET_TARGET Parquet::parquet_static)
    elseif(TARGET Parquet::parquet)
        set(THEMIS_PARQUET_TARGET Parquet::parquet)
    endif()
else()
    # Fallbacks some package layouts expose
    if(TARGET parquet)
        set(THEMIS_PARQUET_TARGET parquet)
    elseif(TARGET parquet_static)
        set(THEMIS_PARQUET_TARGET parquet_static)
    elseif(TARGET parquet_shared)
        set(THEMIS_PARQUET_TARGET parquet_shared)
    endif()
endif()

# v1.1.0: Define ARROW_ENABLED if Arrow is available
if(THEMIS_ARROW_TARGET)
    target_compile_definitions(themis_core PUBLIC ARROW_ENABLED)
    message(STATUS "Arrow enabled - Parquet export available")
endif()

target_link_libraries(themis_core
    PUBLIC
        ${THEMIS_ROCKSDB_TARGET}
        $<$<TARGET_EXISTS:simdjson::simdjson>:simdjson::simdjson>
        $<$<TARGET_EXISTS:TBB::tbb>:TBB::tbb>
        ${THEMIS_ARROW_TARGET}
        ${THEMIS_PARQUET_TARGET}
        fmt::fmt
        spdlog::spdlog
        $<$<TARGET_EXISTS:Boost::system>:Boost::system>
        nlohmann_json::nlohmann_json
    $<$<BOOL:${CURL_FOUND}>:CURL::libcurl>
        ${THEMIS_YAML_TARGET}
    OpenSSL::SSL
    OpenSSL::Crypto
    ZLIB::ZLIB
    $<$<BOOL:${prometheus-cpp_FOUND}>:prometheus-cpp::core>
)

# Link ONNX Runtime when available (ML Serving backend)
if(onnxruntime_FOUND)
    target_link_libraries(themis_core PUBLIC onnxruntime::onnxruntime)
endif()

if(WIN32)
    target_link_libraries(themis_core PUBLIC Secur32)
else()
    # Link libdl for dlopen/dlsym/dlclose used by AdapterRegistry::loadFromPlugin
    target_link_libraries(themis_core PRIVATE ${CMAKE_DL_LIBS})
endif()

# Link libzip (for archive and office processing)
if(libzip_FOUND)
    # libzip may export as libzip::zip or libzip::libzip depending on version
    if(TARGET libzip::zip)
        target_link_libraries(themis_core PUBLIC libzip::zip)
        target_compile_definitions(themis_core PRIVATE THEMIS_HAVE_LIBZIP)
    elseif(TARGET libzip::libzip)
        target_link_libraries(themis_core PUBLIC libzip::libzip)
        target_compile_definitions(themis_core PRIVATE THEMIS_HAVE_LIBZIP)
    else()
        message(WARNING "libzip target not found - please check libzip installation")
    endif()
endif()

# Link pugixml (required for XML/SAML parsing, also used by Office processing)
if(pugixml_FOUND)
    # pugixml should export as pugixml::pugixml or pugixml::static
    if(TARGET pugixml::pugixml)
        target_link_libraries(themis_core PUBLIC pugixml::pugixml)
    elseif(TARGET pugixml::static)
        target_link_libraries(themis_core PUBLIC pugixml::static)
    elseif(TARGET pugixml)
        target_link_libraries(themis_core PUBLIC pugixml)
    else()
        message(WARNING "pugixml target not found - please check pugixml installation")
    endif()

    if(THEMIS_ENABLE_OFFICE)
        message(STATUS "Office support linked: libzip and pugixml")
    else()
        message(STATUS "pugixml linked for core XML/SAML support")
    endif()
endif()

# Link poppler-cpp (for PDF layout-preserving extraction) if enabled
if(THEMIS_ENABLE_PDF AND Poppler_FOUND)
    if(TARGET Poppler::cpp)
        target_link_libraries(themis_core PUBLIC Poppler::cpp)
    elseif(TARGET poppler-cpp)
        target_link_libraries(themis_core PUBLIC poppler-cpp)
    else()
        message(WARNING "Poppler::cpp target not found - please check poppler installation")
    endif()
    message(STATUS "PDF support linked: poppler-cpp")
endif()

# Link libtesseract (for OCR text extraction from images) if enabled
if(THEMIS_ENABLE_OCR AND Tesseract_FOUND)
    if(TARGET Tesseract::libtesseract)
        target_link_libraries(themis_core PUBLIC Tesseract::libtesseract)
    elseif(TARGET tesseract)
        target_link_libraries(themis_core PUBLIC tesseract)
    else()
        message(WARNING "Tesseract target not found - please check libtesseract installation")
    endif()
    message(STATUS "OCR support linked: libtesseract")
endif()

# Link Kerberos/GSSAPI if enabled
if(THEMIS_ENABLE_KERBEROS)
    if(TARGET KRB5::krb5)
        target_link_libraries(themis_core PUBLIC KRB5::krb5)
        message(STATUS "Kerberos support linked: KRB5::krb5")
    endif()
    if(TARGET KRB5::gssapi)
        target_link_libraries(themis_core PUBLIC KRB5::gssapi)
        message(STATUS "Kerberos support linked: KRB5::gssapi")
    endif()
    if(TARGET Kerberos::Kerberos)
        target_link_libraries(themis_core PUBLIC Kerberos::Kerberos)
        message(STATUS "Kerberos support linked: Kerberos::Kerberos")
    endif()
    target_compile_definitions(themis_core PUBLIC THEMIS_HAS_KERBEROS)
    message(STATUS "Kerberos/GSSAPI authentication enabled")
endif()

# Link LDAP / Active Directory if found
if(THEMIS_LDAP_FOUND AND THEMIS_LDAP_LIBRARIES)
    target_link_libraries(themis_core PUBLIC ${THEMIS_LDAP_LIBRARIES})
    if(THEMIS_LDAP_INCLUDE_DIRS)
        target_include_directories(themis_core PUBLIC ${THEMIS_LDAP_INCLUDE_DIRS})
    endif()
    target_compile_definitions(themis_core PUBLIC THEMIS_HAS_LDAP=1)
    message(STATUS "LDAP/AD authentication linked: ${THEMIS_LDAP_LIBRARIES}")
endif()

# Link ZSTD if available and define compile flag
if(DEFINED THEMIS_ZSTD_TARGET)
    target_link_libraries(themis_core PUBLIC ${THEMIS_ZSTD_TARGET})
    target_compile_definitions(themis_core PUBLIC THEMIS_HAS_ZSTD)
endif()

# Link LZ4 if available and define compile flag
if(DEFINED THEMIS_LZ4_TARGET)
    target_link_libraries(themis_core PUBLIC ${THEMIS_LZ4_TARGET})
    target_compile_definitions(themis_core PUBLIC THEMIS_HAS_LZ4)
endif()

# Link Snappy if available (required for Prometheus remote-write decompression)
if(DEFINED THEMIS_SNAPPY_TARGET AND NOT "${THEMIS_SNAPPY_TARGET}" STREQUAL "")
    target_link_libraries(themis_core PUBLIC ${THEMIS_SNAPPY_TARGET})
    target_compile_definitions(themis_core PUBLIC THEMIS_HAS_SNAPPY)
endif()

# Define THEMIS_ENABLE_CURL if CURL is found
if(CURL_FOUND)
    target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_CURL)
endif()

# Link cpp-httplib (HTTP server) if available
if(TARGET httplib::httplib)
    target_link_libraries(themis_core PUBLIC httplib::httplib)
    target_compile_definitions(themis_core PUBLIC THEMIS_HAS_HTTPLIB)
else()
    message(STATUS "cpp-httplib target not found - HTTP server adapter may fail to build")
endif()

# Link MessagePack if available (target names vary by package)
if(TARGET msgpack-cxx)
    target_link_libraries(themis_core PUBLIC msgpack-cxx)
    target_compile_definitions(themis_core PUBLIC THEMIS_HAS_MSGPACK)
elseif(TARGET msgpack::msgpack)
    target_link_libraries(themis_core PUBLIC msgpack::msgpack)
    target_compile_definitions(themis_core PUBLIC THEMIS_HAS_MSGPACK)
else()
    message(STATUS "MessagePack target not found - buffer binary protocol may fail to build")
endif()

# Link hiredis for Redis distributed cache coordination if enabled
if(THEMIS_ENABLE_REDIS AND hiredis_FOUND)
    if(TARGET hiredis::hiredis)
        target_link_libraries(themis_core PUBLIC hiredis::hiredis)
    elseif(TARGET hiredis)
        target_link_libraries(themis_core PUBLIC hiredis)
    endif()
    target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_REDIS=1)
    message(STATUS "Redis distributed cache coordination linked (hiredis)")
endif()

# RFC 3161 Timestamp Authority (TSA) - OpenSSL implementation
# Enable OpenSSL-based Timestamp Authority (RFC 3161) if both OpenSSL and CURL are available
# OpenSSL is required for cryptographic operations, CURL for HTTP/HTTPS communication with TSA servers
if(THEMIS_USE_OPENSSL_TSA AND OpenSSL_FOUND AND CURL_FOUND)
    target_compile_definitions(themis_core PUBLIC THEMIS_USE_OPENSSL_TSA)
    message(STATUS "Timestamp Authority (RFC 3161): OpenSSL implementation enabled")
else()
    if(THEMIS_USE_OPENSSL_TSA AND NOT OpenSSL_FOUND)
        message(WARNING "RFC 3161 TSA: OpenSSL mode requested but OpenSSL not found - falling back to stub mode")
    endif()
    if(THEMIS_USE_OPENSSL_TSA AND NOT CURL_FOUND)
        message(WARNING "RFC 3161 TSA: OpenSSL mode requested but CURL not found - falling back to stub mode")
    endif()
    message(STATUS "Timestamp Authority (RFC 3161): Using stub implementation (development only - NOT for production)")
endif()

# Define version string for runtime use
target_compile_definitions(themis_core PUBLIC 
    THEMIS_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    THEMIS_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    THEMIS_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)

# Phase 2 Performance Optimizations compile definitions
if(THEMIS_ENABLE_WISCKEY)
    target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_WISCKEY)
endif()
if(THEMIS_ENABLE_DOSTOEVSKY)
    target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_DOSTOEVSKY)
endif()
if(THEMIS_ENABLE_CICADA)
    target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_CICADA)
endif()
if(THEMIS_ENABLE_LIGRA)
    target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_LIGRA)
endif()
if(THEMIS_ENABLE_RABITQ)
    target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_RABITQ)
endif()

# Phase 3 Performance Optimizations compile definitions
if(THEMIS_ENABLE_DISKANN)
    target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_DISKANN)
endif()
if(THEMIS_ENABLE_BWTREE)
    target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_BWTREE)
endif()
if(THEMIS_ENABLE_SPLINTERDB)
    target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_SPLINTERDB)
endif()
if(THEMIS_ENABLE_GUNROCK)
    target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_GUNROCK)
endif()
if(THEMIS_ENABLE_BAO)
    target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_BAO)
endif()
if(THEMIS_ENABLE_PER_QUERY_COST_MODEL)
    target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_PER_QUERY_COST_MODEL)
endif()
if(THEMIS_ENABLE_PMEM)
    target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_PMEM)
endif()

if(THEMIS_ENABLE_GPU)
    target_link_libraries(themis_core PUBLIC faiss)
    if(UNIX AND NOT APPLE)
        target_link_libraries(themis_core PUBLIC gfortran quadmath)
    endif()
    target_compile_definitions(themis_core PUBLIC THEMIS_GPU_ENABLED)
elseif(hnswlib_FOUND)
    target_link_libraries(themis_core PUBLIC hnswlib::hnswlib)
    target_compile_definitions(themis_core PUBLIC THEMIS_HNSW_ENABLED)
endif()

# v1.1.0: Link mimalloc (20-40% memory boost, drop-in replacement)
if(THEMIS_ENABLE_MIMALLOC AND mimalloc_FOUND)
    # Use DLL version on Windows (mimalloc-static not available in vcpkg binary cache)
    if(WIN32)
        target_link_libraries(themis_core PUBLIC mimalloc)
    else()
        target_link_libraries(themis_core PUBLIC mimalloc-static)
    endif()
    target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_MIMALLOC)
    message(STATUS "mimalloc linked to themis_core")
endif()

# Phase 3: Link jemalloc as alternative allocator (best fragmentation resistance)
if(THEMIS_ENABLE_JEMALLOC)
    if(jemalloc_FOUND)
        target_link_libraries(themis_core PUBLIC jemalloc::jemalloc)
    elseif(jemalloc_LIBRARIES)
        target_link_libraries(themis_core PUBLIC ${jemalloc_LIBRARIES})
        target_include_directories(themis_core PUBLIC ${jemalloc_INCLUDE_DIRS})
    endif()
    target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_JEMALLOC)
    message(STATUS "jemalloc linked to themis_core")
endif()

# FFmpeg support for video processing (optional)
if(FFMPEG_FOUND)
    target_link_libraries(themis_core PUBLIC FFmpeg::avformat)
    message(STATUS "FFmpeg linked to themis_core - real video processing enabled")
endif()

# ============================================================================
# PHASE 1 PERFORMANCE FEATURES
# ============================================================================

# RCU Index (Read-Copy-Update for lock-free reads, +200-500% for read-heavy workloads)
if(THEMIS_ENABLE_RCU_INDEX)
    target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_RCU_INDEX)
    message(STATUS "RCU Index enabled (+200-500% read performance)")
endif()

# LIRS Cache (Low Inter-reference Recency Set, +30-40% cache hit rate)
if(THEMIS_ENABLE_LIRS_CACHE)
    target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_LIRS_CACHE)
    message(STATUS "LIRS Cache enabled (+30-40% hit rate improvement)")
endif()

# Huge Pages (Linux-only, Windows uses automatic large pages)
if(THEMIS_ENABLE_HUGE_PAGES)
    if(NOT WIN32)
        target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_HUGE_PAGES)
        message(STATUS "Huge pages enabled")
    else()
        message(STATUS "Huge pages disabled on Windows (automatic large pages used)")
    endif()
endif()

# HTTP/2 Protocol Support
if(THEMIS_ENABLE_HTTP2 AND nghttp2_FOUND)
    target_link_libraries(themis_core PUBLIC nghttp2::nghttp2)
    target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_HTTP2)
    message(STATUS "HTTP/2 support enabled and linked to themis_core")
endif()

# HTTP/3 Protocol Support
if(THEMIS_ENABLE_HTTP3 AND nghttp3_FOUND AND ngtcp2_FOUND)
    target_link_libraries(themis_core PUBLIC nghttp3::nghttp3 ngtcp2::ngtcp2)
    if(TARGET ngtcp2::ngtcp2_crypto_ossl)
        target_link_libraries(themis_core PUBLIC ngtcp2::ngtcp2_crypto_ossl)
    endif()
    target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_HTTP3)
    message(STATUS "HTTP/3 support enabled and linked to themis_core")
endif()

# HTTP Server Support
if(THEMIS_ENABLE_HTTP_SERVER)
    target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_HTTP_SERVER)
    message(STATUS "HTTP/REST server enabled")
endif()

# GDAL Geospatial Processing Support
if(THEMIS_ENABLE_GDAL AND GDAL_FOUND)
    target_link_libraries(themis_core PUBLIC GDAL::GDAL)
    target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_GDAL)
    message(STATUS "GDAL geospatial processing enabled and linked to themis_core")
endif()

# GraphQL Protocol Support
if(THEMIS_ENABLE_GRAPHQL)
    target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_GRAPHQL)
    message(STATUS "GraphQL API support enabled")
endif()

# SSE (Server-Sent Events) Support
if(THEMIS_ENABLE_SSE)
    target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_SSE)
    message(STATUS "Server-Sent Events (SSE) support enabled")
endif()

# WebSocket Protocol Support
if(THEMIS_ENABLE_WEBSOCKET)
    target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_WEBSOCKET)
    message(STATUS "WebSocket protocol support enabled")
endif()

# Kafka CDC Producer Support
if(THEMIS_ENABLE_KAFKA)
    find_package(RdKafka CONFIG QUIET)
    if(RdKafka_FOUND)
        target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_KAFKA)
        target_link_libraries(themis_core PUBLIC RdKafka::rdkafka RdKafka::rdkafka++)
        message(STATUS "Kafka CDC producer enabled (librdkafka found)")
    else()
        message(WARNING "THEMIS_ENABLE_KAFKA=ON but librdkafka not found; "
                        "KafkaCDCProducer will compile as a no-op stub. "
                        "Install librdkafka or add it via vcpkg to enable Kafka support.")
    endif()
endif()

# MCP (Model Context Protocol) Support
if(THEMIS_ENABLE_MCP)
    target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_MCP)
    message(STATUS "Model Context Protocol (MCP) support enabled")
    message(STATUS "  MCP Transports: stdio, SSE, WebSocket")
    message(STATUS "  MCP Components: Tools, Resources, Prompts")
endif()

# MQTT Protocol Support
if(THEMIS_ENABLE_MQTT)
    target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_MQTT)
    message(STATUS "MQTT protocol support enabled")
endif()

# PostgreSQL Wire Protocol Support
if(THEMIS_ENABLE_POSTGRES_WIRE)
    target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_POSTGRES_WIRE)
    message(STATUS "PostgreSQL Wire Protocol support enabled")
endif()

# Service mesh sidecar proxy mode (Envoy xDS v3 REST client)
if(THEMIS_ENABLE_SERVICE_MESH)
    target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_SERVICE_MESH)
    message(STATUS "Service mesh sidecar proxy mode enabled (Envoy xDS v3 REST)")
endif()

# gRPC Protocol Support (inter-shard communication - implemented in v1.3.0)
if(THEMIS_ENABLE_GRPC)
    target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_GRPC)
    message(STATUS "gRPC support enabled (inter-shard communication)")
    find_package(gRPC CONFIG)
    find_package(Protobuf CONFIG)
    if(gRPC_FOUND AND Protobuf_FOUND)
        if(NOT TARGET gRPC::grpc++_reflection)
            # vcpkg static triplet may not export reflection target; try to create an imported target
            find_library(GRPCPP_REFLECTION_LIB NAMES grpc++_reflection
                PATHS
                    ${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/lib
                    ${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/debug/lib
                NO_DEFAULT_PATH)
            if(GRPCPP_REFLECTION_LIB)
                add_library(gRPC::grpc++_reflection UNKNOWN IMPORTED)
                set_target_properties(gRPC::grpc++_reflection PROPERTIES
                    IMPORTED_LOCATION ${GRPCPP_REFLECTION_LIB}
                    IMPORTED_LINK_INTERFACE_LANGUAGES "CXX")
                message(STATUS "gRPC reflection library found manually: ${GRPCPP_REFLECTION_LIB}")
            else()
                message(WARNING "gRPC reflection library not found; proceeding without reflection")
            endif()
        endif()

        if(TARGET gRPC::grpc++_reflection)
            target_link_libraries(themis_core PUBLIC gRPC::grpc++ gRPC::grpc++_reflection protobuf::libprotobuf)
            message(STATUS "gRPC and Protobuf found - linking to themis_core (with reflection)")
        else()
            target_link_libraries(themis_core PUBLIC gRPC::grpc++ protobuf::libprotobuf)
            message(WARNING "gRPC::grpc++_reflection not found; linking without reflection to themis_core")
        endif()

        # Link to generated shard proto library (generated outside this block)
        # In modular builds, themis_sharding owns this dependency to avoid
        # duplicate static proto objects in final executables.
        if(TARGET themis_shard_proto AND NOT THEMIS_BUILD_MODULAR)
            target_link_libraries(themis_core PUBLIC themis_shard_proto)
        endif()

        if(NOT THEMIS_BUILD_MODULAR AND NOT TARGET themis_shard_proto)
            set(SHARD_PROTO ${THEMIS_ROOT_DIR}/proto/sharding/shard_rpc.proto)
            set(SHARD_PROTO_GEN_DIR ${CMAKE_BINARY_DIR}/proto_generated)
            file(MAKE_DIRECTORY ${SHARD_PROTO_GEN_DIR})

            set(SHARD_PROTO_SRCS
                ${SHARD_PROTO_GEN_DIR}/shard_rpc.pb.cc
            )
            set(SHARD_PROTO_HDRS
                ${SHARD_PROTO_GEN_DIR}/shard_rpc.pb.h
            )

            if(THEMIS_ENABLE_GRPC)
                find_package(gRPC CONFIG)
                if(gRPC_FOUND)
                    list(APPEND SHARD_PROTO_SRCS ${SHARD_PROTO_GEN_DIR}/shard_rpc.grpc.pb.cc)
                    list(APPEND SHARD_PROTO_HDRS ${SHARD_PROTO_GEN_DIR}/shard_rpc.grpc.pb.h)
                    add_custom_command(
                        OUTPUT ${SHARD_PROTO_SRCS} ${SHARD_PROTO_HDRS}
                        COMMAND $<TARGET_FILE:protobuf::protoc>
                        ARGS --cpp_out ${SHARD_PROTO_GEN_DIR}
                             --grpc_out ${SHARD_PROTO_GEN_DIR}
                             --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>
                             -I ${THEMIS_ROOT_DIR}/proto/sharding
                             ${SHARD_PROTO}
                        DEPENDS ${SHARD_PROTO} protobuf::protoc gRPC::grpc_cpp_plugin
                        COMMENT "Generating protobuf and gRPC sources for shard_rpc.proto"
                        VERBATIM
                    )
                else()
                    message(WARNING "gRPC not found - shard RPC service stubs disabled; generating protobuf messages only")
                    add_custom_command(
                        OUTPUT ${SHARD_PROTO_SRCS} ${SHARD_PROTO_HDRS}
                        COMMAND $<TARGET_FILE:protobuf::protoc>
                        ARGS --cpp_out ${SHARD_PROTO_GEN_DIR}
                             -I ${THEMIS_ROOT_DIR}/proto/sharding
                             ${SHARD_PROTO}
                        DEPENDS ${SHARD_PROTO} protobuf::protoc
                        COMMENT "Generating protobuf sources for shard_rpc.proto"
                        VERBATIM
                    )
                endif()
            else()
                add_custom_command(
                    OUTPUT ${SHARD_PROTO_SRCS} ${SHARD_PROTO_HDRS}
                    COMMAND $<TARGET_FILE:protobuf::protoc>
                    ARGS --cpp_out ${SHARD_PROTO_GEN_DIR}
                         -I ${THEMIS_ROOT_DIR}/proto/sharding
                         ${SHARD_PROTO}
                    DEPENDS ${SHARD_PROTO} protobuf::protoc
                    COMMENT "Generating protobuf sources for shard_rpc.proto"
                    VERBATIM
                )
            endif()

            add_library(themis_shard_proto STATIC ${SHARD_PROTO_SRCS} ${SHARD_PROTO_HDRS})
            target_include_directories(themis_shard_proto PUBLIC ${SHARD_PROTO_GEN_DIR})
            target_link_libraries(themis_shard_proto PUBLIC protobuf::libprotobuf)

            if(THEMIS_ENABLE_GRPC AND gRPC_FOUND)
                target_link_libraries(themis_shard_proto PUBLIC gRPC::grpc++)
            endif()

            if(MSVC)
                target_compile_options(themis_shard_proto PRIVATE /wd4996 /wd4018 /wd4244 /wd4267)
            else()
                target_compile_options(themis_shard_proto PRIVATE -Wno-unused-parameter -Wno-deprecated-declarations -Wno-conversion)
            endif()

            add_dependencies(themis_core themis_shard_proto)
            target_link_libraries(themis_core PUBLIC themis_shard_proto)
            message(STATUS "themis_shard_proto library created for inter-shard communication (monolithic build)")
        endif()

        # ── ThemisDB API proto (client-facing gRPC surface, api module) ──────
        # Generates C++ stubs from proto/themisdb.proto.
        # The library `themis_api_proto` is linked into themis_core so that
        # GrpcApiServer can register ThemisDBService implementations.
        if(TARGET protobuf::protoc AND TARGET gRPC::grpc_cpp_plugin)
            set(THEMISDB_API_PROTO ${THEMIS_ROOT_DIR}/proto/themisdb.proto)
            set(THEMISDB_API_PROTO_GEN_DIR ${CMAKE_BINARY_DIR}/proto_generated)
            file(MAKE_DIRECTORY ${THEMISDB_API_PROTO_GEN_DIR})

            set(THEMISDB_API_PROTO_SRCS
                ${THEMISDB_API_PROTO_GEN_DIR}/themisdb.pb.cc
                ${THEMISDB_API_PROTO_GEN_DIR}/themisdb.grpc.pb.cc
            )
            set(THEMISDB_API_PROTO_HDRS
                ${THEMISDB_API_PROTO_GEN_DIR}/themisdb.pb.h
                ${THEMISDB_API_PROTO_GEN_DIR}/themisdb.grpc.pb.h
            )

            add_custom_command(
                OUTPUT ${THEMISDB_API_PROTO_SRCS} ${THEMISDB_API_PROTO_HDRS}
                COMMAND $<TARGET_FILE:protobuf::protoc>
                ARGS --cpp_out ${THEMISDB_API_PROTO_GEN_DIR}
                     --grpc_out ${THEMISDB_API_PROTO_GEN_DIR}
                     --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>
                     -I ${THEMIS_ROOT_DIR}/proto
                     ${THEMISDB_API_PROTO}
                DEPENDS ${THEMISDB_API_PROTO} protobuf::protoc gRPC::grpc_cpp_plugin
                COMMENT "Generating protobuf and gRPC sources for themisdb.proto (API service)"
                VERBATIM
            )

            add_library(themis_api_proto SHARED
                ${THEMISDB_API_PROTO_SRCS}
                ${THEMISDB_API_PROTO_HDRS}
            )
            target_include_directories(themis_api_proto PUBLIC ${THEMISDB_API_PROTO_GEN_DIR})
            target_link_libraries(themis_api_proto PUBLIC gRPC::grpc++ protobuf::libprotobuf)

            if(MSVC)
                target_compile_options(themis_api_proto PRIVATE
                    /wd4996 /wd4018 /wd4244 /wd4267)
            else()
                target_compile_options(themis_api_proto PRIVATE
                    -Wno-unused-parameter -Wno-deprecated-declarations -Wno-conversion)
            endif()

            target_link_libraries(themis_core PUBLIC themis_api_proto)
            message(STATUS "themis_api_proto shared library created for ThemisDB API gRPC service")
        else()
            message(WARNING "protoc/grpc plugin targets unavailable – themisdb.proto stubs will not be generated")
        endif()
    else()
        message(WARNING "gRPC/Protobuf not found - WAL gRPC service will be disabled at runtime")
    endif()
    # Fallback: if CONFIG packages not found, try pkg-config for apt-installed gRPC
    if(NOT gRPC_FOUND OR NOT Protobuf_FOUND)
        find_package(PkgConfig QUIET)
        if(PkgConfig_FOUND)
            pkg_check_modules(GRPC QUIET grpc++)
            pkg_check_modules(PROTOBUF QUIET protobuf)
            if(GRPC_FOUND AND PROTOBUF_FOUND)
                target_link_libraries(themis_core PUBLIC ${GRPC_LIBRARIES} ${PROTOBUF_LIBRARIES})
                target_include_directories(themis_core PUBLIC ${GRPC_INCLUDE_DIRS} ${PROTOBUF_INCLUDE_DIRS})
                message(STATUS "gRPC and Protobuf found via pkg-config - linking to themis_core")
            endif()
        endif()
    endif()
endif()

# WebSocket Protocol Support (placeholder for future implementation)
if(THEMIS_ENABLE_WEBSOCKET)
    target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_WEBSOCKET)
    message(STATUS "WebSocket support enabled (Boost.Beast has native support)")
    # Note: Boost.Beast already provides WebSocket support, just needs integration
endif()

# Kafka CDC Producer Support (handled above with full librdkafka setup)

# MQTT Protocol Support (placeholder for future implementation)
if(THEMIS_ENABLE_MQTT)
    target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_MQTT)
    message(STATUS "MQTT support enabled (stub - full implementation pending)")
    # TODO: Add MQTT dependencies when implemented
    # find_package(PahoMqttCpp CONFIG)
endif()

# PostgreSQL Wire Protocol Support (placeholder for future implementation)
if(THEMIS_ENABLE_POSTGRES_WIRE)
    target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_POSTGRES_WIRE)
    message(STATUS "PostgreSQL Wire Protocol support enabled (stub - full implementation pending)")
endif()

# v1.1.0: vLLM Co-Location compile definitions
if(THEMIS_VLLM_COLOCATION)
    target_compile_definitions(themis_core PRIVATE
        THEMIS_VLLM_COLOCATION=1
        THEMIS_MAX_GPU_VRAM_MB=2048        # 2 GB per GPU (vLLM has rest)
        THEMIS_MAX_VECTOR_BATCH_SIZE=1024  # Small batches
        THEMIS_GPU_LOW_PRIORITY=1          # Lower than vLLM
        THEMIS_CPU_CORES_RESERVED=50       # 50 of 64 cores
        THEMIS_RAM_GB_ALLOCATED=200        # 200 of 256 GB
    )
    message(STATUS "vLLM Co-Location: Resource limits configured")
endif()

# v1.3.0: LLM Plugin Support with llama.cpp
if(THEMIS_ENABLE_LLM)
    # Add LLM plugin source files
    target_sources(themis_core PRIVATE
        ../src/llm/llama_wrapper.cpp
        ../src/llm/embedded_llm.cpp          # Embedded LLM facade for system-wide use
        ../src/llm/llm_plugin_manager.cpp
        ../src/llm/model_loader.cpp          # Ollama-style lazy loading
        ../src/llm/model_quantization_pipeline.cpp  # GGUF/AWQ/GPTQ quantization pipeline (Phase 3)
        ../src/llm/multi_lora_manager.cpp    # vLLM-style multi-LoRA
        ../src/llm/async_inference_engine.cpp # Async threading (independent from DB)
        ../src/llm/shared_worker_pool.cpp    # Shared work-stealing thread pool (Phase 2)
        # v1.3.1: Cache Reuse Integration (Phase 2)
        ../src/llm/model_metadata_cache.cpp  # Model metadata caching with ConcurrentCache
        ../src/llm/lora_metadata_cache.cpp   # LoRA metadata caching with ConcurrentCache
        
        # LoRA Adapter Framework (Issue #319) - RE-ENABLED WITH FIXED APIs
        ../src/llm/lora_framework/lora_orchestrator.cpp       # LoRA CRUD orchestrator
        # Note: lora_storage_service.cpp removed to avoid duplicate symbol errors
        # Only using lora_storage_service_themisdb.cpp which has complete ThemisDB + FileSystem implementations
        ../src/llm/lora_framework/lora_storage_service_themisdb.cpp  # ThemisDB storage backend (complete implementation)
        ../src/llm/lora_framework/lora_checkpoint_manager.cpp  # Checkpoint lifecycle management
        ../src/llm/lora_framework/lora_training_service.cpp   # Training pipeline (re-enabled for distributed training integration)
        ../src/llm/lora_framework/training_service_registry.cpp  # Service registry for dependency injection
        ../src/llm/lora_framework/lora_layers.cpp             # LoRA layer implementations
        ../src/llm/lora_framework/lora_audit_logger.cpp       # Audit logging
        ../src/llm/lora_framework/feedback_plugin.cpp         # Feedback plugin system
        ../src/llm/lora_framework/lora_feedback_storage.cpp   # Feedback storage service
        ../src/llm/lora_framework/lora_training_config.cpp    # YAML training configuration
        ../src/llm/lora_framework/lora_layers.cpp             # LoRA layers and tensor operations (Phase 1)
        ../src/llm/lora_framework/base_model_adapter.cpp      # Base model loading and LoRA injection (Phase 2)
        ../src/llm/lora_framework/data_loader.cpp             # Text data loading and tokenization (Phase 2)
        ../src/llm/lora_framework/llama_tokenizer.cpp         # llama.cpp native tokenizer integration
        # Production training features
        ../src/llm/lora_framework/mixed_precision.cpp         # Mixed precision training (FP16/BF16)
        ../src/llm/lora_framework/lr_scheduler.cpp            # Learning rate schedulers
        ../src/llm/lora_framework/gradient_utils.cpp          # Gradient clipping & accumulation
        ../src/llm/lora_framework/distributed_trainer.cpp     # Multi-GPU distributed training
        # QLoRA quantization support
        ../src/llm/lora_framework/quantization.cpp            # NF4/INT8 quantization (CPU)
        ../src/llm/lora_framework/quantized_model.cpp         # Quantized model management
        ../src/llm/distributed_training_coordinator.cpp       # Coordinator implementation (link fixes)
        ../src/llm/applications/themis_help_lora.cpp          # Themis Help LoRA app (link fixes)
        ../src/llm/security/signature_verifier.cpp            # LLM signature verification
        ../src/llm/lora_security_validator.cpp                # LoRA security policy validation
        
        # Dynamic batch size adaptation (Issue #39)
        ../src/llm/lora_framework/adaptive_batcher.cpp        # Adaptive batch sizing for optimal GPU utilization
        ../src/llm/lora_framework/sequence_packer.cpp         # Sequence packing to reduce padding waste
        ../src/llm/lora_framework/gpu_utilization_monitor.cpp # GPU utilization monitoring (NVML/ROCm)
        
        ../src/llm/paged_block_manager.cpp   # Block management for PagedAttention (v1.4.0 prep)
        ../src/llm/kv_cache_buffer.cpp       # KV cache batching with VectorAutoBuffer (Phase 2.2)
        ../src/llm/llm_response_cache.cpp    # Response caching with SemanticCache (Phase 2.3)
        
        # Phase 3.2: Grammar-Constrained Generation
        ../src/llm/grammar.cpp               # Grammar parsing and compilation
        ../src/llm/grammar_cache.cpp         # Grammar caching for performance
        ../src/llm/json_schema_converter.cpp # JSON schema → GBNF and tool-call support (Issue #1922)
        
        # Phase 4+5 Week 4-6: llama.cpp Integration
        ../src/llm/gguf_loader.cpp           # GGUF model file loader with memory-mapping
        ../src/llm/llamacpp_inference_engine.cpp # llama.cpp inference engine with PagedAttention
        ../src/llm/llm_model_storage.cpp     # LLM model storage and metadata management
        
        # Week 13-14: Production Validation & Testing (P1 Enterprise Features)
        ../src/llm/production_validator.cpp  # Production-grade LLM validation and benchmarking
        
        # P1: Enterprise Features - LLM Inference Engine Improvements
        ../src/llm/continuous_batch_scheduler.cpp  # vLLM-style continuous batching
        ../src/llm/llm_prefix_cache.cpp           # Context caching with prefix reuse
        ../src/llm/grafana_metrics.cpp            # Prometheus/Grafana metrics integration
        ../src/llm/inference_engine_enhanced.cpp  # Enhanced inference engine with P1 features
        ../src/llm/streaming_handler.cpp          # SSE / chunked-response streaming handler (Phase 2)
        ../src/llm/openai_compat_adapter.cpp      # OpenAI-compatible /v1/chat/completions adapter (Phase 3)
        ../src/llm/speculative_decoder.cpp        # Speculative decoding for latency reduction (Phase 3)
        ../src/llm/federated_inference_coordinator.cpp  # RAID fan-out/fan-in coordinator (Issue #1928)
        ../src/llm/kv_prefix_transfer_manager.cpp  # Phase 5 KV-Prefix Cross-Shard Transfer
        ../src/llm/lookup_decoder.cpp             # N-gram prompt lookup decoding (draft-model-free speculation)
        
        # AI Decision Auditing and Explainability (EU AI Act, GDPR Compliance)
        ../src/llm/ai_decision_auditor.cpp        # AI decision auditing with cryptographic signing
        ../src/llm/decision_record_yaml_processor.cpp # YAML-backed AI decision record persistence
        ../src/llm/explanation_generator.cpp      # Human-readable explanation generation
        ../src/llm/ethical_guidelines_manager.cpp # Ethical guidelines management
        ../src/llm/constitutional_reasoning_engine.cpp # Constitutional-AI constraint checking
        ../src/ai/cai_ethics_integration.cpp     # CAI safety evaluation bridge
        ../src/ethics_ai/ethics_evaluator.cpp    # Core ethics scoring for CAI bridge
        ../src/llm/ethics_aware_confidence_detector.cpp # Ethics-aware confidence scoring
        ../src/llm/moral_analyzer.cpp             # Moral dimension extraction and scoring
        ../src/llm/multi_perspective_generator.cpp # Multi-perspective generation for ethical queries
        ../src/llm/meta_prompt_generator.cpp      # Meta-prompt generation for self-improvement
        ../src/llm/prompt_evaluator.cpp           # Prompt quality evaluation
        ../src/llm/prompt_optimizer.cpp           # Prompt optimisation pipeline

        # AI Orchestrator: YAML-configurable LLM pipeline modes (ask, edit, rag, agentic, ethics, multi_agent)
        ../src/llm/mode_spec_loader.cpp           # ThemisModePack YAML loader + validator
        ../src/llm/ai_orchestrator.cpp            # Orchestrator runtime with ToolRegistry and mode pipelines

        # MCP ↔ Orchestrator bridge (compiled when both THEMIS_ENABLE_LLM and THEMIS_ENABLE_MCP are active)
        $<$<BOOL:${THEMIS_ENABLE_MCP}>:../src/llm/mcp_tool_bridge.cpp>

        # RAG Enhancement: Quality Evaluation and Gap Detection
        ../src/rag/llm_meta_analyzer.cpp          # Base class for LLM-based meta-analysis
        ../src/rag/rag_judge.cpp                  # LLM-as-Judge for RAG quality evaluation
        ../src/rag/knowledge_gap_detector.cpp     # Knowledge gap detection
        ../src/rag/llm_judge_client.cpp           # LLM Judge Client connecting to InferenceEngineEnhanced
        ../src/rag/nli_faithfulness_verifier.cpp  # NLI-based faithfulness verification
        ../src/rag/quality_control_pipeline.cpp   # Multi-stage quality control pipeline
        
        # Continuous Learning Orchestrator (Phase 2)
        ../src/rag/learning_metrics.cpp
        ../src/rag/bayesian_optimizer.cpp
        ../src/rag/ab_testing_framework.cpp
        ../src/rag/continuous_learning_orchestrator.cpp

        # RAG Enhancement: Hallucination Rate Tracking Dashboard (Phase 2)
        ../src/rag/hallucination_dashboard.cpp

        # RAG Enhancement: Citation Highlighting (Phase 3)
        ../src/rag/citation_highlighter.cpp

        # RAG Enhancement: Distributed Evaluation and Security (Phase 5)
        ../src/rag/distributed_rag_evaluator.cpp
        ../src/rag/prompt_injection_detector.cpp

        # RAG Enhancement: LLM-dependent evaluators (Phase 1–4)
        ../src/rag/llm_integration.cpp
        ../src/rag/llm_judge_integration.cpp
        ../src/rag/geval_evaluator.cpp
        ../src/rag/claim_extractor.cpp
        ../src/rag/coherence_evaluator.cpp
        ../src/rag/completeness_evaluator.cpp
        ../src/rag/cot_evaluator.cpp
        ../src/rag/document_summarizer.cpp
        ../src/rag/faithfulness_evaluator.cpp
        ../src/rag/judge_ensemble.cpp
        ../src/rag/pairwise_comparator.cpp
        ../src/rag/quality_control_factory.cpp
        ../src/rag/relevance_evaluator.cpp
        ../src/rag/rubric_evaluator.cpp
        # Phase 7: Multi-hop reasoning and adaptive retrieval depth
        ../src/rag/multi_hop_reasoner.cpp
        ../src/rag/adaptive_retrieval.cpp

        # Model Deployment Plugin (Ollama-inspired, production-ready)
        ../src/llm/model_downloader.cpp           # Model downloading from Ollama/HTTP/local sources
        ../src/llm/llm_deployment_plugin.cpp      # Production LLM deployment with caching and audit
        
        # P3: Vision Support (Multi-Modal)
        # vision_encoder.cpp is added conditionally below via THEMIS_ENABLE_VISION
        ../src/llm/gpu_memory_manager.cpp         # GPU memory management (CPU fallback when CUDA disabled)
        ../src/llm/kernel_fusion.cpp              # Kernel fusion with CPU fallback

        # Inference & Sampling core (Phase 2 refactor)
        ../src/llm/inference_handle.cpp           # InferenceHandle implementation
        ../src/llm/sampling_strategy.cpp          # Token sampling strategies (top-p, top-k, mirostat)

        # Ethics, AI Safety & Multi-Perspective Generation
        ../src/llm/constitutional_reasoning_engine.cpp   # Constitutional AI reasoning
        ../src/ai/cai_ethics_integration.cpp             # CAI safety evaluation bridge
        ../src/ethics_ai/ethics_evaluator.cpp            # Core ethics scoring for CAI bridge
        ../src/llm/ethics_aware_confidence_detector.cpp  # Ethics-aware confidence scoring
        ../src/llm/moral_analyzer.cpp                    # Moral reasoning analysis
        ../src/llm/multi_perspective_generator.cpp       # Multi-perspective response generation

        # Feedback & Security
        ../src/llm/feedback_plugin_basic.cpp      # Basic feedback plugin implementation
        ../src/llm/llm_security_utils.cpp         # LLM security utilities (prompt sanitization)
        ../src/llm/safety/classifier.cpp          # Safety classifier core
        ../src/llm/safety/guardian.cpp            # Prompt guardian rules
        ../src/llm/safety/monitoring.cpp          # Safety monitoring + counters

        # Vision resource monitoring
        ../src/llm/vision_resource_monitor.cpp    # Vision model resource monitoring

        # LoRA framework additions (unconditional)
        ../src/llm/lora_framework/distributed_dataloader.cpp     # Distributed data loading for training
        ../src/llm/lora_framework/kernels/cpu_fused_kernels.cpp  # CPU fused LoRA kernels (non-GPU fallback)
        ../src/llm/lora_framework/paged_optimizer.cpp            # Paged AdamW optimizer for memory-efficient training
    )

    if(THEMIS_ENABLE_GPU)
        target_sources(themis_core PRIVATE
            ../src/llm/lora_framework/gpu_lora_layers.cpp         # GPU LoRA layer implementations
            ../src/llm/lora_framework/gpu_tensor.cpp              # GPU tensor operations
            ../src/llm/lora_framework/gpu_memory.cpp              # GPU memory management
            ../src/llm/lora_framework/vram_allocator.cpp          # VRAM allocation and tracking
            ../src/llm/lora_framework/gpu_data_loader.cpp         # GPU data loader with async transfers
            ../src/llm/lora_framework/gpu_training_loop.cpp       # GPU training loop implementation
            ../src/llm/lora_framework/gpu_embedding_layer.cpp     # GPU embedding layer for real embeddings
        )
        message(STATUS "Including LoRA GPU sources (THEMIS_ENABLE_GPU=ON)")
    else()
        message(STATUS "Skipping LoRA GPU sources (THEMIS_ENABLE_GPU=OFF)")
    endif()

    # Add CLIP vision encoder only when explicitly enabled
    target_sources(themis_core PRIVATE
        ../src/llm/vision_config.cpp
        ../src/llm/vision_resource_monitor.cpp    # Vision resource tracking and VRAM budget
        ../src/llm/inference_handle.cpp           # Per-request inference handle with cancellation
        ../src/llm/llm_security_utils.cpp         # LLM security utilities (API key masking, etc.)
    )

    if(THEMIS_ENABLE_VISION)
        target_sources(themis_core PRIVATE
            ../src/llm/vision_encoder.cpp
        )
        target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_VISION=1)
        message(STATUS "LLM Vision: CLIP vision encoder enabled")
    else()
        message(STATUS "LLM Vision: disabled (set -DTHEMIS_ENABLE_VISION=ON to enable)")
    endif()
    
    # Add CUDA kernel fusion sources when CUDA is enabled
    if(THEMIS_ENABLE_CUDA)
        list(APPEND THEMIS_CORE_SOURCES
            ../src/llm/kernel_fusion.cu                        # Flash Attention CUDA kernels
            ../src/llm/lora_framework/kernels/cuda_kernels.cu  # LoRA CUDA kernels
            ../src/llm/lora_framework/kernels/quantization_kernels.cu  # QLoRA quantization GPU kernels
            ../src/llm/lora_framework/kernels/cuda_fused_kernels.cu  # LoRA fused CUDA kernels (Phase 10.3)
            ../src/llm/lora_framework/kernels/cuda_fp16_kernels.cu  # FP16 mixed precision kernels (Phase 10.4)
            ../src/llm/lora_framework/kernels/cuda_bf16_kernels.cu  # BF16 mixed precision kernels (Phase 10.4)
            ../src/llm/lora_framework/kernels/cuda_flash_lora_kernels.cu  # FlashLoRA memory-efficient kernels
        )
        message(STATUS "LLM: CUDA kernel fusion enabled (Flash Attention + LoRA)")
    endif()
    
    # Add HIP kernel sources when HIP is enabled
    if(THEMIS_ENABLE_HIP)
        list(APPEND THEMIS_CORE_SOURCES
            ../src/llm/lora_framework/kernels/hip_kernels.cpp       # LoRA HIP kernels (AMD)
            ../src/llm/lora_framework/kernels/hip_fused_kernels.cpp # LoRA HIP fused kernels (AMD)
        )
        
        # Link RCCL if available
        if(RCCL_FOUND)
            target_include_directories(themis_core PRIVATE ${RCCL_INCLUDE_DIRS})
            target_link_libraries(themis_core PRIVATE ${RCCL_LIBRARIES})
            message(STATUS "RCCL libraries linked: ${RCCL_LIBRARIES}")
        endif()
        
        message(STATUS "LoRA: HIP kernels enabled (AMD ROCm support)")
    endif()
    
    # Add Vulkan kernel sources when Vulkan is enabled
    if(THEMIS_ENABLE_VULKAN)
        list(APPEND THEMIS_CORE_SOURCES
            ../src/llm/lora_framework/kernels/vulkan_kernels.cpp  # LoRA Vulkan compute
            ../src/llm/lora_framework/vulkan_buffer.cpp           # Vulkan buffer management for LoRA
            ../src/llm/lora_framework/vulkan_context.cpp          # Vulkan context initialization
            ../src/llm/lora_framework/vulkan_pipeline.cpp         # Vulkan compute pipeline for LoRA
        )
        message(STATUS "LoRA: Vulkan compute kernels enabled (cross-platform)")
    endif()
    
    # Threading support required for async inference
    find_package(Threads REQUIRED)
    target_link_libraries(themis_core PRIVATE Threads::Threads)
    
    # Link llama.cpp library
    if(TARGET llama)
        target_link_libraries(themis_core PRIVATE llama)
        # Include headers for llama.cpp
        if(DEFINED LLAMA_SRC_DIR)
            target_include_directories(themis_core PRIVATE ${LLAMA_SRC_DIR}/include)
            # Access internal llama grammar headers when needed
            target_include_directories(themis_core PRIVATE ${LLAMA_SRC_DIR}/src)
        endif()
        message(STATUS "LLM Plugin: Linked llama.cpp library")
    else()
        message(WARNING "LLM Plugin: llama.cpp target not found - plugin will be built without llama.cpp")
    endif()
    
    # LLM is already defined via THEMIS_LLM_ENABLED in earlier section
    message(STATUS "LLM Plugin: Source files added to themis_core")
    message(STATUS "  - Ollama-style lazy model loading enabled")
    message(STATUS "  - vLLM-style multi-LoRA management enabled")
    message(STATUS "  - LoRA Adapter Framework enabled (Issue #319)")
    message(STATUS "    * Full CRUD operations with BaseEntity storage")
    message(STATUS "    * Enterprise security (AES-256-GCM + Ed25519)")
    message(STATUS "    * Async job management for training/loading")
    message(STATUS "    * Complete audit trail for compliance")
    message(STATUS "  - Async inference engine enabled (independent threading)")
    message(STATUS "  - ThemisDB cache reuse integration enabled (Phases 2.1-2.3)")
    message(STATUS "    * ModelMetadataCache (ConcurrentCache)")
    message(STATUS "    * LoRAMetadataCache (ConcurrentCache)")
    message(STATUS "    * PagedBlockManager (foundation for PagedAttention v1.4.0)")
    message(STATUS "    * KVCacheBuffer (VectorAutoBuffer for KV cache batching - Phase 2.2)")
    message(STATUS "    * LLMResponseCache (SemanticCache for response caching - Phase 2.3 - 75x faster)")
endif()


# Propagate Geo feature tiering compile definitions
if(THEMIS_GEO)
    target_compile_definitions(themis_core PUBLIC THEMIS_GEO_ENABLED)
endif()
if(THEMIS_GEO_SIMD)
    target_compile_definitions(themis_core PUBLIC THEMIS_GEO_SIMD_ENABLED)
endif()
if(THEMIS_GEO_GPU)
    target_compile_definitions(themis_core PUBLIC THEMIS_GEO_GPU_ENABLED)
endif()
if(THEMIS_GEO_H3)
    target_compile_definitions(themis_core PUBLIC THEMIS_GEO_H3_ENABLED)
endif()
if(THEMIS_GEO_GEOS_PLUGIN)
    target_compile_definitions(themis_core PUBLIC THEMIS_GEO_GEOS_PLUGIN_ENABLED)
endif()
if(THEMIS_ENTERPRISE)
    target_compile_definitions(themis_core PUBLIC THEMIS_ENTERPRISE_ENABLED)
endif()

if(THEMIS_ENABLE_TRACING)
    # Defensive checks: Ensure OpenTelemetry targets exist before linking
    if(NOT TARGET opentelemetry-cpp::trace)
        message(FATAL_ERROR "Required CMake target 'opentelemetry-cpp::trace' not found. "
                            "Ensure opentelemetry-cpp was found successfully.")
    endif()
    if(NOT TARGET opentelemetry-cpp::otlp_http_exporter)
        message(FATAL_ERROR "Required CMake target 'opentelemetry-cpp::otlp_http_exporter' not found. "
                            "Ensure opentelemetry-cpp was found successfully with otlp-http feature.")
    endif()
    
    target_link_libraries(themis_core PUBLIC
        opentelemetry-cpp::trace
        opentelemetry-cpp::otlp_http_exporter
    )
    target_compile_definitions(themis_core PUBLIC THEMIS_ENABLE_TRACING)
endif()

# End of monolithic build configuration
endif() # THEMIS_BUILD_MODULAR

# ============================================================================
# EXECUTABLES AND TESTS (work with both modular and monolithic builds)
# ============================================================================

# Main executable (demo)
add_executable(themis_demo
    ../src/main.cpp
)

target_link_libraries(themis_demo
    PRIVATE
        themis_core
)

# Encryption demo executable
add_executable(themis_demo_encryption
    ../src/demo_encryption.cpp
)

target_link_libraries(themis_demo_encryption
    PRIVATE
        themis_core
)

# API Server executable
add_executable(themis_server
    ../src/main_server.cpp
)

target_compile_definitions(themis_server PRIVATE THEMIS_SERVER_PROFILE_DEFAULT="${THEMIS_SERVER_PROFILE}")

if(THEMIS_ENABLE_HTTP_SERVER)
    target_compile_definitions(themis_server PRIVATE THEMIS_ENABLE_HTTP_SERVER=1)
endif()

target_link_libraries(themis_server
    PRIVATE
        themis_core
)

# Set output directory to bin/ folder (consistent with test binaries)
    set_target_properties(themis_server PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin_out"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin_out"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin_out"
)

# Ensure themis_server does not directly link mimalloc; rely on themis_core
if(WIN32 AND THEMIS_ENABLE_MIMALLOC)
    get_target_property(_srv_libs themis_server LINK_LIBRARIES)
    if(_srv_libs)
        list(REMOVE_ITEM _srv_libs mimalloc mimalloc-debug mimalloc-static)
        set_target_properties(themis_server PROPERTIES LINK_LIBRARIES "${_srv_libs}")
        message(STATUS "Pruned mimalloc libs from themis_server link libraries")
    endif()
endif()

# Tests - fully delegated to tests/CMakeLists.txt
# All individual test target definitions have been moved there.
if(THEMIS_BUILD_TESTS)
    enable_testing()
    find_package(GTest CONFIG REQUIRED)
    add_subdirectory(${THEMIS_ROOT_DIR}/tests tests)
endif()

# ============================================================================
# BENCHMARKS: Now managed via benchmarks/CMakeLists.txt (included via add_subdirectory)
# ============================================================================
# Previous benchmark definitions have been moved to the dedicated benchmarks/
# subdirectory. All benchmark targets should be defined in benchmarks/CMakeLists.txt
# to avoid duplicate target errors. The benchmarks are included via:
#   add_subdirectory(benchmarks)
# ============================================================================
if(THEMIS_BUILD_BENCHMARKS AND EXISTS "${THEMIS_ROOT_DIR}/benchmarks/CMakeLists.txt")
    add_subdirectory(${THEMIS_ROOT_DIR}/benchmarks benchmarks)
    message(STATUS "Benchmarks enabled – including benchmarks/CMakeLists.txt")
endif()

if(THEMIS_BUILD_DOCKER_RAID_BENCHMARK)
    find_package(benchmark CONFIG REQUIRED)
    find_package(prometheus-cpp CONFIG REQUIRED)
    # Docker RAID Comprehensive Benchmark (v1.3.4+)
    # 1+ hour benchmark suite for Docker multi-container RAID testing
    # Tests RAID0, RAID1, RAID5, RAID6, RAID10 configurations
    # See: ${THEMIS_ROOT_DIR}/benchmarks/DOCKER_RAID_BENCHMARK_SUITE_README.md
    add_executable(bench_docker_raid_comprehensive
        ${CMAKE_CURRENT_SOURCE_DIR}${THEMIS_ROOT_DIR}/benchmarks/bench_docker_raid_comprehensive.cpp
    )
    target_link_libraries(bench_docker_raid_comprehensive PRIVATE 
        themis_core 
        benchmark::benchmark 
        benchmark::benchmark_main
        prometheus-cpp::pull
        prometheus-cpp::core
    )
    
    # GPU Erasure Coding Benchmark (conditionally compiled if CUDA is enabled)
    if(THEMIS_ENABLE_CUDA)
        add_executable(bench_gpu_erasure
            ${THEMIS_ROOT_DIR}/benchmarks/bench_gpu_erasure.cpp
        )
        target_link_libraries(bench_gpu_erasure PRIVATE 
            themis_core 
            benchmark::benchmark 
            benchmark::benchmark_main
        )
    endif()
endif()  # THEMIS_BUILD_DOCKER_RAID_BENCHMARK

set(_themis_runtime_layout_file "${THEMIS_ROOT_DIR}/cmake/packaging/RuntimeLayout.cmake")
if(EXISTS "${_themis_runtime_layout_file}")
    include("${_themis_runtime_layout_file}")
endif()

set(_themis_runtime_dep_install_arg)
if(WIN32)
    set(_themis_runtime_dep_install_arg RUNTIME_DEPENDENCY_SET themis_runtime_deps)
endif()

# Config Migration Scanner CLI Tool
if(EXISTS "${THEMIS_ROOT_DIR}/tools/config_migration_scanner.cpp")
    add_executable(config_migration_scanner
        ${THEMIS_ROOT_DIR}/tools/config_migration_scanner.cpp
    )
    target_compile_features(config_migration_scanner PRIVATE cxx_std_20)
    target_include_directories(config_migration_scanner PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )
    target_link_libraries(config_migration_scanner PRIVATE themis_core)
    message(STATUS "Config Migration Scanner: Enabled (config_migration_scanner)")
    install(TARGETS config_migration_scanner
        OPTIONAL
        RUNTIME DESTINATION bin
        COMPONENT tools
    )
endif()

# Admin Tools (.NET)
option(THEMIS_BUILD_ADMIN_TOOLS "Build .NET admin tools (requires .NET SDK 8.0+)" OFF)
if(THEMIS_BUILD_ADMIN_TOOLS)
    add_subdirectory(${THEMIS_ROOT_DIR}/tools ${CMAKE_CURRENT_BINARY_DIR}/tools)
endif()

# themisctl — Unified Management CLI (C++ tool, can be built independently)
option(THEMIS_BUILD_THEMISCTL "Build themisctl unified management CLI" ON)
if(THEMIS_BUILD_THEMISCTL AND EXISTS "${THEMIS_ROOT_DIR}/tools/themisctl.cpp")
    add_executable(themisctl
        ${THEMIS_ROOT_DIR}/tools/themisctl.cpp
    )

    set_target_properties(themisctl PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin_out"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin_out"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin_out"
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/bin_out"
        RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}/bin_out"
    )

    target_compile_features(themisctl PRIVATE cxx_std_17)
    target_include_directories(themisctl PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )
    if(MSVC)
        target_compile_options(themisctl PRIVATE "/I${CMAKE_HOME_DIRECTORY}/include")
    else()
        target_compile_options(themisctl PRIVATE "-I${CMAKE_HOME_DIRECTORY}/include")
    endif()
    
    # cpp-httplib provides the HTTP client (header-only)
    find_package(httplib QUIET)
    if(httplib_FOUND)
        target_link_libraries(themisctl PRIVATE httplib::httplib)
    endif()
    
    find_package(nlohmann_json QUIET)
    if(nlohmann_json_FOUND)
        target_link_libraries(themisctl PRIVATE nlohmann_json::nlohmann_json)
    endif()
    
    # OpenSSL is optional (HTTPS support via cpp-httplib)
    find_package(OpenSSL QUIET)
    if(OpenSSL_FOUND)
        target_link_libraries(themisctl PRIVATE OpenSSL::SSL OpenSSL::Crypto)
        target_compile_definitions(themisctl PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)
    endif()
    
    # GNU Readline for REPL command history (optional)
    find_package(Readline QUIET)
    if(Readline_FOUND)
        target_link_libraries(themisctl PRIVATE ${READLINE_LIBRARIES})
        target_include_directories(themisctl PRIVATE ${READLINE_INCLUDE_DIR})
        target_compile_definitions(themisctl PRIVATE THEMISCTL_ENABLE_READLINE=1)
        message(STATUS "themisctl: readline found — REPL will use line editing")
    else()
        find_library(READLINE_LIB readline)
        if(READLINE_LIB)
            target_link_libraries(themisctl PRIVATE ${READLINE_LIB})
            target_compile_definitions(themisctl PRIVATE THEMISCTL_ENABLE_READLINE=1)
            message(STATUS "themisctl: readline found (manual)")
        else()
            message(STATUS "themisctl: readline not found — using plain getline()")
        endif()
    endif()
    
    install(TARGETS themisctl
        OPTIONAL
        RUNTIME DESTINATION bin
        COMPONENT tools
    )
    
    # Shell completion scripts (optional)
    if(EXISTS "${THEMIS_ROOT_DIR}/tools/completion/themisctl.bash")
        install(FILES ${THEMIS_ROOT_DIR}/tools/completion/themisctl.bash
            DESTINATION share/bash-completion/completions
            RENAME themisctl
            COMPONENT tools
        )
    endif()
    
    if(EXISTS "${THEMIS_ROOT_DIR}/tools/completion/_themisctl")
        install(FILES ${THEMIS_ROOT_DIR}/tools/completion/_themisctl
            DESTINATION share/zsh/site-functions
            COMPONENT tools
        )
    endif()
    
    if(EXISTS "${THEMIS_ROOT_DIR}/tools/completion/themisctl.fish")
        install(FILES ${THEMIS_ROOT_DIR}/tools/completion/themisctl.fish
            DESTINATION share/fish/vendor_completions.d
            COMPONENT tools
        )
    endif()
    
    message(STATUS "themisctl unified management CLI: ENABLED")
else()
    message(STATUS "themisctl: disabled or source not found")
endif()

# Export CLI Tool (themis-export)
if(EXISTS "${THEMIS_ROOT_DIR}/tools/export_cli.cpp")
    add_executable(themis-export
        ${THEMIS_ROOT_DIR}/tools/export_cli.cpp
    )
    set_target_properties(themis-export PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin_out"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin_out"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin_out"
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/bin_out"
        RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}/bin_out"
    )
    target_link_libraries(themis-export PRIVATE
        themis_core
        nlohmann_json::nlohmann_json
        Threads::Threads
    )
    if(TARGET CURL::libcurl)
        target_link_libraries(themis-export PRIVATE CURL::libcurl)
        target_compile_definitions(themis-export PRIVATE CURL_ENABLED=1)
    endif()
    target_include_directories(themis-export PRIVATE ${THEMIS_ROOT_DIR}/include)
    message(STATUS "Export CLI Tool: Enabled (themis-export)")

    install(TARGETS themis-export
        OPTIONAL
        RUNTIME DESTINATION bin
        COMPONENT tools
    )
endif()

# LLM Model Management CLI Tool (themis-model)
if(THEMIS_ENABLE_LLM)
    if(EXISTS "${THEMIS_ROOT_DIR}/tools/themis_model_cli.cpp")
        add_executable(themis-model
            ${THEMIS_ROOT_DIR}/tools/themis_model_cli.cpp
        )
        set_target_properties(themis-model PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin_out"
            RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin_out"
            RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin_out"
            RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/bin_out"
            RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}/bin_out"
        )
        target_link_libraries(themis-model PRIVATE 
            themis_core
            OpenSSL::SSL
            OpenSSL::Crypto
        )
        if(TARGET CURL::libcurl)
            target_link_libraries(themis-model PRIVATE CURL::libcurl)
            target_compile_definitions(themis-model PRIVATE CURL_ENABLED=1)
        endif()
        if(TARGET yaml-cpp::yaml-cpp)
            target_link_libraries(themis-model PRIVATE yaml-cpp::yaml-cpp)
        elseif(TARGET yaml-cpp)
            target_link_libraries(themis-model PRIVATE yaml-cpp)
        endif()
        target_include_directories(themis-model PRIVATE ${THEMIS_ROOT_DIR}/include)
        message(STATUS "LLM Model CLI Tool: Enabled (themis-model)")
        
        # Install the tool
        install(TARGETS themis-model
            OPTIONAL
            RUNTIME DESTINATION bin
            COMPONENT tools
        )
        if(TARGET CURL::libcurl AND TARGET yaml-cpp::yaml-cpp)
            if(NOT TARGET themis-model)
                add_executable(themis-model
                    ${THEMIS_ROOT_DIR}/tools/themis_model_cli.cpp
                )
            endif()
            set_target_properties(themis-model PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin_out"
                RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin_out"
                RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin_out"
                RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/bin_out"
                RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}/bin_out"
            )
            target_link_libraries(themis-model PRIVATE 
                themis_core
                CURL::libcurl
                OpenSSL::SSL
                OpenSSL::Crypto
                yaml-cpp::yaml-cpp
            )
            target_include_directories(themis-model PRIVATE ${THEMIS_ROOT_DIR}/include)
            message(STATUS "LLM Model CLI Tool: Enabled (themis-model)")
            
            # Install the tool
            install(TARGETS themis-model
                OPTIONAL
                RUNTIME DESTINATION bin
                COMPONENT tools
            )
        else()
            message(STATUS "LLM Model CLI Tool: Disabled (requires CURL::libcurl and yaml-cpp::yaml-cpp)")
        endif()
    endif()
endif()

# Enterprise Plugins
# Plugin feature flags are defined in cmake/features/PluginFeatures.cmake which is
# already included via FeatureDefaults.cmake.
#
# Plugin implementations live flat under src/ (sources + private headers) and
# include/ (public API headers) — no extra plugins/ subdirectory.
# Manifest / configuration files for non-compiled plugins remain in plugins/.
#
# THEMIS_ENABLE_ENTERPRISE_PLUGINS is set by the edition (ON for ENTERPRISE and
# above, OFF for MINIMAL/COMMUNITY).  THEMIS_BUILD_ENTERPRISE_PLUGINS lets users
# override that default at configure time.
if(NOT DEFINED THEMIS_BUILD_ENTERPRISE_PLUGINS)
    if(DEFINED THEMIS_ENABLE_ENTERPRISE_PLUGINS)
        set(_plugin_default ${THEMIS_ENABLE_ENTERPRISE_PLUGINS})
    else()
        set(_plugin_default ON)
    endif()
    option(THEMIS_BUILD_ENTERPRISE_PLUGINS "Build enterprise plugins" ${_plugin_default})
else()
    option(THEMIS_BUILD_ENTERPRISE_PLUGINS "Build enterprise plugins" ON)
endif()

option(THEMIS_PLUGIN_SAGA_ORCHESTRATOR "Build the SAGA orchestrator plugin" ON)

# Ethics AI Plugin: always available when THEMIS_PLUGIN_ETHICS_AI is set,
# regardless of THEMIS_BUILD_ENTERPRISE_PLUGINS (dev-override supported).
if(THEMIS_PLUGIN_ETHICS_AI AND EXISTS "${THEMIS_ROOT_DIR}/src/ethics_ai/CMakeLists.txt")
    if(NOT TARGET ethics_ai_plugin)
        add_subdirectory(${THEMIS_ROOT_DIR}/src/ethics_ai
                         ${CMAKE_CURRENT_BINARY_DIR}/src/ethics_ai)
    endif()
endif()

# SAGA Orchestrator Plugin (DLL) – explicit transaction orchestration module.
# Kept outside enterprise-only gates so it can be used in focused quickwin builds.
if(THEMIS_PLUGIN_SAGA_ORCHESTRATOR AND EXISTS "${THEMIS_ROOT_DIR}/src/transaction/saga_plugin/CMakeLists.txt")
    if(NOT TARGET saga_orchestrator_plugin)
        add_subdirectory(${THEMIS_ROOT_DIR}/src/transaction/saga_plugin
                         ${CMAKE_CURRENT_BINARY_DIR}/src/transaction/saga_plugin)
    endif()
endif()

if(THEMIS_BUILD_ENTERPRISE_PLUGINS)
    # Ethics AI Plugin is now registered above independently of this block.

    # -------------------------------------------------------------------------
    # gRPC RPC Plugin (v1.3.0+) – src/rpc_grpc/
    # Flag: THEMIS_PLUGIN_RPC_GRPC
    # -------------------------------------------------------------------------
    if(THEMIS_PLUGIN_RPC_GRPC AND EXISTS "${THEMIS_ROOT_DIR}/src/rpc_grpc/CMakeLists.txt")
        add_subdirectory(${THEMIS_ROOT_DIR}/src/rpc_grpc
                         ${CMAKE_CURRENT_BINARY_DIR}/src/rpc_grpc)
    endif()

    # -------------------------------------------------------------------------
    # ONNX CLIP Image Analysis Plugin – src/onnx_clip/
    # Flag: THEMIS_PLUGIN_IMAGE_ANALYSIS_ONNX
    # -------------------------------------------------------------------------
    if(THEMIS_PLUGIN_IMAGE_ANALYSIS_ONNX AND EXISTS "${THEMIS_ROOT_DIR}/src/onnx_clip/CMakeLists.txt")
        add_subdirectory(${THEMIS_ROOT_DIR}/src/onnx_clip
                         ${CMAKE_CURRENT_BINARY_DIR}/src/onnx_clip)
    endif()

    # -------------------------------------------------------------------------
    # User Storage Encrypted Plugin (v1.5.0+) – src/user_storage_encrypted/
    # Flag: THEMIS_PLUGIN_USER_STORAGE_ENCRYPTED
    # -------------------------------------------------------------------------
    if(THEMIS_PLUGIN_USER_STORAGE_ENCRYPTED AND EXISTS "${THEMIS_ROOT_DIR}/src/user_storage_encrypted/CMakeLists.txt")
        add_subdirectory(${THEMIS_ROOT_DIR}/src/user_storage_encrypted
                         ${CMAKE_CURRENT_BINARY_DIR}/src/user_storage_encrypted)
    endif()

    # Scraper Plugin – src/scraper/
    # -------------------------------------------------------------------------
    if(EXISTS "${THEMIS_ROOT_DIR}/src/scraper/CMakeLists.txt")
        add_subdirectory(${THEMIS_ROOT_DIR}/src/scraper
                         ${CMAKE_CURRENT_BINARY_DIR}/src/scraper)
    endif()

    # Distributed Knowledge Federation module (Session 1, 2026-04-19)
    # -------------------------------------------------------------------------
    if(EXISTS "${THEMIS_ROOT_DIR}/src/distributed_knowledge/CMakeLists.txt")
        add_subdirectory(${THEMIS_ROOT_DIR}/src/distributed_knowledge
                         ${CMAKE_CURRENT_BINARY_DIR}/src/distributed_knowledge)
    endif()

    # Tensor Mid-Layer Abstractions module (Phase 4, 2026-07-01)
    # -------------------------------------------------------------------------
    if(EXISTS "${THEMIS_ROOT_DIR}/src/tensor/CMakeLists.txt")
        add_subdirectory(${THEMIS_ROOT_DIR}/src/tensor
                         ${CMAKE_CURRENT_BINARY_DIR}/src/tensor)
    endif()

    # Distributed Tensor Artifacts module (EPIC 5428 Phase 3, 2026-07-13)
    # Registration here (inside THEMIS_BUILD_ENTERPRISE_PLUGINS) is superseded by
    # the standalone block below; this guard prevents double-add in enterprise builds.
    # -------------------------------------------------------------------------
    if(EXISTS "${THEMIS_ROOT_DIR}/src/distributed_tensor/CMakeLists.txt" AND
       NOT TARGET themis_distributed_tensor)
        add_subdirectory(${THEMIS_ROOT_DIR}/src/distributed_tensor
                         ${CMAKE_CURRENT_BINARY_DIR}/src/distributed_tensor)
    endif()

    # Non-compiled plugin manifests / legacy orchestration: plugins/
    if(EXISTS "${THEMIS_ROOT_DIR}/plugins/CMakeLists.txt")
        add_subdirectory(${THEMIS_ROOT_DIR}/plugins
                         ${CMAKE_CURRENT_BINARY_DIR}/plugins)
    endif()
endif() # THEMIS_BUILD_ENTERPRISE_PLUGINS

# Public optional module submodules (geo / timeseries) are wired independently
# from enterprise plugin bundles so they can be enabled explicitly in any edition.
if(THEMIS_USE_EXTERNAL_GEO_PLUGIN)
    if(EXISTS "${THEMIS_GEO_PLUGIN_SUBMODULE_DIR}/CMakeLists.txt")
        add_subdirectory("${THEMIS_GEO_PLUGIN_SUBMODULE_DIR}"
                         "${CMAKE_CURRENT_BINARY_DIR}/public_themisdb_geo")
    else()
        message(FATAL_ERROR "THEMIS_USE_EXTERNAL_GEO_PLUGIN is ON, but no CMakeLists.txt was found at ${THEMIS_GEO_PLUGIN_SUBMODULE_DIR}")
    endif()
endif()

if(THEMIS_USE_EXTERNAL_TIMESERIES_PLUGIN)
    if(EXISTS "${THEMIS_TIMESERIES_PLUGIN_SUBMODULE_DIR}/CMakeLists.txt")
        add_subdirectory("${THEMIS_TIMESERIES_PLUGIN_SUBMODULE_DIR}"
                         "${CMAKE_CURRENT_BINARY_DIR}/public_themisdb_timeseries")
    else()
        message(FATAL_ERROR "THEMIS_USE_EXTERNAL_TIMESERIES_PLUGIN is ON, but no CMakeLists.txt was found at ${THEMIS_TIMESERIES_PLUGIN_SUBMODULE_DIR}")
    endif()
endif()


# EPIC 2 Evaluation module — benchmark_matrix and hardware_profile contracts.
# Issue #5438: EPIC 2.2 benchmark matrix implementation.
# Available in all editions (not enterprise-only).
option(THEMIS_BUILD_EVALUATION "Build the EPIC 2 evaluation module (BenchmarkMatrix / HardwareProfile)" ON)
if(THEMIS_BUILD_EVALUATION AND EXISTS "${THEMIS_ROOT_DIR}/src/evaluation/CMakeLists.txt")
    if(NOT TARGET themis_evaluation)
        add_subdirectory(${THEMIS_ROOT_DIR}/src/evaluation
                         ${CMAKE_CURRENT_BINARY_DIR}/src/evaluation)
    endif()
endif()

# EPIC 1 Retrieval module — available in all editions (not enterprise-only).
# Builds themis_retrieval; opt out by passing -DTHEMIS_BUILD_RETRIEVAL=OFF.
option(THEMIS_BUILD_RETRIEVAL "Build the EPIC 1 retrieval module (LoRAPackage / PortableAdapterProduct)" ON)
if(THEMIS_BUILD_RETRIEVAL AND EXISTS "${THEMIS_ROOT_DIR}/src/retrieval/CMakeLists.txt")
    if(NOT TARGET themis_retrieval)
        add_subdirectory(${THEMIS_ROOT_DIR}/src/retrieval
                         ${CMAKE_CURRENT_BINARY_DIR}/src/retrieval)
    endif()
endif() # THEMIS_BUILD_RETRIEVAL

# LLM Wiki module – workspace state persistence + edition gating (Phase 3).
if(EXISTS "${THEMIS_ROOT_DIR}/src/llm_wiki/CMakeLists.txt" AND
   NOT TARGET themis_llm_wiki)
    add_subdirectory(${THEMIS_ROOT_DIR}/src/llm_wiki
                     ${CMAKE_CURRENT_BINARY_DIR}/src/llm_wiki)
endif()

# Distributed Tensor Artifacts module (EPIC 5428) is community-visible and
# must be available outside enterprise-plugin builds as well.
if(EXISTS "${THEMIS_ROOT_DIR}/src/distributed_tensor/CMakeLists.txt" AND
   NOT TARGET themis_distributed_tensor)
    add_subdirectory(${THEMIS_ROOT_DIR}/src/distributed_tensor
                     ${CMAKE_CURRENT_BINARY_DIR}/src/distributed_tensor)
endif()

# Enterprise Modules (new modular DLL architecture)
option(THEMIS_BUILD_ENTERPRISE "Enable enterprise module builds" OFF)
if(THEMIS_BUILD_ENTERPRISE)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/enterprise")
        add_subdirectory(src/enterprise)
    else()
        message(WARNING "Enterprise source code not found. Enterprise features require a commercial license. Contact sales@themisdb.com")
        set(THEMIS_BUILD_ENTERPRISE OFF CACHE BOOL "Enterprise source not available" FORCE)
    endif()
endif()

# Installation
if(THEMIS_BUILD_MODULAR)
    set(_themis_modular_install_targets)
    foreach(_themis_mod_target IN LISTS THEMIS_ALL_MODULES)
        if(TARGET ${_themis_mod_target})
            get_target_property(_themis_mod_type ${_themis_mod_target} TYPE)
            if(NOT _themis_mod_type STREQUAL "INTERFACE_LIBRARY")
                list(APPEND _themis_modular_install_targets ${_themis_mod_target})
            endif()
        endif()
    endforeach()

    if(_themis_modular_install_targets)
        install(TARGETS ${_themis_modular_install_targets}
            ${_themis_runtime_dep_install_arg}
            RUNTIME DESTINATION bin
                COMPONENT runtime
            LIBRARY DESTINATION lib
                COMPONENT runtime
            ARCHIVE DESTINATION lib
                COMPONENT development
        )
    endif()
endif()

install(TARGETS themis_server themis_core
    ${_themis_runtime_dep_install_arg}
    RUNTIME DESTINATION bin
        COMPONENT runtime
    LIBRARY DESTINATION lib
        COMPONENT runtime
    ARCHIVE DESTINATION lib
        COMPONENT development
)

install(DIRECTORY ${THEMIS_ROOT_DIR}/include/
    DESTINATION include
    COMPONENT development
    PATTERN "mock_clip_processor.h" EXCLUDE
)

if(DEFINED THEMIS_RELEASE_RUNTIME_DIRS)
    foreach(_themis_runtime_dir IN LISTS THEMIS_RELEASE_RUNTIME_DIRS)
        if(EXISTS "${THEMIS_ROOT_DIR}/${_themis_runtime_dir}")
            if(UNIX AND NOT APPLE AND _themis_runtime_dir STREQUAL "config")
                install(DIRECTORY "${THEMIS_ROOT_DIR}/${_themis_runtime_dir}/"
                    DESTINATION etc/themisdb
                    COMPONENT runtime
                    OPTIONAL
                )
            else()
                install(DIRECTORY "${THEMIS_ROOT_DIR}/${_themis_runtime_dir}/"
                    DESTINATION ${_themis_runtime_dir}
                    COMPONENT runtime
                    OPTIONAL
                )
            endif()

            # Compatibility layout: when operators start the server from bin/, relative
            # config/* paths should still resolve without requiring a cwd change.
            if(_themis_runtime_dir STREQUAL "config")
                install(DIRECTORY "${THEMIS_ROOT_DIR}/${_themis_runtime_dir}/"
                    DESTINATION bin/${_themis_runtime_dir}
                    COMPONENT runtime
                    OPTIONAL
                )
            endif()
        endif()
    endforeach()
endif()

if(UNIX AND NOT APPLE)
    install(FILES "${THEMIS_ROOT_DIR}/debian/themisdb.service"
        DESTINATION lib/systemd/system
        COMPONENT runtime
        OPTIONAL
    )
endif()
# Central logs/ directory — all runtime log files are written here.
# The .gitkeep placeholder creates the directory in the archive; the server
# auto-creates logs/ at startup if it does not yet exist.
install(FILES "${THEMIS_ROOT_DIR}/logs/.gitkeep"
    DESTINATION logs
    COMPONENT runtime
)
# Compatibility: server is typically started from bin/, so logs/ must also be
# reachable as bin/../logs (already covered by the rule above).

if(WIN32)
    set(_themis_runtime_dependency_dirs "${CMAKE_BINARY_DIR}/bin")
    if(DEFINED VCPKG_INSTALLED_DIR AND DEFINED VCPKG_TARGET_TRIPLET)
        list(APPEND _themis_runtime_dependency_dirs
            "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin")
    endif()

    install(RUNTIME_DEPENDENCY_SET themis_runtime_deps
        DESTINATION bin
        COMPONENT runtime
        DIRECTORIES ${_themis_runtime_dependency_dirs}
        PRE_EXCLUDE_REGEXES "api-ms-.*" "ext-ms-.*"
        POST_EXCLUDE_REGEXES ".*/system32/.*\\.dll" ".*/cmake/.*\\.dll"
    )

    install(FILES "${CMAKE_BINARY_DIR}/bin/faiss.dll"
        DESTINATION bin
        COMPONENT runtime
        OPTIONAL
    )

    foreach(_themis_blas_runtime_dll IN ITEMS
        "openblas.dll"
        "liblapack.dll"
        "libgcc_s_seh-1.dll"
        "libgfortran-5.dll"
        "libquadmath-0.dll"
        "libwinpthread-1.dll"
    )
        install(FILES "${CMAKE_BINARY_DIR}/bin/${_themis_blas_runtime_dll}"
            DESTINATION bin
            COMPONENT runtime
            OPTIONAL
        )
    endforeach()
endif()

# Performance Optimization Benchmarks


# Final MSVC cleanup: strip any invalid '/WX:NO' that might have been injected
if(MSVC)
    get_property(_allTargets DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
    foreach(_t IN LISTS _allTargets)
        foreach(_prop IN ITEMS COMPILE_OPTIONS INTERFACE_COMPILE_OPTIONS)
            get_target_property(_opts ${_t} ${_prop})
            if(_opts)
                list(TRANSFORM _opts REPLACE "/WX:NO" "")
                list(TRANSFORM _opts REPLACE "/WX:No" "")
                list(TRANSFORM _opts REPLACE "/WX:0"  "")
                set_target_properties(${_t} PROPERTIES ${_prop} "${_opts}")
            endif()
        endforeach()
    endforeach()
endif()
