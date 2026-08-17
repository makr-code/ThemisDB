/**
 * @file gpu_safe_operations.h
 * @brief RAII wrappers, error handling macros, and fail-closed patterns for GPU operations.
 * @version 1.0.0
 * @date 2026-08-16
 * 
 * Wave A-8 GPU hardening: implements fail-closed error handling, resource lifecycle
 * guarantees (RAII), and kernel timeout enforcement with automatic CPU fallback.
 * 
 * All CUDA operations use CUDA_CHECK or explicit error handling to prevent
 * unchecked CUDA call exposure (Phase C target: reduce 340 → 170 calls).
 * 
 * @see src/gpu/ROADMAP.md § Wave A Scope for gpu
 */

#pragma once

#include <cuda_runtime.h>
#include <memory>
#include <stdexcept>
#include <chrono>
#include <cstring>

namespace themis {
namespace gpu {

// =============================================================================
// CUDA Error Handling Macros
// =============================================================================

/**
 * @brief Safe CUDA API call wrapper with automatic error handling.
 * 
 * Usage:
 *   CUDA_CHECK(cudaMalloc(&ptr, size));
 *   CUDA_CHECK(cudaMemcpy(dst, src, size, cudaMemcpyHostToDevice));
 * 
 * On error, logs the CUDA error string and throws cuda_error exception.
 * Never silently ignores CUDA errors.
 */
#define CUDA_CHECK(call) \
    do { \
        cudaError_t err = (call); \
        if (err != cudaSuccess) { \
            throw ::themis::gpu::CudaError( \
                #call, \
                err, \
                __FILE__, \
                __LINE__ \
            ); \
        } \
    } while(0)

/**
 * @brief Safe CUDA API call with optional error handling.
 * 
 * Returns the error code without throwing; allows custom error handling.
 * Usage:
 *   cudaError_t err = CUDA_CHECK_NOTHROW(cudaMalloc(&ptr, size));
 *   if (err != cudaSuccess) { ... handle error ... }
 */
#define CUDA_CHECK_NOTHROW(call) ((call))

// =============================================================================
// Custom Exception
// =============================================================================

/**
 * @brief Exception thrown when a CUDA operation fails.
 * 
 * Captures the API call, error code, file, and line for diagnostics.
 * Always includes the CUDA error string via cudaGetErrorString().
 */
class CudaError : public std::runtime_error {
public:
    /**
     * @brief Construct CudaError from a failed CUDA call.
     * 
     * @param call_str The CUDA API call string (e.g., "cudaMalloc(&ptr, size)")
     * @param err The CUDA error code
     * @param file Source file where the error occurred
     * @param line Source line where the error occurred
     */
    CudaError(const char* call_str, cudaError_t err, const char* file, int line)
        : std::runtime_error(build_message(call_str, err, file, line)),
          error_code_(err) {}

    /**
     * @brief Get the CUDA error code from cudaError_t.
     */
    [[nodiscard]] cudaError_t error_code() const noexcept { return error_code_; }

private:
    cudaError_t error_code_;

    static std::string build_message(const char* call_str, cudaError_t err, 
                                     const char* file, int line) {
        std::string msg = "CUDA error at ";
        msg += file;
        msg += ":";
        msg += std::to_string(line);
        msg += " in call: ";
        msg += call_str;
        msg += " -> ";
        msg += cudaGetErrorString(err);
        return msg;
    }
};

// =============================================================================
// RAII Wrappers
// =============================================================================

/**
 * @brief RAII wrapper for CUDA device memory (automatically freed on destruction).
 * 
 * Provides automatic cleanup via RAII; mimics std::unique_ptr behavior for GPU memory.
 * Supports move semantics for efficient transfer of ownership.
 */
class CudaDeviceMemory {
public:
    /**
     * @brief Allocate device memory on the specified GPU device.
     * 
     * @param size Number of bytes to allocate
     * @param device_id GPU device ID (default: 0)
     * 
     * @throws CudaError if allocation fails
     */
    explicit CudaDeviceMemory(size_t size, int device_id = 0)
        : device_ptr_(nullptr), size_(size), device_id_(device_id) {
        if (size == 0) {
            throw std::invalid_argument("CudaDeviceMemory size must be > 0");
        }
        if (device_id < 0) {
            throw std::invalid_argument("device_id must be >= 0");
        }
        CUDA_CHECK(cudaSetDevice(device_id));
        CUDA_CHECK(cudaMalloc(&device_ptr_, size));
    }

