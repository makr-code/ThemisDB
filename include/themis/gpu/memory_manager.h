#pragma once

#include <cstdint>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>
#include "themis/edition.h"

namespace themis {
namespace gpu {

/**
 * @brief Edition-aware GPU VRAM memory manager.
 *
 * Enforces per-edition VRAM limits at runtime.  All limits are set at
 * compile-time via CMakeLists.txt (-DTHEMIS_EDITION).  The manager tracks
 * individual allocations by tag so that callers can obtain aggregate stats,
 * peak usage, and per-tag breakdowns.
 *
 * Thread safety: all public methods are protected by an internal mutex.
 */
class GPUMemoryManager {
public:
    // -----------------------------------------------------------------------
    // Allocation record — one entry per successful TryAllocateGPU() call
    // -----------------------------------------------------------------------
    struct AllocationRecord {
        uint64_t    size_bytes = 0;
        std::string tag;            // caller-supplied reason / owner label
    };

    // -----------------------------------------------------------------------
    // Aggregate statistics
    // -----------------------------------------------------------------------
    struct Stats {
        uint64_t allocated_bytes    = 0;  ///< current live VRAM usage
        uint64_t peak_bytes         = 0;  ///< high-water mark since construction
        uint64_t allocation_count   = 0;  ///< successful TryAllocateGPU() calls
        uint64_t deallocation_count = 0;  ///< successful DeallocateGPU() calls
    };

    // -----------------------------------------------------------------------
    // Singleton
    // -----------------------------------------------------------------------
    static GPUMemoryManager& GetInstance() {
        static GPUMemoryManager instance;
        return instance;
    }

    // -----------------------------------------------------------------------
    // Edition limits (compile-time constants)
    // -----------------------------------------------------------------------
    static constexpr int GetMaxGPUVRAMGB() noexcept {
        return edition::GPU_MAX_VRAM_GB;
    }

    static constexpr uint64_t GetMaxGPUVRAMBytes() noexcept {
        return static_cast<uint64_t>(GetMaxGPUVRAMGB()) * 1024ULL * 1024ULL * 1024ULL;
    }

    // -----------------------------------------------------------------------
    // Allocation / deallocation
    // -----------------------------------------------------------------------

    /**
     * @brief Request VRAM of @p size_bytes tagged with @p tag.
     *
     * Returns true and records the allocation if the request fits within the
     * edition limit.  Returns false (without throwing) if it would exceed the
     * limit.
     */
    bool TryAllocateGPU(uint64_t size_bytes, const std::string& tag = "Unknown");

    /**
     * @brief Release @p size_bytes of previously allocated VRAM.
     *
     * Silently clamps to zero if @p size_bytes exceeds the tracked total to
     * guard against double-free or mis-matched sizes.
     */
    void DeallocateGPU(uint64_t size_bytes);

    /**
     * @brief Validate a proposed allocation; throws std::runtime_error on
     *        rejection instead of returning false.
     */
    void ValidateAllocation(uint64_t size_bytes);

    // -----------------------------------------------------------------------
    // Queries
    // -----------------------------------------------------------------------
    uint64_t GetGPUMemoryUsed() const;
    float    GetGPUMemoryUsagePercent() const;
    bool     IsGPUAccelerationEnabled() const noexcept;
    Stats    GetStats() const;

    std::string GetEditionInfo() const;

    /**
     * @brief Return a snapshot of all currently live allocation records.
     *
     * Useful for debugging and leak detection: callers can inspect which tags
     * still hold VRAM after their workload completes.
     */
    std::vector<AllocationRecord> GetActiveAllocations() const;

private:
    GPUMemoryManager() = default;
    ~GPUMemoryManager() = default;

    GPUMemoryManager(const GPUMemoryManager&) = delete;
    GPUMemoryManager& operator=(const GPUMemoryManager&) = delete;

    mutable std::mutex mutex_;
    uint64_t gpu_memory_allocated_ = 0;
    uint64_t peak_bytes_           = 0;
    uint64_t allocation_count_     = 0;
    uint64_t deallocation_count_   = 0;

    // Per-allocation records for owner/tag tracking and leak detection.
    // On TryAllocateGPU() success a record is appended; on DeallocateGPU()
    // the first record whose size_bytes matches is removed.
    std::vector<AllocationRecord> active_allocations_;
};

// ---------------------------------------------------------------------------
// Free-function helpers
// ---------------------------------------------------------------------------

/**
 * @brief Returns true when the current edition supports GPU acceleration
 *        (i.e. VRAM limit > 0).
 */
inline bool CanUseGPUForVectorSearch() noexcept {
    return edition::GetEditionType() != edition::EditionType::UNKNOWN;
}

/**
 * @brief Human-readable CPU-fallback message for when VRAM is exhausted.
 */
std::string GetGPUFallbackStrategy();

} // namespace gpu
} // namespace themis
