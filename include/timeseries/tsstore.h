/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            tsstore.h                                          ║
  Version:         0.0.21                                             ║
  Last Modified:   2026-02-21 19:20:04                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     293                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#ifndef THEMIS_TSSTORE_H
#define THEMIS_TSSTORE_H

#include <string>
#include <vector>
#include <optional>
#include <cstdint>
#include <memory>
#include <nlohmann/json.hpp>
#include "utils/expected.h"

// Forward declarations for RocksDB types
namespace rocksdb {
    class TransactionDB;
    class ColumnFamilyHandle;
}

namespace themis {

// Forward declaration
class TimeSeriesMetrics;

/**
 * @brief Time-Series Storage MVP (Sprint B)
 * 
 * Simple time-series storage for metrics, logs, and events with range queries
 * and aggregations. Designed for observability and monitoring use cases.
 * 
 * Key Schema: "{metric_name}:{entity_id}:{timestamp_ms}"
 * Value: JSON with fields: {"value": double, "tags": {...}, "metadata": {...}}
 * 
 * Features:
 * - Range queries by time interval
 * - Aggregations: min, max, avg, count, sum
 * - Tag-based filtering
 * - Efficient RocksDB range scans
 * 
 * MVP Scope (No automatic downsampling/retention yet):
 * - Raw data storage
 * - Basic aggregations computed on-the-fly
 * - Manual retention via deleteOldData()
 * 
 * Future Enhancements:
 * - Continuous aggregates (materialized views)
 * - Automatic retention policies
 * - Downsampling (1m → 1h → 1d)
 * 
 * Compression:
 * - Gorilla compression for float64 time-series (10-20x ratio, +15% CPU)
 * - Configurable per-metric compression strategy
 * - NOTE: Compression only applies to batch inserts (putDataPoints), not single inserts (putDataPoint)
 * 
 * Storage Methods:
 * - Single-point inserts: Stored as individual RocksDB entities (ts:{metric}:{entity}:{timestamp})
 * - Batch inserts with compression: Stored as compressed chunks (tsc:{metric}:{entity}:{first_ts}:{last_ts})
 * - See documentation for details on choosing the appropriate method
 */
class TSStore {
public:
    enum class CompressionType {
        None,       // No compression (raw JSON)
        Gorilla     // Gorilla codec for time-series (10-20x ratio)
    };
    
    struct Config {
        // Default to Gorilla compression for better storage efficiency
        CompressionType compression = CompressionType::Gorilla;
        int chunk_size_hours = 24;  // Gorilla chunk size (hours)
    };
    
    struct DataPoint {
        std::string metric;           // Metric name (e.g., "cpu_usage")
        std::string entity;           // Entity ID (e.g., "server01")
        int64_t timestamp_ms;         // Timestamp in milliseconds since epoch
        double value;                 // Numeric value
        nlohmann::json tags;          // Tags for filtering (e.g., {"region": "us-east", "env": "prod"})
        nlohmann::json metadata;      // Additional metadata
        
        // Serialization
        nlohmann::json toJson() const;
        static DataPoint fromJson(const nlohmann::json& j);
    };
    
    struct QueryOptions {
        std::string metric;           // Required: metric name
        std::optional<std::string> entity; // Optional: filter by entity
        int64_t from_timestamp_ms = 0; // Start of time range (inclusive)
        int64_t to_timestamp_ms = INT64_MAX; // End of time range (inclusive)
        size_t limit = 1000;          // Max data points to return
        nlohmann::json tag_filter;    // Optional: filter by tags (exact match)
    };
    
    struct AggregationResult {
        double min = 0.0;
        double max = 0.0;
        double avg = 0.0;
        double sum = 0.0;
        size_t count = 0;
        int64_t first_timestamp_ms = 0;
        int64_t last_timestamp_ms = 0;
    };
    
    struct Stats {
        size_t total_data_points = 0;
        size_t total_metrics = 0;      // Number of unique metrics
        size_t total_size_bytes = 0;
        int64_t oldest_timestamp_ms = 0;
        int64_t newest_timestamp_ms = 0;
    };
    
    /**
     * @brief Construct TSStore
     * @param db RocksDB TransactionDB instance (not owned)
     * @param cf Optional column family handle (nullptr = default CF)
     * @param config Compression and storage configuration
     */
    // Main constructor (explicit): accepts DB, optional CF and Config
    explicit TSStore(rocksdb::TransactionDB* db, 
                     rocksdb::ColumnFamilyHandle* cf,
                     Config config);
    // Convenience ctor: use default Config
    TSStore(rocksdb::TransactionDB* db, rocksdb::ColumnFamilyHandle* cf = nullptr);
    
    ~TSStore() = default;
    
    /**
     * @brief Get current compression configuration
     */
    const Config& getConfig() const { return config_; }
    
