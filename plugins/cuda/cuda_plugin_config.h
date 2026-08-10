/**
 * @file cuda_plugin_config.h
 * @brief Runtime configuration for the ThemisDB standalone CUDA acceleration plugin.
 *
 * Selects vector backend mode (single-GPU / multi-GPU / FAISS-GPU) and the
 * floating-point precision used for distance computations.
 *
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 */

#pragma once

#include "acceleration/compute_backend.h"
#include "acceleration/multi_gpu_backend.h"
#include "acceleration/faiss_gpu_backend.h"
#include <string>

namespace themis {
namespace acceleration {

/// @brief Selects which vector backend the CUDA plugin provides.
enum class CudaPluginVectorMode {
    SINGLE_GPU,  ///< Standard CUDAVectorBackend (default)
    MULTI_GPU,   ///< MultiGPUVectorBackend (sharded, N devices)
    FAISS_GPU,   ///< FaissGPUVectorBackend (FAISS-GPU / cuVS ANN)
};

/// @brief Precision mode for distance computations inside the CUDA plugin.
enum class CudaPluginPrecision {
    FP32,  ///< Default 32-bit float (all sm_ versions)
    FP16,  ///< Half-precision (sm_70+; Tensor Core if available)
    BF16,  ///< BFloat16 (sm_80+; automatically falls back to FP32 on older GPUs)
};

/// @brief Runtime configuration for CUDAAccelerationPlugin.
///
/// Pass an instance of this struct to the two-argument
/// CUDAAccelerationPlugin constructor to override the default
/// single-GPU FP32 behaviour.
struct CudaPluginConfig {
    /// @brief Vector backend flavour to create via createVectorBackend().
    CudaPluginVectorMode vectorMode = CudaPluginVectorMode::SINGLE_GPU;

    /// @brief Floating-point precision for vector distance kernels.
    CudaPluginPrecision precision = CudaPluginPrecision::FP32;

    /// @brief Sub-configuration used when vectorMode == MULTI_GPU.
    MultiGPUVectorBackend::Config multiGpuConfig;

    /// @brief Sub-configuration used when vectorMode == FAISS_GPU.
    FaissGPUVectorBackend::Config faissConfig;
};

} // namespace acceleration
} // namespace themis
