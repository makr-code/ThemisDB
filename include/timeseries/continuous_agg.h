/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            continuous_agg.h                                   ║
  Version:         0.0.34                                             ║
  Last Modified:   2026-03-09 03:55:55                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     178                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#ifndef THEMIS_CONTINUOUS_AGG_H
#define THEMIS_CONTINUOUS_AGG_H

#include <string>
#include <chrono>
#include <optional>
#include <vector>
#include <functional>
#include <memory>
#include <limits>

namespace themis {

class TSStore;

struct AggWindow {
    std::chrono::milliseconds size{std::chrono::minutes(1)};
};

enum class AggFunc { Min, Max, Avg, Sum, Count };

struct AggConfig {
    std::string metric;
    std::optional<std::string> entity; // nullopt = for all entities (not supported in MVP)
    AggWindow window;
    // For MVP we always compute min/max/avg/sum/count and store avg as value with metadata for others
};

/**
 * Rollup hierarchy definition.
 * Each level aggregates the previous level's output.
 * E.g.: {1m, 5m, 1h, 1d} means:
 *   raw → 1m aggregates → 5m aggregates → 1h aggregates → 1d aggregates
 */
struct RollupHierarchy {
    std::string metric;
    std::optional<std::string> entity;
    std::vector<std::chrono::milliseconds> levels;  // ordered from smallest to largest

    // Default hierarchy: 1m → 5m → 1h → 1d
    static RollupHierarchy defaultHierarchy(const std::string& metric,
                                             const std::optional<std::string>& entity = std::nullopt);
};

// ============================================================
// Multi-Shard Aggregation
// ============================================================

/**
 * @brief Result from one shard's aggregate computation.
 *
 * When aggregating across multiple shards/nodes, each shard
 * computes a partial AggShardResult; the coordinator merges
 * all partial results with mergeShardResults().
 */
struct AggShardResult {
    std::string metric;
    std::string entity;
    int64_t from_ms{0};
    int64_t to_ms{0};
    double sum{0.0};
    double min{std::numeric_limits<double>::max()};
    double max{std::numeric_limits<double>::lowest()};
    size_t count{0};
    bool valid{false};   ///< false = shard had no data or returned an error

    double avg() const { return count > 0 ? sum / count : 0.0; }
};

/**
 * @brief Merge multiple partial shard results into one global result.
 *
 * The coordinator calls this after collecting all shards' AggShardResults.
 * Returns a single AggShardResult that reflects the global aggregate.
 */
AggShardResult mergeShardResults(const std::vector<AggShardResult>& shards);

/**
 * @brief Distributed / multi-shard aggregation coordinator.
 *
 * In single-node mode this is a thin wrapper around ContinuousAggregateManager.
 * In multi-shard mode the caller provides a shard query callback; the
 * coordinator fans out the refresh request to all shards in parallel and
 * merges the partial results.
 */
class DistributedAggregateCoordinator {
public:
    /**
     * Callback type for querying a single shard.
     * The coordinator calls this for each registered shard.
     * @param shard_id   Numeric shard identifier (0-based)
     * @param cfg        Aggregate configuration to compute
     * @param from_ms    Start of the refresh window
     * @param to_ms      End of the refresh window
     * @return           Partial AggShardResult for the shard
     */
    using ShardQueryFn = std::function<AggShardResult(
        int shard_id,
        const AggConfig& cfg,
        int64_t from_ms,
        int64_t to_ms)>;

    /**
     * @param local_store    Local TSStore (used when shard_query is nullptr)
     * @param shard_count    Number of shards (1 = single-node)
     * @param shard_query    Callback to query a remote shard; pass nullptr for
     *                       single-node mode (local_store is used directly)
     */
    explicit DistributedAggregateCoordinator(
        TSStore* local_store,
        int shard_count = 1,
        ShardQueryFn shard_query = nullptr);

