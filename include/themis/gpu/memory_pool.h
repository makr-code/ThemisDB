/**
 * @file memory_pool.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=5; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=2, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis {
namespace gpu {

/**
 * @brief Slab-based VRAM memory pool.
 *
 * Pre-divides a fixed VRAM budget into equal-sized slabs and serves
 * allocation requests from the pool.  When no slab is large enough to
 * satisfy a request the caller receives a "pool miss" (returns false) so
 * it can fall back to the on-demand allocator or CPU path.
 *
 * Design notes
 * ------------
 * - Slabs are logical records only (no real cudaMalloc).  The pool tracks
 *   VRAM budget at the bookkeeping level; real device allocation is gated
 *   behind THEMIS_ENABLE_CUDA / THEMIS_ENABLE_HIP.
 * - All methods are protected by a single internal mutex; this is
 *   intentionally simple — a production implementation would use lock-free
 *   free-lists per size class.
 * - Fragmentation is tracked as the fraction of wasted bytes across all
 *   live allocations.
 *
 * Thread safety: all public methods are protected by an internal mutex.
 */
class GPUMemoryPool {
public:
    // -----------------------------------------------------------------------
    // Slab descriptor
    // -----------------------------------------------------------------------
    struct Slab {
        uint64_t    offset       = 0;    ///< Byte offset within pool
        uint64_t    size         = 0;    ///< Slab size in bytes
        uint64_t    request_size = 0;    ///< Actual bytes requested (≤ size)
        bool        is_free      = true;
        std::string owner_tag;           ///< Caller-supplied tag when occupied
    };

    // -----------------------------------------------------------------------
    // Defragmentation result
    // -----------------------------------------------------------------------
    struct DefragResult {
        size_t   slabs_moved      = 0;    ///< Number of occupied slabs relocated
        uint64_t bytes_compacted  = 0;    ///< Total bytes in relocated slabs
        float    frag_before      = 0.0f; ///< Fragmentation ratio before defrag
        float    frag_after       = 0.0f; ///< Fragmentation ratio after defrag
        bool     ran              = false; ///< true if defrag ran (threshold met)
        size_t   data_move_errors = 0;    ///< CUDA/HIP device-copy failures (0 on success)
        /// Old-offset → new-offset map for each relocated slab.
        /// Callers that hold raw device pointers (base + old_offset) must
        /// remap them to (base + new_offset) after a successful defragment().
        std::unordered_map<uint64_t, uint64_t> offset_map;
    };

    // -----------------------------------------------------------------------
    // Statistics
    // -----------------------------------------------------------------------
    struct Stats {
        uint64_t total_bytes     = 0;  ///< Pool capacity
        uint64_t allocated_bytes = 0;  ///< Bytes currently occupied
        uint64_t free_bytes      = 0;  ///< Bytes available in free slabs
        uint64_t peak_bytes      = 0;  ///< High-water mark
        size_t   total_slabs     = 0;  ///< Total number of slabs
        size_t   free_slabs      = 0;  ///< Slabs not currently in use
        size_t   alloc_hits      = 0;  ///< Successful pool allocations
        size_t   alloc_misses    = 0;  ///< Requests that couldn't be served
        size_t   zeroed_slabs    = 0;  ///< Slabs zeroed on release (zero_on_free)
        float    fragmentation   = 0.0f; ///< 0.0–1.0 wasted / total
    };

    // -----------------------------------------------------------------------
    // Construction
    // -----------------------------------------------------------------------

    /**
     * @param total_bytes  Total VRAM budget managed by this pool.
     * @param slab_size    Size of each slab.  All allocations occupy exactly
     *                     one slab (first-fit).  Requests larger than
     *                     slab_size result in a pool miss.
     * @param num_slabs    Number of pre-allocated slabs.  If 0, computed as
     *                     total_bytes / slab_size.
     */
    GPUMemoryPool(uint64_t total_bytes, uint64_t slab_size,
                  size_t num_slabs = 0);

    // Disable copy.
    GPUMemoryPool(const GPUMemoryPool&) = delete;
    GPUMemoryPool& operator=(const GPUMemoryPool&) = delete;

    // -----------------------------------------------------------------------
    // Configuration
    // -----------------------------------------------------------------------

