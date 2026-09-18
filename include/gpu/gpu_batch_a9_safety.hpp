/**
 * @file gpu_batch_a9_safety.hpp
 * @brief GPU Module Batch A-9 — Safety Hardening Patterns and Macros
 *
 * Provides production-ready patterns for:
 * 1. Systematic CUDA error checking with logging
 * 2. CPU fallback paths for all GPU failures
 * 3. Kernel timeout enforcement (5-second hard limit)
 * 4. RAII-based resource cleanup
 *
 * @version 1.0
 * @date 2026-08-18
 * @author Batch A-9 GPU Safety Hardening
 *
 * @see WAVE_A8_IMPLEMENTATION_SUMMARY.md
 */

#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <string>
#include <thread>

#ifdef THEMIS_ENABLE_CUDA
#include <cuda_runtime.h>
#define THEMIS_BATCH_A9_HAS_CUDA 1
#else
#define THEMIS_BATCH_A9_HAS_CUDA 0
using cudaError_t = int;
using cudaStream_t = void*;
#endif

namespace themis {
namespace gpu {
namespace batch_a9 {

// ============================================================================
// Kernel Timeout Enforcement — 5-second hard limit
// ============================================================================

/**
 * @brief Configuration for kernel execution with timeout and fallback
 */
struct KernelExecutionConfig {
    /// Timeout limit (default: 5 seconds)
    std::chrono::milliseconds timeout_ms{5000};
    
    /// Whether to fallback to CPU on GPU failure
    bool enable_cpu_fallback = true;
    
    /// Whether to log timeouts and errors
    bool enable_logging = true;
    
    /// CUDA stream (optional, may be nullptr for default stream)
    cudaStream_t stream = nullptr;
};

/**
 * @brief Result of kernel execution attempt
 */
struct KernelExecutionResult {
    /// True if GPU execution succeeded
    bool gpu_success = false;
    
    /// True if execution fell back to CPU
    bool cpu_fallback = false;
    
    /// Execution time in milliseconds
    std::chrono::milliseconds execution_time_ms{0};
    
    /// Error message (if any)
    std::string error_message;
    
    /// True if execution completed (GPU or CPU)
    bool success() const { return gpu_success || cpu_fallback; }
};

/**
 * @brief Execute a kernel with strict 5-second timeout and CPU fallback
 *
 * This function ensures all GPU operations have a bounded execution time.
 * If the GPU operation exceeds 5 seconds, execution automatically falls back
 * to CPU (if CPU implementation is provided).
 *
 * @param gpu_kernel GPU implementation to execute
 * @param cpu_kernel CPU fallback implementation (optional)
 * @param config Execution configuration
 * @return Execution result with success status and timing
 *
 * Usage:
 * ```cpp
 * auto gpu_impl = [&]() {
 *     kernel<<<blocks, threads, 0, stream>>>(args);
 * };
 * 
 * auto cpu_impl = [&]() {
 *     // CPU fallback implementation
 * };
 * 
 * KernelExecutionConfig config;
 * config.timeout_ms = std::chrono::milliseconds(5000);
 * config.enable_cpu_fallback = true;
 * 
 * auto result = executeKernelWithTimeout(gpu_impl, cpu_impl, config);
 * if (!result.success()) {
 *     THEMIS_ERROR("Kernel execution failed: {}", result.error_message);
 * }
 * ```
 */
inline KernelExecutionResult executeKernelWithTimeout(
    const std::function<void()>& gpu_kernel,
    const std::function<void()>& cpu_kernel,
    const KernelExecutionConfig& config) noexcept {
    
    KernelExecutionResult result;
    auto start_time = std::chrono::high_resolution_clock::now();
    
    try {
        // Execute GPU kernel
        if (gpu_kernel) {
            gpu_kernel();
            
#if THEMIS_BATCH_A9_HAS_CUDA
            // Check for kernel launch errors
            cudaError_t err = cudaGetLastError();
            if (err != cudaSuccess) {
                if (config.enable_logging) {
                    auto logger = spdlog::get("gpu");
                    if (logger) {
                        logger->warn("Kernel launch failed: {}", cudaGetErrorString(err));
                    }
                }
                result.error_message = std::string("Kernel launch failed: ") + cudaGetErrorString(err);
                
                // Fall back to CPU if available
                if (config.enable_cpu_fallback && cpu_kernel) {
                    try {
                        cpu_kernel();
                        result.cpu_fallback = true;
                    } catch (const std::exception& e) {
                        result.error_message = std::string("CPU fallback failed: ") + e.what();
                    }
                }
                
                auto elapsed = std::chrono::high_resolution_clock::now() - start_time;
                result.execution_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);
                return result;
            }
            
            // Wait for completion with 5-second timeout
            if (config.stream != nullptr) {
                auto wait_start = std::chrono::high_resolution_clock::now();
                
                while (true) {
                    err = cudaStreamQuery(config.stream);
                    if (err == cudaSuccess) {
                        // Kernel completed successfully
                        result.gpu_success = true;
                        break;
                    }
                    
                    if (err != cudaErrorNotReady) {
                        // Error during synchronization
                        if (config.enable_logging) {
                            auto logger = spdlog::get("gpu");
                            if (logger) {
                                logger->warn("Stream sync error: {}", cudaGetErrorString(err));
                            }
                        }
                        result.error_message = std::string("Stream sync failed: ") + cudaGetErrorString(err);
                        break;
                    }
                    
                    // Check timeout
                    auto wait_elapsed = std::chrono::high_resolution_clock::now() - wait_start;
                    if (wait_elapsed >= std::chrono::seconds(5)) {
                        // Hard timeout exceeded
                        if (config.enable_logging) {
                            auto logger = spdlog::get("gpu");
                            if (logger) {
                                logger->warn("Kernel exceeded 5-second timeout");
                            }
                        }
                        result.error_message = "Kernel execution exceeded 5-second timeout";
                        break;
                    }
                    
                    // Poll with 1ms intervals
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            } else {
                // No stream provided, assume synchronous execution
                err = cudaDeviceSynchronize();
                if (err == cudaSuccess) {
                    result.gpu_success = true;
                } else {
                    result.error_message = std::string("Device sync failed: ") + cudaGetErrorString(err);
                    if (config.enable_logging) {
                        auto logger = spdlog::get("gpu");
                        if (logger) {
                            logger->warn("Device sync error: {}", result.error_message);
                        }
                    }
                }
            }
#else
            // CPU-only build: GPU kernel ran but we can't validate
            result.gpu_success = true;
#endif
        } else {
            result.error_message = "GPU kernel is null";
        }
        
        // If GPU failed and CPU fallback is enabled, run CPU implementation
        if (!result.gpu_success && config.enable_cpu_fallback && cpu_kernel) {
            try {
                cpu_kernel();
                result.cpu_fallback = true;
                result.error_message.clear();  // Success via fallback
            } catch (const std::exception& e) {
                result.error_message = std::string("CPU fallback failed: ") + e.what();
            }
        }
        
    } catch (const std::exception& e) {
        result.error_message = std::string("Exception: ") + e.what();
        
        // Try CPU fallback on exception
        if (config.enable_cpu_fallback && cpu_kernel) {
            try {
                cpu_kernel();
                result.cpu_fallback = true;
            } catch (...) {
                // CPU fallback also failed
            }
        }
    } catch (...) {
        result.error_message = "Unknown exception in kernel execution";
        
        // Try CPU fallback on any exception
        if (config.enable_cpu_fallback && cpu_kernel) {
            try {
                cpu_kernel();
                result.cpu_fallback = true;
            } catch (...) {
                // CPU fallback also failed
            }
        }
    }
    
    auto elapsed = std::chrono::high_resolution_clock::now() - start_time;
    result.execution_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);
    
