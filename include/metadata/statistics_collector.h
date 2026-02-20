// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <optional>
#include <memory>
#include <chrono>
#include <shared_mutex>
#include <nlohmann/json.hpp>

namespace themis {

// Forward declarations
class RocksDBWrapper;

using json = nlohmann::json;

/// Histogram bucket for value distribution analysis
struct HistogramBucket {
    double lower_bound;     ///< Lower bound (inclusive)
    double upper_bound;     ///< Upper bound (exclusive)
    size_t frequency;       ///< Number of values in this bucket

    json toJSON() const;
};

/// Per-column statistics for query optimization
struct ColumnStats {
    std::string column_name;                        ///< Column/property name
    size_t distinct_count = 0;                      ///< Number of distinct values (cardinality)
    size_t null_count = 0;                          ///< Number of NULL values
    size_t total_count = 0;                         ///< Total number of rows sampled
    double selectivity = 1.0;                       ///< Fraction of rows returned by equality predicate
    double null_fraction = 0.0;                     ///< Fraction of NULL values
    std::optional<std::vector<HistogramBucket>> histogram;  ///< Value distribution histogram
    std::optional<std::string> most_common_value;   ///< Most frequently occurring value
    std::optional<double> min_value;                ///< Minimum numeric value (if numeric)
    std::optional<double> max_value;                ///< Maximum numeric value (if numeric)

    json toJSON() const;
};

/// Per-table statistics for query planning and cardinality estimation
struct TableStats {
    std::string table_name;                         ///< Table/collection name
    size_t row_count = 0;                           ///< Estimated total row count
    uint64_t total_size_bytes = 0;                  ///< Estimated total size in bytes
    double avg_row_size_bytes = 0.0;                ///< Average row size in bytes
    std::map<std::string, ColumnStats> column_stats; ///< Per-column statistics
    std::chrono::system_clock::time_point last_updated; ///< When statistics were last refreshed
    size_t sample_size = 0;                         ///< Number of rows sampled during collection

    json toJSON() const;
};

/// Error codes for statistics collection operations
enum class StatsErrorCode {
    OK = 0,
    TABLE_NOT_FOUND,
    COLLECTION_FAILED,
    STORAGE_ERROR,
    ITERATOR_ERROR,
    SERIALIZATION_ERROR,
};

/// Result type wrapping a value or an error code
template<typename T>
struct StatsResult {
    bool ok = false;
    T value{};
    StatsErrorCode error = StatsErrorCode::OK;
    std::string error_message;

    static StatsResult<T> success(T v) {
        StatsResult<T> r;
        r.ok = true;
        r.value = std::move(v);
        return r;
    }

    static StatsResult<T> failure(StatsErrorCode code, std::string msg) {
        StatsResult<T> r;
        r.ok = false;
        r.error = code;
        r.error_message = std::move(msg);
        return r;
    }
};

/// StatisticsCollector - Table and column statistics for query optimization
///
/// Collects cardinality estimates, histograms, selectivity, and NULL ratios
/// via sampled scans of RocksDB key-value pairs.  Results are cached in-memory
/// and can be persisted back to RocksDB under the "stats:" key prefix.
///
/// Thread-safety: Uses std::shared_mutex; multiple concurrent reads are safe.
/// Write operations (collectStats / updateStats) serialize via exclusive lock.
///
/// Usage:
///   StatisticsCollector stats_collector(db);
///   auto result = stats_collector.collectStats("users");
///   if (result.ok) {
///       auto& stats = result.value;
///       std::cout << stats.row_count << " rows\n";
///   }
class StatisticsCollector {
public:
    /// Default sample size used when collecting column statistics
    static constexpr size_t kDefaultSampleSize = 1000;

    /// Default number of histogram buckets
    static constexpr size_t kDefaultHistogramBuckets = 20;

    /// Constructor
    /// @param db RocksDB wrapper for key scanning and persisting statistics
    explicit StatisticsCollector(RocksDBWrapper& db);

    ~StatisticsCollector() = default;

    // Disable copy, allow move
    StatisticsCollector(const StatisticsCollector&) = delete;
    StatisticsCollector& operator=(const StatisticsCollector&) = delete;
    StatisticsCollector(StatisticsCollector&&) = default;
    StatisticsCollector& operator=(StatisticsCollector&&) = default;

    // ========================================================================
    // Public API
    // ========================================================================

    /// Collect full statistics for a table by sampling stored entities.
    /// Writes the result to in-memory cache and persists it to RocksDB.
    /// @param table_name  Table/collection name
    /// @param sample_size Number of rows to sample (0 = use kDefaultSampleSize)
    StatsResult<TableStats> collectStats(
        std::string_view table_name,
        size_t sample_size = 0
    );

    /// Retrieve cached statistics for a table (no re-scan).
    /// Loads from RocksDB persistence if not already in memory.
    /// @param table_name  Table/collection name
    StatsResult<TableStats> getStats(std::string_view table_name);

    /// Force a fresh collection pass and update persisted statistics.
    /// Equivalent to collectStats() but returns only a success/failure bool.
    /// @param table_name  Table/collection name
    StatsResult<bool> updateStats(std::string_view table_name);

    /// Remove cached and persisted statistics for a table.
    /// @param table_name  Table/collection name
    StatsResult<bool> clearStats(std::string_view table_name);

    /// Export all cached statistics as a JSON object.
    json toJSON() const;

private:
    // ========================================================================
    // Internal helpers
    // ========================================================================

    /// Build column statistics from a set of sampled raw values.
    ColumnStats buildColumnStats(
        std::string_view column_name,
        const std::vector<std::string>& values,
        size_t num_histogram_buckets = kDefaultHistogramBuckets
    );

    /// Build an equi-height histogram from a sorted list of double values.
    std::vector<HistogramBucket> buildHistogram(
        const std::vector<double>& sorted_values,
        size_t num_buckets
    );

    /// Persist TableStats to RocksDB under "stats:<table_name>".
    void persistStats(const TableStats& stats);

    /// Load TableStats from RocksDB; returns nullopt if not found.
    std::optional<TableStats> loadStats(std::string_view table_name);

    // ========================================================================
    // Member variables
    // ========================================================================

    RocksDBWrapper& db_;                                        ///< Database reference
    std::map<std::string, TableStats> stats_cache_;             ///< In-memory cache
    mutable std::shared_mutex cache_mutex_;                     ///< Read-write lock
};

} // namespace themis
