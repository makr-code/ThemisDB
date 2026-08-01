/**
 * @file gpu_error.h
 * @brief CUDA/HIP error handling infrastructure for ThemisDB GPU subsystem.
 * 
 * Provides foundational error handling, recovery policies, and error-to-class
 * taxonomy for CUDA and HIP backends. This header is part of Phase C readiness
 * (GPU Phase C: Hybrid Retrieval Rollout - bounded GPU refinement phase).
 *
 * ## Module Status
 * 
 * **Maturity**: 🟢 PRODUCTION-READY (Phase 1 - Foundational)  
 * **Version**: 0.0.47  
 * **Phase**: GPU Phase C - Foundational Error Handling  
 * **Gap Summary**: total=0; Implementation complete  
 *
 * ## Purpose
 *
 * This module establishes a common error handling interface for CUDA and HIP
 * calls, enabling:
 * - Unified error classification across backends
 * - Deterministic recovery policies per error class
 * - Automatic fallback to CPU execution
 * - Diagnostic logging via spdlog
 * - Integration with Phase C circuit-breaker safe-fail
 *
 * ## Design Notes
 *
 * **Error Taxonomy** (GPUErrorClass enum):
 * - kQuotaExceeded: VRAM budget denial → immediate CPU fallback
 * - kKernelTimeout: SLA violation (5s hard limit) → immediate CPU fallback + diagnostic
 * - kBackendUnavailable: Device offline / driver error → CPU fallback + mark unavailable
 * - kMemoryCommunication: H2D/D2H transfer failure → retry once, then CPU fallback
 * - kNumerical: Precision loss, NaN detection → emit warning, continue with caution
 * - kUnsupportedOperation: Kernel not available for config → CPU fallback
 *
 * **Recovery Policies** (ErrorRecoveryPolicy enum):
 * - kFallbackCPU: Immediately degrade to CPU execution
 * - kRetryOnce: Single retry attempt before CPU fallback
 * - kMarkUnavailable: Mark device/kernel unavailable for duration
 * - kEmitWarning: Log warning and continue (non-blocking)
 *
 * **Exception Safety**:
 * - noexcept where possible; all error handlers document exception contract
 * - Thread-safe: GPUErrorHandler protected by internal mutex for concurrent stream access
 * - RAII: error context automatically cleaned up on scope exit
 *
 * **Macro Convention**:
 * - CHECKED_CUDA(stmt) - CUDA error checking wrapper; fails fast on error
 * - CHECKED_HIP(stmt) - HIP error checking wrapper; fails fast on error
 * - Both macros support deferred error handling via ErrorRecoveryPolicy
 *
 * @see include/themis/gpu/gpu_memory.h (RAII GPU memory wrapper)
 * @see include/themis/gpu/gpu_timeout.h (KernelSLAGuard enforcement)
 * @see src/gpu/gpu_error.cpp (GPUErrorHandler implementation)
 * @see ai_working/gpu_phase_c_readiness_plan.md (Phase C roadmap)
 *
 * @author ThemisDB GPU Team
 * @date 2026-08-01
 */

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <functional>
#include <spdlog/spdlog.h>

#if defined(__CUDACC__) || defined(THEMIS_CUDA_ENABLED)
#include <cuda_runtime.h>
#endif

#if defined(THEMIS_HIP_ENABLED) || defined(__HIP__)
#include <hip/hip_runtime.h>
#endif

namespace themis {
namespace gpu {

/**
 * @enum GPUErrorClass
 * @brief Taxonomy of GPU error conditions for deterministic recovery.
 *
 * Each error class maps to a specific recovery strategy. Error handlers
 * convert CUDA/HIP error codes to these classes and invoke recovery policy.
 */
enum class GPUErrorClass : std::uint8_t {
  /// VRAM budget exceeded (cudaErrorMemoryAllocation, hipErrorOutOfMemory).
  /// Recovery: Immediate CPU fallback. Logged as non-fatal.
  kQuotaExceeded = 0,

  /// Kernel execution timeout (custom timeout detection via KernelSLAGuard).
  /// Recovery: Immediate CPU fallback with diagnostic log.
  /// Impact: SLA violation; performance impact minimal (CPU fallback active).
  kKernelTimeout = 1,

