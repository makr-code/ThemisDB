# GPU/Acceleration Backend Sources
# Multiple backend implementations: CUDA, HIP, NCCL, RCCL, OpenCL, OneAPI, etc.
# Requires: THEMIS_ENABLE_GPU

if(THEMIS_ENABLE_GPU)
# Multi-threaded CPU backends
list(APPEND THEMIS_CORE_SOURCES
    ../src/acceleration/cpu_backend_mt.cpp
    ../src/acceleration/cpu_backend_tbb.cpp
)

# DirectX Windows backend
if(WIN32)
    list(APPEND THEMIS_CORE_SOURCES
        ../src/acceleration/directx_backend_full.cpp
    )
endif()

# HIP backend (AMD GPUs)
if(THEMIS_ENABLE_HIP)
    list(APPEND THEMIS_CORE_SOURCES
        ../src/acceleration/hip_backend.cpp
        ../src/llm/lora_framework/kernels/hip_fused_kernels.cpp
        ../src/index/rotary_embeddings_hip.cpp
    )
endif()

# NCCL backend (NVIDIA multi-GPU)
if(THEMIS_ENABLE_CUDA)
    list(APPEND THEMIS_CORE_SOURCES
        ../src/llm/lora_framework/nccl_backend.cpp
        ../src/index/rotary_embeddings_cuda.cu
    )
endif()

# RCCL backend (AMD multi-GPU)
if(THEMIS_ENABLE_HIP)
    list(APPEND THEMIS_CORE_SOURCES
        ../src/llm/lora_framework/rccl_backend.cpp
    )
endif()

# Intel OneAPI backend
if(THEMIS_ENABLE_ONEAPI)
    list(APPEND THEMIS_CORE_SOURCES
        ../src/acceleration/oneapi_backend.cpp
    )
endif()

# OpenCL backend (cross-platform)
if(THEMIS_ENABLE_OPENCL)
    list(APPEND THEMIS_CORE_SOURCES
        ../src/acceleration/opencl_backend.cpp
    )
endif()

# FAISS GPU backend for vector search
if(THEMIS_ENABLE_CUDA OR THEMIS_ENABLE_HIP)
    list(APPEND THEMIS_CORE_SOURCES
        ../src/acceleration/faiss_gpu_backend.cpp
    )
endif()

# Memory management for multi-GPU scenarios
list(APPEND THEMIS_CORE_SOURCES
    ../src/llm/lora_framework/paged_memory_manager.cpp
    ../src/llm/lora_framework/custom_allreduce.cpp
)
endif()