    /**
     * Compute and persist aggregates across all shards.
     * In single-node mode delegates to ContinuousAggregateManager.
     * In multi-shard mode fans out to all shards and merges.
     *
     * @return Merged aggregate result
     */
    AggShardResult refreshAggregate(
        const AggConfig& cfg,
        int64_t from_ms,
        int64_t to_ms);

    int shardCount() const { return shard_count_; }

private:
    TSStore* local_store_;
    int shard_count_;
    ShardQueryFn shard_query_;
};

// ============================================================
// Watermark Store for Continuous Aggregates
// ============================================================

/**
 * @brief Persistent watermark store for continuous aggregates.
 *
 * Each aggregate has an associated watermark — the upper boundary of the
 * time range that has already been aggregated. This store persists watermarks
 * in the TSStore's underlying RocksDB instance under the "wm:cagg:" key
 * prefix so they survive node restarts (WAL-durable).
 *
 * Thread safety: individual get/set operations are individually atomic via
 * RocksDB's single-key Put/Get, but callers must serialize concurrent
 * refreshes for the same aggregate_id.
 */
class ContinuousAggWatermarkStore {
public:
    explicit ContinuousAggWatermarkStore(TSStore* store) : store_(store) {}

    /**
     * @brief Read the current watermark for @p agg_id.
     * @return Milliseconds-since-epoch of the last successfully processed
     *         upper boundary, or 0 if no watermark has been set yet.
     */
    int64_t getWatermark(const std::string& agg_id) const;

    /**
     * @brief Persist the watermark to @p watermark_ms for @p agg_id.
     *
     * This write goes through RocksDB's WAL, so it survives node restarts.
     */
    void setWatermark(const std::string& agg_id, int64_t watermark_ms);

    /**
     * @brief Remove the watermark entry (e.g., when an aggregate is deleted).
     */
    void deleteWatermark(const std::string& agg_id);

private:
    TSStore* store_;
    static constexpr const char* WM_KEY_PREFIX = "wm:cagg:";
};

class ContinuousAggregateManager {
public:
    explicit ContinuousAggregateManager(TSStore* store) : store_(store) {}

    // Compute aggregates for [from,to] and store as derived metric
    // Derived metric name: metric + "__agg_" + window_ms
    void refresh(const AggConfig& cfg, int64_t from_ms, int64_t to_ms);

    /**
     * @brief Incremental refresh using watermark pushdown.
     *
     * Reads the current watermark for @p agg_id from @p wm_store, scans
     * only the range [watermark, to_ms) in TSStore (skipping already-processed
     * data), writes the aggregate points, and advances the watermark
     * atomically to @p to_ms after a successful write.
     *
     * If no watermark exists yet the full range [0, to_ms) is processed,
     * which provides a correct initial catch-up.
     *
     * @param cfg      Aggregate configuration (metric, entity, window).
     * @param agg_id   Unique aggregate identifier used as the watermark key.
     * @param to_ms    Upper bound of the refresh window (ms since epoch).
     * @param wm_store Watermark store for reading/writing the per-aggregate
     *                 watermark.
     * @return Number of aggregate windows written.
     */
    size_t refreshIncremental(const AggConfig& cfg,
                               const std::string& agg_id,
                               int64_t to_ms,
                               ContinuousAggWatermarkStore& wm_store);

    /**
     * Refresh all levels of a rollup hierarchy.
     * Processes levels from smallest window to largest.
     * Each level reads from the previous level's output (or raw data for the first level).
     * @param hierarchy   Rollup level definitions
     * @param from_ms     Start of refresh window (milliseconds)
     * @param to_ms       End of refresh window (milliseconds)
     */
    void refreshHierarchy(const RollupHierarchy& hierarchy, int64_t from_ms, int64_t to_ms);

    static std::string derivedMetricName(const std::string& base, std::chrono::milliseconds win);

private:
    TSStore* store_;
};

} // namespace themis

#endif // THEMIS_CONTINUOUS_AGG_H
