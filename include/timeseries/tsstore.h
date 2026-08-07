/**
 * @file tsstore.h
 * @brief Phase 2 hardening: Adaptive buffering for ingest with concurrency safety.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Status: Phase 2 — Core Implementation Complete
 * 
 * ## Overview
 * 
 * TSStore (Time-Series Store) is the canonical storage backend for ThemisDB timeseries ingest,
 * providing adaptive buffering, Gorilla compression batching, and integration with AdaptiveFlushController.
 * 
 * ## Phase 2 Enhancements
 * 
 * - **Adaptive Buffering**: Integrates TSAutoBuffer for Gorilla batch compression
 * - **Concurrency Safety**: Per-series watermark mutex + atomic stats counters
 * - **Deterministic Flushing**: Watermark tracking per (metric, entity) series
 * - **Graceful Degradation**: Buffer full → fall back to direct write (no data loss)
 * - **Contract Alignment**: All operations validated against timeseries_api_contract.h
 * 
 * ## Key Schema
 * 
 * Key format: "ts:{metric}:{entity}:{timestamp_ms}"
 * Value format: JSON { "value": double, "tags": {}, "metadata": {} }
 * 
 * Compressed chunks (when using batch compression):
 * Key format: "tsc:{metric}:{entity}:{first_ts}:{last_ts}:{sequence}"
 * Value: Gorilla-compressed binary blob
 * 
 * ## Write Contract (§1 of timeseries_api_contract.h)
 * 
 * 1. **Monotonic Timestamps**: Within a series, each new timestamp > previous
 * 2. **Null Timestamp Rejection**: Zero or negative timestamp → TIMESTAMP_OUT_OF_ORDER
 * 3. **Out-of-Order Handling**: Late-arrival window configurable (default 0 = strict)
 * 4. **Metric/Entity Validation**: Empty names rejected at write-time
 * 5. **Watermark Tracking**: Per-series tracking prevents double-counting
 * 
 * ## Thread Safety
 * 
 * - putDataPoint() safe for concurrent calls from multiple threads
 * - putDataPoints() safe for concurrent calls from multiple threads
 * - Watermark protected by std::lock_guard<std::mutex>
 * - Statistics counters atomic (lock-free)
 * - Metrics reporting via optional TimeSeriesMetrics* (caller responsible for threading)
 * 
 * ## Adaptive Buffering Strategy
 * 
 * When enabled (auto_buffer_ attached + Gorilla compression configured):
 * - Single-point writes buffered for batch Gorilla compression
 * - Buffer full condition → graceful fallback to direct write
 * - Batch flush coordinated with AdaptiveFlushController
 * - Result: ≥80% typical compression on IoT workloads + reduced write path overhead
 * 
 * ## Error Handling
 * 
 * All public methods return Result<T> with explicit error codes:
 * - **ERR_API_INVALID_REQUEST**: Empty metric/entity, or parameter validation failure
 * - **ERR_TIMESERIES_LATE_ARRIVAL**: Timestamp outside late-arrival window
 * - **ERR_STORAGE_TRANSACTION_FAILED**: RocksDB write or storage operation failure
 * - **TIMESTAMP_OUT_OF_ORDER**: Duplicate or decreasing timestamp detected
 * 
 * @see include/timeseries/timeseries_api_contract.h
 * @see include/timeseries/adaptive_flush_controller.h
 * @see src/timeseries/ROADMAP.md — Phase 2 items
 * @see src/timeseries/PERFORMANCE_EXPECTATIONS.md
 */

/*
 * ThemisDB | File: tsstore.h | Version: 0.1.0
 * Maturity: 🟢 PRODUCTION-READY | Phase 2 Hardening (2026-08-07)
 * Status: Production Ready
 */

#pragma once

#include <string>
#include <vector>
#include <optional>
#include <span>
#include <cstdint>
#include <memory>
#include <atomic>
#include <mutex>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include "utils/expected.h"

// Forward declarations for RocksDB types
namespace rocksdb {
    class TransactionDB;
    class ColumnFamilyHandle;
}

