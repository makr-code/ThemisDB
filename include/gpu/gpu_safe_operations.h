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

#if defined(THEMIS_ENABLE_CUDA) && THEMIS_ENABLE_CUDA && defined(__has_include)
#if __has_include(<cuda_runtime.h>)
#include <cuda_runtime.h>
#define THEMIS_GPU_SAFE_OPS_HAS_CUDA 1
#endif
#endif

#ifndef THEMIS_GPU_SAFE_OPS_HAS_CUDA
#define THEMIS_GPU_SAFE_OPS_HAS_CUDA 0
using cudaError_t = int;
using cudaStream_t = void*;
constexpr cudaError_t cudaSuccess = 0;
constexpr cudaError_t cudaErrorNotReady = 34;
constexpr cudaError_t cudaErrorMemoryAllocation = 2;
constexpr cudaError_t cudaErrorInvalidConfiguration = 9;
constexpr cudaError_t cudaErrorInvalidValue = 11;
constexpr unsigned cudaStreamDefault = 0;

inline const char* cudaGetErrorString(cudaError_t) noexcept {
    return "CUDA runtime unavailable";
}

inline cudaError_t cudaSetDevice(int) noexcept {
    return cudaSuccess;
}

inline cudaError_t cudaMalloc(void**, size_t) noexcept {
    return 1;
}

inline cudaError_t cudaFree(void*) noexcept {
    return cudaSuccess;
}

inline cudaError_t cudaStreamCreateWithPriority(cudaStream_t*, unsigned, int) noexcept {
    return 1;
}

inline cudaError_t cudaStreamDestroy(cudaStream_t) noexcept {
    return cudaSuccess;
}

inline cudaError_t cudaStreamSynchronize(cudaStream_t) noexcept {
    return cudaSuccess;
}

inline cudaError_t cudaStreamQuery(cudaStream_t) noexcept {
    return cudaSuccess;
}
#endif

#include <memory>
#include <stdexcept>
#include <chrono>
#include <cstring>

namespace themis {
namespace gpu {

// =============================================================================
// CUDA Error Handling Macros
// =============================================================================

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

#define CUDA_CHECK_NOTHROW(call) ((call))

// =============================================================================
// Custom Exception
// =============================================================================

class CudaError : public std::runtime_error {
public:
    CudaError(const char* call_str, cudaError_t err, const char* file, int line)
        : std::runtime_error(build_message(call_str, err, file, line)),
          error_code_(err) {}

    [[nodiscard]] cudaError_t error_code() const noexcept { return error_code_; }

private:
    cudaError_t error_code_;

    /**
     * @brief Build message.
     * @param[in] call_str Input parameter.
     * @param[in] err Input parameter.
     * @param[in] file Input parameter.
     * @param[in] line Input parameter.
     * @return Return value.
     * @details Calls: std::to_string(), cudaGetErrorString().
     */
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

class CudaDeviceMemory {
public:
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

    CudaDeviceMemory(CudaDeviceMemory&& other) noexcept
        : device_ptr_(other.device_ptr_), size_(other.size_), device_id_(other.device_id_) {
        other.device_ptr_ = nullptr;
        other.size_ = 0;
    }

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

    [[nodiscard]] void* get() const noexcept { return device_ptr_; }

    [[nodiscard]] size_t size() const noexcept { return size_; }

    [[nodiscard]] int device_id() const noexcept { return device_id_; }

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

class CudaStreamGuard {
public:
    explicit CudaStreamGuard(int device_id = 0, int priority = 0)
        : device_id_(device_id), stream_(nullptr) {
        if (device_id < 0) {
            throw std::invalid_argument("device_id must be >= 0");
        }
        CUDA_CHECK(cudaSetDevice(device_id));
        CUDA_CHECK(cudaStreamCreateWithPriority(&stream_, cudaStreamDefault, priority));
    }

    ~CudaStreamGuard() noexcept {
        if (stream_) {
            cudaSetDevice(device_id_);
            cudaStreamDestroy(stream_);
        }
    }

    // Delete copy operations.
    CudaStreamGuard(const CudaStreamGuard&) = delete;
    CudaStreamGuard& operator=(const CudaStreamGuard&) = delete;

    CudaStreamGuard(CudaStreamGuard&& other) noexcept
        : device_id_(other.device_id_), stream_(other.stream_) {
        other.stream_ = nullptr;
    }

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

    [[nodiscard]] cudaStream_t get() const noexcept { return stream_; }

    /**
     * @brief Synchronize.
     * @details Calls: CUDA_CHECK(), cudaStreamSynchronize().
     */
    void synchronize() {
        if (stream_) {
            CUDA_CHECK(cudaStreamSynchronize(stream_));
        }
    }

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

class KernelExecutionGuard {
public:
    explicit KernelExecutionGuard(uint64_t timeout_ms = 5000)
        : timeout_ms_(timeout_ms), 
          start_time_(std::chrono::high_resolution_clock::now()),
          cpu_fallback_triggered_(false) {}

    [[nodiscard]] bool has_timed_out() const noexcept {
        if (timeout_ms_ == 0) {
            return false;  // No timeout configured.
        }
        auto elapsed = std::chrono::high_resolution_clock::now() - start_time_;
        auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
        return elapsed_ms > static_cast<int64_t>(timeout_ms_);
    }

    void trigger_cpu_fallback() noexcept {
        cpu_fallback_triggered_ = true;
    }

    [[nodiscard]] bool is_cpu_fallback_triggered() const noexcept {
        return cpu_fallback_triggered_;
    }

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

[[nodiscard]] inline std::string cuda_error_to_string(cudaError_t err) noexcept {
    return std::string(cudaGetErrorString(err));
}

[[nodiscard]] inline uint64_t ms_to_us(uint64_t ms) noexcept {
    return ms * 1000;
}

[[nodiscard]] inline uint64_t us_to_ms(uint64_t us) noexcept {
    return us / 1000;
}

}  // namespace gpu
}  // namespace themis
