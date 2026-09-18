/**
 * @file gpu_resource_handles.h
 * @brief Enhanced RAII Handles for GPU Resources — Automatic Lifecycle Management
 *
 * Provides type-safe RAII wrappers for GPU resources with automatic cleanup,
 * move semantics, and exception safety. Prevents:
 * - Memory leaks (RAII automatic cleanup)
 * - Use-after-free (move-only semantics)
 * - Double-free (moved-from state tracking)
 * - Resource exhaustion (bounded lifetimes)
 *
 * @version 1.0
 * @date 2026-08-18
 * @see Batch A-9 GPU Safety Hardening
 */

#pragma once

#include <atomic>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>

#ifdef THEMIS_ENABLE_CUDA
#include <cuda_runtime.h>
#include <cublas_v2.h>
#define THEMIS_HAS_CUDA 1
#else
#define THEMIS_HAS_CUDA 0
using cudaStream_t = void*;
using cublasHandle_t = void*;
#endif

#ifdef THEMIS_ENABLE_HIP
#include <hip/hip_runtime.h>
#include <hipblas/hipblas.h>
#define THEMIS_HAS_HIP 1
#else
#define THEMIS_HAS_HIP 0
#endif

namespace themis {
namespace gpu {

// ============================================================================
// GPU Stream Handle — RAII wrapper for cudaStream_t
// ============================================================================

class GPUStreamHandle {
 public:
    /**
     * @brief GPUStream Handle.
     * @return Return value.
     */
    explicit GPUStreamHandle();

    ~GPUStreamHandle() noexcept;

    // Delete copy operations
    GPUStreamHandle(const GPUStreamHandle&) = delete;
    GPUStreamHandle& operator=(const GPUStreamHandle&) = delete;

    // Move operations allowed
    GPUStreamHandle(GPUStreamHandle&& other) noexcept
        : stream_(std::exchange(other.stream_, nullptr)) {}

    GPUStreamHandle& operator=(GPUStreamHandle&& other) noexcept {
        if (this != &other) {
            destroy();
            stream_ = std::exchange(other.stream_, nullptr);
        }
        return *this;
    }

    cudaStream_t get() const noexcept { return stream_; }

    bool isValid() const noexcept { return stream_ != nullptr; }

    /**
     * @brief Synchronize.
     */
    void synchronize();

 private:
    cudaStream_t stream_;

    /**
     * @brief Destroy.
     * @note Exception safety: noexcept.
     */
    void destroy() noexcept;
};

// ============================================================================
// GPU Event Handle — RAII wrapper for cudaEvent_t
// ============================================================================

class GPUEventHandle {
 public:
    /**
     * @brief GPUEvent Handle.
     * @return Return value.
     */
    explicit GPUEventHandle();

    ~GPUEventHandle() noexcept;

    // Delete copy operations
    GPUEventHandle(const GPUEventHandle&) = delete;
    GPUEventHandle& operator=(const GPUEventHandle&) = delete;

    // Move operations allowed
    GPUEventHandle(GPUEventHandle&& other) noexcept
        : event_(std::exchange(other.event_, nullptr)) {}

    GPUEventHandle& operator=(GPUEventHandle&& other) noexcept {
        if (this != &other) {
            destroy();
            event_ = std::exchange(other.event_, nullptr);
        }
        return *this;
    }

#ifdef THEMIS_HAS_CUDA
    cudaEvent_t get() const noexcept { return event_; }
#else
    void* get() const noexcept { return event_; }
#endif

    bool isValid() const noexcept { return event_ != nullptr; }

    /**
     * @brief Record.
     * @param[in] stream Input parameter.
     */
    void record(cudaStream_t stream);

    /**
     * @brief Is Completed.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
    bool isCompleted() noexcept;

    /**
     * @brief Wait.
     */
    void wait();

 private:
#ifdef THEMIS_HAS_CUDA
    cudaEvent_t event_;
#else
    void* event_;
#endif

    /**
     * @brief Destroy.
     * @note Exception safety: noexcept.
     */
    void destroy() noexcept;
};

// ============================================================================
// GPU Kernel Timeout Guard — Enforce strict timeout limits
// ============================================================================

class GPUKernelTimeoutGuard {
 public:
    explicit GPUKernelTimeoutGuard(
        cudaStream_t stream = nullptr,
        std::chrono::milliseconds timeout = std::chrono::seconds(5)
    );

    ~GPUKernelTimeoutGuard() noexcept;

    // Delete copy operations
    GPUKernelTimeoutGuard(const GPUKernelTimeoutGuard&) = delete;
    GPUKernelTimeoutGuard& operator=(const GPUKernelTimeoutGuard&) = delete;

    // Delete move operations to keep thread ownership unique
    GPUKernelTimeoutGuard(GPUKernelTimeoutGuard&&) = delete;
    GPUKernelTimeoutGuard& operator=(GPUKernelTimeoutGuard&&) = delete;

    void markCompleted() noexcept {
        completed_.store(true, std::memory_order_release);
    }

    bool didTimeout() const noexcept {
        return timed_out_.load(std::memory_order_acquire);
    }

    /**
     * @brief Get Remaining Budget.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    std::chrono::milliseconds getRemainingBudget() const noexcept;

 private:
    cudaStream_t stream_;
    std::chrono::milliseconds timeout_;
    std::chrono::steady_clock::time_point start_time_;
    std::atomic<bool> completed_{false};
    std::atomic<bool> timed_out_{false};
    std::thread monitor_thread_;

    /**
     * @brief Monitor Thread.
     * @note Exception safety: noexcept.
     */
    void monitorThread() noexcept;
    /**
     * @brief Cleanup Stream.
     * @note Exception safety: noexcept.
     */
    void cleanupStream() noexcept;
};

// ============================================================================
// Factory Functions
// ============================================================================

/**
 * @brief Create GPUStream.
 * @return Return value.
 * @details Calls: GPUStreamHandle().
 */
inline GPUStreamHandle createGPUStream() {
    return GPUStreamHandle();
}

/**
 * @brief Create GPUEvent.
 * @return Return value.
 * @details Calls: GPUEventHandle().
 */
inline GPUEventHandle createGPUEvent() {
    return GPUEventHandle();
}

}  // namespace gpu
}  // namespace themis

#endif  // THEMIS_GPU_RESOURCE_HANDLES_H
