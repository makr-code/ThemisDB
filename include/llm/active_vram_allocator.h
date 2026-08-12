/**
 * @file active_vram_allocator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=7; TODO=1, Stub=3, Unimpl=0, Mock=1, Sim=2, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief ActiveVRAMAllocator — physisch GPU-basierter VRAM-Verwalter für LLM-Workloads
 *
 * Implements the LLM-MISSING-001 requirement: a production-grade GPU memory
 * allocator with OOM detection, waste reporting, and recovery strategies.
 *
 * Recovery strategy order (attempted in sequence on OOM):
 *   1. Eviction   — free the least-recently-used allocation
 *   2. Defragmentation — compact free fragments into contiguous blocks
 *   3. CPU Spilling   — migrate an allocation to CPU/pinned memory
 *
 * Usage example:
 * @code
 *   ActiveVRAMAllocator::Config cfg;
 *   cfg.max_vram_bytes = 24ULL << 30;
 *   cfg.oom_threshold_fraction = 0.90f;
 *   ActiveVRAMAllocator alloc(cfg);
 *
 *   auto handle = alloc.allocate(model_bytes, "llama-7b");
 *   if (!handle) {
 *       bool ok = alloc.handleOOM(model_bytes);
 *       if (ok) handle = alloc.allocate(model_bytes, "llama-7b");
 *   }
 * @endcode
 *
 * Thread safety: all public methods are thread-safe.
 */
class ActiveVRAMAllocator {
public:
    // -----------------------------------------------------------------------
    // Configuration
    // -----------------------------------------------------------------------

    struct Config {
        /// Hard upper bound for GPU VRAM usage (bytes). 0 = auto-detect.
        size_t max_vram_bytes = 0;

        /// Hard upper bound for CPU spill memory (bytes). 0 = no spilling.
        size_t max_cpu_spill_bytes = 16ULL * 1024 * 1024 * 1024;  // 16 GB

        /// Fraction of max_vram_bytes that triggers OOM warning (0..1).
        float oom_threshold_fraction = 0.90f;

        /// Minimum free VRAM to preserve as reserve after allocation (bytes).
        size_t min_free_vram_reserve = 512ULL * 1024 * 1024;  // 512 MB

        /// Enable defragmentation as a recovery step.
        bool enable_defragmentation = true;

        /// Enable CPU spilling as a last-resort recovery step.
        bool enable_cpu_spilling = true;

        /// GPU device ID to use (-1 = auto-select).
        int gpu_device_id = -1;

        /// Block alignment in bytes (must be power-of-two, ≥ 256).
        size_t block_alignment = 4096;
    };

    // -----------------------------------------------------------------------
    // Allocation handle
    // -----------------------------------------------------------------------

    /**
     * @brief Opaque handle returned by allocate().
     *
     * Holds all metadata needed to address and free a VRAM (or spilled CPU)
     * allocation. The handle is movable but not copyable; the destructor does
     * NOT automatically free the memory — call ActiveVRAMAllocator::free().
     */
    struct AllocationHandle {
        /// Unique allocation ID (monotonically increasing).
        uint64_t id = 0;

        /// Logical owner (e.g. model name / layer tag).
        std::string owner_id;

        /// Requested size (bytes, before alignment rounding).
        size_t requested_bytes = 0;

        /// Actual size after block-alignment (bytes).
        size_t allocated_bytes = 0;

        /// Pointer to GPU memory (nullptr if spilled to CPU).
        void* gpu_ptr = nullptr;

        /// Pointer to CPU memory (non-null if allocation was spilled).
        void* cpu_ptr = nullptr;

        /// True when this allocation lives in CPU memory, not VRAM.
        bool is_spilled = false;

        /// True when the handle is valid and has not been freed.
        bool valid = false;

        /// True when this handle tracks externally-managed memory (e.g., llama.cpp model
        /// weights loaded by the inference runtime).  free() updates accounting stats but
        /// does NOT release any GPU or CPU memory — the external owner remains responsible.
        bool is_external = false;

        /// Timestamp of allocation (ms since epoch, for LRU eviction).
        int64_t allocated_at_ms = 0;

