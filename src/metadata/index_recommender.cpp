/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            index_recommender.cpp                              ║
  Version:         0.0.7                                              ║
  Last Modified:   2026-02-21 11:48:44                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     245                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3089438e7  2026-02-21  Add RocksDB option files and manifest for caching ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include "metadata/index_recommender.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <set>

namespace themis {

// ============================================================================
// ColumnAccess
// ============================================================================

json ColumnAccess::toJSON() const {
    return {
        {"table_name",       table_name},
        {"column_name",      column_name},
        {"filter_count",     filter_count},
        {"sort_count",       sort_count},
        {"avg_selectivity",  avg_selectivity},
    };
}

// ============================================================================
// IndexRecommendation
// ============================================================================

json IndexRecommendation::toJSON() const {
    return {
        {"table_name",    table_name},
        {"column_name",   column_name},
        {"index_type",    index_type},
        {"action",        (action == Action::ADD) ? "ADD" : "DROP"},
        {"benefit_score", benefit_score},
        {"rationale",     rationale},
    };
}

// ============================================================================
// IndexRecommender – public API
// ============================================================================

void IndexRecommender::recordAccess(
    std::string_view table_name,
    std::string_view column_name,
    AccessType       access_type,
    double           selectivity)
{
    std::lock_guard<std::mutex> lock(mutex_);

    auto& col_access = stats_[std::string(table_name)][std::string(column_name)];
    col_access.table_name  = std::string(table_name);
    col_access.column_name = std::string(column_name);

    if (access_type == AccessType::FILTER) {
        ++col_access.filter_count;
        // Running average of selectivity
        uint64_t n = col_access.filter_count;
        col_access.avg_selectivity =
            col_access.avg_selectivity * static_cast<double>(n - 1) / static_cast<double>(n)
            + selectivity / static_cast<double>(n);
    } else {
        ++col_access.sort_count;
    }

    spdlog::trace("IndexRecommender: Access recorded for {}.{} ({})",
                  table_name, column_name,
                  (access_type == AccessType::FILTER) ? "filter" : "sort");
}

void IndexRecommender::recordQuery() {
    ++total_queries_;
}

std::vector<IndexRecommendation> IndexRecommender::recommend(
    std::string_view table_name,
    const std::vector<std::string>& existing_indexes) const
{
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<IndexRecommendation> recs;

    auto t_it = stats_.find(std::string(table_name));
    if (t_it == stats_.end()) {
        return recs;  // No data for this table
    }

    // Build a set of currently indexed columns for fast lookup
    std::set<std::string> indexed_set(existing_indexes.begin(), existing_indexes.end());

    for (const auto& [col_name, ca] : t_it->second) {
        double score = computeBenefit(ca);
        bool   is_indexed = (indexed_set.count(col_name) > 0);

        if (!is_indexed && score >= kAddThreshold) {
            IndexRecommendation rec;
            rec.table_name  = std::string(table_name);
            rec.column_name = col_name;
            // Choose index type heuristically: range index for high sort usage
            rec.index_type  = (ca.sort_count > ca.filter_count) ? "range" : "regular";
            rec.action      = IndexRecommendation::Action::ADD;
            rec.benefit_score = score;
            rec.rationale   =
                "Column '" + col_name + "' appeared in " +
                std::to_string(ca.filter_count) + " filter(s) and " +
                std::to_string(ca.sort_count)   + " sort(s); " +
                "average selectivity " +
                std::to_string(static_cast<int>((1.0 - ca.avg_selectivity) * 100)) +
                "% (higher = more selective).";
            recs.push_back(std::move(rec));

        } else if (is_indexed && score < kDropThreshold) {
            IndexRecommendation rec;
            rec.table_name    = std::string(table_name);
            rec.column_name   = col_name;
            rec.index_type    = "regular";
            rec.action        = IndexRecommendation::Action::DROP;
            rec.benefit_score = score;
            rec.rationale     =
                "Index on '" + col_name + "' is rarely used (benefit_score=" +
                std::to_string(score) + "); consider dropping to reduce write overhead.";
            recs.push_back(std::move(rec));
        }
    }

    // Sort by benefit_score descending
    std::sort(recs.begin(), recs.end(), [](const auto& a, const auto& b) {
        return a.benefit_score > b.benefit_score;
    });

    return recs;
}

std::map<std::string, std::vector<IndexRecommendation>> IndexRecommender::recommendAll(
    const std::map<std::string, std::vector<std::string>>& existing_indexes) const
{
    // Collect table names first (brief lock) then call recommend() per table
    std::vector<std::string> table_names;
    {
        std::lock_guard<std::mutex> lk(mutex_);
        for (const auto& [tn, _] : stats_) {
            table_names.push_back(tn);
        }
    }

    std::map<std::string, std::vector<IndexRecommendation>> result;
    for (const auto& tn : table_names) {
        std::vector<std::string> existing;
        auto it = existing_indexes.find(tn);
        if (it != existing_indexes.end()) {
            existing = it->second;
        }
        result[tn] = recommend(tn, existing);
    }

    return result;
}

std::vector<ColumnAccess> IndexRecommender::getAccessStats(std::string_view table_name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<ColumnAccess> result;

    auto t_it = stats_.find(std::string(table_name));
    if (t_it == stats_.end()) return result;

    for (const auto& [_, ca] : t_it->second) {
        result.push_back(ca);
    }
    return result;
}

void IndexRecommender::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    stats_.clear();
    total_queries_.store(0);
}

json IndexRecommender::toJSON() const {
    std::lock_guard<std::mutex> lock(mutex_);

    json j = json::object();
    for (const auto& [table_name, cols] : stats_) {
        json col_arr = json::array();
        for (const auto& [_, ca] : cols) {
            col_arr.push_back(ca.toJSON());
        }
        j[table_name] = col_arr;
    }
    return j;
}

// ============================================================================
// Private helpers
// ============================================================================

double IndexRecommender::computeBenefit(const ColumnAccess& ca) const {
    uint64_t total = total_queries_.load();
    if (total == 0 || (ca.filter_count == 0 && ca.sort_count == 0)) {
        return 0.0;
    }

    // Weighted access count: filter is more valuable than sort for index use
    double weighted_accesses =
        static_cast<double>(ca.filter_count) * 2.0 +
        static_cast<double>(ca.sort_count)   * 1.0;

    // Selectivity: (1 - avg_selectivity) is 1 when very selective (rare match),
    // 0 when not selective (matches everything); an index helps more on selective columns.
    double selectivity_bonus = 1.0 - ca.avg_selectivity;

    // Normalise by total queries so high-traffic tables don't dominate over time
    double score = (weighted_accesses * (1.0 + selectivity_bonus) * 100.0)
                    / static_cast<double>(total);

    return std::min(score, 100.0);
}

} // namespace themis
