/**
 * @file distributed_vector_index.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

// Distributed Vector Index across Shards (Issue #1879)
//
// Provides scatter-gather KNN search across multiple independent shard indexes.
// Vectors are routed to shards using configurable partitioning strategies:
//   - HASH         : shard = std::hash(primary_key) % num_shards
//   - RANGE        : shards are assigned contiguous key-range buckets
//   - CONSISTENT_HASH : minimal-movement rehashing ring (default 150 vnodes/shard)
//
// Each shard owns an IAnnIndex (default: ScaNN).  Insert routes the vector to its
// shard; search fans out to every shard, collects partial top-k lists and merges
// them into a globally sorted top-k result.
//
// References:
//   FUTURE_ENHANCEMENTS.md – "Distributed Index Partitioning" (v1.7.0)
//   ROADMAP.md – Phase 3, "Distributed vector index across shards"

#include "index/ann_index.h"

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <optional>

namespace themis {
namespace index {

/// Partitioning strategy for distributing vectors across shards.
enum class ShardingStrategy {
    HASH,            ///< shard = hash(pk) % num_shards
    RANGE,           ///< Hash-based bucket assignment (alias for HASH).
                     ///  Proper lexicographic range partitioning requires
                     ///  pre-defined boundary keys and is out-of-scope for
                     ///  this initial implementation.
    CONSISTENT_HASH  ///< consistent-hashing ring for minimal rehashing
};

/// Configuration for DistributedVectorIndex.
struct DistributedVectorIndexConfig {
    size_t           num_shards         = 4;    ///< Number of shards to create
    ShardingStrategy strategy           = ShardingStrategy::CONSISTENT_HASH;
    size_t           virtual_nodes      = 150;  ///< Virtual nodes per shard (consistent hash)
    size_t           replication_factor = 1;    ///< Future: replicate vectors across N shards
};

/// Per-shard statistics returned by getShardStats().
struct DistributedShardStats {
    size_t shard_index = 0;   ///< Zero-based shard index
    size_t vector_count = 0;  ///< Number of vectors in this shard
};

/// Aggregated statistics over all shards.
struct DistributedVectorIndexStats {
    size_t total_vectors    = 0;
    size_t num_shards       = 0;
    size_t max_shard_size   = 0;
    size_t min_shard_size   = 0;
    double load_imbalance   = 0.0; ///< (max - min) / mean; 0 = perfectly balanced
};

/// Distributed vector index that partitions an embedding space across multiple
/// independent IAnnIndex shards using scatter-gather KNN queries.
///
/// Thread-safety: individual methods are protected by a single mutex.  For
/// high-throughput workloads, consider using a striped lock or lock-free
/// strategies (out of scope for this initial implementation).
class DistributedVectorIndex {
public:
    /// Construct with an explicit config; shards are created automatically
    /// using new ScaNN(ScaNNConfig{}) instances.
    explicit DistributedVectorIndex(const DistributedVectorIndexConfig& config = {});

    /// Construct with pre-built shard indexes.  The caller transfers ownership.
    /// @param shards  Exactly config.num_shards IAnnIndex instances.
    DistributedVectorIndex(const DistributedVectorIndexConfig& config,
                           std::vector<std::unique_ptr<IAnnIndex>> shards);

    ~DistributedVectorIndex() = default;

    // Non-copyable, movable.
    DistributedVectorIndex(const DistributedVectorIndex&) = delete;
    DistributedVectorIndex& operator=(const DistributedVectorIndex&) = delete;
    DistributedVectorIndex(DistributedVectorIndex&&) noexcept;
    DistributedVectorIndex& operator=(DistributedVectorIndex&&) noexcept;

    // -------------------------------------------------------------------------
    // Mutation
    // -------------------------------------------------------------------------

    /// Insert or update a vector identified by @p primary_key.
    /// If the key already exists on a shard, the old entry is replaced.
    /// @param primary_key  Unique string key for this vector.
    /// @param vector       Pointer to @p dim floats.
    /// @param dim          Dimensionality of the vector.
    /// @return true on success.
    [[nodiscard]] bool insert(const std::string& primary_key, const float* vector, size_t dim);

    /// Convenience overload accepting std::vector<float>.
    [[nodiscard]] bool insert(const std::string& primary_key, const std::vector<float>& vector);

    /// Remove the vector identified by @p primary_key from its shard.
    /// No-op (returns false) when the key is unknown.
    [[nodiscard]] bool remove(const std::string& primary_key);

    // -------------------------------------------------------------------------
    // Query – scatter-gather KNN
    // -------------------------------------------------------------------------

    /// Search for the @p k nearest neighbours of @p query across ALL shards.
    ///
    /// Implementation:
    ///   1. Scatter: query every shard for up to @p k candidates.
    ///   2. Gather:  collect all partial results.
    ///   3. Merge:   globally sort by distance and return top @p k.
    ///
    /// @param query  Pointer to @p dim floats.
    /// @param dim    Dimensionality of the query vector.
    /// @param k      Number of nearest neighbours to return.
    /// @return Sorted (closest first) list of AnnSearchResult; may be shorter
    ///         than @p k when fewer vectors are indexed.
    std::vector<AnnSearchResult> search(const float* query, size_t dim, int k) const;

    /// Convenience overload accepting std::vector<float>.
    std::vector<AnnSearchResult> search(const std::vector<float>& query, int k) const;

    // -------------------------------------------------------------------------
    // Introspection
    // -------------------------------------------------------------------------

    /// Total number of vectors across all shards.
    size_t size() const;

    /// Number of shards.
    size_t numShards() const;

    /// Per-shard statistics.
    std::vector<DistributedShardStats> getShardStats() const;

    /// Aggregated statistics.
    DistributedVectorIndexStats getStats() const;

    /// Resolve which shard index owns @p primary_key (deterministic).
    size_t shardFor(const std::string& primary_key) const;

    /// Current configuration.
    const DistributedVectorIndexConfig& config() const noexcept { return config_; }

private:
    DistributedVectorIndexConfig config_;

    // Shard index instances
    std::vector<std::unique_ptr<IAnnIndex>> shards_;

    // Maps primary_key → (shard_index, internal_id) for routing
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::pair<size_t, int64_t>> pk_to_shard_;
    std::unordered_map<std::string, int64_t> pk_to_global_id_;
    std::unordered_map<int64_t, uint64_t> global_versions_;

    // Per-shard mapping: local ANN ID -> stable global ID returned by search().
    std::vector<std::unordered_map<int64_t, int64_t>> local_to_global_id_;
    std::vector<std::unordered_map<int64_t, uint64_t>> local_to_global_version_;

    // Per-shard next-ID counters (monotonically increasing; IDs are never reused)
    std::vector<int64_t> next_id_;
    int64_t next_global_id_ = 0;

    // Per-shard sets of currently-alive vector IDs.
    // Maintained in sync with pk_to_shard_: an ID is alive iff it maps to a
    // live pk entry.  Used in search() to filter out ghost entries left in
    // ScaNN after remove() (ScaNN has no removal primitive).
    std::vector<std::unordered_set<int64_t>> alive_ids_;

    // Consistent-hash ring state (used when strategy == CONSISTENT_HASH)
    std::map<uint64_t, size_t> ring_; ///< token → shard_index

    void buildRing_();
    uint64_t hashKey_(const std::string& key) const noexcept;
    size_t shardFor_(const std::string& key) const noexcept;
    static std::optional<int64_t> parseGlobalIdFromKey_(const std::string& key);
};

} // namespace index
} // namespace themis
