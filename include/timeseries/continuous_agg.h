/**
 * @file continuous_agg.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <chrono>
#include <optional>
#include <vector>
#include <functional>
#include <memory>
#include <limits>
#include <unordered_map>
#include "timeseries/tsstore.h"

namespace themis {

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

/** @brief Continuous aggregate manager component. */
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

// ============================================================================
// ContinuousAggDefinition — named TimescaleDB-style continuous aggregate
// ============================================================================

/**
 * @brief Status of a registered continuous aggregate.
 */
enum class ContinuousAggStatus {
    ACTIVE,     ///< Registered and eligible for refresh
    STALE,      ///< Watermark is behind to_ms by more than refresh_interval
    INACTIVE    ///< Manually disabled; skipped during refreshAll()
};

/**
 * @brief Named continuous aggregate definition.
 *
 * Analogous to TimescaleDB's:
 *   CREATE MATERIALIZED VIEW <name> WITH (timescaledb.continuous) AS
 *     SELECT time_bucket(<window>, time_col) bucket,
 *            min(val) AS min, max(val) AS max, avg(val) AS avg,
 *            sum(val) AS sum, count(*) AS count
 *     FROM <source_metric> GROUP BY bucket;
 *
 * The derived metric is stored in TSStore under the name produced by
 * ContinuousAggregateManager::derivedMetricName(config.metric, config.window.size).
 */
struct ContinuousAggDefinition {
    std::string name;          ///< Human-readable aggregate name (must be unique)
    AggConfig   config;        ///< Metric, entity, and window configuration
    bool        auto_refresh{true};  ///< Include in refreshAll() calls
    ContinuousAggStatus status{ContinuousAggStatus::ACTIVE};

    // Derived field — computed once at registration time
    std::string agg_id;        ///< Watermark key = name + "::" + derived_metric_name
};

/**
 * @brief Per-aggregate materialization status snapshot.
 */
struct ContinuousAggMaterializationStatus {
    std::string name;
    std::string derived_metric;
    int64_t     watermark_ms{0};     ///< Upper bound of already-materialized data
    ContinuousAggStatus status{ContinuousAggStatus::ACTIVE};
    size_t      windows_written{0};  ///< Windows written in the last refresh
};

// ============================================================================
// ContinuousAggMaterializationEngine
// ============================================================================

/**
 * @brief TimescaleDB-style continuous aggregate materialization engine.
 *
 * Manages a registry of named continuous aggregate definitions and provides
 * incremental refresh, status queries, and query routing to materialized data.
 *
 * ### Typical lifecycle
 * @code
 *   ContinuousAggMaterializationEngine engine(&tsstore);
 *
 *   // 1. Define a continuous aggregate (like CREATE MATERIALIZED VIEW)
 *   ContinuousAggDefinition def;
 *   def.name   = "cpu_5min";
 *   def.config = { "cpu_usage", "server01", AggWindow{std::chrono::minutes(5)} };
 *   engine.createAggregate(def);
 *
 *   // 2. Refresh incrementally (called by a scheduler or on-demand)
 *   engine.refreshAggregate("cpu_5min", now_ms);
 *   // - or -
 *   engine.refreshAll(now_ms);
 *
 *   // 3. Query materialized results
 *   auto pts = engine.queryMaterialized("cpu_5min", from_ms, to_ms);
 *
 *   // 4. Drop when no longer needed (like DROP MATERIALIZED VIEW)
 *   engine.dropAggregate("cpu_5min");
 * @endcode
 *
 * Thread-safety: individual public methods are NOT thread-safe.  External
 * synchronization is required when calling from multiple threads.
 */
class ContinuousAggMaterializationEngine {
public:
    explicit ContinuousAggMaterializationEngine(TSStore* store);

    // ------------------------------------------------------------------
    // Definition registry
    // ------------------------------------------------------------------

    /**
     * @brief Register a new continuous aggregate definition.
     *
     * The definition's name must be unique within this engine instance.
     * The agg_id field is populated automatically from name and the
     * derived metric name.
     *
     * @return true on success; false if a definition with the same name
     *         already exists.
     */
    bool createAggregate(ContinuousAggDefinition def);

    /**
     * @brief Remove a continuous aggregate definition.
     *
     * Deletes the watermark entry from the TSStore and removes the definition
     * from the registry.  The materialized data points themselves are NOT
     * deleted; callers must clean them up via TSStore retention policies if
     * desired.
     *
     * @return true if the definition was found and removed; false otherwise.
     */
    bool dropAggregate(const std::string& name);

    /**
     * @brief Return the definition for the given aggregate name, or nullopt.
     */
    std::optional<ContinuousAggDefinition> getAggregate(const std::string& name) const;

    /**
     * @brief List the names of all registered aggregates.
     */
    std::vector<std::string> listAggregates() const;

    // ------------------------------------------------------------------
    // Refresh
    // ------------------------------------------------------------------

    /**
     * @brief Incrementally refresh a single named aggregate up to to_ms.
     *
     * Uses the persisted watermark to scan only new data, then advances the
     * watermark after a successful write.  Returns the number of aggregate
     * windows written (0 if already up-to-date or the aggregate is INACTIVE).
     *
     * @param name   Aggregate name (must have been registered via createAggregate).
     * @param to_ms  Upper bound of the refresh window (ms since epoch).
     * @return Number of windows written, or 0 if not found / already current.
     */
    size_t refreshAggregate(const std::string& name, int64_t to_ms);

    /**
     * @brief Refresh all ACTIVE aggregates with auto_refresh == true.
     *
     * Iterates over registered definitions in insertion order and calls
     * refreshAggregate() for each eligible aggregate.
     *
     * @param to_ms  Upper bound applied to every aggregate.
     * @return Total number of aggregate windows written across all aggregates.
     */
    size_t refreshAll(int64_t to_ms);

    // ------------------------------------------------------------------
    // Query
    // ------------------------------------------------------------------

    /**
     * @brief Query materialized aggregate data for a named aggregate.
     *
     * Reads the derived metric from TSStore for the time range [from_ms, to_ms].
     * Only previously-materialized windows are returned; this method does NOT
     * trigger a refresh.
     *
     * @param name     Aggregate name.
     * @param from_ms  Start of the query window (inclusive, ms since epoch).
     * @param to_ms    End of the query window (inclusive, ms since epoch).
     * @return TSStore data points, or an empty vector if not found / no data.
     */
    std::vector<TSStore::DataPoint> queryMaterialized(const std::string& name,
                                                       int64_t from_ms,
                                                       int64_t to_ms) const;

    // ------------------------------------------------------------------
    // Status
    // ------------------------------------------------------------------

    /**
     * @brief Return the materialization status of a named aggregate.
     * @return Status snapshot, or nullopt if the aggregate is not registered.
     */
    std::optional<ContinuousAggMaterializationStatus>
    getAggregateStatus(const std::string& name) const;

    /**
     * @brief Return the materialization status of all registered aggregates.
     */
    std::vector<ContinuousAggMaterializationStatus> getAllStatus() const;

private:
    TSStore*                                               store_;
    ContinuousAggWatermarkStore                            wm_store_;
    ContinuousAggregateManager                             mgr_;

    // Ordered registry: map for O(1) lookup + vector for stable insertion order.
    std::unordered_map<std::string, ContinuousAggDefinition> defs_;
    std::vector<std::string>                                  def_order_;
};

} // namespace themis