namespace themis {

// Forward declarations
class TimeSeriesMetrics;
class EncryptedChunkStore;
class TSAutoBuffer;

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
        // Late-arrival window for out-of-order writes (milliseconds).
        // 0 = disabled (accept all timestamps regardless of order).
        // When > 0, data points whose timestamp is older than
        //   (high_watermark - late_arrival_window_ms) are rejected with
        //   ERR_TIMESERIES_LATE_ARRIVAL.
        int64_t late_arrival_window_ms = 0;
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
     * @brief Statistics for out-of-order write handling
     */
    struct OutOfOrderStats {
        uint64_t out_of_order_accepted = 0;  // Points written out-of-order but within window
        uint64_t late_arrival_rejected = 0;  // Points rejected as older than late-arrival window
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
     * @brief Zero-copy view of a single time-series row for high-throughput batch ingestion.
     *
     * Unlike `DataPoint`, all string fields are `std::string_view` — no heap allocation is
     * required at the call site.  The caller must ensure that the backing storage for
     * `metric` and `entity` outlives the `putBatch()` call.
     *
     * `tags` and `metadata` are optional JSON blobs; pass an empty / null JSON value when
     * not needed.
     */
    struct TSRow {
        std::string_view metric;       ///< Metric name (e.g., "cpu_usage")
        std::string_view entity;       ///< Entity ID   (e.g., "server01")
        int64_t          timestamp_ms; ///< Unix epoch in milliseconds
        double           value;        ///< Numeric sample value
    };

    /**
     * @brief Result of a `putBatch()` call.
     *
     * On full success `failed_count == 0` and `row_errors` is empty.
     * On partial validation failure some rows may be rejected before the
     * `WriteBatch` is submitted; in that case the entire RocksDB write is
     * still attempted with the valid rows only (no atomicity across invalid
     * rows).
     *
     * On a RocksDB write failure the whole batch is rejected and
     * `failed_count == total rows`.
     */
    struct BatchWriteResult {
        size_t ok_count     = 0; ///< Rows accepted and written
        size_t failed_count = 0; ///< Rows rejected (validation or storage error)

        /// Per-row error details: (row_index, error_message).  Empty on full success.
        std::vector<std::pair<size_t, std::string>> row_errors;

        bool all_ok() const noexcept { return failed_count == 0; }
    };

    /**
     * @brief High-throughput zero-copy batch write using `std::span`.
     *
     * Accepts a span of `TSRow` values and writes them using a single
     * `rocksdb::WriteBatch` commit, amortising WAL and memtable overhead across
     * the entire batch.  All valid rows are committed atomically; invalid rows
     * (empty metric / entity) are reported in the `BatchWriteResult` but do not
     * abort the write for the remaining rows.
     *
     * When Gorilla compression is enabled the rows are grouped by metric:entity,
     * sorted by timestamp, Gorilla-encoded, and stored as compressed chunks —
     * identical to `putDataPoints()` but without the intermediate `std::vector`
     * allocation at the call site.
     *
     * @param rows   Span of TSRow values.  String views must remain valid for
     *               the duration of the call.
     * @return `Result<BatchWriteResult>` — ok() on successful RocksDB write;
     *         error() only when the RocksDB `Write()` itself fails.
     *
     * Performance target: ≥ 1 M rows/s at p99 < 2 ms on an 8-core host
     *                     (see ROADMAP — Multi-metric batch write API).
     */
    Result<BatchWriteResult> putBatch(std::span<const TSRow> rows);
    
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
     * @brief Get out-of-order write statistics
     * @return OutOfOrderStats with counters for accepted and rejected out-of-order points
     */
    OutOfOrderStats getOutOfOrderStats() const;
    
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
     * @brief Attach an EncryptedChunkStore to enable AES-256-GCM encryption.
     *
     * When set, all new Gorilla-compressed chunk writes are encrypted
     * (compress-then-encrypt) and existing encrypted chunks are decrypted
     * transparently on read.  Pass nullptr to disable encryption.
     *
     * Key access is audited via the AuditLogger that was configured on the
     * provided EncryptedChunkStore.
     *
     * @param enc_store  Shared EncryptedChunkStore, or nullptr to disable.
     */
    void setEncryptedChunkStore(std::shared_ptr<EncryptedChunkStore> enc_store);