    /**
     * @brief When enabled, each slab is conceptually zeroed on release.
     *
     * In a real CUDA implementation this calls `cudaMemset(ptr, 0, slab_size)`
     * before returning the slab to the free list, preventing one tenant from
     * reading another tenant's data.  In the bookkeeping-only simulation the
     * flag increments a counter that is visible in Stats.
     */
    void setZeroOnFree(bool z) noexcept { zero_on_free_ = z; }
    bool getZeroOnFree()       const noexcept { return zero_on_free_; }

    // -----------------------------------------------------------------------
    // Allocation / deallocation
    // -----------------------------------------------------------------------

    /**
     * @brief Attempt to allocate @p size_bytes from the pool.
     *
     * @param size_bytes  Must be ≤ slab_size.  Larger requests are pool
     *                    misses.
     * @param tag         Caller-supplied owner label for diagnostics.
     * @param[out] offset Set to the byte offset within the pool when the
     *                    allocation succeeds.
     * @return true if a free slab was found and marked occupied.
     */
    bool tryAcquire(uint64_t size_bytes, const std::string& tag,
                    uint64_t& offset);

    /**
     * @brief Return a slab to the pool by offset.
     *
     * @param offset  The offset previously returned by tryAcquire().
     * @return true if the slab was found and freed.
     */
    bool release(uint64_t offset);

    // -----------------------------------------------------------------------
    // Queries
    // -----------------------------------------------------------------------
    Stats  getStats()    const;
    size_t slabSize()    const { return slab_size_; }
    size_t numSlabs()    const;
    size_t freeSlabs()   const;

    /**
     * @brief Fragmentation ratio: (allocated - useful) / total.
     *
     * Because each request occupies exactly one slab, fragmentation equals
     * the fraction of allocated slab space that exceeds the actual request
     * size. This is always 0 when slab_size == request_size.
     */
    float fragmentation() const;

    /**
     * @brief Return a read-only snapshot of the slab table (for diagnostics).
     */
    std::vector<Slab> slabSnapshot() const;

    /**
     * @brief Defragment the pool by compacting occupied slabs toward offset 0.
     *
     * Occupied slabs are reassigned contiguous offsets starting at 0 and
     * the internal fragmentation counters are recalculated from the stored
     * per-slab request sizes.  Free slabs are assigned the remaining offsets.
     *
     * When a non-zero device base pointer has been set via setDeviceBasePtr(),
     * physical VRAM data is moved via cudaMemcpy (THEMIS_ENABLE_CUDA) or
     * hipMemcpy (THEMIS_ENABLE_HIP) to keep the device memory consistent with
     * the updated logical offsets.  In the CPU bookkeeping simulation (no
     * device pointer set) it performs logical compaction only.
     *
     * @param threshold  Only run if the current fragmentation ratio exceeds
     *                   this value (0.0–1.0).  Pass 0.0 to always compact.
     * @return DefragResult with before/after metrics, a moved-slab count, and
     *         an offset_map (old_offset → new_offset) for each relocated slab
     *         so callers can update any raw device pointers they hold.
     */
    DefragResult defragment(float threshold = 0.05f);

    // -----------------------------------------------------------------------
    // Device memory binding (optional)
    // -----------------------------------------------------------------------

    /**
     * @brief Bind a real device base pointer to the pool.
     *
     * When set (non-zero), defragment() will use cudaMemcpy / hipMemcpy to
     * physically move slab data in device memory.  The pool does not own this
     * pointer; the caller is responsible for its lifetime.
     */
    void     setDeviceBasePtr(uintptr_t ptr) noexcept { device_base_ptr_ = ptr; }
    uintptr_t getDeviceBasePtr()             const noexcept { return device_base_ptr_; }

private:
    friend class SlabStateGuard;

    uint64_t            total_bytes_;
    uint64_t            slab_size_;
    std::vector<Slab>   slabs_;
    mutable std::mutex  mutex_;
    uint64_t            allocated_bytes_ = 0;
    uint64_t            peak_bytes_      = 0;
    uint64_t            wasted_bytes_    = 0;  ///< sum of (slab_size - request) per live slab
    size_t              alloc_hits_      = 0;
    size_t              alloc_misses_    = 0;
    size_t              zeroed_slabs_    = 0;
    bool                zero_on_free_    = false;
    uintptr_t           device_base_ptr_ = 0;  ///< Optional: real VRAM base pointer
};

} // namespace gpu
} // namespace themis
