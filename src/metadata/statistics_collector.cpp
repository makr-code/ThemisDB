/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            statistics_collector.cpp                           ║
  Version:         0.0.11                                             ║
  Last Modified:   2026-02-21 14:07:54                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   90.0/100                                       ║
    • Total Lines:     566                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include "metadata/statistics_collector.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <cmath>
#include <set>
#include <stdexcept>

namespace themis {

// ============================================================================
// HistogramBucket
// ============================================================================

json HistogramBucket::toJSON() const {
    return {
        {"lower_bound", lower_bound},
        {"upper_bound", upper_bound},
        {"frequency",   frequency},
    };
}

// ============================================================================
// ColumnStats
// ============================================================================

json ColumnStats::toJSON() const {
    json j;
    j["column_name"]    = column_name;
    j["distinct_count"] = distinct_count;
    j["null_count"]     = null_count;
    j["total_count"]    = total_count;
    j["selectivity"]    = selectivity;
    j["null_fraction"]  = null_fraction;

    if (most_common_value.has_value()) {
        j["most_common_value"] = *most_common_value;
    }
    if (min_value.has_value()) {
        j["min_value"] = *min_value;
    }
    if (max_value.has_value()) {
        j["max_value"] = *max_value;
    }
    if (histogram.has_value()) {
        json buckets = json::array();
        for (const auto& b : *histogram) {
            buckets.push_back(b.toJSON());
        }
        j["histogram"] = buckets;
    }
    return j;
}

// ============================================================================
// TableStats
// ============================================================================

json TableStats::toJSON() const {
    json j;
    j["table_name"]         = table_name;
    j["row_count"]          = row_count;
    j["total_size_bytes"]   = total_size_bytes;
    j["avg_row_size_bytes"] = avg_row_size_bytes;
    j["sample_size"]        = sample_size;

    auto time_t_val = std::chrono::system_clock::to_time_t(last_updated);
    char buf[64];
    std::tm tm_buf{};
#ifdef _WIN32
    gmtime_s(&tm_buf, &time_t_val);
#else
    gmtime_r(&time_t_val, &tm_buf);
#endif
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm_buf);
    j["last_updated"] = buf;

    json cols = json::object();
    for (const auto& [col, stats] : column_stats) {
        cols[col] = stats.toJSON();
    }
    j["column_stats"] = cols;

    return j;
}

// ============================================================================
// StatisticsCollector
// ============================================================================

StatisticsCollector::StatisticsCollector(RocksDBWrapper& db)
    : db_(db)
{
    spdlog::debug("StatisticsCollector: Initialized");
}

StatisticsCollector::~StatisticsCollector() {
    stopRefresh();
}

void StatisticsCollector::setRefreshInterval(std::chrono::seconds interval) {
    // Stop any existing thread first
    stopRefresh();

    refresh_interval_ = interval;
    if (interval.count() <= 0) {
        spdlog::debug("StatisticsCollector: auto-refresh disabled");
        return;
    }

    stop_refresh_.store(false);
    refresh_thread_ = std::thread([this] { refreshLoop_(); });
    spdlog::info("StatisticsCollector: auto-refresh started (interval={}s)", interval.count());
}

void StatisticsCollector::stopRefresh() noexcept {
    stop_refresh_.store(true);
    refresh_cv_.notify_all();
    if (refresh_thread_.joinable()) {
        refresh_thread_.join();
    }
}

void StatisticsCollector::refreshLoop_() {
    while (!stop_refresh_.load()) {
        std::unique_lock<std::mutex> lk(refresh_mutex_);
        refresh_cv_.wait_for(lk, refresh_interval_,
                             [this] { return stop_refresh_.load(); });

        if (stop_refresh_.load()) break;

        // Collect the set of known table names (read lock)
        std::vector<std::string> tables;
        {
            std::shared_lock<std::shared_mutex> sl(cache_mutex_);
            tables.reserve(stats_cache_.size());
            for (const auto& [name, _] : stats_cache_) {
                tables.push_back(name);
            }
        }

        for (const auto& table : tables) {
            if (stop_refresh_.load()) break;
            auto result = collectStats(table);
            if (!result.ok) {
                spdlog::warn("StatisticsCollector: auto-refresh failed for '{}': {}",
                             table, result.error_message);
            }
        }
    }
    spdlog::debug("StatisticsCollector: auto-refresh thread exited");
}

// ----------------------------------------------------------------------------
// Public API
// ----------------------------------------------------------------------------