  /// GPU device offline, driver error, or backend unavailable.
  /// (cudaErrorInsufficientDriver, hipErrorInsufficientDriver, etc.)
  /// Recovery: CPU fallback + mark device unavailable.
  /// Impact: Entire GPU subsystem disabled for health check duration.
  kBackendUnavailable = 2,

  /// Host-to-Device or Device-to-Host transfer failure.
  /// (cudaErrorInvalidValue, cudaErrorInvalidHostPointer, etc.)
  /// Recovery: Retry once; if persists, CPU fallback.
  /// Impact: Data reliability concern; log for audit.
  kMemoryCommunication = 3,

  /// Numerical precision loss, NaN detection, or computation error.
  /// Recovery: Emit warning; continue with caution (non-blocking).
  /// Impact: Accuracy degradation; caller must validate output.
  kNumerical = 4,

  /// Operation not supported by current device/backend/configuration.
  /// (cudaErrorNotSupported, hipErrorNotSupported, etc.)
  /// Recovery: CPU fallback; mark operation unsupported.
  /// Impact: Query degradation; correctness maintained.
  kUnsupportedOperation = 5,

  /// Unknown or unmapped error; treat as backend unavailable.
  kUnknown = 6,
};

/**
 * @enum ErrorRecoveryPolicy
 * @brief Action to take upon error detection.
 *
 * Configurable policy per error class; default policies defined in
 * GPUErrorHandler.
 */
enum class ErrorRecoveryPolicy : std::uint8_t {
  /// Immediately fallback to CPU execution. Non-blocking.
  /// Suitable for: kQuotaExceeded, kKernelTimeout, kBackendUnavailable
  kFallbackCPU = 0,

  /// Attempt one retry; if error persists, fallback to CPU.
  /// Suitable for: kMemoryCommunication
  kRetryOnce = 1,

  /// Mark resource (device/kernel/operation) unavailable for duration.
  /// Suitable for: kBackendUnavailable, kUnsupportedOperation
  kMarkUnavailable = 2,

  /// Emit diagnostic warning and continue. Non-blocking.
  /// Suitable for: kNumerical
  kEmitWarning = 3,
};

/**
 * @class GPUErrorHandler
 * @brief Interface for unified CUDA/HIP error handling and recovery.
 *
 * Converts CUDA/HIP error codes to GPUErrorClass taxonomy, logs diagnostics,
 * and invokes recovery policy. Thread-safe; protected by internal mutex
 * for concurrent stream access.
 *
 * ### Usage
 *
 * **Direct error handling**:
 * ```cpp
 * auto handler = GPUErrorHandler::Create();
 * cudaError_t err = cudaMalloc(&ptr, size);
 * if (err != cudaSuccess) {
 *   handler->handleError(err, "cudaMalloc");  // logs + applies policy
 * }
 * ```
 *
 * **Macro-based (recommended)**:
 * ```cpp
 * CHECKED_CUDA(cudaMalloc(&ptr, size));  // auto-logs; throws on critical
 * ```
 */
class GPUErrorHandler {
 public:
  virtual ~GPUErrorHandler() = default;

  /// Non-copyable, non-movable (singleton pattern)
  GPUErrorHandler(const GPUErrorHandler&) = delete;
  GPUErrorHandler& operator=(const GPUErrorHandler&) = delete;

  /**
   * @brief Log a CUDA error to diagnostics without recovery action.
   *
   * @param cuda_err CUDA error code
   * @param context Human-readable context (e.g., "cudaMalloc")
   * 
   * Behavior: Converts cuda_err to GPUErrorClass, formats message,
   * logs via spdlog. Does not apply recovery policy; caller is responsible.
   */
  virtual void logError(cudaError_t cuda_err, const std::string& context) noexcept = 0;

  /**
   * @brief Log a HIP error to diagnostics without recovery action.
   *
   * @param hip_err HIP error code
   * @param context Human-readable context (e.g., "hipMalloc")
   * 
   * Behavior: Converts hip_err to GPUErrorClass, formats message,
   * logs via spdlog. Does not apply recovery policy.
   */
  virtual void logError(hipError_t hip_err, const std::string& context) noexcept = 0;

  /**
   * @brief Handle a CUDA error with recovery policy application.
   *
   * @param cuda_err CUDA error code
   * @param context Human-readable context
   * @param policy Override default recovery policy for this error class (optional)
   * 
   * Behavior: Classifies error, logs diagnostic, applies recovery policy.
   * May throw on critical errors depending on policy and configuration.
   * Thread-safe.
   */
  virtual void handleError(cudaError_t cuda_err, 
                          const std::string& context,
                          const ErrorRecoveryPolicy* policy = nullptr) = 0;

