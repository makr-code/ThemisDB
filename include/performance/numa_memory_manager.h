/**
 * @file numa_memory_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2026 ThemisDB
// Licensed under MIT License

#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis {
namespace performance {

// ============================================================================
// NUMAMemoryManager — Issue #228 (v1.9.0)
// ============================================================================

/**
 * @brief System NUMA topology descriptor used by NUMAMemoryManager.
 */
struct NUMATopologyInfo {
    /// Number of NUMA nodes detected on this machine.
    size_t num_nodes = 1;
    /// Per-node total memory in MiB.
    std::vector<size_t> node_memory_mb;
    /// Symmetric latency matrix: node_distances[i][j] = relative cost i->j.
    std::vector<std::vector<size_t>> node_distances;
};

/**
 * @brief Hint supplied by the caller to guide NUMA allocation decisions.
 */
struct AllocationHint {
    /// Preferred NUMA node (-1 = auto-detect from calling thread).
    int preferred_node = -1;
    /// Allow the kernel to migrate this allocation to a better node later.
    bool allow_migration = true;
    /// Request huge-page backing (transparent huge-pages on Linux).
    bool use_huge_pages = false;
};

/**
 * @brief Cumulative statistics maintained by NUMAMemoryManager.
 */
struct NUMAStats {
    /// Total allocations that resolved to the thread's local node.
    uint64_t local_accesses = 0;
    /// Total allocations that resolved to a remote node.
    uint64_t remote_accesses = 0;
    /// local_accesses / (local + remote), in [0.0, 1.0].
    double locality_ratio = 1.0;
    /// Bytes allocated per node (indexed by node id).
    std::vector<uint64_t> per_node_allocations;
};

/**
 * @brief NUMA-aware memory allocator (v1.9.0, Issue #228).
 *
 * Optimises memory allocation and data placement for NUMA architectures by:
 *  - Detecting the system NUMA topology on construction.
 *  - Allocating memory on the calling thread's local node when possible.
 *  - Supporting explicit node placement for performance-critical structures.
 *  - Providing a migration helper to move allocations to a preferred node.
 *  - Falling back transparently to standard malloc/free when NUMA primitives
 *    are unavailable (containers, non-Linux).
 *
 * Research basis: "NUMA-aware Memory Management" (ASPLOS'15).
 *
 * Performance targets:
 *  - Local access ratio  > 90 %
 *  - Remote access penalty: -60 % vs unoptimised allocation
 *  - Throughput: +30-80 % on NUMA systems vs single-node allocation
 *
 * Thread safety: all public methods are thread-safe.
 */
class NUMAMemoryManager {
public:
    // =========================================================================
    // Construction
    // =========================================================================

    NUMAMemoryManager();
    ~NUMAMemoryManager();

    NUMAMemoryManager(const NUMAMemoryManager&)            = delete;
    NUMAMemoryManager& operator=(const NUMAMemoryManager&) = delete;
    NUMAMemoryManager(NUMAMemoryManager&&)                 = default;
    NUMAMemoryManager& operator=(NUMAMemoryManager&&)      = default;

    // =========================================================================
    // Core allocation API
    // =========================================================================

    /**
     * @brief Allocate size bytes on the specified NUMA node.
     *
     * When node is negative, behaves identically to allocate_local().
     * Falls back to malloc on systems without NUMA support.
     *
     * @return Pointer to allocated memory; throws std::bad_alloc on exhaustion.
     */
    void* allocate_on_node(size_t size, int node);

    /**
     * @brief Allocate size bytes on the calling thread's local NUMA node.
     */
    void* allocate_local(size_t size);

    /**
     * @brief Allocate memory with an explicit AllocationHint.
     */
    void* allocate(size_t size, const AllocationHint& hint = {});

    /**
     * @brief Free a pointer previously returned by this manager.
     *
     * size is used to update per-node accounting; passing 0 is safe.
     */
    void deallocate(void* ptr, size_t size = 0) noexcept;

    // =========================================================================
    // Data migration
    // =========================================================================

    /**
     * @brief Advisory: migrate an allocation to a different NUMA node.
     *
     * On Linux with libnuma available this issues mbind(MPOL_BIND) to rebind
     * the pages to target_node.  On other platforms it is a no-op.
     */
    void migrate_to_node(void* ptr, size_t size, int target_node);

    // =========================================================================
    // Topology queries
    // =========================================================================

    /**
     * @brief Return the NUMA node the calling thread currently runs on.
     *
     * Returns 0 when NUMA topology is unavailable.
     */
    int get_current_node() const noexcept;

    /**
     * @brief Return the detected system NUMA topology.
     */
    NUMATopologyInfo get_topology() const;

    /**
     * @brief Return true when more than one NUMA node is available.
     */
    bool is_numa_available() const noexcept;

    // =========================================================================
    // Statistics
    // =========================================================================

    /** @brief Return a snapshot of current allocation statistics. */
    NUMAStats get_stats() const;

    /** @brief Reset all statistics counters to zero. */
    void reset_stats();

private:
    // Internal helpers
    void* do_allocate(size_t size, int node, bool use_huge_pages);
    void  update_alloc_stats(int node, size_t size);
    int   resolve_node(int hint_node) const noexcept;

    NUMATopologyInfo topology_;

    // F-006: Replace stats_mutex_ with lock-free atomics on the hot allocation
    // path.  stats_mutex_ is removed.  getStats() reads the atomics and
    // assembles a NUMAStats snapshot without any lock.
    std::atomic<uint64_t> stat_local_{0};
    std::atomic<uint64_t> stat_remote_{0};
    // Per-node byte counts.  Size = topology_.num_nodes, allocated in the ctor.
    // std::atomic is not movable, so we use a unique_ptr to a raw array.
    size_t                            num_nodes_{0};
    std::unique_ptr<std::atomic<int64_t>[]> per_node_bytes_;

    // Per-allocation tracking: map raw pointer -> (node, size).
    // Protected by bucket mutexes.
    mutable std::mutex alloc_mutex_;
    struct AllocEntry { int node; size_t size; };
    // Use a simple flat array of buckets to avoid heavy dependencies.
    // The map is keyed by (ptr >> 3) % kBuckets for O(1) average lookup.
    static constexpr size_t kBuckets = 1024;
    struct Bucket {
        std::mutex                          mtx;
        std::unordered_map<void*, AllocEntry> entries;
    };
    Bucket buckets_[kBuckets];

    // Lookup/insert/erase helpers
    void   track_alloc(void* ptr, int node, size_t size);
    bool   untrack_alloc(void* ptr, int* out_node, size_t* out_size) noexcept;
};

}  // namespace performance
}  // namespace themis
