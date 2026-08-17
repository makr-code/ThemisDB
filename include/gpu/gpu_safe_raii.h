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

#if defined(THEMIS_ENABLE_CUDA) && defined(__has_include)
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

/**
 * @brief Raise a detailed CUDA runtime error.
 *
 * @param call Stringified CUDA expression.
 * @param file Source file where the failure originated.
 * @param line Source line where the failure originated.
 * @param detail CUDA runtime detail string.
 *
 * @throws std::runtime_error Always throws.
 */
[[noreturn]] void throwCudaError(const char* call,
                                 const char* file,
                                 int line,
                                 const std::string& detail);

/**
 * @brief Raise a deterministic "CUDA unavailable" failure for CPU-only builds.
 *
 * @param call Stringified CUDA expression that was requested.
 * @param file Source file where the failure originated.
 * @param line Source line where the failure originated.
 *
 * @throws std::runtime_error Always throws.
 */
[[noreturn]] void throwCudaUnavailable(const char* call,
                                       const char* file,
                                       int line);

/**
 * @brief Best-effort device free for no-throw cleanup paths.
 *
 * Destructors and move-assignment cleanup call this helper so that CUDA-enabled
 * builds release device memory without throwing while CPU-only builds remain
 * compilable and behave as a no-op.
 *
 * @param ptr Device pointer to release. nullptr is ignored.
 */
void destroyDeviceMemoryNoThrow(void* ptr) noexcept;

} // namespace detail

// ============================================================================
// CUDA_CHECK MACRO — Automatic error checking for all CUDA calls
// ============================================================================

/**
 * @brief Macro for safe CUDA calls with automatic error checking
 * 
 * Usage:
 * ```cpp
 * CUDA_CHECK(cudaMalloc(&ptr, size));  // Throws on error
 * CUDA_CHECK(cudaMemcpy(...));
 * CUDA_CHECK(cudaLaunchKernel(...));
 * ```
 * 
 * @error Throws std::runtime_error on CUDA failure with error string
 */
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

/**
 * @class DeviceMemoryGuard
 * @brief RAII wrapper for CUDA device memory allocation
 *
 * Automatically allocates and deallocates GPU memory. Prevents:
 * - Memory leaks (destructor frees memory even on exception)
 * - Use-after-free (move-only semantics)
 * - Double-free (moved-from state)
 *
 * @tparam T Type of elements to allocate
 */
template<typename T>
class DeviceMemoryGuard {
public:
    /// @brief Allocate memory for count elements of type T
    /// @param count Number of elements to allocate
    /// @throws std::runtime_error if CUDA allocation fails
    explicit DeviceMemoryGuard(size_t count) : ptr_(nullptr), size_(0) {
        if (count > 0) {
            CUDA_CHECK(cudaMalloc(&ptr_, sizeof(T) * count));
            size_ = count;
        }
    }

    /// @brief Default constructor (empty allocation)
    DeviceMemoryGuard() noexcept : ptr_(nullptr), size_(0) {}

    /// @brief Destructor — frees GPU memory
    /// Safe on moved-from allocators (no double-free)
    ~DeviceMemoryGuard() noexcept {
        if (ptr_) {
            detail::destroyDeviceMemoryNoThrow(ptr_);
        }
    }

    // --- Move semantics (enabled) ---
    
    /// @brief Move constructor
    DeviceMemoryGuard(DeviceMemoryGuard&& other) noexcept 
        : ptr_(std::exchange(other.ptr_, nullptr)),
          size_(std::exchange(other.size_, 0)) {}

    /// @brief Move assignment
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
    
    /// @brief Delete copy constructor (move-only semantics)
    DeviceMemoryGuard(const DeviceMemoryGuard&) = delete;

    /// @brief Delete copy assignment (move-only semantics)
    DeviceMemoryGuard& operator=(const DeviceMemoryGuard&) = delete;

    // --- Accessors ---

    /// @brief Get device pointer
    /// @return Raw GPU pointer (void*)
    void* get() const noexcept {
        return ptr_;
    }

    /// @brief Get device pointer typed as T*
    /// @return Typed GPU pointer
    T* getTyped() const noexcept {
        return static_cast<T*>(ptr_);
    }

    /// @brief Get allocation size in elements
    /// @return Number of elements allocated
    size_t size() const noexcept {
        return size_;
    }

    /// @brief Check if allocation is valid
    /// @return true if non-nullptr; false if empty
    bool isValid() const noexcept {
        return ptr_ != nullptr;
    }

    /// @brief Release ownership (manual management)
    /// @return Device pointer; caller must manually free with cudaFree
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

/**
 * @class KernelTimeoutGuard
 * @brief RAII wrapper for monitoring and enforcing kernel execution timeouts
 *
 * Prevents long-running kernels from hanging the calling path. The guard is
 * cooperative: it records timeout state without destroying or resetting the
 * caller-owned stream. Callers should mark completion only after any explicit
 * stream synchronization they require.
 *
 * Usage:
 * ```cpp
 * {
 *     KernelTimeoutGuard guard(stream, 10000);  // 10 second timeout
 *     kernel<<<blocks, threads, 0, stream>>>(args);
 *     // Destructor waits for kernel + enforces timeout
 * }
 * ```
 */
class KernelTimeoutGuard {
public:
    /// @brief Construct timeout guard for a CUDA stream
    /// @param stream CUDA stream for kernel execution
    /// @param timeout_ms Timeout in milliseconds (default: 10 seconds)
    explicit KernelTimeoutGuard(cudaStream_t stream, uint32_t timeout_ms = 10000);

    /// @brief Destructor — stops monitoring and joins the watchdog thread
    ~KernelTimeoutGuard() noexcept;

    // Delete copy operations (unique ownership)
    KernelTimeoutGuard(const KernelTimeoutGuard&) = delete;
    KernelTimeoutGuard& operator=(const KernelTimeoutGuard&) = delete;

    // Delete move operations to keep watchdog/thread ownership single-owner.
    KernelTimeoutGuard(KernelTimeoutGuard&&) = delete;
    KernelTimeoutGuard& operator=(KernelTimeoutGuard&&) = delete;

    /// @brief Mark kernel execution as completed
    /// Called after kernel launches to indicate successful completion
    void markCompleted() noexcept {
        completed_.store(true, std::memory_order_release);
    }

    /// @brief Check if timeout was exceeded
    /// @return true if kernel exceeded timeout; false if completed in time
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

    /// @brief Monitor thread function
    void monitorThread();
};

// ============================================================================
// Helper functions
// ============================================================================

/// @brief Create a unique_ptr-like device memory buffer
/// @tparam T Element type
/// @param count Number of elements to allocate
/// @return DeviceMemoryGuard managing the allocation
template<typename T>
inline DeviceMemoryGuard<T> makeDeviceMemory(size_t count) {
    return DeviceMemoryGuard<T>(count);
}

}} // namespace themis::gpu
