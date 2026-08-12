/**
 * @file statistics_collector.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <optional>
#include <memory>
#include <atomic>
#include <chrono>
#include <algorithm>
#include <cmath>
#include <condition_variable>
#include <mutex>
#include <shared_mutex>
#include <thread>
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

/// Upper-bound epsilon added to histogram's last bucket to make it right-closed
inline constexpr double kHistogramUpperBoundEpsilon = 1e-9;

/// Tolerance for floating-point comparisons (e.g., "all values identical")
inline constexpr double kFloatingPointTolerance = 1e-12;

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

    /// Estimate selectivity for a range predicate [low, high] using the histogram.
    /// Returns fraction of rows in [low, high]; falls back to uniform distribution
    /// when no histogram is available.
    double estimateRangeSelectivity(double low, double high) const {
        if (!histogram.has_value() || histogram->empty()) {
            // Fallback: uniform distribution over [min_value, max_value]
            if (!min_value.has_value() || !max_value.has_value() ||
                std::abs(*max_value - *min_value) < kFloatingPointTolerance) {
                return selectivity;
            }
            double range       = *max_value - *min_value;
            double query_range = std::max(0.0,
                std::min(high, *max_value) - std::max(low, *min_value));
            return query_range / range;
        }

        double total_freq    = 0.0;
        double matching_freq = 0.0;
        for (const auto& b : *histogram) {
            total_freq += static_cast<double>(b.frequency);
            double b_lo = b.lower_bound;
            double b_hi = b.upper_bound;
            if (b_hi <= low || b_lo >= high) {
                continue; // No overlap
            }
            double overlap_lo   = std::max(b_lo, low);
            double overlap_hi   = std::min(b_hi, high);
            double bucket_width = b_hi - b_lo;
            if (bucket_width < kFloatingPointTolerance) {
                matching_freq += static_cast<double>(b.frequency);
            } else {
                double overlap_frac = (overlap_hi - overlap_lo) / bucket_width;
                matching_freq += static_cast<double>(b.frequency) * overlap_frac;
            }
        }
        if (total_freq <= 0.0) return selectivity;
        return std::max(0.0, std::min(1.0, matching_freq / total_freq));
    }
};

/// Per-index statistics exported from the index module to the metadata module
struct IndexStats {
    std::string table;                              ///< Table/collection name
    std::string column;                             ///< Column name (or col1+col2 for composite)
    std::string type;                               ///< "regular", "range", "sparse", "geo", "ttl", "fulltext", "composite"
    size_t entry_count = 0;                         ///< Number of index entries
    size_t estimated_size_bytes = 0;                ///< Estimated storage size in bytes
    bool unique = false;                            ///< Unique constraint
    std::string additional_info;                    ///< Type-specific info (e.g., "sorted", "geohash", "ttl_seconds=3600")
    std::chrono::system_clock::time_point last_updated; ///< Timestamp of last export

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

    // ========================================================================
    // Metrics hook interface (Prometheus / OpenTelemetry integration)
    // ========================================================================

    /// Lightweight hook interface for emitting Prometheus / OTel metrics.
    ///
    /// Implement this interface and call setMetricsHook() to receive real-time
    /// stats events.  A no-op default implementation is used when not set.
    ///
    /// Metric names follow the `themis_stats_*` convention:
    ///   - `themis_stats_collection_duration_ms`   – time for one full scan
    ///   - `themis_stats_cache_hits_total`          – in-memory cache hits
    ///   - `themis_stats_errors_total`              – collection errors
    struct IMetricsHook {
        virtual ~IMetricsHook() = default;

        /// Called after a successful or failed stats collection attempt.
        /// @param table_name   Affected table
        /// @param duration_ms  Wall-clock duration in milliseconds
        /// @param rows_sampled Number of rows actually scanned
        /// @param success      Whether collection succeeded
        virtual void onCollect(std::string_view table_name,
                               double duration_ms,
                               size_t rows_sampled,
                               bool   success) = 0;

        /// Called whenever in-memory cache satisfies a getStats() request.
        virtual void onCacheHit(std::string_view table_name) = 0;

        /// Called whenever getStats() results in a cache miss (loads from RocksDB or re-collects).
        virtual void onCacheMiss(std::string_view table_name) = 0;

        /// Called on any internal error (iterator failure, parse error, etc.).
        /// @param error_code  StatsErrorCode cast to int
        virtual void onError(std::string_view table_name, int error_code) = 0;
    };

    /// Attach a metrics hook.  The pointer is non-owning; caller manages lifetime.
    /// Pass nullptr to remove the hook.
    void setMetricsHook(IMetricsHook* hook) noexcept { metrics_hook_ = hook; }

    /// Constructor
    /// @param db RocksDB wrapper for key scanning and persisting statistics
    explicit StatisticsCollector(RocksDBWrapper& db);

    /// Destructor – stops the background refresh thread if running.
    ~StatisticsCollector();

    // Disable copy and move (mutex/condition_variable are not movable)
    StatisticsCollector(const StatisticsCollector&) = delete;
    StatisticsCollector& operator=(const StatisticsCollector&) = delete;
    StatisticsCollector(StatisticsCollector&&) = delete;
    StatisticsCollector& operator=(StatisticsCollector&&) = delete;

    // ========================================================================
    // Auto-refresh
    // ========================================================================

    /// Configure the background auto-refresh interval.
    ///
    /// When @p interval > 0, a background thread wakes every @p interval seconds
    /// and calls collectStats() for every table that has already been sampled at
    /// least once.  Calling this again updates the interval live.
    /// Passing std::chrono::seconds(0) (the default) stops the background thread.
    ///
    /// The background thread does NOT block the caller; it runs at low priority
    /// and skips a table if collectStats() is already running for it.
    ///
    /// Thread-safety: safe to call from any thread.
    void setRefreshInterval(std::chrono::seconds interval);

    /// Stop the background refresh thread immediately (blocking until it exits).
    /// Called automatically by the destructor.
    void stopRefresh() noexcept;

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

    /// Import index statistics for a table exported from the index module.
    /// Stores the stats in-memory and persists them to RocksDB under
    /// "idxstats:<table_name>".
    /// @param table_name  Table/collection name
    /// @param stats       Index stats to import (replaces any previously cached stats)
    StatsResult<bool> importIndexStats(
        std::string_view table_name,
        const std::vector<IndexStats>& stats
    );

    /// Retrieve cached index statistics for a table.
    /// Loads from RocksDB persistence if not already in memory.
    /// @param table_name  Table/collection name
    StatsResult<std::vector<IndexStats>> getIndexStats(std::string_view table_name);

    /// Remove cached and persisted index statistics for a table.
    /// @param table_name  Table/collection name
    StatsResult<bool> clearIndexStats(std::string_view table_name);

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

    /// Persist index stats to RocksDB under "idxstats:<table_name>".
    void persistIndexStats(std::string_view table_name, const std::vector<IndexStats>& stats);

    /// Load index stats from RocksDB; returns nullopt if not found.
    std::optional<std::vector<IndexStats>> loadIndexStats(std::string_view table_name);

    // ========================================================================
    // Member variables
    // ========================================================================

    RocksDBWrapper&  db_;                                        ///< Database reference
    std::map<std::string, TableStats> stats_cache_;              ///< In-memory cache
    std::map<std::string, std::vector<IndexStats>> index_stats_cache_; ///< In-memory index stats cache
    mutable std::shared_mutex cache_mutex_;                      ///< Read-write lock
    IMetricsHook* metrics_hook_ = nullptr;                       ///< Optional metrics sink (non-owning)

    // ========================================================================
    // Auto-refresh members
    // ========================================================================
    std::chrono::seconds   refresh_interval_{0};     ///< 0 = disabled
    std::thread            refresh_thread_;           ///< Background refresh thread
    std::atomic<bool>      stop_refresh_{false};      ///< Signal to terminate
    std::condition_variable refresh_cv_;              ///< Wakes thread on interval/stop
    std::mutex             refresh_mutex_;            ///< Guards refresh_cv_

    /// Background refresh loop (runs on refresh_thread_).
    void refreshLoop_();
};

} // namespace themis