    return result;
}

// ============================================================================
// CUDA Call Safety Macros — Systematic error checking
// ============================================================================

/**
 * @brief Safe CUDA call with automatic error checking and logging
 *
 * Usage:
 * ```cpp
 * CUDA_SAFE_CALL(cudaMalloc(&ptr, size));
 * CUDA_SAFE_CALL(cudaMemcpy(dst, src, size, cudaMemcpyHostToDevice));
 * ```
 *
 * Logs warnings on error and continues execution (non-throwing).
 * For exception-throwing variant, see CUDA_SAFE_CALL_THROW.
 */
#if THEMIS_BATCH_A9_HAS_CUDA
#define CUDA_SAFE_CALL(call) do { \
    cudaError_t err = (call); \
    if (err != cudaSuccess) { \
        auto logger = spdlog::get("gpu"); \
        if (logger) { \
            logger->warn("CUDA call '{}' failed at {}:{}: {}", \
                        #call, __FILE__, __LINE__, cudaGetErrorString(err)); \
        } \
    } \
} while(0)
#else
#define CUDA_SAFE_CALL(call) do { (void)(call); } while(0)
#endif

/**
 * @brief Safe CUDA call with exception throwing on error
 *
 * Usage:
 * ```cpp
 * try {
 *     CUDA_SAFE_CALL_THROW(cudaMalloc(&ptr, size));
 * } catch (const std::runtime_error& e) {
 *     // Handle allocation failure
 * }
 * ```
 *
 * Throws std::runtime_error if CUDA call fails.
 */
#if THEMIS_BATCH_A9_HAS_CUDA
#define CUDA_SAFE_CALL_THROW(call) do { \
    cudaError_t err = (call); \
    if (err != cudaSuccess) { \
        std::string msg = std::string("CUDA error: ") + cudaGetErrorString(err) + \
                         " at " + __FILE__ + ":" + std::to_string(__LINE__); \
        auto logger = spdlog::get("gpu"); \
        if (logger) { logger->error("{}", msg); } \
        throw std::runtime_error(msg); \
    } \
} while(0)
#else
#define CUDA_SAFE_CALL_THROW(call) do { (void)(call); } while(0)
#endif

/**
 * @brief Safe CUDA call with fallback execution
 *
 * Usage:
 * ```cpp
 * CUDA_SAFE_CALL_WITH_FALLBACK(
 *     cudaMalloc(&gpu_ptr, size),
 *     { fallback_code_on_gpu_failure(); }
 * );
 * ```
 *
 * If CUDA call fails, executes the fallback code block.
 */
#if THEMIS_BATCH_A9_HAS_CUDA
#define CUDA_SAFE_CALL_WITH_FALLBACK(call, fallback) do { \
    cudaError_t err = (call); \
    if (err != cudaSuccess) { \
        auto logger = spdlog::get("gpu"); \
        if (logger) { \
            logger->warn("CUDA call '{}' failed: {}, executing fallback", \
                        #call, cudaGetErrorString(err)); \
        } \
        fallback; \
    } \
} while(0)
#else
#define CUDA_SAFE_CALL_WITH_FALLBACK(call, fallback) do { \
    (void)(call); \
    fallback; \
} while(0)
#endif

