# ThemisDB GPU and Acceleration Features
# GPU backends: Vulkan (default), CUDA, HIP, OneAPI, OpenCL

# Main GPU feature (already set by edition, but allow user override)
if(NOT DEFINED THEMIS_ENABLE_GPU)
    option(THEMIS_ENABLE_GPU "Enable GPU acceleration" ON)
endif()

# CUDA backend
if(NOT DEFINED THEMIS_ENABLE_CUDA)
    option(THEMIS_ENABLE_CUDA "Enable CUDA backend" OFF)
endif()

# HIP backend (AMD)
if(NOT DEFINED THEMIS_ENABLE_HIP)
    option(THEMIS_ENABLE_HIP "Enable HIP backend" OFF)
endif()

# OneAPI backend (Intel)
if(NOT DEFINED THEMIS_ENABLE_ONEAPI)
    option(THEMIS_ENABLE_ONEAPI "Enable Intel OneAPI backend" OFF)
endif()

# OpenCL backend
if(NOT DEFINED THEMIS_ENABLE_OPENCL)
    option(THEMIS_ENABLE_OPENCL "Enable OpenCL backend" OFF)
endif()

# Distributed training (multi-GPU)
if(NOT DEFINED THEMIS_ENABLE_DISTRIBUTED_TRAINING)
    option(THEMIS_ENABLE_DISTRIBUTED_TRAINING "Enable distributed multi-GPU training" OFF)
endif()

# Display GPU features
if(THEMIS_ENABLE_GPU)
    message(STATUS "  GPU Acceleration: Enabled")
    if(THEMIS_ENABLE_CUDA)
        message(STATUS "    CUDA: Enabled")
    endif()
    if(THEMIS_ENABLE_HIP)
        message(STATUS "    HIP (AMD): Enabled")
    endif()
    if(THEMIS_ENABLE_ONEAPI)
        message(STATUS "    OneAPI (Intel): Enabled")
    endif()
    if(THEMIS_ENABLE_OPENCL)
        message(STATUS "    OpenCL: Enabled")
    endif()
    if(THEMIS_ENABLE_DISTRIBUTED_TRAINING)
        message(STATUS "    Distributed training: Enabled")
    endif()
else()
    message(STATUS "  GPU Acceleration: Disabled")
endif()
