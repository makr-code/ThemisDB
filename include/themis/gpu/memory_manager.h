/**
 * @file memory_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <cstdint>
#include <mutex>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
#include "themis/edition.h"
#include "themis/gpu/ivram_policy.h"

namespace themis {
namespace gpu {

/**
 * @brief Edition-aware GPU VRAM memory manager — canonical `IVRAMPolicy` implementation.
 *
 * Enforces per-edition VRAM limits at runtime.  All limits are set at
 * compile-time via CMakeLists.txt (-DTHEMIS_EDITION).  The manager tracks
 * individual allocations by tag so that callers can obtain aggregate stats,
 * peak usage, and per-tag breakdowns.
 *
 * Tenant isolation: callers may register per-tenant VRAM quotas via
 * SetTenantQuota().  Allocations carrying a tenant_id are checked against
 * both the global edition limit and the per-tenant quota.
 *
 * Thread safety: all public methods are protected by an internal mutex.
 *
 * ### Unified hierarchy role
 *
 * This class is the **canonical** `IVRAMPolicy` implementation for the
 * ThemisDB GPU memory hierarchy.  Subsystem managers (e.g.,
 * `themis::llm::GPUMemoryManager`, `themis::llm::lora::GPUMemoryManager`)
 * delegate their allocation accounting to the singleton instance so that
 * global VRAM budgets and per-tenant quotas are enforced at a single point.
 */
class GPUMemoryManager : public IVRAMPolicy {
public:
    // -----------------------------------------------------------------------
    // Allocation record — one entry per successful TryAllocateGPU() call
    // -----------------------------------------------------------------------
    struct AllocationRecord {
        uint64_t    size_bytes = 0;
        std::string tag;            // caller-supplied reason / owner label
        std::string tenant_id;      // empty = no tenant / global
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
    // Per-tenant statistics
    // -----------------------------------------------------------------------
    struct TenantStats {
        std::string tenant_id;
        uint64_t    quota_bytes     = 0;  ///< 0 = no per-tenant cap (global limit applies)
        uint64_t    allocated_bytes = 0;  ///< live VRAM owned by this tenant
        uint64_t    peak_bytes      = 0;  ///< high-water mark for this tenant
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
    // Tenant quota management
    // -----------------------------------------------------------------------

    /**
     * @brief Register or update a per-tenant VRAM quota.
     *
     * @param tenant_id  Opaque tenant identifier.
     * @param quota_bytes Maximum VRAM this tenant may use concurrently.
     *                   Pass 0 to remove the per-tenant cap (global limit applies).
     */
    void SetTenantQuota(const std::string& tenant_id, uint64_t quota_bytes);

    /**
     * @brief Remove the per-tenant VRAM cap (sets quota to 0 / unlimited).
     *
     * The tenant entry is kept in the internal map so that usage tracking
     * continues until all its allocations have been freed.
     */
    void RemoveTenantQuota(const std::string& tenant_id);

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
     * @brief Tenant-aware variant of TryAllocateGPU().
     *
     * Checks both the global edition VRAM limit and the per-tenant quota
     * (if one has been set with SetTenantQuota()).
     *
     * @param size_bytes  Bytes to reserve.
     * @param tag         Owner / reason label.
     * @param tenant_id   Tenant identifier; empty string = no tenant check.
     * @return true if allocation was granted, false if either limit was
     *         exceeded (without modifying state).
     */
    bool TryAllocateGPU(uint64_t size_bytes,
                        const std::string& tag,
                        const std::string& tenant_id);

    /**
     * @brief Release @p size_bytes of previously allocated VRAM.
     *
     * Silently clamps to zero if @p size_bytes exceeds the tracked total to
     * guard against double-free or mis-matched sizes.
     */
    void DeallocateGPU(uint64_t size_bytes);

    /**
     * @brief Tenant-aware deallocation.
     *
     * Decrements both the global counter and the per-tenant counter.
     */
    void DeallocateGPU(uint64_t size_bytes, const std::string& tenant_id);

    /**
     * @brief Validate a proposed allocation; throws std::runtime_error on
     *        rejection instead of returning false.
     */
    void ValidateAllocation(uint64_t size_bytes);

    // -----------------------------------------------------------------------
    // Pre-allocation hints
    // -----------------------------------------------------------------------

    /**
     * @brief Handle returned by ReserveHint().  Use the id to cancel or
     *        consume the hint.  A zero id indicates failure.
     */
    struct HintHandle {
        uint64_t    id         = 0;     ///< 0 = invalid
        uint64_t    bytes      = 0;
        std::string tag;
        std::string tenant_id;
    };

