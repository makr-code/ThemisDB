# ThemisDB GPU and Acceleration Features
# GPU backends: Vulkan (default), CUDA, HIP, OneAPI, OpenCL
# AI hardware backends: Apple ANE, Intel NPU, Qualcomm QNN, ARM Ethos, NNAPI, ONNX Runtime

# Main GPU feature (already set by edition, but allow user override)
if(NOT DEFINED THEMIS_ENABLE_GPU)
    option(THEMIS_ENABLE_GPU "Enable GPU acceleration" OFF)
endif()

# CUDA backend
if(NOT DEFINED THEMIS_ENABLE_CUDA)
    option(THEMIS_ENABLE_CUDA "Enable CUDA backend" OFF)
endif()

# cuVS/RAFT ANN acceleration (requires CUDA backend)
if(NOT DEFINED THEMIS_ENABLE_CUVS)
    option(THEMIS_ENABLE_CUVS "Enable cuVS/RAFT CUDA ANN dispatch path" OFF)
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

# GPU Vector Search (default ON if GPU enabled)
if(NOT DEFINED THEMIS_ENABLE_VECTOR_SEARCH)
    if(THEMIS_ENABLE_GPU)
        option(THEMIS_ENABLE_VECTOR_SEARCH "Enable GPU-accelerated vector search" ON)
    else()
        option(THEMIS_ENABLE_VECTOR_SEARCH "Enable GPU-accelerated vector search" OFF)
    endif()
endif()

# Vulkan backend (cross-platform, default)
if(NOT DEFINED THEMIS_ENABLE_VULKAN)
    option(THEMIS_ENABLE_VULKAN "Enable Vulkan compute backend" ON)
endif()

# ── AI Hardware / NPU backends ────────────────────────────────────────────────
# All NPU backends use the inverted-flag convention: active when the SDK is
# present and -DTHEMIS_ENABLE_* is set; disable individual backends with
# -DTHEMIS_DISABLE_* without affecting others.
#
# Apple ANE (Core ML): auto-enabled on Apple platforms; disable with
#   -DTHEMIS_DISABLE_NPU_APPLE=ON
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin" OR CMAKE_SYSTEM_NAME STREQUAL "iOS")
    if(NOT DEFINED THEMIS_DISABLE_NPU_APPLE)
        option(THEMIS_DISABLE_NPU_APPLE "Disable Apple Neural Engine (active on Apple platforms)" OFF)
    endif()
    if(NOT THEMIS_DISABLE_NPU_APPLE)
        add_compile_definitions(THEMIS_HAS_NPU_APPLE=1)
        message(STATUS "    Apple ANE (Core ML): ON  (disable: -DTHEMIS_DISABLE_NPU_APPLE=ON)")
    endif()
endif()

# Intel NPU (OpenVINO): opt-in; disable with -DTHEMIS_DISABLE_NPU_INTEL=ON
if(NOT DEFINED THEMIS_ENABLE_NPU_INTEL)
    option(THEMIS_ENABLE_NPU_INTEL "Enable Intel NPU via OpenVINO" OFF)
endif()
if(THEMIS_ENABLE_NPU_INTEL AND NOT THEMIS_DISABLE_NPU_INTEL)
    add_compile_definitions(THEMIS_ENABLE_NPU_INTEL=1)
    find_package(OpenVINO QUIET)
    if(OpenVINO_FOUND)
        message(STATUS "    Intel NPU (OpenVINO): ON  (OpenVINO ${OpenVINO_VERSION} found)")
    else()
        message(STATUS "    Intel NPU (OpenVINO): headers not found at configure time — runtime probe will fail")
    endif()
endif()

# Qualcomm AI Engine (QNN): opt-in; disable with -DTHEMIS_DISABLE_NPU_QUALCOMM=ON
if(NOT DEFINED THEMIS_ENABLE_NPU_QUALCOMM)
    option(THEMIS_ENABLE_NPU_QUALCOMM "Enable Qualcomm QNN / Hexagon DSP" OFF)
