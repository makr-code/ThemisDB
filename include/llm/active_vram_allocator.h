/**
 * @file active_vram_allocator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <optional>

namespace themis {
namespace llm {

// Forward declarations
class GPUMemoryManager;

class ActiveVRAMAllocator {
public:
    // -----------------------------------------------------------------------
    // Configuration
    // -----------------------------------------------------------------------

    struct Config {
        size_t max_vram_bytes = 0;

        size_t max_cpu_spill_bytes = 16ULL * 1024 * 1024 * 1024;  // 16 GB

        float oom_threshold_fraction = 0.90f;

        size_t min_free_vram_reserve = 512ULL * 1024 * 1024;  // 512 MB

        bool enable_defragmentation = true;

        bool enable_cpu_spilling = true;

        int gpu_device_id = -1;

        size_t block_alignment = 4096;
    };

    // -----------------------------------------------------------------------
    // Allocation handle
    // -----------------------------------------------------------------------

    struct AllocationHandle {
        uint64_t id = 0;

        std::string owner_id;

        size_t requested_bytes = 0;

        size_t allocated_bytes = 0;

        void* gpu_ptr = nullptr;

        void* cpu_ptr = nullptr;

        bool is_spilled = false;

        bool valid = false;

        bool is_external = false;

        int64_t allocated_at_ms = 0;

        int64_t last_used_at_ms = 0;
    };

    // -----------------------------------------------------------------------
    // Statistics & error reporting
    // -----------------------------------------------------------------------

    struct Stats {
        size_t total_vram_bytes = 0;

        size_t used_vram_bytes = 0;

        size_t free_vram_bytes = 0;

        size_t peak_vram_bytes = 0;

        size_t wasted_padding_bytes = 0;

        size_t spilled_cpu_bytes = 0;

        size_t live_allocation_count = 0;

        uint64_t oom_event_count = 0;

        uint64_t oom_recovery_count = 0;

        uint64_t eviction_count = 0;

        uint64_t defrag_count = 0;

        uint64_t spill_count = 0;

        float fragmentation_pct = 0.0f;

        bool oom_threshold_exceeded = false;
    };

    // -----------------------------------------------------------------------
    // OOM event notification
    // -----------------------------------------------------------------------

    enum class OOMRecoveryStrategy {
        Eviction,
        Defragmentation,
        CPUSpilling,
        Failed  ///< All strategies exhausted without success
    };

    struct OOMEvent {
        size_t requested_bytes = 0;

        OOMRecoveryStrategy strategy = OOMRecoveryStrategy::Failed;

        bool recovered = false;

        size_t bytes_recovered = 0;
    };

    using OOMCallback = std::function<void(const OOMEvent&)>;

    // -----------------------------------------------------------------------
    // Construction
    // -----------------------------------------------------------------------

    /**
     * @brief Active VRAMAllocator.
     * @param[in] cfg Input parameter.
     * @return Return value.
     */
    explicit ActiveVRAMAllocator(const Config& cfg);

    ActiveVRAMAllocator();

    ~ActiveVRAMAllocator();

    // Non-copyable, movable.
    ActiveVRAMAllocator(const ActiveVRAMAllocator&) = delete;
    ActiveVRAMAllocator& operator=(const ActiveVRAMAllocator&) = delete;
    ActiveVRAMAllocator(ActiveVRAMAllocator&&) noexcept;
    ActiveVRAMAllocator& operator=(ActiveVRAMAllocator&&) noexcept;

    // -----------------------------------------------------------------------
    // Core allocation API
    // -----------------------------------------------------------------------

    std::optional<AllocationHandle> allocate(
        size_t bytes,
        const std::string& owner_id,
        int gpu_device_id = -1);

    std::optional<AllocationHandle> allocateOrRecover(
        size_t bytes,
        const std::string& owner_id,
        int gpu_device_id = -1);

    /**
     * @brief Free.
     * @param[in,out] handle Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool free(AllocationHandle& handle);

    /**
     * @brief Touch.
     * @param[in,out] handle Input/output parameter.
     */
    void touch(AllocationHandle& handle);

    // -----------------------------------------------------------------------
    // OOM recovery API
    // -----------------------------------------------------------------------

    bool handleOOM(size_t need_bytes = 0);

    /**
     * @brief Evict LRU.
     * @return Return value.
     */
    size_t evictLRU();

    /**
     * @brief Evict Owner.
     * @param[in] owner_id Identifier of the owner.
     * @return Return value.
     */
    size_t evictOwner(const std::string& owner_id);

    /**
     * @brief Defragment.
     * @return True when the operation succeeds.
     */
    bool defragment();

    /**
     * @brief Spill LRUTo CPU.
     * @return Return value.
     */
    size_t spillLRUToCPU();

    /**
     * @brief Restore From CPU.
     * @param[in,out] handle Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool restoreFromCPU(AllocationHandle& handle);

    // -----------------------------------------------------------------------
    // Monitoring & introspection
    // -----------------------------------------------------------------------

    /**
     * @brief Get Stats.
     * @return Return value.
     */
    Stats getStats() const;

    /**
     * @brief Is OOMThreshold Exceeded.
     * @return True when the operation succeeds.
     */
    bool isOOMThresholdExceeded() const;

    /**
     * @brief Set OOMCallback.
     * @param[in] cb Input parameter.
     */
    void setOOMCallback(OOMCallback cb);

    /**
     * @brief List Allocations.
     * @return Return value.
     */
    std::vector<AllocationHandle> listAllocations() const;

    /**
     * @brief Gpu Device Id.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    int gpuDeviceId() const noexcept;

    /**
     * @brief Is GPUAvailable.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
    bool isGPUAvailable() const noexcept;

    /**
     * @brief ----------------------------------------------------------------------- External-memory registration (for externally-managed GPU memory) -----------------------------------------------------------------------
     * @param[in] bytes Input parameter.
     * @param[in] owner_id Identifier of the owner.
     * @return Return value.
     */

    AllocationHandle registerExternal(size_t bytes, const std::string& owner_id);

    /**
     * @brief ----------------------------------------------------------------------- Integration with AdaptiveVRAMAllocator (bridge API) -----------------------------------------------------------------------
     * @param[in] bytes Input parameter.
     * @param[in,out] ptr Input/output parameter.
     * @return True when the operation succeeds.
     */

    bool allocateWithFragmentation(size_t bytes, void** ptr);

    /**
     * @brief Handle Out Of Memory.
     * @return True when the operation succeeds.
     */
    bool handleOutOfMemory();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace llm
} // namespace themis