        /// Timestamp of last access (ms since epoch, for LRU eviction).
        int64_t last_used_at_ms = 0;
    };

    // -----------------------------------------------------------------------
    // Statistics & error reporting
    // -----------------------------------------------------------------------

    struct Stats {
        /// Total VRAM managed by this allocator (bytes).
        size_t total_vram_bytes = 0;

        /// VRAM currently allocated to live handles (bytes).
        size_t used_vram_bytes = 0;

        /// Free VRAM (total − used, bytes).
        size_t free_vram_bytes = 0;

        /// Peak VRAM usage since construction (bytes).
        size_t peak_vram_bytes = 0;

        /// VRAM wasted due to alignment padding (bytes).
        size_t wasted_padding_bytes = 0;

        /// CPU spill memory currently in use (bytes).
        size_t spilled_cpu_bytes = 0;

        /// Number of live allocations.
        size_t live_allocation_count = 0;

        /// Number of OOM events since construction.
        uint64_t oom_event_count = 0;

        /// Number of successful OOM recoveries.
        uint64_t oom_recovery_count = 0;

        /// Number of evictions performed.
        uint64_t eviction_count = 0;

        /// Number of defragmentation passes performed.
        uint64_t defrag_count = 0;

        /// Number of CPU-spill operations performed.
        uint64_t spill_count = 0;

        /// Estimated fragmentation as a percentage (0–100).
        float fragmentation_pct = 0.0f;

        /// True when VRAM usage exceeds oom_threshold_fraction.
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
        /// Bytes that triggered the OOM.
        size_t requested_bytes = 0;

        /// Strategy that was attempted.
        OOMRecoveryStrategy strategy = OOMRecoveryStrategy::Failed;

        /// True if recovery succeeded after this strategy.
        bool recovered = false;

        /// Bytes freed / recovered by this strategy.
        size_t bytes_recovered = 0;
    };

    /// Callback invoked on each OOM event (strategy + outcome).
    using OOMCallback = std::function<void(const OOMEvent&)>;

    // -----------------------------------------------------------------------
    // Construction
    // -----------------------------------------------------------------------

    /**
     * @brief Construct allocator with the given configuration.
     *
     * If cfg.gpu_device_id == -1 the least-loaded available GPU is selected.
     * If no GPU is available the allocator falls back to CPU simulation mode
     * (useful for CI / test environments without hardware).
     */
    explicit ActiveVRAMAllocator(const Config& cfg);

    /**
     * @brief Construct with default configuration.
     */
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

    /**
     * @brief Allocate `bytes` of VRAM for the given owner.
     *
     * On success returns a valid AllocationHandle. On failure (OOM) returns
     * an empty optional. Use handleOOM() + retry, or call allocateOrRecover().
     *
     * @param bytes        Number of bytes to allocate.
     * @param owner_id     Logical owner tag (model name, layer, etc.).
     * @param gpu_device_id Override GPU device (-1 = use allocator default).
     */
    std::optional<AllocationHandle> allocate(
        size_t bytes,
        const std::string& owner_id,
        int gpu_device_id = -1);

    /**
     * @brief Allocate VRAM, automatically triggering OOM recovery on failure.
     *
     * Equivalent to: allocate → on failure → handleOOM → retry.
     *
     * @return Valid handle on success, empty optional if recovery also fails.
     */
    std::optional<AllocationHandle> allocateOrRecover(
        size_t bytes,
        const std::string& owner_id,
        int gpu_device_id = -1);

    /**
     * @brief Free memory associated with the given handle.
     *
     * Updates the last-used timestamp before freeing (for LRU accounting).
     * Sets handle.valid = false on success.
     *
     * @return true if the memory was freed successfully.
     */
    bool free(AllocationHandle& handle);

    /**
     * @brief Touch an allocation (update last_used_at_ms).
     *
     * Call this whenever an allocation is actively used so LRU eviction
     * does not consider it a stale candidate.
     */
    void touch(AllocationHandle& handle);

    // -----------------------------------------------------------------------
    // OOM recovery API
    // -----------------------------------------------------------------------