    /**
     * @brief Update compression configuration
     * @note Changes only affect new data points; existing data remains unchanged
     */
    void setConfig(const Config& config) { config_ = config; }
    
    /**
     * @brief Write a data point
     * @param point Data point to store
     * @return Result<void> - success or error with detailed context
     * 
     * @note STORAGE METHOD: Singular RocksDB Entity
     * Single data points are always stored as individual RocksDB entities
     * with key format ts:{metric}:{entity}:{timestamp_ms}, regardless of
     * the compression configuration. For Gorilla compression, use putDataPoints()
     * with multiple points for batch compression.
     */
    Result<void> putDataPoint(const DataPoint& point);
    
    /**
     * @brief Write multiple data points (batch operation)
     * @param points Vector of data points
     * @return Result<void> - success or error with detailed context
     * 
     * @note STORAGE METHOD: Batch with Gorilla Compression (if enabled)
     * When compression is enabled (config.compression = CompressionType::Gorilla),
     * points are grouped by metric:entity, sorted by timestamp, and compressed
     * as chunks with key format tsc:{metric}:{entity}:{first_ts}:{last_ts}.
     * This provides 10-20x compression ratio. Without compression, points are
     * stored as individual entities like putDataPoint().
     */
    Result<void> putDataPoints(const std::vector<DataPoint>& points);
    
    /**
     * @brief Query data points with filters
     * @param options Query options (time range, entity, tags)
     * @return Result<std::vector<DataPoint>> - data points or error
     */
    Result<std::vector<DataPoint>> query(const QueryOptions& options) const;
    
    /**
     * @brief Compute aggregations over time range
     * @param options Query options
     * @return Result<AggregationResult> - aggregation result or error
     * 
     * Automatically uses pre-computed aggregates when available for better performance.
     */
    Result<AggregationResult> aggregate(const QueryOptions& options) const;
    
    /**
     * @brief Compute aggregations with optimizer hints
     * @param options Query options
     * @param use_optimizer Enable query optimization (default: true)
     * @return Result<AggregationResult> - aggregation result or error
     */
    Result<AggregationResult> aggregateOptimized(
        const QueryOptions& options,
        bool use_optimizer = true) const;
    
    /**
     * @brief Get time-series statistics
     * @return Stats struct
     */
    Stats getStats() const;
    
    /**
     * @brief Delete data older than specified timestamp (retention policy)
     * @param before_timestamp_ms Delete data points with timestamp < this value
     * @return Number of data points deleted
     */
    size_t deleteOldData(int64_t before_timestamp_ms);

    /**
     * @brief Delete old data for a specific metric (retention policy)
     * @param metric Metric name
     * @param before_timestamp_ms Delete data points with timestamp < this value
     * @return Number of data points deleted for that metric
     */
    size_t deleteOldDataForMetric(const std::string& metric, int64_t before_timestamp_ms);
    
    /**
     * @brief Delete all data for a specific metric
     * @param metric Metric name
     * @return Result<void> - success or error
     */
    Result<void> deleteMetric(const std::string& metric);
    
    /**
     * @brief Clear all time-series data (admin operation)
     */
    void clear();
    
    /**
     * @brief Set metrics collector for monitoring
     * @param metrics Shared pointer to TimeSeriesMetrics instance
     */
    void setMetrics(std::shared_ptr<TimeSeriesMetrics> metrics);
    
    /**
     * @brief Get metrics collector
     * @return Shared pointer to TimeSeriesMetrics instance (may be null)
     */
    std::shared_ptr<TimeSeriesMetrics> getMetrics() const { return metrics_; }

private:
    rocksdb::TransactionDB* db_;
    rocksdb::ColumnFamilyHandle* cf_;
    Config config_;
    std::shared_ptr<TimeSeriesMetrics> metrics_; // Optional metrics collector
    
    static constexpr const char* KEY_PREFIX = "ts:";
    static constexpr const char* GORILLA_CHUNK_PREFIX = "tsc:";
    
    // Key format: "ts:{metric}:{entity}:{timestamp_ms}"
    std::string makeKey(const std::string& metric, 
                       const std::string& entity, 
                       int64_t timestamp_ms) const;
    
    // Parse key to extract components
    struct KeyComponents {
        std::string metric;
        std::string entity;
        int64_t timestamp_ms;
    };
    // Internal helper that returns std::optional for compatibility
    std::optional<KeyComponents> parseKeyInternal(const std::string& key) const;
    
    // Public Result-based API
    Result<KeyComponents> parseKey(const std::string& key) const;
    
    // Check if data point matches tag filter
    bool matchesTagFilter(const DataPoint& point, const nlohmann::json& tag_filter) const;
};

} // namespace themis

#endif // THEMIS_TSSTORE_H
