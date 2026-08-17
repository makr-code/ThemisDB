/**
 * @file kernel_timeout_enforcer.cpp
 * @brief KernelTimeoutEnforcer implementation
 */

#include "gpu/kernel_timeout_enforcer.h"

#include <chrono>
#include <thread>

namespace themis {
namespace gpu {

bool KernelTimeoutEnforcer::executeWithTimeout(
    const std::function<void()>& kernel_lambda,
    const KernelConfig& config) {
    if (!kernel_lambda) {
        throw std::runtime_error("KernelTimeoutEnforcer: kernel_lambda is null");
    }

    kernel_timed_out_ = false;
    const auto start = std::chrono::steady_clock::now();
    kernel_lambda();
    return waitForCompletion(config.stream, config.timeout_ms, start);
}

bool KernelTimeoutEnforcer::executeWithFallback(
    const std::function<void()>& gpu_kernel,
    const std::function<void()>& cpu_kernel,
    const KernelConfig& config) {
    try {
        const bool gpu_success = executeWithTimeout(gpu_kernel, config);
        if (!gpu_success && config.enable_fallback && cpu_kernel) {
            cpu_kernel();
            return false;
        }
        return gpu_success;
    } catch (...) {
        if (config.enable_fallback && cpu_kernel) {
            cpu_kernel();
            return false;
        }
        throw;
    }
}

bool KernelTimeoutEnforcer::waitForCompletion(cudaStream_t stream,
                                              uint32_t timeout_ms,
                                              std::chrono::steady_clock::time_point start_time) {
#if THEMIS_GPU_SAFE_RAII_HAS_CUDA
    if (stream != nullptr) {
        while (true) {
            const cudaError_t status = cudaStreamQuery(stream);
            if (status == cudaSuccess) {
                kernel_timed_out_ = false;
                return true;
            }
            if (status != cudaErrorNotReady) {
                detail::throwCudaError("cudaStreamQuery", __FILE__, __LINE__, cudaGetErrorString(status));
            }

            const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start_time);
            if (elapsed.count() >= timeout_ms) {
                kernel_timed_out_ = true;
                CUDA_CHECK(cudaStreamSynchronize(stream));
                return false;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
#else
    static_cast<void>(stream);
#endif

    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start_time);
    kernel_timed_out_ = elapsed.count() >= timeout_ms;
    return !kernel_timed_out_;
}

}} // namespace themis::gpu
