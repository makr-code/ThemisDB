/*
 * GPU Memory Manager with Edition-Specific Constraints
 * ======================================================
 * Enforces GPU VRAM limits based on edition at runtime.
 * Edition constraints are set at compile-time via CMakeLists.txt.
 */

#include "themis/gpu/memory_manager.h"

namespace themis {
namespace gpu {

// ============================================================================
// GPUMemoryManager — method implementations
// ============================================================================

bool GPUMemoryManager::TryAllocateGPU(uint64_t size_bytes, const std::string& tag) {
    std::lock_guard<std::mutex> lock(mutex_);

    const uint64_t max_vram  = GetMaxGPUVRAMBytes();
    const uint64_t new_total = gpu_memory_allocated_ + size_bytes;

    if (new_total > max_vram) {
        return false;
    }

    gpu_memory_allocated_ = new_total;
    if (gpu_memory_allocated_ > peak_bytes_) {
        peak_bytes_ = gpu_memory_allocated_;
    }
    ++allocation_count_;
    active_allocations_.push_back({size_bytes, tag});
    return true;
}

void GPUMemoryManager::DeallocateGPU(uint64_t size_bytes) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (gpu_memory_allocated_ >= size_bytes) {
        gpu_memory_allocated_ -= size_bytes;
    } else {
        gpu_memory_allocated_ = 0;  // guard against mis-matched sizes
    }
    ++deallocation_count_;

    // Remove the first active record whose size matches (FIFO / best-effort).
    for (auto it = active_allocations_.begin(); it != active_allocations_.end(); ++it) {
        if (it->size_bytes == size_bytes) {
            active_allocations_.erase(it);
            break;
        }
    }
}

void GPUMemoryManager::ValidateAllocation(uint64_t size_bytes) {
    const uint64_t max_vram = GetMaxGPUVRAMBytes();

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
    const uint64_t new_total = gpu_memory_allocated_ + size_bytes;

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

uint64_t GPUMemoryManager::GetGPUMemoryUsed() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return gpu_memory_allocated_;
}

float GPUMemoryManager::GetGPUMemoryUsagePercent() const {
    const uint64_t max_vram = GetMaxGPUVRAMBytes();
    if (max_vram == 0) return 0.0f;

    std::lock_guard<std::mutex> lock(mutex_);
    return (static_cast<float>(gpu_memory_allocated_) / static_cast<float>(max_vram)) * 100.0f;
}

bool GPUMemoryManager::IsGPUAccelerationEnabled() const noexcept {
    return GetMaxGPUVRAMGB() > 0;
}

GPUMemoryManager::Stats GPUMemoryManager::GetStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    Stats s;
    s.allocated_bytes    = gpu_memory_allocated_;
    s.peak_bytes         = peak_bytes_;
    s.allocation_count   = allocation_count_;
    s.deallocation_count = deallocation_count_;
    return s;
}

std::vector<GPUMemoryManager::AllocationRecord>
GPUMemoryManager::GetActiveAllocations() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return active_allocations_;
}

std::string GPUMemoryManager::GetEditionInfo() const {
    const auto info = edition::EditionInfo::Get();
    std::string result = "Edition: ";
    result += std::string(info.name);
    result += " | GPU VRAM: ";
    result += std::to_string(info.gpu_max_vram_gb);
    result += "GB | Max Nodes: ";
    result += std::to_string(info.sharding_max_nodes);
    return result;
}

// ============================================================================
// GetGPUFallbackStrategy — free function implementation
// ============================================================================

std::string GetGPUFallbackStrategy() {
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