StatsResult<TableStats> StatisticsCollector::collectStats(
    std::string_view table_name,
    size_t sample_size)
{
    if (table_name.empty()) {
        if (metrics_hook_) metrics_hook_->onError(table_name, static_cast<int>(StatsErrorCode::TABLE_NOT_FOUND));
        return StatsResult<TableStats>::failure(
            StatsErrorCode::TABLE_NOT_FOUND, "Table name cannot be empty");
    }

    if (sample_size == 0) {
        sample_size = kDefaultSampleSize;
    }

    auto collect_start = std::chrono::steady_clock::now();

    spdlog::debug("StatisticsCollector: Collecting stats for '{}' (sample={})",
                  table_name, sample_size);

    TableStats stats;
    stats.table_name  = std::string(table_name);
    stats.last_updated = std::chrono::system_clock::now();
    stats.sample_size  = sample_size;

    // Prefix scan -- iterate over table's keys
    std::string prefix = std::string(table_name) + ":";

    auto it_result = db_.newIterator();
    if (!it_result) {
        auto duration_ms = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - collect_start).count();
        spdlog::error("StatisticsCollector: Iterator error: table='{}' duration_ms={:.2f} rows_sampled=0 error_code={}",
                      table_name, duration_ms, static_cast<int>(StatsErrorCode::ITERATOR_ERROR));
        if (metrics_hook_) {
            metrics_hook_->onCollect(table_name, duration_ms, 0, false);
            metrics_hook_->onError(table_name, static_cast<int>(StatsErrorCode::ITERATOR_ERROR));
        }
        return StatsResult<TableStats>::failure(
            StatsErrorCode::ITERATOR_ERROR, "Failed to create RocksDB iterator");
    }
    auto& it = it_result.value();

    it->Seek(prefix);

    // Per-column raw value samples (string representation for cardinality)
    std::map<std::string, std::vector<std::string>> col_samples;
    size_t row_count = 0;
    uint64_t total_bytes = 0;

    while (it->Valid() && row_count < sample_size) {
        std::string key = it->key().ToString();
        if (key.rfind(prefix, 0) != 0) {
            break;  // Passed the table prefix
        }

        const auto& raw_value = it->value();
        total_bytes += raw_value.size();

        // Parse entity fields
        try {
            std::vector<uint8_t> data(raw_value.data(),
                                      raw_value.data() + raw_value.size());
            // Extract the primary key from the full key (table:pk)
            std::string pk = key.substr(prefix.size());
            BaseEntity entity = BaseEntity::deserialize(pk, data);
            const auto& fields = entity.getAllFields();

            for (const auto& [col, val] : fields) {
                std::string col_str = std::string(col);
                std::string val_str;
                std::visit([&val_str](const auto& v) {
                    using T = std::decay_t<decltype(v)>;
                    if constexpr (std::is_same_v<T, std::string>) {
                        val_str = v;
                    } else if constexpr (std::is_same_v<T, int64_t>) {
                        val_str = std::to_string(v);
                    } else if constexpr (std::is_same_v<T, double>) {
                        val_str = std::to_string(v);
                    } else if constexpr (std::is_same_v<T, bool>) {
                        val_str = v ? "true" : "false";
                    }
                    // Ignore std::monostate (null) – leave val_str empty
                }, val);
                col_samples[col_str].push_back(val_str);
            }
        } catch (const std::exception& e) {
            spdlog::warn("StatisticsCollector: Failed to parse row for '{}': {}",
                         table_name, e.what());
        }

        ++row_count;
        it->Next();
    }

    // Count total rows by continuing past sample window
    size_t total_rows = row_count;
    while (it->Valid()) {
        std::string key = it->key().ToString();
        if (key.rfind(prefix, 0) != 0) break;
        ++total_rows;
        it->Next();
    }

    stats.row_count         = total_rows;
    stats.total_size_bytes  = (row_count > 0) ? total_bytes * total_rows / row_count : 0;
    stats.avg_row_size_bytes = (row_count > 0)
        ? static_cast<double>(total_bytes) / static_cast<double>(row_count)
        : 0.0;

    // Build per-column stats from samples
    for (const auto& [col, values] : col_samples) {
        stats.column_stats[col] = buildColumnStats(col, values);
        stats.column_stats[col].total_count = row_count;
    }

    // Persist and cache
    persistStats(stats);
    {
        std::unique_lock<std::shared_mutex> lock(cache_mutex_);
        stats_cache_[stats.table_name] = stats;
    }

    auto duration_ms = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - collect_start).count();

    // Structured log: all key fields on one line for log-scraping / alerting
    spdlog::info("StatisticsCollector: collect table='{}' duration_ms={:.2f} rows_sampled={} total_rows={} cols={} error_code=0",
                 table_name, duration_ms, row_count, total_rows, stats.column_stats.size());

    if (metrics_hook_) {
        metrics_hook_->onCollect(table_name, duration_ms, row_count, true);
    }

    return StatsResult<TableStats>::success(stats);
}

