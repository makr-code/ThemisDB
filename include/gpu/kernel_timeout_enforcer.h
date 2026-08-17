/**
 * @file kernel_timeout_enforcer.h
 * @brief Kernel Timeout Enforcement — Prevent Long-Running Kernel Hangs
 *
 * @version v1.0
 * @note Maturity: 🟡 BETA
 * @note Status: Wave A Batch A-8 Implementation
 *
 * Monitors GPU kernel execution and enforces timeout limits. If a kernel
 * exceeds its timeout window, execution fails closed and the caller may
 * degrade to CPU without destroying the caller-owned stream.
 *
 * ## Execution Model
 * 1. Kernel starts with timeout budget metadata
 * 2. The enforcer measures wall-clock time and, when a stream is supplied,
 *    polls stream completion with bounded waiting
 * 3. If timeout exceeded, the enforcer reports failure and drains the stream
 *    safely when one was provided
 * 4. Fallback to CPU execution (if enabled)
 *
 * @error 7700: Kernel timeout exceeded
 * @error 7701: Kernel execution failed
 */

#pragma once

#include <cstdint>
#include <functional>

#include "gpu/gpu_safe_raii.h"

namespace themis {
namespace gpu {

/**
 * @class KernelTimeoutEnforcer
 * @brief Enforce timeout limits on GPU kernel execution
 */
class KernelTimeoutEnforcer {
public:
    /// @brief Configuration for kernel timeout
    struct KernelConfig {
        uint32_t timeout_ms = 10000;      ///< Timeout in milliseconds (default: 10s)
        bool enable_fallback = true;      ///< Fall back to CPU on timeout
        cudaStream_t stream = nullptr;    ///< CUDA stream for execution
    };

    /// @brief Construct enforcer
    KernelTimeoutEnforcer() = default;

    /// @brief Execute kernel with timeout protection
    /// @param kernel_lambda Function containing kernel launch code
    /// @param config Kernel configuration and timeout settings
    /// @return true if kernel completed within timeout; false if timed out
    /// @throws std::runtime_error on invalid input or backend synchronization errors
    [[nodiscard]] bool executeWithTimeout(
        const std::function<void()>& kernel_lambda,
        const KernelConfig& config
    );

    /// @brief Execute kernel with CPU fallback
    /// @param gpu_kernel GPU kernel function
    /// @param cpu_kernel CPU fallback function
    /// @param config Kernel configuration
    /// GPU exceptions trigger CPU fallback when enabled; otherwise they are
    /// rethrown to the caller.
    ///
    /// @return true if GPU kernel completed; false if execution degraded to CPU
    [[nodiscard]] bool executeWithFallback(
       const std::function<void()>& gpu_kernel,
       const std::function<void()>& cpu_kernel,
       const KernelConfig& config
    );

private:
    bool kernel_timed_out_ = false;

    [[nodiscard]] bool waitForCompletion(cudaStream_t stream,
                                         uint32_t timeout_ms,
                                         std::chrono::steady_clock::time_point start_time);
};

}} // namespace themis::gpu
