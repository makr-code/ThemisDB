/*
 * GPU Memory Manager with Edition-Specific Constraints
 * ======================================================
 * Enforces GPU VRAM limits based on edition at runtime.
 * Edition constraints are set at compile-time via CMakeLists.txt.
 */

#include <stdint.h>
#include <string>
#include <mutex>
#include <stdexcept>
#include "themis/edition.h"

namespace themis {
namespace gpu {

// ============================================================================
// GPU MEMORY MANAGER - EDITION-AWARE
// ============================================================================

class GPUMemoryManager {
public:
    // Singleton instance
    static GPUMemoryManager& GetInstance() {
        static GPUMemoryManager instance;
        return instance;
    }

    // Get maximum GPU VRAM available for this edition (in GB)
    static constexpr int GetMaxGPUVRAMGB() {
        return edition::GPU_MAX_VRAM_GB;
    }

    // Get maximum GPU VRAM available for this edition (in bytes)
    static constexpr uint64_t GetMaxGPUVRAMBytes() {
        return static_cast<uint64_t>(GetMaxGPUVRAMGB()) * 1024ULL * 1024ULL * 1024ULL;
    }

    // Allocate GPU memory with edition-specific limits
    // Returns true if allocation is allowed, false if would exceed limit
    bool TryAllocateGPU(uint64_t size_bytes, const std::string& reason = "Unknown") {
        std::lock_guard<std::mutex> lock(mutex_);

        uint64_t max_vram = GetMaxGPUVRAMBytes();
        uint64_t new_total = gpu_memory_allocated_ + size_bytes;

        if (new_total > max_vram) {
            // Log that allocation would exceed limit
            // In production: LOG_WARNING or similar
            return false;
        }

        gpu_memory_allocated_ = new_total;
        // Could track individual allocations here for better telemetry
        return true;
    }

    // Deallocate GPU memory
    void DeallocateGPU(uint64_t size_bytes) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (gpu_memory_allocated_ >= size_bytes) {
            gpu_memory_allocated_ -= size_bytes;
        }
    }

    // Get current GPU memory usage
    uint64_t GetGPUMemoryUsed() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return gpu_memory_allocated_;
    }

    // Get percentage of GPU memory used
    float GetGPUMemoryUsagePercent() const {
        uint64_t max_vram = GetMaxGPUVRAMBytes();
        if (max_vram == 0) return 0.0f;
        
        std::lock_guard<std::mutex> lock(mutex_);
        return (static_cast<float>(gpu_memory_allocated_) / static_cast<float>(max_vram)) * 100.0f;
    }

    // Check if GPU acceleration is available in this edition
    bool IsGPUAccelerationEnabled() const {
        // In Community edition with 24GB limit, GPU might still be available
        // (consumer RTX 4090 has 24GB)
        // Return false only if explicitly disabled at build time
        return GetMaxGPUVRAMGB() > 0;
    }

    // Get human-readable edition information
    std::string GetEditionInfo() const {
        const auto info = edition::EditionInfo::Get();
        std::string result = "Edition: ";
        result += std::string(info.name);
        result += " | GPU VRAM: ";
        result += std::to_string(info.gpu_max_vram_gb);
        result += "GB | Max Nodes: ";
        result += std::to_string(info.sharding_max_nodes);
        return result;
    }

    // Validate GPU memory request (throws exception on invalid request)
    void ValidateAllocation(uint64_t size_bytes) {
        uint64_t max_vram = GetMaxGPUVRAMBytes();
        
        if (size_bytes > max_vram) {
            std::string error = "GPU allocation request (";
            error += std::to_string(size_bytes / (1024ULL * 1024ULL * 1024ULL));
            error += "GB) exceeds edition limit (";
            error += std::to_string(GetMaxGPUVRAMGB());
            error += "GB). Edition: ";
            error += std::string(edition::EDITION_STRING);
            throw std::runtime_error(error);
        }

        std::lock_guard<std::mutex> lock(mutex_);
        uint64_t new_total = gpu_memory_allocated_ + size_bytes;
        
        if (new_total > max_vram) {
            std::string error = "GPU memory exhausted. Current usage: ";
            error += std::to_string(gpu_memory_allocated_ / (1024ULL * 1024ULL * 1024ULL));
            error += "GB, requested: ";
            error += std::to_string(size_bytes / (1024ULL * 1024ULL * 1024ULL));
            error += "GB, limit: ";
            error += std::to_string(GetMaxGPUVRAMGB());
            error += "GB";
            throw std::runtime_error(error);
        }
    }

private:
    GPUMemoryManager() : gpu_memory_allocated_(0) {}
    ~GPUMemoryManager() = default;

    // Prevent copying
    GPUMemoryManager(const GPUMemoryManager&) = delete;
    GPUMemoryManager& operator=(const GPUMemoryManager&) = delete;

    mutable std::mutex mutex_;
    uint64_t gpu_memory_allocated_;  // Current GPU memory in use (bytes)
};

// ============================================================================
// RUNTIME EDITION-AWARE UTILITY FUNCTIONS
// ============================================================================

// Check if vector search with GPU is possible in this edition
inline bool CanUseGPUForVectorSearch() {
    // GPU available in Community (24GB) and higher editions
    return edition::GetEditionType() != edition::EditionType::UNKNOWN;
}

// Get fallback strategy when GPU memory is exhausted
// For Community: Fall back to CPU (with warning)
// For Enterprise/Hyperscaler: Use CPU as fallback, suggest upgrade
inline std::string GetGPUFallbackStrategy() {
    const auto info = edition::EditionInfo::Get();
    std::string strategy = "GPU memory limit exceeded (";
    strategy += std::to_string(info.gpu_max_vram_gb);
    strategy += "GB for ";
    strategy += std::string(info.name);
    strategy += " edition). Falling back to CPU vector search.";
    
    if (info.type == edition::EditionType::COMMUNITY) {
        strategy += " Consider Community GPU guidelines or upgrade to Enterprise for higher limits.";
    }
    return strategy;
}

} // namespace gpu
} // namespace themis
