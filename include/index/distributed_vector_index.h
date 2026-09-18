/**
 * @file distributed_vector_index.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

enum class ShardingStrategy {
    HASH,            ///< shard = hash(pk) % num_shards
    RANGE,           ///< Hash-based bucket assignment (alias for HASH).
    CONSISTENT_HASH  ///< consistent-hashing ring for minimal rehashing
};

struct DistributedVectorIndexConfig {
    size_t           num_shards         = 4;    ///< Number of shards to create
    ShardingStrategy strategy           = ShardingStrategy::CONSISTENT_HASH;
    size_t           virtual_nodes      = 150;  ///< Virtual nodes per shard (consistent hash)
    size_t           replication_factor = 1;    ///< Future: replicate vectors across N shards
};

struct DistributedShardStats {
    size_t shard_index = 0;   ///< Zero-based shard index
    size_t vector_count = 0;  ///< Number of vectors in this shard
};

struct DistributedVectorIndexStats {
    size_t total_vectors    = 0;
    size_t num_shards       = 0;
    size_t max_shard_size   = 0;
    size_t min_shard_size   = 0;
    double load_imbalance   = 0.0; ///< (max - min) / mean; 0 = perfectly balanced
};

class DistributedVectorIndex {
public:
    explicit DistributedVectorIndex(const DistributedVectorIndexConfig& config = {});

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

    [[nodiscard]] bool insert(const std::string& primary_key, const float* vector, size_t dim);

    [[nodiscard]] bool insert(const std::string& primary_key, const std::vector<float>& vector);

    [[nodiscard]] bool remove(const std::string& primary_key);

    // -------------------------------------------------------------------------
    // Query – scatter-gather KNN
    // -------------------------------------------------------------------------

    /**
     * @brief Search.
     * @param[in] query Input parameter.
     * @param[in] dim Input parameter.
     * @param[in] k Input parameter.
     * @return Return value.
     */
    std::vector<AnnSearchResult> search(const float* query, size_t dim, int k) const;

    /**
     * @brief Search.
     * @param[in] query Input parameter.
     * @param[in] k Input parameter.
     * @return Return value.
     */
    std::vector<AnnSearchResult> search(const std::vector<float>& query, int k) const;

    // -------------------------------------------------------------------------
    // Introspection
    // -------------------------------------------------------------------------

    /**
     * @brief Size.
     * @return Return value.
     */
    size_t size() const;

    /**
     * @brief Num Shards.
     * @return Return value.
     */
    size_t numShards() const;

    /**
     * @brief Get Shard Stats.
     * @return Return value.
     */
    std::vector<DistributedShardStats> getShardStats() const;

    /**
     * @brief Get Stats.
     * @return Return value.
     */
    DistributedVectorIndexStats getStats() const;

    /**
     * @brief Shard For.
     * @param[in] primary_key Input parameter.
     * @return Return value.
     */
    size_t shardFor(const std::string& primary_key) const;

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

    /**
     * @brief Build Ring.
     */
    void buildRing_();
    /**
     * @brief Hash Key.
     * @param[in] key Input parameter.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    uint64_t hashKey_(const std::string& key) const noexcept;
    /**
     * @brief Shard For.
     * @param[in] key Input parameter.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    size_t shardFor_(const std::string& key) const noexcept;
    /**
     * @brief Parse Global Id From Key.
     * @param[in] key Input parameter.
     * @return Return value.
     */
    static std::optional<int64_t> parseGlobalIdFromKey_(const std::string& key);
};

} // namespace index
} // namespace themis