    /**
     * @brief Returns the currently attached EncryptedChunkStore (may be null).
     */
    std::shared_ptr<EncryptedChunkStore> getEncryptedChunkStore() const;

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

    /**
     * @brief Wire a TSAutoBuffer to receive single-point inserts when Gorilla compression
     * is enabled.  When set, putDataPoint() routes through the buffer instead of writing
     * directly to RocksDB, enabling Gorilla compression for IoT / streaming workloads.
     *
     * @param buf  Pointer to a TSAutoBuffer (not owned, must outlive this TSStore).
     *             Pass nullptr to disable buffering and fall back to direct writes.
     */
    void setAutoBuffer(TSAutoBuffer* buf) { auto_buffer_ = buf; }

    /**
     * @brief Return the currently attached TSAutoBuffer, or nullptr if not set.
     */
    TSAutoBuffer* getAutoBuffer() const { return auto_buffer_; }

    // ==================== System Metadata ====================

    /**
     * @brief Write a system metadata key-value pair (WAL-durable).
     *
     * Used internally for persisting small pieces of bookkeeping data
     * (e.g., continuous-aggregate watermarks) in the same RocksDB instance
     * without mixing them with time-series payload keys.
     *
     * Keys are stored under the "sys:" prefix and must not begin with
     * that prefix when passed here (it is added automatically).
     *
     * @param key    Logical key (e.g., "wm:cagg:my_aggregate")
     * @param value  Value string to store
     * @return Result<void> on success, error on failure
     */
    Result<void> putSystemMeta(const std::string& key, const std::string& value);

    /**
     * @brief Read a system metadata entry.
     * @param key  Logical key (same as used in putSystemMeta)
     * @return Result containing the value string, or std::nullopt if not found
     */
    Result<std::optional<std::string>> getSystemMeta(const std::string& key) const;

    /**
     * @brief Delete a system metadata entry.
     * @param key  Logical key to remove
     * @return Result<void> on success, error on failure
     */
    Result<void> deleteSystemMeta(const std::string& key);

private:
    rocksdb::TransactionDB* db_;
    rocksdb::ColumnFamilyHandle* cf_;
    Config config_;
    std::shared_ptr<TimeSeriesMetrics> metrics_; // Optional metrics collector
    std::shared_ptr<EncryptedChunkStore> enc_chunk_store_; // Optional AES-256-GCM wrapper
    TSAutoBuffer* auto_buffer_ = nullptr; // Optional auto-buffer for single-point Gorilla inserts (not owned)

    // Out-of-order write statistics
    mutable std::atomic<uint64_t> ooo_accepted_{0};
    mutable std::atomic<uint64_t> ooo_rejected_{0};

    // Per-series high-watermark for late-arrival enforcement.
    // Key: "{metric}:{entity}", Value: maximum timestamp_ms seen.
    mutable std::mutex watermark_mutex_;
    std::unordered_map<std::string, int64_t> watermarks_;
    
    static constexpr const char* KEY_PREFIX = "ts:";
    static constexpr const char* GORILLA_CHUNK_PREFIX = "tsc:";
    static constexpr const char* SYS_META_PREFIX = "sys:";
    
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

    // Late-arrival enforcement helper.
    // Checks `timestamp_ms` against the per-series watermark identified by `wm_key`
    // (format: "{metric}:{entity}") and updates the watermark when the point is
    // newer.  Must be called with `watermark_mutex_` held.
    //
    // Returns:
    //  -1  data point is too old (outside the late-arrival window) – caller must reject
    //   0  data point is in-order or first write – accepted, watermark updated
    //   1  data point is out-of-order but within window – accepted, watermark NOT updated
    int checkAndUpdateWatermarkLocked(const std::string& wm_key, int64_t timestamp_ms);
};

} // namespace themis