    /**
     * @brief Reserve @p bytes of VRAM headroom for a future allocation.
     *
     * The reserved bytes count against the edition limit and against the
     * tenant quota (if set) so that other callers cannot consume the capacity
     * before the reservation is consumed or cancelled.
     *
     * @return HintHandle with id > 0 on success, id == 0 if the reservation
     *         cannot be honoured (limit already exceeded).
     */
    HintHandle ReserveHint(uint64_t bytes,
                           const std::string& tag        = "hint",
                           const std::string& tenant_id  = "");

    /**
     * @brief Release a previously reserved hint without allocating.
     *
     * Safe to call with an invalid (id == 0) handle.
     */
    void CancelHint(uint64_t hint_id);

    /**
     * @brief Convert a hint into a real allocation (atomic swap).
     *
     * The reserved bytes remain occupied in the VRAM budget; only their
     * classification changes from "hint" to "active allocation".
     *
     * @return true if the hint was found and converted; false if the hint_id
     *         was not found (already consumed or cancelled).
     */
    bool ConsumeHint(uint64_t hint_id);

    /** @brief Total bytes currently held by outstanding hints. */
    uint64_t GetHintReservedBytes() const;

    // -----------------------------------------------------------------------
    // IVRAMPolicy implementation
    // -----------------------------------------------------------------------

    /**
     * @brief Check if @p size_bytes can be allocated (IVRAMPolicy contract).
     *
     * Equivalent to a non-committing TryAllocateGPU() probe: checks the
     * global edition VRAM limit and the per-tenant quota (when tenant_id is
     * non-empty) without modifying any state.
     */
    [[nodiscard]] bool canAllocate(uint64_t size_bytes,
                                   const std::string& tenant_id = "") const override;

    /**
     * @brief Record a successful allocation (IVRAMPolicy contract).
     *
     * Updates the global and per-tenant accounting as if TryAllocateGPU()
     * had been called.  Use this when physical memory is allocated outside
     * the manager but its VRAM footprint must be tracked here.
     */
    void onAllocate(uint64_t size_bytes,
                    const std::string& tag,
                    const std::string& tenant_id = "") override;

    /**
     * @brief Record a deallocation (IVRAMPolicy contract).
     *
     * Decrements global and per-tenant accounting.
     */
    void onDeallocate(uint64_t size_bytes,
                      const std::string& tenant_id = "") override;

    /**
     * @brief Return currently tracked VRAM usage (IVRAMPolicy contract).
     */
    [[nodiscard]] uint64_t usedBytes() const override;

    /**
     * @brief Return true when the current edition allows GPU acceleration
     *        (IVRAMPolicy contract).
     */
    [[nodiscard]] bool isGPUEnabled() const noexcept override;

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

    /**
     * @brief Return stats for a specific tenant.
     *
     * Returns a zero-filled TenantStats if the tenant has never allocated or
     * had a quota set.
     */
    TenantStats GetTenantStats(const std::string& tenant_id) const;

    /**
     * @brief Return a snapshot of stats for all tenants that have a quota or
     *        at least one live allocation.
     */
    std::vector<TenantStats> GetAllTenantStats() const;

    /**
     * @brief Return how many bytes the tenant may still allocate.
     *
     * Returns the lesser of (global_remaining) and (tenant_quota - tenant_used).
     * If the tenant has no quota registered, only the global limit is considered.
     */
    uint64_t GetTenantHeadroom(const std::string& tenant_id) const;

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
    // the first record whose size_bytes (and tenant_id, if provided) matches
    // is removed.
    std::vector<AllocationRecord> active_allocations_;

    // Pre-allocation hints.
    struct HintRecord {
        uint64_t    id         = 0;
        uint64_t    bytes      = 0;
        std::string tag;
        std::string tenant_id;
    };
    uint64_t             hint_reserved_bytes_ = 0;
    uint64_t             next_hint_id_        = 1;
    std::vector<HintRecord> active_hints_;

    // Per-tenant state — keyed by tenant_id.
    struct TenantState {
        uint64_t quota_bytes     = 0;  // 0 = no cap
        uint64_t allocated_bytes = 0;
        uint64_t peak_bytes      = 0;
    };
    std::unordered_map<std::string, TenantState> tenant_states_;

    // Internal helper called under lock.
    bool TryAllocateUnderLock(uint64_t size_bytes,
                              const std::string& tag,
                              const std::string& tenant_id);

    // Rollback helper (Phase 3 Hardening) — called under lock for exception recovery.
    // Decrements both global and per-tenant counters; idempotent and noexcept.
    void RollbackAllocationUnderLock(const std::string& tenant_id, uint64_t size_bytes);
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

