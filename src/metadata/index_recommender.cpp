/**
 * @file index_recommender.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=14, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include "metadata/index_recommender.h"
#include "metadata/statistics_collector.h"
#include "observability/metrics_collector.h"
#include "storage/rocksdb_wrapper.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <cmath>
#include <set>

namespace themis {
namespace metadata {

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
// IndexRecommender – constructors / destructor
// ============================================================================

IndexRecommender::IndexRecommender(
    RocksDBWrapper* db,
    std::chrono::milliseconds persist_interval)
    : db_(db), persist_interval_(persist_interval)
{
    loadStats();

    if (db_ && persist_interval_.count() > 0) {
        stop_persist_.store(false);
        persist_thread_ = std::thread([this] { persistLoop_(); });
        spdlog::debug("IndexRecommender: background persist thread started (interval={}ms)",
                      persist_interval_.count());
    }
}

IndexRecommender::~IndexRecommender() {
    stop_persist_.store(true);
    persist_cv_.notify_all();
    if (persist_thread_.joinable()) {
        persist_thread_.join();
    }
    // Final flush on graceful shutdown
    persistStats();
}

// ============================================================================
// IndexRecommender – public API
// ============================================================================

void IndexRecommender::setStatisticsCollector(StatisticsCollector* collector) {
    stats_collector_ = collector;
}

void IndexRecommender::setMetricsCollector(observability::MetricsCollector* metrics) {
    metrics_collector_ = metrics;
}

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

    // Optionally fetch table statistics for the cost-model benefit formula.
    const TableStats* tbl_stats_ptr = nullptr;
    std::optional<TableStats> fetched_stats;
    if (stats_collector_) {
        auto result = stats_collector_->getStats(table_name);
        if (result.ok) {
            fetched_stats = std::move(result.value);
            tbl_stats_ptr = &fetched_stats.value();
        }
    }

    for (const auto& [col_name, ca] : t_it->second) {
        double score = tbl_stats_ptr
                        ? computeCostModelBenefit(ca, *tbl_stats_ptr)
                        : computeBenefit(ca);
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

    // Emit recommendation telemetry counter if a MetricsCollector is attached.
    if (metrics_collector_) {
        metrics_collector_->addCounter(
            "metadata.index_recommendation.generated_total",
            1,
            {{"table", std::string(table_name)}});
    }

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
    std::vector<std::string> table_names_to_delete;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& [tn, _] : stats_) {
            table_names_to_delete.push_back(tn);
        }
        stats_.clear();
        total_queries_.store(0);
    }

    // Remove persisted keys for all cleared tables
    if (db_) {
        for (const auto& tn : table_names_to_delete) {
            std::string key = "meta_idx_stats::" + tn;
            db_->del(key);
        }
        // Also remove the total_queries key
        db_->del("meta_idx_stats::__total_queries__");
        spdlog::debug("IndexRecommender: cleared {} persisted table(s) from RocksDB",
                      table_names_to_delete.size());
    }
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

void IndexRecommender::persistStats() {
    if (!db_) return;

    // Snapshot stats under the lock, then write outside the lock
    std::map<std::string, std::map<std::string, ColumnAccess>> snapshot;
    uint64_t total_q = 0;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        snapshot  = stats_;
        total_q   = total_queries_.load();
    }

    try {
        for (const auto& [table_name, cols] : snapshot) {
            json arr = json::array();
            for (const auto& [_, ca] : cols) {
                arr.push_back(ca.toJSON());
            }
            std::string key   = "meta_idx_stats::" + table_name;
            std::string value = arr.dump();
            if (!db_->put(key, value)) {
                spdlog::warn("IndexRecommender: failed to persist stats for table '{}'",
                             table_name);
            }
        }

        // Persist total_queries separately so it survives across restarts
        std::string tq_key   = "meta_idx_stats::__total_queries__";
        std::string tq_value = std::to_string(total_q);
        if (!db_->put(tq_key, tq_value)) {
            spdlog::warn("IndexRecommender: failed to persist total_queries");
        }

        spdlog::debug("IndexRecommender: persisted stats for {} table(s) to RocksDB",
                      snapshot.size());
    } catch (const std::exception& e) {
        spdlog::error("IndexRecommender: exception during persistStats: {}", e.what());
    }
}

void IndexRecommender::loadStats() {
    if (!db_) return;

    static constexpr std::string_view PREFIX = "meta_idx_stats::";

    std::string start_key(PREFIX);
    std::string end_key = start_key;
    end_key.back()++;  // "meta_idx_stats::" → "meta_idx_stats:;"

    std::vector<std::pair<std::string, std::string>> corrupt_entries;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        db_->iterateRange(start_key, end_key,
            [&](std::string_view key, std::string_view value) -> bool {
                if (key.size() <= PREFIX.size()) return true;
                std::string table_name(key.substr(PREFIX.size()));

                // Skip the total_queries pseudo-table
                if (table_name == "__total_queries__") {
                    try {
                        uint64_t tq = std::stoull(std::string(value));
                        total_queries_.store(tq);
                    } catch (const std::exception& e) {
                        corrupt_entries.emplace_back(table_name, e.what());
                    }
                    return true;
                }

                try {
                    auto arr = json::parse(value);
                    if (!arr.is_array()) return true;

                    for (const auto& item : arr) {
                        ColumnAccess ca;
                        ca.table_name      = item.value("table_name",      table_name);
                        ca.column_name     = item.value("column_name",     std::string{});
                        ca.filter_count    = item.value("filter_count",    uint64_t{0});
                        ca.sort_count      = item.value("sort_count",      uint64_t{0});
                        ca.avg_selectivity = item.value("avg_selectivity", 1.0);

                        if (ca.column_name.empty()) continue;

                        // Merge: accumulated counts from persisted state + any in-memory
                        auto& slot = stats_[table_name][ca.column_name];
                        if (slot.filter_count == 0 && slot.sort_count == 0) {
                            // First time seeing this column — adopt persisted values
                            slot = ca;
                        } else {
                            // Column already has in-memory activity; accumulate counts
                            // and update the running average selectivity
                            uint64_t combined = slot.filter_count + ca.filter_count;
                            if (combined > 0) {
                                slot.avg_selectivity =
                                    (slot.avg_selectivity * static_cast<double>(slot.filter_count) +
                                     ca.avg_selectivity  * static_cast<double>(ca.filter_count))
                                    / static_cast<double>(combined);
                            }
                            slot.filter_count += ca.filter_count;
                            slot.sort_count   += ca.sort_count;
                            if (slot.table_name.empty())  slot.table_name  = table_name;
                            if (slot.column_name.empty()) slot.column_name = ca.column_name;
                        }
                    }
                } catch (const std::exception& e) {
                    corrupt_entries.emplace_back(table_name, e.what());
                }
                return true;
            });
    }

    for (const auto& [tname, err] : corrupt_entries) {
        spdlog::warn("IndexRecommender: skipping corrupted stats entry for '{}': {}",
                     tname, err);
    }

    spdlog::debug("IndexRecommender: loaded persisted stats for {} table(s)",
                  stats_.size());
}

void IndexRecommender::persistLoop_() {
    while (!stop_persist_.load()) {
        std::unique_lock<std::mutex> lk(persist_mutex_);
        persist_cv_.wait_for(lk, persist_interval_,
                             [this] { return stop_persist_.load(); });
        if (stop_persist_.load()) break;
        persistStats();
    }
}

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

double IndexRecommender::computeCostModelBenefit(
    const ColumnAccess& ca,
    const TableStats&   tbl_stats) const
{
    uint64_t total = total_queries_.load();
    if (total == 0 || (ca.filter_count == 0 && ca.sort_count == 0)) {
        return 0.0;
    }

    // 1) Access frequency — same weighting as the heuristic model.
    double weighted_accesses =
        static_cast<double>(ca.filter_count) * 2.0 +
        static_cast<double>(ca.sort_count)   * 1.0;

    // 2) Selectivity from StatisticsCollector column stats (more precise than
    //    the running-average avg_selectivity tracked at query time).
    double col_selectivity = ca.avg_selectivity;  // fallback to tracked value
    auto col_it = tbl_stats.column_stats.find(ca.column_name);
    if (col_it != tbl_stats.column_stats.end()) {
        col_selectivity = col_it->second.selectivity;
    }
    // selectivity_bonus: 1 when very selective (0 rows returned per predicate),
    // 0 when not selective (all rows match); index is more beneficial on
    // selective columns.
    double selectivity_bonus = 1.0 - std::clamp(col_selectivity, 0.0, 1.0);

    // 3) Write-amplification penalty: each write must also update the index.
    //    Penalty grows logarithmically with table row count, capped at 20%.
    //    Empty tables incur no penalty; a 10 M-row table has ~20% reduction.
    double write_amplification = 0.0;
    if (tbl_stats.row_count > 0) {
        // log10(1) = 0, log10(10 000 000) = 7 → normalise to [0, 1] at 10 M rows
        write_amplification =
            std::min(1.0, std::log10(1.0 + static_cast<double>(tbl_stats.row_count)) / 7.0)
            * 0.20;
    }

    // Raw score (0–100) then reduced by the write-amplification factor.
    double raw_score = (weighted_accesses * (1.0 + selectivity_bonus) * 100.0)
                       / static_cast<double>(total);
    double score = raw_score * (1.0 - write_amplification);

    return std::min(score, 100.0);
}

} // namespace metadata
} // namespace themis

