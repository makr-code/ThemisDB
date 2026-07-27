/**
 * @file index_recommender.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: index_recommender.h | Version: 0.0.47 | Last Modified: 2026-05-31 12:49:01
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 94/100 | Lines: 216
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): #5058 [Docs][Module] metadata â€”... (2026-05-13) | #4303 feat(metadata): IndexRecomm... (2026-03-16)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <nlohmann/json.hpp>

namespace themis {

class RocksDBWrapper;
class StatisticsCollector;
struct TableStats;

namespace observability {
class MetricsCollector;
} // namespace observability

using json = nlohmann::json;

namespace metadata {

/// A single observed query access pattern for a column
struct ColumnAccess {
    std::string table_name;
    std::string column_name;
    uint64_t    filter_count   = 0;   ///< Times used in WHERE / FILTER
    uint64_t    sort_count     = 0;   ///< Times used in ORDER BY / SORT
    double      avg_selectivity = 1.0; ///< Average selectivity (0=highly selective,1=not)

    json toJSON() const;
};

/// A single index recommendation
struct IndexRecommendation {
    enum class Action { ADD, DROP };

    std::string table_name;
    std::string column_name;
    std::string index_type;       ///< "regular", "range", "composite", …
    Action      action;
    double      benefit_score;    ///< Estimated benefit (0–100)
    std::string rationale;        ///< Human-readable explanation

    json toJSON() const;
};

/// IndexRecommender – lightweight auto index recommendation engine
///
/// Records column access patterns via `recordAccess()` and then analyses
/// them with `recommend()` to suggest indexes that would improve query
/// performance.
///
/// The engine uses a simple scoring model:
///   benefit_score = (filter_count × (1 – avg_selectivity) × 100) / total_queries
///
/// Recommendations with benefit_score > ADD_THRESHOLD are returned as ADD.
/// Indexed columns that have never been accessed and have benefit_score < DROP_THRESHOLD
/// are returned as DROP (requires the caller to supply existing_indexes).
///
/// Thread-safety: `recordAccess()` is thread-safe (uses a mutex).
///                `recommend()` is also thread-safe.
///
/// Usage:
///   IndexRecommender rec;
///   rec.recordAccess("users", "email", IndexRecommender::AccessType::FILTER, 0.01);
///   auto recs = rec.recommend("users");
class IndexRecommender {
public:
    /// Controls whether a column was used as a filter predicate or for sorting
    enum class AccessType { FILTER, SORT };

    /// Benefit score above which an ADD recommendation is generated
    static constexpr double kAddThreshold  = 20.0;

    /// Benefit score below which a DROP recommendation is generated
    static constexpr double kDropThreshold = 5.0;

    /// Default constructor — in-memory only (no persistence).
    IndexRecommender() = default;

    /// Persistence-enabled constructor.
    /// @param db               RocksDB instance for loading and persisting access stats.
    ///                         Pass nullptr for in-memory-only mode (same as default ctor).
    ///                         The pointed-to instance MUST outlive this IndexRecommender.
    /// @param persist_interval Background thread flush interval.
    ///                         Defaults to 5 minutes.  Pass 0 to disable the background thread
    ///                         (stats are still flushed on destruction and reset()).
    explicit IndexRecommender(
        RocksDBWrapper* db,
        std::chrono::milliseconds persist_interval = std::chrono::seconds(300)
    );

    /// Destructor — stops the background persist thread and flushes stats to RocksDB.
    ~IndexRecommender();

    // Disable copy
    IndexRecommender(const IndexRecommender&) = delete;
    IndexRecommender& operator=(const IndexRecommender&) = delete;

    // ========================================================================
    // Public API
    // ========================================================================

    /// Record that a column was accessed in a query.
    /// @param table_name  Table the column belongs to
    /// @param column_name Column that was accessed
    /// @param access_type Whether the column was used as a filter or sort key
    /// @param selectivity Fraction of rows that matched the predicate (0 = very selective)
    void recordAccess(
        std::string_view table_name,
        std::string_view column_name,
        AccessType       access_type,
        double           selectivity = 1.0
    );

    /// Increment the total query counter (used for normalising benefit scores).
    void recordQuery();

    /// Generate recommendations for a specific table.
    /// @param table_name      Table to analyse
    /// @param existing_indexes Names of columns that already have an index
    /// @return                Sorted vector of recommendations (highest benefit first)
    std::vector<IndexRecommendation> recommend(
        std::string_view table_name,
        const std::vector<std::string>& existing_indexes = {}
    ) const;

    /// Generate recommendations for all tracked tables.
    std::map<std::string, std::vector<IndexRecommendation>> recommendAll(
        const std::map<std::string, std::vector<std::string>>& existing_indexes = {}
    ) const;

    /// Return the raw access statistics for a table.
    std::vector<ColumnAccess> getAccessStats(std::string_view table_name) const;

    /// Reset all tracked access statistics.
    void reset();

    /// Serialise all access stats to JSON.
    json toJSON() const;

    /// Flush all in-memory access stats to RocksDB immediately.
    /// No-op when no RocksDB instance was provided at construction.
    void persistStats();

    /// Attach a StatisticsCollector to enable cost-model benefit scoring.
    /// When set, `recommend()` uses StatisticsCollector cardinality and
    /// selectivity data together with a write-amplification penalty to produce
    /// more accurate benefit scores than the simple heuristic model.
    /// Pass nullptr to revert to the heuristic model.
    /// The pointed-to instance MUST outlive this IndexRecommender.
    void setStatisticsCollector(StatisticsCollector* collector);

    /// Attach a MetricsCollector for emitting recommendation telemetry.
    /// When set, each call to `recommend()` increments the counter
    /// `metadata.index_recommendation.generated_total` labelled with the
    /// table name.  Pass nullptr to stop emitting metrics.
    /// The pointed-to instance MUST outlive this IndexRecommender.
    void setMetricsCollector(observability::MetricsCollector* metrics);

private:
    // ========================================================================
    // Internal helpers
    // ========================================================================

    /// Compute the benefit score for a ColumnAccess record.
    double computeBenefit(const ColumnAccess& ca) const;

    /// Compute the benefit score using StatisticsCollector data (cost-model).
    /// Uses StatisticsCollector column selectivity for a more accurate estimate
    /// and applies a write-amplification penalty based on table row count.
    double computeCostModelBenefit(const ColumnAccess& ca, const TableStats& tbl_stats) const;

    /// Load access stats from RocksDB into stats_ (called once in constructor).
    void loadStats();

    /// Background persist loop — wakes every persist_interval_ and calls persistStats().
    void persistLoop_();

    // ─── Persistence ────────────────────────────────────────────────────────
    RocksDBWrapper*           db_{nullptr};           ///< Optional RocksDB backend
    std::chrono::milliseconds persist_interval_{0};   ///< 0 = no background thread

    std::atomic<bool>    stop_persist_{false};
    std::mutex           persist_mutex_;               ///< Protects condition variable
    std::condition_variable persist_cv_;
    std::thread          persist_thread_;

    // ─── Cost-model + metrics ────────────────────────────────────────────────
    StatisticsCollector*               stats_collector_{nullptr}; ///< Optional, for cost-model scoring
    observability::MetricsCollector*   metrics_collector_{nullptr}; ///< Optional, for telemetry

    // ─── Access stats ────────────────────────────────────────────────────────
    // table_name -> (column_name -> ColumnAccess)
    mutable std::mutex                                     mutex_;
    std::map<std::string, std::map<std::string, ColumnAccess>> stats_;
    std::atomic<uint64_t>                                  total_queries_{0};
};

} // namespace metadata
} // namespace themis
