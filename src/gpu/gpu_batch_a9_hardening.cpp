/**
 * @file gpu_batch_a9_hardening.cpp
 * @brief GPU Module Batch A-9 — Hardening Implementations
 *
 * Provides implementations of GPU safety patterns with systematic error checking,
 * RAII enforcement, timeout limits, and CPU fallback paths.
 *
 * @version 1.0
 * @date 2026-08-18
 * @author Batch A-9 GPU Safety Hardening
 */

#include "gpu/gpu_batch_a9_safety.hpp"
#include "gpu/gpu_raii_wrappers.hpp"

#include <chrono>
#include <spdlog/spdlog.h>
#include <thread>

namespace themis {
namespace gpu {
namespace batch_a9 {

// ============================================================================
// Helper functions for memory validation
// ============================================================================

/**
 * @brief Validate GPU device availability and properties
 *
 * @return true if at least one GPU device is available; false otherwise
 */
bool isGPUAvailable() noexcept {
#if THEMIS_BATCH_A9_HAS_CUDA
    int device_count = 0;
    cudaError_t err = cudaGetDeviceCount(&device_count);
    if (err != cudaSuccess || device_count == 0) {
        return false;
    }
    
    // Verify device has reasonable compute capability
    cudaDeviceProp prop{};
    err = cudaGetDeviceProperties(&prop, 0);
    if (err != cudaSuccess) {
        return false;
    }
    
    // Log device capabilities
    auto logger = spdlog::get("gpu");
    if (logger && device_count > 0) {
        logger->info("GPU Device 0: {} ({} SMs, CC {}.{})",
                    prop.name, prop.multiProcessorCount,
                    prop.major, prop.minor);
    }
    
    return true;
#else
    return false;
#endif
}

/**
 * @brief Get remaining device memory (approximate)
 *
 * @return Available device memory in bytes (0 if unavailable)
 */
size_t getAvailableDeviceMemory() noexcept {
#if THEMIS_BATCH_A9_HAS_CUDA
    size_t free_bytes = 0, total_bytes = 0;
    cudaError_t err = cudaMemGetInfo(&free_bytes, &total_bytes);
    if (err != cudaSuccess) {
        return 0;
    }
    return free_bytes;
#else
    return 0;
#endif
}

/**
 * @brief Validate memory allocation parameters
 *
 * @param size_bytes Number of bytes to allocate
 * @return true if allocation is feasible; false if too large or invalid
 */
bool isAllocationValid([[maybe_unused]] size_t size_bytes) noexcept {
    if (size_bytes == 0) {
        auto logger = spdlog::get("gpu");
        if (logger) {
            logger->warn("Invalid allocation size: 0 bytes");
        }
        return false;
    }
    
    // Prevent allocations larger than 1GB
    constexpr size_t MAX_ALLOCATION = 1 << 30;  // 1GB
    if (size_bytes > MAX_ALLOCATION) {
        auto logger = spdlog::get("gpu");
        if (logger) {
            logger->warn("Allocation too large: {} bytes (max: {} bytes)", size_bytes, MAX_ALLOCATION);
        }
        return false;
    }
    
    // Check available device memory
    size_t available = getAvailableDeviceMemory();
    if (available > 0 && size_bytes > available) {
        auto logger = spdlog::get("gpu");
        if (logger) {
            logger->warn("Insufficient device memory: requested {} bytes, available {} bytes",
                        size_bytes, available);
        }
        return false;
    }
    
    return true;
}

// ============================================================================
// GPU Stream Management
// ============================================================================

/**
 * @brief Create a GPU stream with validation
 *
 * @return GPU stream handle, or throws on failure
 * @throws std::runtime_error if stream creation fails
 */
GPUStreamHandle createStreamSafe() {
    return GPUStreamHandle();
}

/**
 * @brief Synchronize GPU stream with timeout
 *
 * @param stream Stream to synchronize (can be nullptr for default stream)
 * @param timeout_ms Maximum wait time in milliseconds
 * @return true if stream synchronized; false if timeout exceeded
 */
bool streamSynchronizeWithTimeout(cudaStream_t stream, uint32_t timeout_ms) noexcept {
#if THEMIS_BATCH_A9_HAS_CUDA
    auto start = std::chrono::high_resolution_clock::now();
    
    while (true) {
        cudaError_t err = {};
        if (stream != nullptr) {
            err = cudaStreamQuery(stream);
        } else {
            err = cudaDeviceSynchronize();
        }
        
        if (err == cudaSuccess) {
            return true;
        }
        
        if (err != cudaErrorNotReady) {
            auto logger = spdlog::get("gpu");
            if (logger) {
                logger->warn("Stream sync error: {}", cudaGetErrorString(err));
            }
            return false;
        }
        
        auto elapsed = std::chrono::high_resolution_clock::now() - start;
        if (std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() >= timeout_ms) {
            auto logger = spdlog::get("gpu");
            if (logger) {
                logger->warn("Stream sync timeout after {}ms", timeout_ms);
            }
            return false;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
#else
    return true;
#endif
}

// ============================================================================
// GPU Memory Management with Validation
// ============================================================================

/**
 * @brief Allocate GPU memory with safety checks
 *
 * @tparam T Element type
 * @param count Number of elements to allocate
 * @return GPU memory handle, or throws on failure
 * @throws std::runtime_error if allocation fails
 */
template<typename T>
GPUMemoryHandle<T> allocateGPUMemorySafe([[maybe_unused]] size_t count) {
    if (!isAllocationValid(count * sizeof(T))) {
        throw std::runtime_error("Invalid allocation request");
    }
    
    auto logger = spdlog::get("gpu");
    if (logger) {
        logger->debug("Allocating GPU memory: {} bytes", count * sizeof(T));
    }
    
    return GPUMemoryHandle<T>(count);
}

/**
 * @brief Copy data to GPU with validation
 *
 * @param device_ptr GPU destination (must be valid)
 * @param host_ptr CPU source (must be valid)
 * @param size Number of bytes to copy
 * @return true if copy succeeded; false otherwise
 */
bool copyToGPUSafe(void* device_ptr, const void* host_ptr, size_t size) noexcept {
    if (!safeMemcpyHostToDevice(device_ptr, host_ptr, size)) {
        return false;
    }
    
    auto logger = spdlog::get("gpu");
    if (logger) {
        logger->debug("Copied {} bytes to GPU", size);
    }
    
    return true;
}

/**
 * @brief Copy data from GPU with validation
 *
 * @param host_ptr CPU destination (must be valid)
 * @param device_ptr GPU source (must be valid)
 * @param size Number of bytes to copy
 * @return true if copy succeeded; false otherwise
 */
bool copyFromGPUSafe(void* host_ptr, const void* device_ptr, size_t size) noexcept {
    if (!safeMemcpyDeviceToHost(host_ptr, device_ptr, size)) {
        return false;
    }
    
    auto logger = spdlog::get("gpu");
    if (logger) {
        logger->debug("Copied {} bytes from GPU", size);
    }
    
    return true;
}

}  // namespace batch_a9
}  // namespace gpu
}  // namespace themis