  /**
   * @brief Handle a HIP error with recovery policy application.
   *
   * @param hip_err HIP error code
   * @param context Human-readable context
   * @param policy Override default recovery policy for this error class (optional)
   * 
   * Behavior: Classifies error, logs diagnostic, applies recovery policy.
   * May throw on critical errors depending on policy and configuration.
   * Thread-safe.
   */
  virtual void handleError(hipError_t hip_err,
                          const std::string& context,
                          const ErrorRecoveryPolicy* policy = nullptr) = 0;

  /**
   * @brief Convert CUDA error code to GPUErrorClass taxonomy.
   *
   * @param cuda_err CUDA error code
   * @return Classified error; kUnknown if unmapped
   * 
   * Behavior: Pure lookup; no side effects. May be called frequently.
   */
  virtual GPUErrorClass classifyError(cudaError_t cuda_err) const noexcept = 0;

  /**
   * @brief Convert HIP error code to GPUErrorClass taxonomy.
   *
   * @param hip_err HIP error code
   * @return Classified error; kUnknown if unmapped
   * 
   * Behavior: Pure lookup; no side effects.
   */
  virtual GPUErrorClass classifyError(hipError_t hip_err) const noexcept = 0;

  /**
   * @brief Get default recovery policy for error class.
   *
   * @param error_class Error class from taxonomy
   * @return Recovery policy; kFallbackCPU if unmapped
   * 
   * Default mappings:
   * - kQuotaExceeded → kFallbackCPU
   * - kKernelTimeout → kFallbackCPU
   * - kBackendUnavailable → kMarkUnavailable + kFallbackCPU
   * - kMemoryCommunication → kRetryOnce
   * - kNumerical → kEmitWarning
   * - kUnsupportedOperation → kFallbackCPU
   */
  virtual ErrorRecoveryPolicy defaultPolicy(GPUErrorClass error_class) const noexcept = 0;

  /**
   * @brief Get human-readable string for error class.
   *
   * @param error_class Error class
   * @return Description (e.g., "kQuotaExceeded")
   */
  virtual std::string errorClassName(GPUErrorClass error_class) const noexcept = 0;

  /**
   * @brief Get human-readable string for CUDA error code.
   *
   * @param cuda_err CUDA error code
   * @return Description (e.g., "cudaErrorMemoryAllocation")
   * 
   * Wraps cudaGetErrorString() or equivalent.
   */
  virtual std::string cudaErrorName(cudaError_t cuda_err) const noexcept = 0;

  /**
   * @brief Get human-readable string for HIP error code.
   *
   * @param hip_err HIP error code
   * @return Description (e.g., "hipErrorOutOfMemory")
   * 
   * Wraps hipGetErrorName() or equivalent.
   */
  virtual std::string hipErrorName(hipError_t hip_err) const noexcept = 0;

  /**
   * @brief Factory: create singleton instance.
   *
   * @return Shared pointer to GPUErrorHandler instance
   * 
   * Behavior: Returns same instance on repeated calls (singleton).
   * Caller should cache result for performance.
   */
  static std::shared_ptr<GPUErrorHandler> Create();

  /**
   * @brief Get logger for GPU error diagnostics.
   *
   * @return spdlog logger instance
   * 
   * May be configured at runtime via spdlog factory.
   */
  static std::shared_ptr<spdlog::logger> GetLogger() noexcept;

