/**
 * @file federated_summaries.h
 * @brief Federated and cross-shard tensor summaries.
 *
 * Aggregates tensor summaries distributed across multiple shards into a
 * unified representation usable by the local tensor mid-layer and the
 * hybrid query planner.
 *
 * Planned in: docs/EPIC1_FEDERATED_SUMMARIES.md
 * Sub-issue:   #5427
 */

#pragma once

#include "tensor_midlayer.h"

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis::retrieval {

/// Summary-first retrieval mode for federated queries.
enum class FederatedMode {
    LocalFirst,      ///< Use local summaries; fan-out only on cache miss
    BroadcastAll,    ///< Request summaries from all shards in parallel
    TopKShards,      ///< Fan-out to the k shards with highest estimated relevance
};

/// A merged summary record combining data from multiple shards.
struct FederatedSummary {
    std::uint64_t            global_id;    ///< Cross-shard canonical ID
    std::vector<std::string> shard_keys;   ///< Contributing shards
    std::vector<std::uint8_t> merged_payload; ///< Merged compressed vector
    float                    merge_confidence = 1.0f;
};

/// Request descriptor for a federated summary lookup.
struct FederatedQuery {
    std::vector<float> embedding;
    std::uint32_t      top_k = 10;
    FederatedMode      mode = FederatedMode::LocalFirst;
    std::uint32_t      fanout_limit = 8;         ///< Max shards to contact
    std::vector<std::string> shard_allowlist;     ///< Empty = all shards
};

/// Result of a federated summary lookup.
struct FederatedResult {
    std::vector<FederatedSummary> summaries;
    std::unordered_map<std::string, bool> shard_availability;
    double latency_ms = 0.0;
    std::uint32_t shards_contacted = 0;
};

/// Configuration for the federated summary layer.
struct FederatedSummariesConfig {
    FederatedMode  default_mode = FederatedMode::LocalFirst;
    std::uint32_t  default_fanout = 4;
    std::chrono::milliseconds per_shard_timeout{200};
    bool           deduplicate = true; ///< Merge identical global IDs
    std::string    local_shard_key;
};

/**
 * @brief Federated Summaries interface.
 *
 * Manages cross-shard tensor summary aggregation for summary-first retrieval.
 */
class IFederatedSummaries {
public:
    virtual ~IFederatedSummaries() = default;

    /// Execute a federated summary lookup.
    virtual FederatedResult query(const FederatedQuery& fq) = 0;

    /// Register a shard endpoint for future fan-out requests.
    virtual void registerShard(const std::string& key,
                                const std::string& endpoint) = 0;

    /// Remove a shard from the registry.
    virtual void deregisterShard(const std::string& key) = 0;

    /// Return availability of all registered shards.
    virtual std::unordered_map<std::string, bool> shardHealth() const = 0;
};

/// Factory: create a FederatedSummaries layer from configuration.
std::unique_ptr<IFederatedSummaries> makeFederatedSummaries(
    const FederatedSummariesConfig& cfg);

} // namespace themis::retrieval
