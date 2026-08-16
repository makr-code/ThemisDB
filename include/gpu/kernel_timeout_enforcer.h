/**
 * @file kernel_timeout_enforcer.h
 * @brief Kernel Timeout Enforcement — Prevent Long-Running Kernel Hangs
 *
 * @version v1.0
 * @note Maturity: 🟡 BETA
 * @note Status: Wave A Batch A-8 Implementation
 *
 * Monitors GPU kernel execution and enforces timeout limits. If a kernel
 * exceeds its timeout window, execution is preempted and falls back to CPU.
 *
 * ## Execution Model
 * 1. Kernel starts with timeout guard
 * 2. Background thread monitors elapsed time
 * 3. If timeout exceeded, kernel stream is destroyed
 * 4. Fallback to CPU execution (if enabled)
 *
 * @error 7700: Kernel timeout exceeded
 * @error 7701: Kernel execution failed
 */

#pragma once

#include <cstdint>
#include <functional>
#include <cuda_runtime.h>

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
    /// @throws std::runtime_error on CUDA errors
    bool executeWithTimeout(
        const std::function<void()>& kernel_lambda,
        const KernelConfig& config
    );

    /// @brief Execute kernel with CPU fallback
    /// @param gpu_kernel GPU kernel function
    /// @param cpu_kernel CPU fallback function
    /// @param config Kernel configuration
    /// @return true if GPU kernel completed; false if fell back to CPU
    bool executeWithFallback(
        const std::function<void()>& gpu_kernel,
        const std::function<void()>& cpu_kernel,
        const KernelConfig& config
    );

private:
    bool kernel_timed_out_ = false;

    void monitorKernelTimeout(cudaStream_t stream, uint32_t timeout_ms);
};

}} // namespace themis::gpu
