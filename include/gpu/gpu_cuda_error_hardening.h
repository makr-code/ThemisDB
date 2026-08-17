/**
 * @file gpu_cuda_error_hardening.h
 * @brief Hardened CUDA error handling with diagnostic emission and fail-closed fallback.
 * @version 1.0.0
 * @date 2026-08-17
 * 
 * Phase C hardening: Maps CUDA errors to GPUDispatchErrorCode taxonomy,
 * emits structured diagnostics, and implements fail-closed CPU fallback patterns.
 * 
 * This header bridges the low-level CUDA runtime errors and the high-level
 * GPU backend dispatch contract framework.
 * 
 * @see include/gpu/gpu_backend_dispatch_contract.h (Error code taxonomy)
 * @see include/gpu/gpu_backend_dispatch_diagnostics.h (Diagnostic emission)
 */

#pragma once

#include "gpu_backend_dispatch_contract.h"
#include "gpu_backend_dispatch_diagnostics.h"
#include <spdlog/spdlog.h>

#ifdef THEMIS_ENABLE_CUDA
#include <cuda_runtime.h>
#endif

#ifdef THEMIS_ENABLE_HIP
#include <hip/hip_runtime.h>
#endif

namespace themis {
namespace gpu {

/**
 * @brief Map CUDA error code to GPUDispatchErrorCode taxonomy.
 * 
 * @param cuda_err CUDA error code
 * @return Corresponding GPUDispatchErrorCode
 * 
 * Mapping:
 * - cudaErrorMemoryAllocation → ALLOC_DEVICE_FAILURE
 * - cudaErrorInvalidValue → ALLOC_INVALID_PARAMS
 * - cudaErrorInvalidDevice → BACKEND_NO_DEVICE_AVAILABLE
 * - cudaErrorNotSupported → BACKEND_CAPABILITY_MISMATCH
 * - cudaErrorInsufficientDriver → BACKEND_DEGRADED
 * - cudaErrorInvalidDevicePointer → INTERNAL_ERROR
 * - Other → INTERNAL_ERROR
 */
#ifdef THEMIS_ENABLE_CUDA
inline GPUDispatchErrorCode mapCudaErrorToDispatchCode(cudaError_t cuda_err) noexcept {
    switch (cuda_err) {
        case cudaSuccess:
            return GPUDispatchErrorCode::SUCCESS;
        case cudaErrorMemoryAllocation:
            return GPUDispatchErrorCode::ALLOC_DEVICE_FAILURE;
        case cudaErrorInvalidValue:
            return GPUDispatchErrorCode::ALLOC_INVALID_PARAMS;
        case cudaErrorInvalidDevice:
        case cudaErrorDeviceNotDiagnosticated:
        case cudaErrorDevicesUnavailable:
            return GPUDispatchErrorCode::BACKEND_NO_DEVICE_AVAILABLE;
        case cudaErrorNotSupported:
            return GPUDispatchErrorCode::BACKEND_CAPABILITY_MISMATCH;
        case cudaErrorInsufficientDriver:
            return GPUDispatchErrorCode::BACKEND_DEGRADED;
        case cudaErrorInvalidDevicePointer:
        case cudaErrorInvalidHostPointer:
            return GPUDispatchErrorCode::INTERNAL_ERROR;
        default:
            return GPUDispatchErrorCode::INTERNAL_ERROR;
    }
}
#endif

/**
 * @brief Map HIP error code to GPUDispatchErrorCode taxonomy.
 * 
 * @param hip_err HIP error code
 * @return Corresponding GPUDispatchErrorCode
 */
#ifdef THEMIS_ENABLE_HIP
inline GPUDispatchErrorCode mapHipErrorToDispatchCode(hipError_t hip_err) noexcept {
    switch (hip_err) {
        case hipSuccess:
            return GPUDispatchErrorCode::SUCCESS;
        case hipErrorOutOfMemory:
        case hipErrorMemoryAllocation:
            return GPUDispatchErrorCode::ALLOC_DEVICE_FAILURE;
        case hipErrorInvalidValue:
            return GPUDispatchErrorCode::ALLOC_INVALID_PARAMS;
        case hipErrorInvalidDevice:
            return GPUDispatchErrorCode::BACKEND_NO_DEVICE_AVAILABLE;
        case hipErrorNotSupported:
            return GPUDispatchErrorCode::BACKEND_CAPABILITY_MISMATCH;
        case hipErrorInsufficientDriver:
            return GPUDispatchErrorCode::BACKEND_DEGRADED;
        case hipErrorInvalidDevicePointer:
        case hipErrorInvalidHostPointer:
            return GPUDispatchErrorCode::INTERNAL_ERROR;
        default:
            return GPUDispatchErrorCode::INTERNAL_ERROR;
    }
}
#endif

/**
 * @brief Check CUDA error, emit diagnostic, and return error code.
 * 
 * @param cuda_err CUDA error code to check
 * @param context Human-readable context (function name, operation description)
 * @param device_id GPU device ID (-1 if unknown)
 * @return GPUDispatchErrorCode (SUCCESS if no error)
 * 
 * Behavior:
 * - On success: returns SUCCESS (no diagnostic emitted)
 * - On error: maps error, emits diagnostic, returns error code
 * 
 * Example:
 * ```cpp
 * GPUDispatchErrorCode err = checkCudaError(
 *     cudaMalloc(&ptr, size),
 *     "cudaMalloc",
 *     device_id);
 * if (err != GPUDispatchErrorCode::SUCCESS) {
 *     // Handle error: implement CPU fallback
 * }
 * ```
 */
#ifdef THEMIS_ENABLE_CUDA
inline GPUDispatchErrorCode checkCudaError(
    cudaError_t cuda_err,
    const std::string& context,
    int device_id = -1) noexcept {
    
    if (cuda_err == cudaSuccess) {
        return GPUDispatchErrorCode::SUCCESS;
    }
    
    GPUDispatchErrorCode code = mapCudaErrorToDispatchCode(cuda_err);
    std::string detail = "CUDA " + context + ": " + cudaGetErrorString(cuda_err);
    
    GPUBackendDispatchDiagnostics::emitDiagnostic(code, device_id, detail);
    
    return code;
}
#endif

/**
 * @brief Check HIP error, emit diagnostic, and return error code.
 * 
 * @param hip_err HIP error code to check
 * @param context Human-readable context
 * @param device_id GPU device ID (-1 if unknown)
 * @return GPUDispatchErrorCode (SUCCESS if no error)
 */
#ifdef THEMIS_ENABLE_HIP
inline GPUDispatchErrorCode checkHipError(
    hipError_t hip_err,
    const std::string& context,
    int device_id = -1) noexcept {
    
    if (hip_err == hipSuccess) {
        return GPUDispatchErrorCode::SUCCESS;
    }
    
    GPUDispatchErrorCode code = mapHipErrorToDispatchCode(hip_err);
    std::string detail = "HIP " + context + ": " + hipGetErrorString(hip_err);
    
    GPUBackendDispatchDiagnostics::emitDiagnostic(code, device_id, detail);
    
    return code;
}
#endif

/**
 * @brief RAII guard for fallback-on-error pattern.
 * 
 * Usage pattern for CUDA operations with automatic CPU fallback:
 * ```cpp
 * GPUDispatchErrorCode final_error = GPUDispatchErrorCode::SUCCESS;
 * {
 *     CudaFallbackGuard guard(final_error);
 *     CHECKED_CUDA(cudaMalloc(&ptr, size));  // captured in guard
 *     CHECKED_CUDA(cudaMemcpy(...));
 *     // On scope exit, guard's destructor checks if error occurred
 * }
 * if (final_error != GPUDispatchErrorCode::SUCCESS) {
 *     // Execute CPU fallback
 * }
 * ```
 */
class CudaFallbackGuard {
 public:
    explicit CudaFallbackGuard(GPUDispatchErrorCode& error_out, int device_id = -1)
        : error_out_(error_out), device_id_(device_id), caught_error_(false) {}
    
