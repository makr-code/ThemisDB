/**
 * @file gpu_safe_raii.h
 * @brief GPU Safe RAII Wrappers — Automatic Memory Management and Error Checking
 *
 * @version v1.0
 * @note Maturity: 🟡 BETA
 * @note Status: Wave A Batch A-8 Implementation
 *
 * Provides exception-safe wrappers for CUDA operations with automatic error checking
 * and resource management via RAII pattern. Prevents:
 * - Unchecked CUDA errors (exceptions thrown on failure)
 * - Memory leaks (automatic cleanup on scope exit)
 * - Use-after-free (move-only semantics, unique ownership)
 * - Double-free (moved-from state tracking)
 *
 * ## Usage Example
 * ```cpp
 * // Automatic allocation and deallocation
 * {
 *     DeviceMemoryGuard<float> d_buffer(1024);  // Allocates 1024 floats
 *     // Use d_buffer.get() as void* device pointer
 * }  // Automatically frees memory on scope exit
 *
 * // CUDA error checking macro
 * CUDA_CHECK(cudaMemcpy(host_ptr, device_ptr, size, cudaMemcpyDeviceToHost));
 *
 * // Kernel execution with timeout protection
 * {
 *     KernelTimeoutGuard timeout_guard(stream, 10000);  // 10 second timeout
 *     kernel<<<blocks, threads, 0, stream>>>(args);
 * }  // Automatically monitors and enforces timeout
 * ```
 *
 * @error 7500: CUDA error (with detailed error string)
 * @error 7501: Memory allocation failed
 * @error 7502: Kernel timeout exceeded
 */

#pragma once

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>

#if defined(THEMIS_ENABLE_CUDA) && THEMIS_ENABLE_CUDA && defined(__has_include)
#if __has_include(<cuda_runtime.h>)
#define THEMIS_GPU_SAFE_RAII_HAS_CUDA 1
#include <cuda_runtime.h>
#endif
#endif

#ifndef THEMIS_GPU_SAFE_RAII_HAS_CUDA
#define THEMIS_GPU_SAFE_RAII_HAS_CUDA 0
using cudaStream_t = void*;
#endif

namespace themis {
namespace gpu {

namespace detail {

[[noreturn]] void throwCudaError(const char* call,
                                 const char* file,
                                 int line,
                                 const std::string& detail);

[[noreturn]] void throwCudaUnavailable(const char* call,
                                       const char* file,
                                       int line);

/**
 * @brief Destroy Device Memory No Throw.
 * @param[in,out] ptr Input/output parameter.
 * @note Exception safety: noexcept.
 */
void destroyDeviceMemoryNoThrow(void* ptr) noexcept;

} // namespace detail

// ============================================================================
// CUDA_CHECK MACRO — Automatic error checking for all CUDA calls
// ============================================================================

#if THEMIS_GPU_SAFE_RAII_HAS_CUDA
#define CUDA_CHECK(call) do { \
    const cudaError_t err = (call); \
    if (err != cudaSuccess) { \
        ::themis::gpu::detail::throwCudaError(#call, __FILE__, __LINE__, cudaGetErrorString(err)); \
    } \
} while(0)
#else
#define CUDA_CHECK(call) do { \
    ::themis::gpu::detail::throwCudaUnavailable(#call, __FILE__, __LINE__); \
} while(0)
#endif

// ============================================================================
// DeviceMemoryGuard — RAII wrapper for GPU memory allocation
// ============================================================================

template<typename T>
class DeviceMemoryGuard {
public:
    explicit DeviceMemoryGuard(size_t count) : ptr_(nullptr), size_(0) {
        if (count > 0) {
            CUDA_CHECK(cudaMalloc(&ptr_, sizeof(T) * count));
            size_ = count;
        }
    }

    DeviceMemoryGuard() noexcept : ptr_(nullptr), size_(0) {}

    ~DeviceMemoryGuard() noexcept {
        if (ptr_) {
            detail::destroyDeviceMemoryNoThrow(ptr_);
        }
    }

    // --- Move semantics (enabled) ---
    
    DeviceMemoryGuard(DeviceMemoryGuard&& other) noexcept 
        : ptr_(std::exchange(other.ptr_, nullptr)),
          size_(std::exchange(other.size_, 0)) {}

    DeviceMemoryGuard& operator=(DeviceMemoryGuard&& other) noexcept {
        if (this != &other) {
            if (ptr_) {
                detail::destroyDeviceMemoryNoThrow(ptr_);
            }
            ptr_ = std::exchange(other.ptr_, nullptr);
            size_ = std::exchange(other.size_, 0);
        }
        return *this;
    }

    // --- Copy semantics (deleted) ---
    
    DeviceMemoryGuard(const DeviceMemoryGuard&) = delete;

    DeviceMemoryGuard& operator=(const DeviceMemoryGuard&) = delete;

    // --- Accessors ---

    void* get() const noexcept {
        return ptr_;
    }

    T* getTyped() const noexcept {
        return static_cast<T*>(ptr_);
    }

    size_t size() const noexcept {
        return size_;
    }

    bool isValid() const noexcept {
        return ptr_ != nullptr;
    }

    void* release() noexcept {
        size_ = 0;
        return std::exchange(ptr_, nullptr);
    }

private:
    void* ptr_;
    size_t size_;
};

// ============================================================================
// KernelTimeoutGuard — RAII wrapper for kernel execution timeout
// ============================================================================

class KernelTimeoutGuard {
public:
    explicit KernelTimeoutGuard(cudaStream_t stream, uint32_t timeout_ms = 10000);

    ~KernelTimeoutGuard() noexcept;

    // Delete copy operations (unique ownership)
    KernelTimeoutGuard(const KernelTimeoutGuard&) = delete;
    KernelTimeoutGuard& operator=(const KernelTimeoutGuard&) = delete;

    // Delete move operations to keep watchdog/thread ownership single-owner.
    KernelTimeoutGuard(KernelTimeoutGuard&&) = delete;
    KernelTimeoutGuard& operator=(KernelTimeoutGuard&&) = delete;

    void markCompleted() noexcept {
        completed_.store(true, std::memory_order_release);
    }

    bool didTimeout() const noexcept {
        return timed_out_.load(std::memory_order_acquire);
    }

private:
    cudaStream_t stream_;
    uint32_t timeout_ms_;
    std::atomic<bool> completed_{false};
    std::atomic<bool> timed_out_{false};
    std::thread monitor_thread_;
    std::chrono::milliseconds poll_interval_{1};

    /**
     * @brief Monitor Thread.
     */
    void monitorThread();
};

// ============================================================================
// Helper functions
// ============================================================================

template<typename T>
/**
 * @brief Make Device Memory.
 * @param[in] count Input parameter.
 * @return Return value.
 * @details Implements makeDeviceMemory without additional internal calls.
 */
inline DeviceMemoryGuard<T> makeDeviceMemory(size_t count) {
    return DeviceMemoryGuard<T>(count);
}

}} // namespace themis::gpu
