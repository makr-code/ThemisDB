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

/**
 * @class GPUStreamHandle
 * @brief RAII wrapper for GPU stream lifecycle management
 *
 * Automatically creates stream on construction and destroys on destruction.
 * Prevents stream handle leaks and ensures proper synchronization.
 */
class GPUStreamHandle {
 public:
    /// Create a new GPU stream
    /// @throws std::runtime_error if stream creation fails
    explicit GPUStreamHandle();

    /// Destructor — destroy the stream
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

    /// Get the underlying CUDA stream
    cudaStream_t get() const noexcept { return stream_; }

    /// Check if stream is valid
    bool isValid() const noexcept { return stream_ != nullptr; }

    /// Synchronize (wait for all pending operations)
    /// @throws std::runtime_error if synchronization fails
    void synchronize();

 private:
    cudaStream_t stream_;

    void destroy() noexcept;
};

// ============================================================================
// GPU Event Handle — RAII wrapper for cudaEvent_t
// ============================================================================

/**
 * @class GPUEventHandle
 * @brief RAII wrapper for GPU event lifecycle management
 *
 * Automatically creates and destroys GPU events for synchronization.
 * Prevents event handle leaks and ensures proper timing.
 */
class GPUEventHandle {
 public:
    /// Create a new GPU event
    /// @throws std::runtime_error if event creation fails
    explicit GPUEventHandle();

    /// Destructor — destroy the event
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

    /// Get the underlying CUDA event
#ifdef THEMIS_HAS_CUDA
    cudaEvent_t get() const noexcept { return event_; }
#else
    void* get() const noexcept { return event_; }
#endif

    /// Check if event is valid
    bool isValid() const noexcept { return event_ != nullptr; }

    /// Record event in stream
    /// @param stream CUDA stream to record in
    /// @throws std::runtime_error if recording fails
    void record(cudaStream_t stream);

    /// Query if event has completed
    bool isCompleted() noexcept;

    /// Wait for event to complete
    /// @throws std::runtime_error if wait fails
    void wait();

 private:
#ifdef THEMIS_HAS_CUDA
    cudaEvent_t event_;
#else
    void* event_;
#endif

    void destroy() noexcept;
};

// ============================================================================
// GPU Kernel Timeout Guard — Enforce strict timeout limits
// ============================================================================

/**
 * @class GPUKernelTimeoutGuard
 * @brief RAII wrapper for enforcing kernel execution timeouts
 *
 * Monitors kernel execution and enforces a strict 5-second timeout.
 * On timeout, cleans up resources and marks failure for CPU fallback.
 * 
 * This is separate from KernelTimeoutGuard in gpu_safe_raii.h and
 * provides enhanced timeout enforcement with resource cleanup.
 */
class GPUKernelTimeoutGuard {
 public:
    /// Create timeout guard with 5-second default SLA
    /// @param stream CUDA stream to monitor (optional)
    explicit GPUKernelTimeoutGuard(
        cudaStream_t stream = nullptr,
        std::chrono::milliseconds timeout = std::chrono::seconds(5)
    );

    /// Destructor — stop monitoring and cleanup
    ~GPUKernelTimeoutGuard() noexcept;

    // Delete copy operations
    GPUKernelTimeoutGuard(const GPUKernelTimeoutGuard&) = delete;
    GPUKernelTimeoutGuard& operator=(const GPUKernelTimeoutGuard&) = delete;

    // Delete move operations to keep thread ownership unique
    GPUKernelTimeoutGuard(GPUKernelTimeoutGuard&&) = delete;
    GPUKernelTimeoutGuard& operator=(GPUKernelTimeoutGuard&&) = delete;

    /// Mark kernel as completed within timeout
    void markCompleted() noexcept {
        completed_.store(true, std::memory_order_release);
    }

    /// Check if timeout was exceeded
    bool didTimeout() const noexcept {
        return timed_out_.load(std::memory_order_acquire);
    }

    /// Get remaining timeout budget (milliseconds)
    std::chrono::milliseconds getRemainingBudget() const noexcept;

 private:
    cudaStream_t stream_;
    std::chrono::milliseconds timeout_;
    std::chrono::steady_clock::time_point start_time_;
    std::atomic<bool> completed_{false};
    std::atomic<bool> timed_out_{false};
    std::thread monitor_thread_;

    void monitorThread() noexcept;
    void cleanupStream() noexcept;
};

// ============================================================================
// Factory Functions
// ============================================================================

/// Create a GPU stream with automatic cleanup
inline GPUStreamHandle createGPUStream() {
    return GPUStreamHandle();
}

/// Create a GPU event with automatic cleanup
inline GPUEventHandle createGPUEvent() {
    return GPUEventHandle();
}

}  // namespace gpu
}  // namespace themis

#endif  // THEMIS_GPU_RESOURCE_HANDLES_H