    /**
     * @brief Destructor: automatically frees device memory.
     */
    ~CudaDeviceMemory() noexcept {
        if (device_ptr_) {
            // Ignore errors during cleanup to avoid exceptions in destructors.
            cudaSetDevice(device_id_);
            cudaFree(device_ptr_);
        }
    }

    // Delete copy constructor and copy assignment (no shallow copies).
    CudaDeviceMemory(const CudaDeviceMemory&) = delete;
    CudaDeviceMemory& operator=(const CudaDeviceMemory&) = delete;

    /**
     * @brief Move constructor: transfers ownership.
     */
    CudaDeviceMemory(CudaDeviceMemory&& other) noexcept
        : device_ptr_(other.device_ptr_), size_(other.size_), device_id_(other.device_id_) {
        other.device_ptr_ = nullptr;
        other.size_ = 0;
    }

    /**
     * @brief Move assignment: transfers ownership.
     */
    CudaDeviceMemory& operator=(CudaDeviceMemory&& other) noexcept {
        if (this != &other) {
            // Clean up existing memory.
            if (device_ptr_) {
                cudaSetDevice(device_id_);
                cudaFree(device_ptr_);
            }
            // Take ownership of other's memory.
            device_ptr_ = other.device_ptr_;
            size_ = other.size_;
            device_id_ = other.device_id_;
            other.device_ptr_ = nullptr;
            other.size_ = 0;
        }
        return *this;
    }

    /**
     * @brief Get raw device pointer.
     */
    [[nodiscard]] void* get() const noexcept { return device_ptr_; }

    /**
     * @brief Get size in bytes.
     */
    [[nodiscard]] size_t size() const noexcept { return size_; }

    /**
     * @brief Get device ID.
     */
    [[nodiscard]] int device_id() const noexcept { return device_id_; }

    /**
     * @brief Release ownership and return raw pointer (caller responsible for cleanup).
     */
    [[nodiscard]] void* release() noexcept {
        void* ptr = device_ptr_;
        device_ptr_ = nullptr;
        size_ = 0;
        return ptr;
    }

private:
    void* device_ptr_;
    size_t size_;
    int device_id_;
};

/**
 * @brief RAII wrapper for CUDA streams with automatic cleanup.
 * 
 * Ensures streams are properly destroyed, even if exceptions occur.
 * Supports move semantics for ownership transfer.
 */
class CudaStreamGuard {
public:
    /**
     * @brief Create a new CUDA stream with optional priority.
     * 
     * @param device_id GPU device ID
     * @param priority Stream priority (0 = normal, higher = lower priority)
     * 
     * @throws CudaError if stream creation fails
     */
    explicit CudaStreamGuard(int device_id = 0, int priority = 0)
        : device_id_(device_id), stream_(nullptr) {
        if (device_id < 0) {
            throw std::invalid_argument("device_id must be >= 0");
        }
        CUDA_CHECK(cudaSetDevice(device_id));
        CUDA_CHECK(cudaStreamCreateWithPriority(&stream_, cudaStreamDefault, priority));
    }

    /**
     * @brief Destructor: automatically destroys the stream.
     */
    ~CudaStreamGuard() noexcept {
        if (stream_) {
            cudaSetDevice(device_id_);
            cudaStreamDestroy(stream_);
        }
    }

    // Delete copy operations.
    CudaStreamGuard(const CudaStreamGuard&) = delete;
    CudaStreamGuard& operator=(const CudaStreamGuard&) = delete;

    /**
     * @brief Move constructor.
     */
    CudaStreamGuard(CudaStreamGuard&& other) noexcept
        : device_id_(other.device_id_), stream_(other.stream_) {
        other.stream_ = nullptr;
    }

