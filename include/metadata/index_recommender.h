/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            index_recommender.h                                ║
  Version:         0.0.26                                             ║
  Last Modified:   2026-02-22 08:38:38                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     156                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <atomic>
#include <mutex>
#include <nlohmann/json.hpp>

namespace themis {

using json = nlohmann::json;

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

    IndexRecommender() = default;
    ~IndexRecommender() = default;

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

private:
    // ========================================================================
    // Internal helpers
    // ========================================================================

    /// Compute the benefit score for a ColumnAccess record.
    double computeBenefit(const ColumnAccess& ca) const;

    // table_name -> (column_name -> ColumnAccess)
    mutable std::mutex                                     mutex_;
    std::map<std::string, std::map<std::string, ColumnAccess>> stats_;
    std::atomic<uint64_t>                                  total_queries_{0};
};

} // namespace themis