StatsResult<TableStats> StatisticsCollector::getStats(std::string_view table_name) {
    if (table_name.empty()) {
        if (metrics_hook_) metrics_hook_->onError(table_name, static_cast<int>(StatsErrorCode::TABLE_NOT_FOUND));
        return StatsResult<TableStats>::failure(
            StatsErrorCode::TABLE_NOT_FOUND, "Table name cannot be empty");
    }

    // Check in-memory cache
    {
        std::shared_lock<std::shared_mutex> lock(cache_mutex_);
        auto it = stats_cache_.find(std::string(table_name));
        if (it != stats_cache_.end()) {
            if (metrics_hook_) metrics_hook_->onCacheHit(table_name);
            return StatsResult<TableStats>::success(it->second);
        }
    }

    // Cache miss – try to load from RocksDB
    if (metrics_hook_) metrics_hook_->onCacheMiss(table_name);

    auto maybe_stats = loadStats(table_name);
    if (!maybe_stats.has_value()) {
        return StatsResult<TableStats>::failure(
            StatsErrorCode::TABLE_NOT_FOUND,
            "No statistics found for table '" + std::string(table_name) + "'");
    }

    {
        std::unique_lock<std::shared_mutex> lock(cache_mutex_);
        stats_cache_[std::string(table_name)] = *maybe_stats;
    }

    return StatsResult<TableStats>::success(std::move(*maybe_stats));
}

StatsResult<bool> StatisticsCollector::updateStats(std::string_view table_name) {
    auto result = collectStats(table_name);
    if (!result.ok) {
        return StatsResult<bool>::failure(result.error, result.error_message);
    }
    return StatsResult<bool>::success(true);
}

StatsResult<bool> StatisticsCollector::clearStats(std::string_view table_name) {
    if (table_name.empty()) {
        return StatsResult<bool>::failure(
            StatsErrorCode::TABLE_NOT_FOUND, "Table name cannot be empty");
    }

    {
        std::unique_lock<std::shared_mutex> lock(cache_mutex_);
        stats_cache_.erase(std::string(table_name));
    }

    std::string key = "stats:" + std::string(table_name);
    db_.del(key);

    spdlog::debug("StatisticsCollector: Cleared stats for '{}'", table_name);
    return StatsResult<bool>::success(true);
}

json StatisticsCollector::toJSON() const {
    std::shared_lock<std::shared_mutex> lock(cache_mutex_);
    json j = json::object();
    for (const auto& [name, stats] : stats_cache_) {
        j[name] = stats.toJSON();
    }
    return j;
}

// ----------------------------------------------------------------------------
// Internal helpers
// ----------------------------------------------------------------------------

ColumnStats StatisticsCollector::buildColumnStats(
    std::string_view column_name,
    const std::vector<std::string>& values,
    size_t num_histogram_buckets)
{
    ColumnStats cs;
    cs.column_name = std::string(column_name);

    if (values.empty()) {
        return cs;
    }

    // Cardinality / NULL counting
    std::map<std::string, size_t> freq_map;
    size_t null_count = 0;
    for (const auto& v : values) {
        if (v.empty()) {
            ++null_count;
        } else {
            ++freq_map[v];
        }
    }

    cs.null_count    = null_count;
    cs.distinct_count = freq_map.size();
    cs.null_fraction  = static_cast<double>(null_count) / static_cast<double>(values.size());
    cs.selectivity    = (freq_map.empty())
        ? 1.0
        : 1.0 / static_cast<double>(freq_map.size());

    // Most common value
    if (!freq_map.empty()) {
        auto max_it = std::max_element(
            freq_map.begin(), freq_map.end(),
            [](const auto& a, const auto& b) { return a.second < b.second; });
        cs.most_common_value = max_it->first;
    }

    // Numeric histogram (attempt conversion; silently skip non-numeric)
    std::vector<double> numeric_vals;
    numeric_vals.reserve(values.size());
    for (const auto& v : values) {
        if (v.empty()) continue;
        try {
            numeric_vals.push_back(std::stod(v));
        } catch (...) {
            // Not numeric
        }
    }

    if (!numeric_vals.empty()) {
        std::sort(numeric_vals.begin(), numeric_vals.end());
        cs.min_value = numeric_vals.front();
        cs.max_value = numeric_vals.back();
        if (num_histogram_buckets > 0) {
            cs.histogram = buildHistogram(numeric_vals, num_histogram_buckets);
        }
    }

    return cs;
}

