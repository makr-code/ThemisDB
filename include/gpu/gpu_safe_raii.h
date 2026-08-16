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

#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <thread>
#include <atomic>
#include <chrono>
#include <cuda_runtime.h>

namespace themis {
namespace gpu {

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
#define CUDA_CHECK(call) do { \
    cudaError_t err = (call); \
    if (err != cudaSuccess) { \
        std::string error_msg = std::string("CUDA error: ") + \
            cudaGetErrorString(err) + \
            " (in " + __FILE__ + ":" + std::to_string(__LINE__) + ")"; \
        throw std::runtime_error(error_msg); \
    } \
} while(0)

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
            cudaFree(ptr_);  // No error checking in destructor (no-throw guarantee)
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
                cudaFree(ptr_);
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
 * Prevents long-running kernels from hanging the GPU. Uses a background thread
 * to monitor execution time and forcibly cancel kernels that exceed timeout.
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

    /// @brief Destructor — waits for kernel and enforces timeout
    ~KernelTimeoutGuard() noexcept;

    // Delete copy operations (unique ownership)
    KernelTimeoutGuard(const KernelTimeoutGuard&) = delete;
    KernelTimeoutGuard& operator=(const KernelTimeoutGuard&) = delete;

    // Allow move operations
    KernelTimeoutGuard(KernelTimeoutGuard&&) noexcept = default;
    KernelTimeoutGuard& operator=(KernelTimeoutGuard&&) noexcept = default;

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