    ~CudaFallbackGuard() noexcept {
        if (caught_error_) {
            error_out_ = GPUDispatchErrorCode::ALLOC_DEVICE_FAILURE;
        }
    }
    
    // Called when CUDA error occurs
    void recordError(GPUDispatchErrorCode code) noexcept {
        error_out_ = code;
        caught_error_ = true;
    }
    
 private:
    GPUDispatchErrorCode& error_out_;
    int device_id_;
    bool caught_error_;
};

/**
 * @def CHECKED_CUDA_WITH_FALLBACK(stmt, fallback_block)
 * @brief CUDA error checking with fallback block execution.
 * 
 * Executes stmt. On CUDA error:
 * 1. Maps error to GPUDispatchErrorCode
 * 2. Emits diagnostic
 * 3. Executes fallback_block
 * 
 * Example:
 * ```cpp
 * bool gpu_success = false;
 * CHECKED_CUDA_WITH_FALLBACK(
 *     cudaMalloc(&ptr, size),
 *     {
 *         gpu_success = false;
 *         // implement CPU fallback here
 *     });
 * if (gpu_success) { /* use GPU result */ }
 * ```
 */
#ifdef THEMIS_ENABLE_CUDA
#define CHECKED_CUDA_WITH_FALLBACK(stmt, fallback_block) \
    do { \
        cudaError_t _cuda_err = (stmt); \
        if (_cuda_err != cudaSuccess) { \
            GPUDispatchErrorCode _dispatch_err = themis::gpu::mapCudaErrorToDispatchCode(_cuda_err); \
            std::string _detail = "CUDA " #stmt ": " + std::string(cudaGetErrorString(_cuda_err)); \
            themis::gpu::GPUBackendDispatchDiagnostics::emitDiagnostic(_dispatch_err, -1, _detail); \
            fallback_block; \
        } \
    } while (0)
#else
#define CHECKED_CUDA_WITH_FALLBACK(stmt, fallback_block) (stmt)
#endif

/**
 * @def CHECKED_HIP_WITH_FALLBACK(stmt, fallback_block)
 * @brief HIP error checking with fallback block execution.
 * 
 * Identical to CHECKED_CUDA_WITH_FALLBACK but for HIP calls.
 */
#ifdef THEMIS_ENABLE_HIP
#define CHECKED_HIP_WITH_FALLBACK(stmt, fallback_block) \
    do { \
        hipError_t _hip_err = (stmt); \
        if (_hip_err != hipSuccess) { \
            GPUDispatchErrorCode _dispatch_err = themis::gpu::mapHipErrorToDispatchCode(_hip_err); \
            std::string _detail = "HIP " #stmt ": " + std::string(hipGetErrorString(_hip_err)); \
            themis::gpu::GPUBackendDispatchDiagnostics::emitDiagnostic(_dispatch_err, -1, _detail); \
            fallback_block; \
        } \
    } while (0)
#else
#define CHECKED_HIP_WITH_FALLBACK(stmt, fallback_block) (stmt)
#endif

}  // namespace gpu
}  // namespace themis
