/**
 * @file gpu_checked_ops.h
 * @brief Checked CUDA/HIP operation macros for safe GPU calls.
 *
 * Provides convenience macros for error-checked GPU operations:
 * - CHECKED_CUDA(stmt) - Error checking for CUDA calls
 * - CHECKED_HIP(stmt) - Error checking for HIP calls
 * - TRY_CUDA(stmt, fallback) - Custom error handling for CUDA
 *
 * This header includes and re-exports the checked operation macros
 * defined in gpu_error.h for convenient access.
 *
 * ## Purpose
 *
 * Wraps raw CUDA/HIP calls with automatic error detection, diagnostic
 * logging, and recovery policy application. Enables fail-closed behavior:
 * on GPU errors, automatically degrade to CPU execution.
 *
 * ## Supported Operations
 *
 * Safe to use with:
 * - Memory operations: cudaMalloc, cudaFree, cudaMemcpy, hipMalloc, hipFree, hipMemcpy
 * - Kernel launches: cudaLaunchKernel, hipLaunchKernel
 * - Stream/event operations: cudaStreamCreate, cudaEventRecord, etc.
 * - Device queries: cudaGetDeviceCount, hipGetDeviceCount, etc.
 *
 * ## Usage Example
 *
 * ```cpp
 * #include "themis/gpu/gpu_checked_ops.h"
 * using namespace themis::gpu;
 *
 * // CHECKED_CUDA macro: auto-logs on error, applies recovery policy
 * float* d_data = nullptr;
 * CHECKED_CUDA(cudaMalloc(&d_data, num_bytes));  // throws on critical error
 *
 * // TRY_CUDA macro: custom error handling
 * int retry_count = 0;
 * TRY_CUDA(cudaMemcpy(d_data, h_data, num_bytes, cudaMemcpyHostToDevice), {
 *   if (++retry_count < 3) {
 *     std::this_thread::sleep_for(std::chrono::milliseconds(100));
 *     // retry logic here
 *   }
 * });
 *
 * // RAII cleanup (no manual cudaFree needed)
 * {
 *   auto gpu_data = make_unique_gpu<float>(count);
 *   CHECKED_CUDA(cudaMemcpy(gpu_data.get(), host_data, count * sizeof(float),
 *                           cudaMemcpyHostToDevice));
 *   // ... use gpu_data ...
 * }  // automatically freed on scope exit
 * ```
 *
 * ## Error Handling Strategy
 *
 * **CHECKED_CUDA/CHECKED_HIP** (recommended):
 * - Executes stmt and captures error code
 * - On error: logs diagnostic via spdlog, applies recovery policy
 * - Policy is per error class (see GPUErrorClass taxonomy)
 * - May throw on critical errors (depending on policy and build config)
 * - Suitable for: critical path operations (malloc, kernel launch)
 *
 * **TRY_CUDA** (advanced):
 * - Executes stmt and captures error code
 * - On error: logs diagnostic, executes custom fallback_action
 * - Does NOT apply handler recovery policy
 * - Useful for: retry patterns, custom error handling
 *
 * ## Macros
 *
 * @def CHECKED_CUDA(stmt)
 * Safe CUDA operation wrapper. On error, automatically handles via
 * GPUErrorHandler::handleError(). Thread-safe; zero overhead on success.
 *
 * @def CHECKED_HIP(stmt)
 * Safe HIP operation wrapper. Identical to CHECKED_CUDA but for HIP calls.
 *
 * @def TRY_CUDA(stmt, fallback_action)
 * Custom CUDA error handler. On error, executes fallback_action instead
 * of applying default recovery policy.
 *
 * ## Error Codes
 *
 * Each CUDA/HIP error code maps to a GPUErrorClass taxonomy:
 * - kQuotaExceeded: Memory allocation failures (OOM)
 * - kKernelTimeout: Kernel execution exceeds SLA (5 seconds)
 * - kBackendUnavailable: Device offline or driver error
 * - kMemoryCommunication: Host<->Device transfer failure
 * - kNumerical: NaN or precision loss detection
 * - kUnsupportedOperation: Kernel not supported by device
 *
 * Recovery policy is automatically selected per error class.
 * See GPUErrorHandler::defaultPolicy() for detailed mappings.
 *
 * ## Integration with RAII
 *
 * Use with gpu_memory.h for fully safe GPU memory management:
 * - unique_gpu_ptr: RAII ownership (like std::unique_ptr)
 * - make_unique_gpu: Factory for safe allocation
 * - shared_gpu_ptr: Reference-counted shared ownership
 * - make_shared_gpu: Factory for shared allocation
 *
 * @see include/themis/gpu/gpu_error.h (GPUErrorHandler, GPUErrorClass)
 * @see include/themis/gpu/gpu_memory.h (RAII wrappers)
 * @see include/themis/gpu/gpu_timeout.h (KernelSLAGuard)
 * @see ai_working/gpu_phase_c_readiness_plan.md (Phase C roadmap)
 *
 * @author ThemisDB GPU Team
 * @date 2026-08-18
 */

#pragma once

#include "themis/gpu/gpu_error.h"

namespace themis {
namespace gpu {

/// @brief Convenience alias: use CHECKED_CUDA for safe CUDA operations
/// @see CHECKED_CUDA macro in gpu_error.h
/// 
/// Thread-safe; safe to use in concurrent contexts.
/// Zero overhead on success path (inline, branch-prediction friendly).

/// @brief Convenience alias: use CHECKED_HIP for safe HIP operations
/// @see CHECKED_HIP macro in gpu_error.h
/// 
/// Thread-safe; safe to use in concurrent HIP streams.
/// Zero overhead on success path.

/// @brief Convenience alias: use TRY_CUDA for custom error handling
/// @see TRY_CUDA macro in gpu_error.h
/// 
/// Useful for retry patterns and custom fallback logic.
/// Does not apply automatic recovery policy.

}  // namespace gpu
}  // namespace themis

/*
 * NOTE: CHECKED_CUDA, CHECKED_HIP, and TRY_CUDA macros are defined in
 * gpu_error.h (lines 382-482). This header provides convenient documentation
 * and re-export of those macros for GPU operations using CHECKED_* patterns.
 */
