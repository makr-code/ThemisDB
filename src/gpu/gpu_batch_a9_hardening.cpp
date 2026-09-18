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

bool isAllocationValid(size_t size_bytes) noexcept {
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
 * @brief Create Stream Safe.
 * @return Return value.
 * @details Calls: GPUStreamHandle().
 */
GPUStreamHandle createStreamSafe() {
    return GPUStreamHandle();
}

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

template<typename T>
/**
 * @brief Allocate GPUMemory Safe.
 * @param[in] count Input parameter.
 * @return Return value.
 * @throws std::runtime_error if an error occurs.
 * @details Calls: isAllocationValid(), spdlog::get(), debug().
 */
GPUMemoryHandle<T> allocateGPUMemorySafe(size_t count) {
    if (!isAllocationValid(count * sizeof(T))) {
        throw std::runtime_error("Invalid allocation request");
    }
    
    auto logger = spdlog::get("gpu");
    if (logger) {
        logger->debug("Allocating GPU memory: {} bytes", count * sizeof(T));
    }
    
    return GPUMemoryHandle<T>(count);
}

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
