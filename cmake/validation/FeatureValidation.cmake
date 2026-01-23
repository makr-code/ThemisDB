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
    # CUDA requires CUDA toolkit to be found
    # This will be checked later in Dependencies.cmake, just warn here
    message(STATUS "  CUDA backend enabled - CUDA toolkit must be available")
endif()

# Static build warnings
if(THEMIS_STATIC_BUILD)
    message(WARNING "Static build enabled. This may cause issues with some dependencies.")
    
    if(THEMIS_ENABLE_GRPC)
        message(WARNING "  gRPC with static build may require special handling")
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

message(STATUS "Feature validation: OK")