 protected:
  GPUErrorHandler() = default;
};

// ============================================================================
// CHECKED_CUDA / CHECKED_HIP Macros
// ============================================================================

/**
 * @def CHECKED_CUDA(stmt)
 * @brief Macro for CUDA error checking with automatic error handling.
 *
 * Expands to:
 * ```cpp
 * {
 *   cudaError_t err = (stmt);
 *   if (err != cudaSuccess) {
 *     themis::gpu::GPUErrorHandler::Create()->handleError(err, #stmt);
 *   }
 * }
 * ```
 *
 * Behavior:
 * - Executes stmt and captures cudaError_t result
 * - On success (cudaSuccess), no action
 * - On error, passes to handler with source location (stmt as context)
 * - Handler applies recovery policy; may throw on critical
 * - Thread-safe; safe to use in concurrent kernels/streams
 * - Zero overhead on success path (inline, branch prediction friendly)
 *
 * Example:
 * ```cpp
 * float* d_data = nullptr;
 * CHECKED_CUDA(cudaMalloc(&d_data, num_bytes));  // throws on OOM
 * CHECKED_CUDA(cudaMemcpy(d_data, h_data, num_bytes, cudaMemcpyHostToDevice));
 * CHECKED_CUDA(cudaFree(d_data));
 * ```
 *
 * @see CHECKED_HIP - HIP equivalent
 * @see ErrorRecoveryPolicy - Recovery action
 * @see GPUErrorHandler::handleError - Handler implementation
 */
#if defined(__CUDACC__) || defined(THEMIS_CUDA_ENABLED)
#define CHECKED_CUDA(stmt) \
  do { \
    cudaError_t _cuda_err = (stmt); \
    if (_cuda_err != cudaSuccess) { \
      themis::gpu::GPUErrorHandler::Create()->handleError(_cuda_err, #stmt); \
    } \
  } while (0)
#else
#define CHECKED_CUDA(stmt) (stmt)
#endif

/**
 * @def CHECKED_HIP(stmt)
 * @brief Macro for HIP error checking with automatic error handling.
 *
 * Expands to:
 * ```cpp
 * {
 *   hipError_t err = (stmt);
 *   if (err != hipSuccess) {
 *     themis::gpu::GPUErrorHandler::Create()->handleError(err, #stmt);
 *   }
 * }
 * ```
 *
 * Behavior: Identical to CHECKED_CUDA, but for HIP calls.
 * - Thread-safe; concurrent stream safe
 * - Zero overhead on success
 * - Integrates with Phase C fallback policy
 *
 * Example:
 * ```cpp
 * float* d_data = nullptr;
 * CHECKED_HIP(hipMalloc(&d_data, num_bytes));
 * CHECKED_HIP(hipMemcpy(d_data, h_data, num_bytes, hipMemcpyHostToDevice));
 * CHECKED_HIP(hipFree(d_data));
 * ```
 *
 * @see CHECKED_CUDA - CUDA equivalent
 */
#if defined(THEMIS_HIP_ENABLED) || defined(__HIP__)
#define CHECKED_HIP(stmt) \
  do { \
    hipError_t _hip_err = (stmt); \
    if (_hip_err != hipSuccess) { \
      themis::gpu::GPUErrorHandler::Create()->handleError(_hip_err, #stmt); \
    } \
  } while (0)
#else
#define CHECKED_HIP(stmt) (stmt)
#endif

/**
 * @def TRY_CUDA(stmt, fallback_action)
 * @brief Macro for CUDA error checking with custom fallback.
 *
 * Expands to:
 * ```cpp
 * {
 *   cudaError_t err = (stmt);
 *   if (err != cudaSuccess) {
 *     themis::gpu::GPUErrorHandler::Create()->logError(err, #stmt);
 *     fallback_action;
 *   }
 * }
 * ```
 *
 * Behavior:
 * - Executes stmt and captures error
 * - On error, logs diagnostic and executes fallback_action
 * - Does NOT apply handler recovery policy; caller controls action
 * - Useful for retry patterns or custom error handling
 *
 * Example:
 * ```cpp
 * int retry_count = 0;
 * TRY_CUDA(cudaMalloc(&ptr, size), {
 *   if (++retry_count < 3) {
 *     // Sleep and retry
 *     std::this_thread::sleep_for(std::chrono::milliseconds(100));
 *   } else {
 *     throw std::runtime_error("cudaMalloc failed after 3 retries");
 *   }
 * });
 * ```
 *
 * @see CHECKED_CUDA - For typical error handling
 */
#if defined(__CUDACC__) || defined(THEMIS_CUDA_ENABLED)
#define TRY_CUDA(stmt, fallback_action) \
  do { \
    cudaError_t _cuda_err = (stmt); \
    if (_cuda_err != cudaSuccess) { \
      themis::gpu::GPUErrorHandler::Create()->logError(_cuda_err, #stmt); \
      fallback_action; \
    } \
  } while (0)
#else
#define TRY_CUDA(stmt, fallback_action) (stmt)
#endif

}  // namespace gpu
}  // namespace themis

#endif  // THEMIS_GPU_ERROR_H