std::vector<HistogramBucket> StatisticsCollector::buildHistogram(
    const std::vector<double>& sorted_values,
    size_t num_buckets)
{
    if (sorted_values.empty() || num_buckets == 0) {
        return {};
    }

    num_buckets = std::min(num_buckets, sorted_values.size());

    double min_v = sorted_values.front();
    double max_v = sorted_values.back();

    if (std::abs(max_v - min_v) < 1e-12) {
        // All values identical
        HistogramBucket b;
        b.lower_bound = min_v;
        b.upper_bound = max_v;
        b.frequency   = sorted_values.size();
        return {b};
    }

    double bucket_width = (max_v - min_v) / static_cast<double>(num_buckets);
    std::vector<HistogramBucket> buckets(num_buckets);

    for (size_t i = 0; i < num_buckets; ++i) {
        buckets[i].lower_bound = min_v + static_cast<double>(i) * bucket_width;
        buckets[i].upper_bound = min_v + static_cast<double>(i + 1) * bucket_width;
        buckets[i].frequency   = 0;
    }
    // Make the last bucket closed on the right
    buckets.back().upper_bound = max_v + 1e-9;

    for (double v : sorted_values) {
        size_t idx = static_cast<size_t>((v - min_v) / bucket_width);
        if (idx >= num_buckets) idx = num_buckets - 1;
        ++buckets[idx].frequency;
    }

    return buckets;
}

void StatisticsCollector::persistStats(const TableStats& stats) {
    try {
        std::string key   = "stats:" + stats.table_name;
        std::string value = stats.toJSON().dump();
        std::vector<uint8_t> data(value.begin(), value.end());
        if (!db_.put(key, data)) {
            spdlog::warn("StatisticsCollector: Failed to persist stats for '{}'",
                         stats.table_name);
        }
    } catch (const std::exception& e) {
        spdlog::error("StatisticsCollector: Exception persisting stats for '{}': {}",
                      stats.table_name, e.what());
    }
}

std::optional<TableStats> StatisticsCollector::loadStats(std::string_view table_name) {
    try {
        std::string key = "stats:" + std::string(table_name);
        auto result = db_.get(key);
        if (!result.has_value() || result->empty()) {
            return std::nullopt;
        }

        std::string json_str(result->begin(), result->end());
        json j = json::parse(json_str);

        TableStats stats;
        stats.table_name         = j.value("table_name",         std::string(table_name));
        stats.row_count          = j.value("row_count",          size_t{0});
        stats.total_size_bytes   = j.value("total_size_bytes",   uint64_t{0});
        stats.avg_row_size_bytes = j.value("avg_row_size_bytes", 0.0);
        stats.sample_size        = j.value("sample_size",        size_t{0});
        stats.last_updated       = std::chrono::system_clock::now();  // Approximate

        if (j.contains("column_stats") && j["column_stats"].is_object()) {
            for (const auto& [col, cj] : j["column_stats"].items()) {
                ColumnStats cs;
                cs.column_name    = col;
                cs.distinct_count = cj.value("distinct_count", size_t{0});
                cs.null_count     = cj.value("null_count",     size_t{0});
                cs.total_count    = cj.value("total_count",    size_t{0});
                cs.selectivity    = cj.value("selectivity",    1.0);
                cs.null_fraction  = cj.value("null_fraction",  0.0);
                if (cj.contains("most_common_value") && cj["most_common_value"].is_string()) {
                    cs.most_common_value = cj["most_common_value"].get<std::string>();
                }
                if (cj.contains("min_value") && cj["min_value"].is_number()) {
                    cs.min_value = cj["min_value"].get<double>();
                }
                if (cj.contains("max_value") && cj["max_value"].is_number()) {
                    cs.max_value = cj["max_value"].get<double>();
                }
                stats.column_stats[col] = std::move(cs);
            }
        }

        return stats;
    } catch (const std::exception& e) {
        spdlog::warn("StatisticsCollector: Failed to load stats for '{}': {}",
                     table_name, e.what());
        return std::nullopt;
    }
}

} // namespace themis
