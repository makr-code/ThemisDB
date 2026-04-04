# ThemisDB Feature Validation
# Validates feature dependencies and compatibility

if(NOT THEMIS_FEATURES_CONFIGURED)
    message(FATAL_ERROR "FeatureValidation.cmake requires FeatureDefaults.cmake to be included first")
endif()

message(STATUS "Validating feature dependencies...")

# Vision requires LLM
if(THEMIS_ENABLE_VISION AND NOT THEMIS_ENABLE_LLM)
    message(FATAL_ERROR "Vision support (THEMIS_ENABLE_VISION) requires LLM to be enabled (THEMIS_ENABLE_LLM=ON)")
endif()

# Content processors require content ingestion
if(THEMIS_ENABLE_CONTENT_PROCESSORS AND NOT THEMIS_ENABLE_CONTENT)
    message(WARNING "Content processors enabled without content ingestion module. Consider enabling THEMIS_ENABLE_CONTENT.")
endif()

# Distributed training requires GPU
if(THEMIS_ENABLE_DISTRIBUTED_TRAINING AND NOT THEMIS_ENABLE_GPU)
    message(FATAL_ERROR "Distributed training requires GPU acceleration to be enabled (THEMIS_ENABLE_GPU=ON)")
endif()

# GPU backends require main GPU feature
if((THEMIS_ENABLE_CUDA OR THEMIS_ENABLE_HIP OR THEMIS_ENABLE_ONEAPI OR THEMIS_ENABLE_OPENCL) 
   AND NOT THEMIS_ENABLE_GPU)
    message(FATAL_ERROR "GPU backends (CUDA/HIP/OneAPI/OpenCL) require GPU acceleration to be enabled (THEMIS_ENABLE_GPU=ON)")
endif()

# Warn about multiple GPU backends
set(_gpu_backend_count 0)
if(THEMIS_ENABLE_CUDA)
    math(EXPR _gpu_backend_count "${_gpu_backend_count} + 1")
endif()
if(THEMIS_ENABLE_HIP)
    math(EXPR _gpu_backend_count "${_gpu_backend_count} + 1")
endif()
if(THEMIS_ENABLE_ONEAPI)
    math(EXPR _gpu_backend_count "${_gpu_backend_count} + 1")
endif()
if(THEMIS_ENABLE_OPENCL)
    math(EXPR _gpu_backend_count "${_gpu_backend_count} + 1")
endif()

if(_gpu_backend_count GREATER 1)
    message(WARNING "Multiple GPU backends enabled. This may increase binary size and build time.")
endif()

# CUDA-specific checks
if(THEMIS_ENABLE_CUDA)
    # Check if CUDA is actually available on the system
    # Note: Full CUDA toolkit detection happens in Dependencies.cmake
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        message(STATUS "  CUDA backend enabled - CUDA toolkit must be available")
        
        # Check for CUDA environment variables
        if(NOT DEFINED ENV{CUDA_HOME} AND NOT DEFINED ENV{CUDA_PATH} AND NOT DEFINED ENV{CUDA_TOOLKIT_ROOT_DIR})
            message(WARNING "CUDA environment variables not set. CUDA may not be found.")
            message(WARNING "  Set CUDA_HOME, CUDA_PATH, or CUDA_TOOLKIT_ROOT_DIR environment variable")
        endif()
    endif()
    
    # CUDA requires GPU feature
    if(NOT THEMIS_ENABLE_GPU)
        message(WARNING "CUDA enabled but GPU feature disabled - enabling GPU automatically")
        set(THEMIS_ENABLE_GPU ON CACHE BOOL "GPU enabled automatically for CUDA" FORCE)
    endif()
endif()

# HIP-specific checks
if(THEMIS_ENABLE_HIP)
    if(NOT THEMIS_ENABLE_GPU)
        message(WARNING "HIP enabled but GPU feature disabled - enabling GPU automatically")
        set(THEMIS_ENABLE_GPU ON CACHE BOOL "GPU enabled automatically for HIP" FORCE)
    endif()
    
    # Check for ROCm environment
    if(NOT DEFINED ENV{ROCM_PATH} AND NOT DEFINED ENV{HIP_PATH})
        message(WARNING "ROCm/HIP environment variables not set. HIP may not be found.")
        message(WARNING "  Set ROCM_PATH or HIP_PATH environment variable")
    endif()
endif()

# DiskANN requires GPU
if(THEMIS_ENABLE_DISKANN AND NOT THEMIS_ENABLE_GPU)
    message(FATAL_ERROR "DiskANN vector indexing requires GPU acceleration to be enabled (THEMIS_ENABLE_GPU=ON)")