endif()
if(THEMIS_ENABLE_NPU_QUALCOMM AND NOT THEMIS_DISABLE_NPU_QUALCOMM)
    add_compile_definitions(THEMIS_ENABLE_NPU_QUALCOMM=1)
    message(STATUS "    Qualcomm AI Engine (QNN): ON  (requires QNN SDK in path)")
endif()

# ARM Ethos-N: opt-in; disable with -DTHEMIS_DISABLE_NPU_ARM=ON
if(NOT DEFINED THEMIS_ENABLE_NPU_ARM)
    option(THEMIS_ENABLE_NPU_ARM "Enable ARM Ethos-N NPU" OFF)
endif()
if(THEMIS_ENABLE_NPU_ARM AND NOT THEMIS_DISABLE_NPU_ARM)
    add_compile_definitions(THEMIS_ENABLE_NPU_ARM=1)
    message(STATUS "    ARM Ethos-N NPU: ON  (requires Ethos-N runtime)")
endif()

# Android NNAPI: auto-enabled on Android; disable with -DTHEMIS_DISABLE_NNAPI=ON
if(CMAKE_SYSTEM_NAME STREQUAL "Android")
    if(NOT DEFINED THEMIS_DISABLE_NNAPI)
        option(THEMIS_DISABLE_NNAPI "Disable Android NNAPI (active on Android)" OFF)
    endif()
    if(NOT THEMIS_DISABLE_NNAPI)
        add_compile_definitions(THEMIS_HAS_NNAPI=1)
        message(STATUS "    Android NNAPI: ON  (disable: -DTHEMIS_DISABLE_NNAPI=ON)")
    endif()
endif()

# ONNX Runtime: ON by default when headers are found; disable with
#   -DTHEMIS_DISABLE_ONNX_RUNTIME=ON
if(NOT DEFINED THEMIS_DISABLE_ONNX_RUNTIME)
    option(THEMIS_DISABLE_ONNX_RUNTIME "Disable ONNX Runtime AI inference" OFF)
endif()
if(NOT THEMIS_DISABLE_ONNX_RUNTIME)
    find_package(onnxruntime QUIET)
    if(onnxruntime_FOUND OR TARGET onnxruntime::onnxruntime)
        message(STATUS "    ONNX Runtime: ON  (found via find_package)")
    else()
        # Check for header-only availability (manual installation)
        find_path(ORT_INCLUDE_DIR
            NAMES onnxruntime_c_api.h
            PATHS /usr/include/onnxruntime
                  /usr/local/include/onnxruntime
                  $ENV{ORT_ROOT}/include
                  ${CMAKE_PREFIX_PATH}/include/onnxruntime)
        if(ORT_INCLUDE_DIR)
            message(STATUS "    ONNX Runtime: ON  (headers found at ${ORT_INCLUDE_DIR})")
            include_directories(${ORT_INCLUDE_DIR})
        else()
            message(STATUS "    ONNX Runtime: headers not found — runtime probe will select CPUExecutionProvider")
        endif()
    endif()
endif()

# Display GPU features
if(THEMIS_ENABLE_GPU)
    message(STATUS "  GPU Acceleration: Enabled")
    if(THEMIS_ENABLE_VULKAN)
        message(STATUS "    Vulkan: Enabled")
    endif()
    if(THEMIS_ENABLE_CUDA)
        message(STATUS "    CUDA: Enabled")
        if(THEMIS_ENABLE_CUVS)
            message(STATUS "    CUDA cuVS/RAFT dispatch: Enabled")
        endif()
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
    if(THEMIS_ENABLE_VECTOR_SEARCH)
        message(STATUS "    GPU Vector Search: Enabled")
    endif()
else()
    message(STATUS "  GPU Acceleration: Disabled")
endif()