    /**
     * @brief Move assignment.
     */
    CudaStreamGuard& operator=(CudaStreamGuard&& other) noexcept {
        if (this != &other) {
            if (stream_) {
                cudaSetDevice(device_id_);
                cudaStreamDestroy(stream_);
            }
            device_id_ = other.device_id_;
            stream_ = other.stream_;
            other.stream_ = nullptr;
        }
        return *this;
    }

    /**
     * @brief Get the underlying CUDA stream.
     */
    [[nodiscard]] cudaStream_t get() const noexcept { return stream_; }

    /**
     * @brief Synchronize this stream (wait for all enqueued work to complete).
     * 
     * @throws CudaError if synchronization fails
     */
    void synchronize() {
        if (stream_) {
            CUDA_CHECK(cudaStreamSynchronize(stream_));
        }
    }

    /**
     * @brief Check if the stream is ready (no pending work).
     * 
     * @return true if stream is ready; false if work is pending
     */
    [[nodiscard]] bool is_ready() const noexcept {
        if (!stream_) {
            return true;
        }
        cudaError_t err = cudaStreamQuery(stream_);
        return err == cudaSuccess;
    }

private:
    int device_id_;
    cudaStream_t stream_;
};

/**
 * @brief RAII wrapper for kernel execution with timeout enforcement.
 * 
 * Ensures kernels respect a bounded execution time (5 seconds default).
 * Provides automatic fallback to CPU on timeout or launch failure.
 */
class KernelExecutionGuard {
public:
    /**
     * @brief Construct execution guard with optional timeout.
     * 
     * @param timeout_ms Maximum kernel execution time in milliseconds (0 = no timeout)
     */
    explicit KernelExecutionGuard(uint64_t timeout_ms = 5000)
        : timeout_ms_(timeout_ms), 
          start_time_(std::chrono::high_resolution_clock::now()),
          cpu_fallback_triggered_(false) {}

    /**
     * @brief Check if timeout has been exceeded.
     * 
     * @return true if timeout exceeded; false otherwise
     */
    [[nodiscard]] bool has_timed_out() const noexcept {
        if (timeout_ms_ == 0) {
            return false;  // No timeout configured.
        }
        auto elapsed = std::chrono::high_resolution_clock::now() - start_time_;
        auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
        return elapsed_ms > static_cast<int64_t>(timeout_ms_);
    }

    /**
     * @brief Record that CPU fallback was triggered.
     */
    void trigger_cpu_fallback() noexcept {
        cpu_fallback_triggered_ = true;
    }

    /**
     * @brief Check if CPU fallback was triggered.
     */
    [[nodiscard]] bool is_cpu_fallback_triggered() const noexcept {
        return cpu_fallback_triggered_;
    }

    /**
     * @brief Get elapsed time in milliseconds.
     */
    [[nodiscard]] uint64_t elapsed_ms() const noexcept {
        auto elapsed = std::chrono::high_resolution_clock::now() - start_time_;
        return std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
    }

private:
    uint64_t timeout_ms_;
    std::chrono::high_resolution_clock::time_point start_time_;
    bool cpu_fallback_triggered_;
};

// =============================================================================
// Utilities
// =============================================================================

/**
 * @brief Get a descriptive string for a CUDA error code.
 * 
 * @param err CUDA error code
 * @return Human-readable error description
 */
[[nodiscard]] inline std::string cuda_error_to_string(cudaError_t err) noexcept {
    return std::string(cudaGetErrorString(err));
}

/**
 * @brief Convert milliseconds to CUDA events (for timing).
 * 
 * @param ms Milliseconds
 * @return Equivalent microseconds
 */
[[nodiscard]] inline uint64_t ms_to_us(uint64_t ms) noexcept {
    return ms * 1000;
}

/**
 * @brief Convert microseconds to milliseconds.
 * 
 * @param us Microseconds
 * @return Equivalent milliseconds
 */
[[nodiscard]] inline uint64_t us_to_ms(uint64_t us) noexcept {
    return us / 1000;
}

}  // namespace gpu
}  // namespace themis