endif()

# gRPC protocol feature validation
if(THEMIS_ENABLE_GRPC)
    message(STATUS "  gRPC enabled - inter-shard communication available")
    
    # Check if protobuf is needed for other features
    if(THEMIS_ENABLE_TRACING)
        message(STATUS "    gRPC + Tracing: OTLP gRPC exporter available")
    endif()
endif()

# Static build warnings
if(THEMIS_STATIC_BUILD)
    message(WARNING "Static build enabled. This may cause issues with some dependencies.")
    
    if(THEMIS_ENABLE_GRPC)
        message(WARNING "  gRPC with static build may require special handling")
    endif()
    
    if(THEMIS_ENABLE_CUDA)
        message(WARNING "  CUDA with static build is not recommended")
    endif()
endif()

# Address Sanitizer incompatibilities
if(THEMIS_ENABLE_ASAN)
    if(THEMIS_STATIC_BUILD)
        message(WARNING "AddressSanitizer with static build may not work correctly")
    endif()
    
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        message(WARNING "AddressSanitizer is typically used with Debug builds")
    endif()
    
    if(THEMIS_ENABLE_CUDA)
        message(WARNING "AddressSanitizer may not work correctly with CUDA code")
    endif()
endif()

# Performance optimization conflicts
if(THEMIS_QNAP_BUILD AND THEMIS_ENABLE_AVX2)
    message(WARNING "AVX2 should be disabled for QNAP builds (Celeron N5095 limitation)")
    message(WARNING "  Setting THEMIS_ENABLE_AVX2=OFF")
    set(THEMIS_ENABLE_AVX2 OFF CACHE BOOL "AVX2 disabled for QNAP" FORCE)
endif()

# Huge pages platform check
if(THEMIS_ENABLE_HUGE_PAGES)
    if(NOT THEMIS_TARGET_PLATFORM MATCHES "^(linux|docker)$")
        message(WARNING "Huge pages are Linux-specific. Disabling on ${THEMIS_TARGET_PLATFORM}")
        set(THEMIS_ENABLE_HUGE_PAGES OFF CACHE BOOL "Huge pages not supported on ${THEMIS_TARGET_PLATFORM}" FORCE)
    endif()
endif()

# HTTP/3 requires HTTP/2 (QUIC builds on HTTP/2 concepts)
if(THEMIS_ENABLE_HTTP3 AND NOT THEMIS_ENABLE_HTTP2)
    message(WARNING "HTTP/3 typically works best with HTTP/2 also enabled for fallback")
endif()

# MCP requires LLM for meaningful integration
if(THEMIS_ENABLE_MCP AND NOT THEMIS_ENABLE_LLM)
    message(WARNING "MCP (Model Context Protocol) is most useful with LLM integration enabled")
endif()

# Tracing with minimal overhead check
if(THEMIS_ENABLE_TRACING AND CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
    message(WARNING "Tracing adds overhead - consider disabling for minimal size builds")
endif()

# Validate build configuration completeness
message(STATUS "Feature validation: OK")

# Calculate total GPU backends for summary
set(_gpu_backend_count 0)
if(THEMIS_ENABLE_CUDA)
    math(EXPR _gpu_backend_count "${_gpu_backend_count} + 1")
endif()
if(THEMIS_ENABLE_HIP)
    math(EXPR _gpu_backend_count "${_gpu_backend_count} + 1")
endif()
if(THEMIS_ENABLE_ONEAPI)
    math(EXPR _gpu_backend_count "${_gpu_backend_count} + 1")
endif()
if(THEMIS_ENABLE_OPENCL)
    math(EXPR _gpu_backend_count "${_gpu_backend_count} + 1")
endif()

message(STATUS "  Total features enabled: ${_gpu_backend_count} GPU backend(s)")

# Summary of key features
set(_key_features "")
if(THEMIS_ENABLE_LLM)
    list(APPEND _key_features "LLM")
endif()
if(THEMIS_ENABLE_GPU)
    list(APPEND _key_features "GPU")
endif()
if(THEMIS_ENABLE_GRPC)
    list(APPEND _key_features "gRPC")
endif()
if(THEMIS_ENABLE_TRACING)
    list(APPEND _key_features "Tracing")
endif()

if(_key_features)
    string(REPLACE ";" ", " _features_str "${_key_features}")
    message(STATUS "  Key features: ${_features_str}")
else()
    message(STATUS "  Key features: None (minimal build)")
endif()