    /**
     * @brief Attempt to recover from OOM for a future allocation of `need_bytes`.
     *
     * Tries recovery strategies in priority order:
     *   1. evictLRU()       — free the oldest unused allocation
     *   2. defragment()     — compact memory
     *   3. spillLRUToCPU()  — migrate an allocation to CPU memory
     *
     * @param need_bytes Bytes that need to be freed for the upcoming allocation.
     * @return true if at least `need_bytes` were recovered.
     */
    bool handleOOM(size_t need_bytes = 0);

    /**
     * @brief Evict the allocation with the oldest last_used_at_ms timestamp.
     *
     * @return Bytes freed on success, 0 on failure (no evictable allocations).
     */
    size_t evictLRU();

    /**
     * @brief Evict all allocations belonging to `owner_id`.
     *
     * @return Bytes freed.
     */
    size_t evictOwner(const std::string& owner_id);

    /**
     * @brief Defragment VRAM by consolidating free blocks.
     *
     * Delegates to the underlying GPUMemoryManager::defragment() and updates
     * internal fragmentation metrics.
     *
     * @return true if defragmentation ran and reduced fragmentation.
     */
    bool defragment();

    /**
     * @brief Spill the LRU allocation from VRAM to CPU memory.
     *
     * If cpu spilling is disabled or max_cpu_spill_bytes is exhausted,
     * returns false.
     *
     * @return Bytes moved to CPU on success, 0 on failure.
     */
    size_t spillLRUToCPU();

    /**
     * @brief Restore a previously spilled allocation back to VRAM.
     *
     * @return true if the allocation was successfully restored to GPU memory.
     */
    bool restoreFromCPU(AllocationHandle& handle);

    // -----------------------------------------------------------------------
    // Monitoring & introspection
    // -----------------------------------------------------------------------

    /// Current statistics snapshot.
    Stats getStats() const;

    /// True when VRAM usage exceeds the configured OOM threshold.
    bool isOOMThresholdExceeded() const;

    /// Register a callback invoked on every OOM event.
    void setOOMCallback(OOMCallback cb);

    /// List all live allocation handles (for introspection / debugging).
    std::vector<AllocationHandle> listAllocations() const;

    /// Return the configured GPU device ID (resolved, not -1).
    int gpuDeviceId() const noexcept;

    /// True when backed by a real GPU (false in CPU-simulation fallback mode).
    bool isGPUAvailable() const noexcept;

    // -----------------------------------------------------------------------
    // External-memory registration (for externally-managed GPU memory)
    // -----------------------------------------------------------------------

    /**
     * @brief Register externally-managed VRAM (e.g., a model loaded by llama.cpp).
     *
     * Does NOT allocate GPU or CPU memory — only updates accounting stats so that
     * VRAM pressure monitoring, OOM threshold detection, and `getStats()` reflect
     * the true system-wide VRAM usage.
     *
     * The returned handle must be passed to `free()` when the external owner
     * releases the memory; `free()` will update stats but skip any actual dealloc.
     *
     * Thread-safe.
     *
     * @param bytes    Bytes of VRAM managed externally.
     * @param owner_id Logical owner tag (e.g., model name / plugin id).
     * @return A valid AllocationHandle with `is_external = true`.
     */
    AllocationHandle registerExternal(size_t bytes, const std::string& owner_id);

    // -----------------------------------------------------------------------
    // Integration with AdaptiveVRAMAllocator (bridge API)
    // -----------------------------------------------------------------------

    /**
     * @brief Fragmentation-aware block allocation (replaces AdaptiveVRAMAllocator stub).
     *
     * Allocates memory aligned to block_alignment, using OOM recovery if needed.
     * Sets `*ptr` to the GPU (or CPU-spill) memory address on success.
     *
     * @param bytes Size to allocate.
     * @param ptr   Output — pointer to allocated memory.
     * @return true on success.
     */
    bool allocateWithFragmentation(size_t bytes, void** ptr);

    /**
     * @brief Handle out-of-memory (replaces AdaptiveVRAMAllocator stub).
     *
     * Implements the full eviction → defrag → CPU-spill recovery sequence.
     *
     * @return true if recovery freed enough memory for a subsequent allocation.
     */
    bool handleOutOfMemory();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace llm
} // namespace themis
