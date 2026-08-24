/**
 * @file graph_cache.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=1, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>

namespace themis {
namespace gpu {

// ============================================================================
// QueryShape — identifies a recurring GPU query execution pattern.
//
// Queries that share the same OpType, row count, and parameter hash are
// considered structurally identical and can share a captured CUDA graph.
// ============================================================================

struct QueryShape {
    enum class OpType : uint8_t {
        SCAN       = 0,
        SORT       = 1,
        AGGREGATE  = 2,
        JOIN       = 3,
        ANN_SEARCH = 4,  ///< Approximate nearest-neighbor vector similarity search
        TOPK       = 5,  ///< Partial sort — return top-k rows by key (GPU: thrust::partial_sort)
    };

    OpType   op         = OpType::SCAN;
    size_t   row_count  = 0;    ///< Primary (or total) row count
    uint64_t param_hash = 0;    ///< FNV-1a hash of op-specific parameters

    bool operator==(const QueryShape& o) const noexcept {
        return op == o.op && row_count == o.row_count && param_hash == o.param_hash;
    }
};

/// FNV-1a–inspired hash for QueryShape.
struct QueryShapeHash {
    size_t operator()(const QueryShape& s) const noexcept {
        // FNV-1a 64-bit basis and prime
        constexpr uint64_t kBasis = 14695981039346656037ULL;
        constexpr uint64_t kPrime = 1099511628211ULL;
        uint64_t h = kBasis;
        h ^= static_cast<uint64_t>(s.op);
        h *= kPrime;
        h ^= static_cast<uint64_t>(s.row_count);
        h *= kPrime;
        h ^= s.param_hash;
        h *= kPrime;
        return static_cast<size_t>(h);
    }
};

// ============================================================================
// GraphEntry — one captured execution graph for a specific QueryShape.
//
// In a production CUDA build this struct would own a cudaGraph_t and a
// cudaGraphExec_t.  In this CPU-simulation build it holds only the
// bookkeeping counters needed for cache hit/miss tracking and LRU eviction.
// ============================================================================

struct GraphEntry {
    QueryShape shape;

    uint64_t capture_count = 0;   ///< How many times this shape was captured
    uint64_t replay_count  = 0;   ///< How many times it was served from cache
    uint64_t last_access   = 0;   ///< Monotonic counter for LRU eviction

    // Production CUDA members (populated when THEMIS_ENABLE_CUDA is defined):
    //   cudaGraph_t     graph = nullptr;
    //   cudaGraphExec_t exec  = nullptr;
    //   void*           d_input_buf  = nullptr;
    //   void*           d_output_buf = nullptr;
};

// ============================================================================
// GPUGraphCache — CUDA graph capture cache for recurring query execution
// patterns.
//
// On the first call for a given QueryShape the cache records ("captures") the
// shape.  Subsequent lookups for the same shape return the existing entry and
// increment its replay counter.  When the number of entries exceeds
// kMaxEntries the least-recently-used entry is evicted.
//
// Thread safety: all public methods are mutex-protected.
// ============================================================================

/** @brief Thread safety: all public methods are mutex-protected. */
class GPUGraphCache {
public:
    /// Maximum number of captured graphs kept in the cache.
    static constexpr size_t kMaxEntries = 32;

    // -----------------------------------------------------------------------
    // Statistics
    // -----------------------------------------------------------------------
    struct Stats {
        size_t entries   = 0;   ///< Current number of cached entries
        size_t hits      = 0;   ///< Lookups that found a cached graph
        size_t misses    = 0;   ///< Lookups that found no cached graph
        size_t evictions = 0;   ///< Number of LRU evictions performed
    };

    // -----------------------------------------------------------------------
    // Cache operations
    // -----------------------------------------------------------------------

    /**
     * @brief Look up a shape in the cache.
     *
     * On a hit, increments the entry's replay counter and last_access stamp,
     * then returns a pointer to the entry (valid until the next mutating call
     * on this cache object).  Returns nullptr on a miss.
     */
    const GraphEntry* lookup(const QueryShape& shape);

    /**
     * @brief Capture (record) a new graph entry for @p shape.
     *
     * If an entry for @p shape already exists its capture_count is
     * incremented (idempotent).  Otherwise a new entry is inserted,
     * evicting the LRU entry first if the cache is full.
     */
    void capture(const QueryShape& shape);

    /**
     * @brief Remove the entry for @p shape, if present.
     *
     * In a production CUDA build this would also call cudaGraphExecDestroy /
     * cudaGraphDestroy to release device resources.
     */
    void invalidate(const QueryShape& shape);

    /**
     * @brief Remove all entries from the cache.
     */
    void clear();

    size_t size()     const;
    Stats  getStats() const;

private:
    void evictLRU();  ///< Remove the least-recently-used entry (O(n), n ≤ 32)

    mutable std::mutex mutex_;
    std::unordered_map<QueryShape, GraphEntry, QueryShapeHash> entries_;
    uint64_t access_counter_ = 0;
    Stats    stats_;
};

} // namespace gpu
} // namespace themis