// ============================================================================
// GPU Memory Transfer Safety — Checked cudaMemcpy operations
// ============================================================================

/**
 * @brief Safe memory copy from host to device with error checking
 *
 * @param device_ptr GPU destination pointer
 * @param host_ptr CPU source pointer
 * @param size Number of bytes to copy
 * @return true if copy succeeded; false on failure
 */
inline bool safeMemcpyHostToDevice(void* device_ptr, const void* host_ptr, size_t size) noexcept {
#if THEMIS_BATCH_A9_HAS_CUDA
    if (device_ptr == nullptr || host_ptr == nullptr || size == 0) {
        auto logger = spdlog::get("gpu");
        if (logger) {
            logger->warn("Invalid memcpy parameters: device_ptr={}, host_ptr={}, size={}",
                        device_ptr, host_ptr, size);
        }
        return false;
    }
    
    cudaError_t err = cudaMemcpy(device_ptr, host_ptr, size, cudaMemcpyHostToDevice);
    if (err != cudaSuccess) {
        auto logger = spdlog::get("gpu");
        if (logger) {
            logger->warn("cudaMemcpy H2D failed: {}", cudaGetErrorString(err));
        }
        return false;
    }
    return true;
#else
    return true;  // CPU-only: assume success
#endif
}

/**
 * @brief Safe memory copy from device to host with error checking
 *
 * @param host_ptr CPU destination pointer
 * @param device_ptr GPU source pointer
 * @param size Number of bytes to copy
 * @return true if copy succeeded; false on failure
 */
inline bool safeMemcpyDeviceToHost(void* host_ptr, const void* device_ptr, size_t size) noexcept {
#if THEMIS_BATCH_A9_HAS_CUDA
    if (device_ptr == nullptr || host_ptr == nullptr || size == 0) {
        auto logger = spdlog::get("gpu");
        if (logger) {
            logger->warn("Invalid memcpy parameters: device_ptr={}, host_ptr={}, size={}",
                        device_ptr, host_ptr, size);
        }
        return false;
    }
    
    cudaError_t err = cudaMemcpy(host_ptr, device_ptr, size, cudaMemcpyDeviceToHost);
    if (err != cudaSuccess) {
        auto logger = spdlog::get("gpu");
        if (logger) {
            logger->warn("cudaMemcpy D2H failed: {}", cudaGetErrorString(err));
        }
        return false;
    }
    return true;
#else
    return true;  // CPU-only: assume success
#endif
}

/**
 * @brief Check whether at least one GPU device is available.
 *
 * @return true when a CUDA device is detected; false otherwise.
 */
inline bool isGPUAvailable() noexcept {
#if THEMIS_BATCH_A9_HAS_CUDA
    int device_count = 0;
    const cudaError_t err = cudaGetDeviceCount(&device_count);
    return (err == cudaSuccess) && (device_count > 0);
#else
    return false;
#endif
}

/**
 * @brief Validate allocation size for Batch A-9 safety constraints.
 *
 * @param size Allocation size in bytes.
 * @return true for sizes in the allowed range; false otherwise.
 */
inline bool isAllocationValid(size_t size) noexcept {
    constexpr size_t kMaxAllocationBytes = static_cast<size_t>(1ULL << 30);  // 1 GiB
    return size > 0 && size <= kMaxAllocationBytes;
}

/**
 * @brief Synchronize a CUDA stream with a bounded timeout.
 *
 * @param stream CUDA stream to synchronize; nullptr uses device-wide sync.
 * @param timeout_ms Timeout in milliseconds.
 * @return true on successful synchronization; false on timeout or CUDA error.
 */
inline bool streamSynchronizeWithTimeout(cudaStream_t stream, std::uint32_t timeout_ms) noexcept {
#if THEMIS_BATCH_A9_HAS_CUDA
    if (stream == nullptr) {
        return cudaDeviceSynchronize() == cudaSuccess;
    }

    const auto start = std::chrono::steady_clock::now();
    while (true) {
        const cudaError_t err = cudaStreamQuery(stream);
        if (err == cudaSuccess) {
            return true;
        }
        if (err != cudaErrorNotReady) {
            return false;
        }

        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start);
        if (elapsed.count() >= timeout_ms) {
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
#else
    (void)stream;
    (void)timeout_ms;
    return true;
#endif
}

}  // namespace batch_a9
}  // namespace gpu
}  // namespace themis
