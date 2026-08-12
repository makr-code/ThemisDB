/**
 * @file ts_auto_buffer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB Time Series Auto-Batching Buffer
 * 
 * Automatic buffering of single data points for batch compression.
 * Inspired by CEPEngine event buffering and BackpressureProtocol adaptive strategies.
 * 
 * Features:
 * - Configurable buffer size and flush intervals
 * - Per-metric:entity buffering
 * - Thread-safe operation
 * - Automatic flush on size/time thresholds
 * - Manual flush support
 * - Integration with existing TSStore
 * 
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "timeseries/tsstore.h"
#include <deque>
#include <map>
#include <mutex>
#include <atomic>
#include <thread>
#include <condition_variable>
#include <chrono>
#include <memory>
#include <string>
#include <cstddef>
#include <algorithm>

namespace themis {

class TimeSeriesMetrics; // forward declaration – caller includes timeseries_metrics.h if needed

/**
 * @brief Configuration for time series auto-batching
 */
struct TSAutoBufferConfig {
    // Buffer size thresholds
    size_t max_points_per_buffer = 1000;      // Max points per metric:entity before flush
    size_t max_total_points = 10000;          // Max total buffered points across all metrics
    
    // Time-based flush
    std::chrono::milliseconds flush_interval{5000};  // Auto-flush every 5 seconds
    
    // Memory management
    size_t max_memory_bytes = 100 * 1024 * 1024;  // 100 MB max buffer memory
    size_t max_memory_per_metric_bytes = 0;        // 0 = unlimited; per-metric memory cap

    // Deduplication
    bool enable_dedup = false;                // Deduplicate points with identical timestamp
    
    // Performance tuning
    bool async_flush = true;                  // Flush in background thread
    size_t flush_batch_size = 500;           // Points per flush operation
    
    // Gorilla single-point insert buffering
    size_t gorilla_batch_size = 128;          // Accumulate this many points before Gorilla-encoding
    // Non-blocking backpressure threshold for push(): returns BUFFER_FULL when total
    // in-memory buffer bytes exceed this value. 0 = disabled (push() never returns BUFFER_FULL).
    size_t max_buffer_bytes = 0;

    // Compression (inherited from TSStore)
    TSStore::CompressionType compression = TSStore::CompressionType::Gorilla;
    int chunk_size_hours = 24;

    // Adaptive flush with backpressure signalling
    bool enable_adaptive_flush = false;       // Enable FlushController adaptive batching
    double backpressure_slo_ms = 50.0;       // Write-latency SLO (ms); above this triggers backpressure
    double ewma_alpha = 0.1;                 // EWMA smoothing factor for latency estimation
    size_t adaptive_batch_min = 100;         // Minimum adaptive batch size
    size_t adaptive_batch_max = 5000;        // Maximum adaptive batch size
    size_t backpressure_high_watermark = 8000; // Total buffered points above which producers block
    size_t backpressure_low_watermark = 2000;  // Total buffered points below which blocked producers resume

    // A buffer is considered "overdue" when its oldest point is older than
    // flush_interval * overdue_flush_multiplier.  Overdue buffers emit a metrics
    // alert via TimeSeriesMetrics::recordOverdueFlush() and a WARN log.
    unsigned overdue_flush_multiplier = 2;   // Default: 2× the flush interval

    // Optional metrics integration – when set, backpressure events and overdue flushes are
    // reported via TimeSeriesMetrics::recordBackpressure() / recordOverdueFlush().
    // Not owned; must outlive the TSAutoBuffer.
    TimeSeriesMetrics* metrics = nullptr;
};

/**
 * @brief Statistics for auto-batching buffer
 */
struct TSAutoBufferStats {
    std::atomic<uint64_t> points_buffered{0};
    std::atomic<uint64_t> points_flushed{0};
    std::atomic<uint64_t> flush_count{0};
    std::atomic<uint64_t> auto_flush_count{0};
    std::atomic<uint64_t> manual_flush_count{0};
    std::atomic<uint64_t> size_triggered_flush{0};
    std::atomic<uint64_t> time_triggered_flush{0};
    std::atomic<uint64_t> buffer_overflow_count{0};
    std::atomic<uint64_t> dedup_dropped_count{0};         // Points dropped by deduplication
    std::atomic<uint64_t> memory_limit_rejected_count{0}; // Points rejected due to per-metric limit
    std::atomic<uint64_t> backpressure_events{0};         // Times producers were blocked by backpressure

    size_t current_buffer_size{0};
    size_t current_buffer_memory{0};
    double current_ewma_latency_ms{0.0};     // Latest EWMA latency (FlushController)
    size_t current_adaptive_batch_size{0};   // Latest adaptive batch size (FlushController)
    
    std::chrono::steady_clock::time_point last_flush_time;

    TSAutoBufferStats() = default;

    TSAutoBufferStats(const TSAutoBufferStats& other)
        : points_buffered(other.points_buffered.load())
        , points_flushed(other.points_flushed.load())
        , flush_count(other.flush_count.load())
        , auto_flush_count(other.auto_flush_count.load())
        , manual_flush_count(other.manual_flush_count.load())
        , size_triggered_flush(other.size_triggered_flush.load())
        , time_triggered_flush(other.time_triggered_flush.load())
        , buffer_overflow_count(other.buffer_overflow_count.load())
        , dedup_dropped_count(other.dedup_dropped_count.load())
        , memory_limit_rejected_count(other.memory_limit_rejected_count.load())
        , backpressure_events(other.backpressure_events.load())
        , current_buffer_size(other.current_buffer_size)
        , current_buffer_memory(other.current_buffer_memory)
        , current_ewma_latency_ms(other.current_ewma_latency_ms)
        , current_adaptive_batch_size(other.current_adaptive_batch_size)
        , last_flush_time(other.last_flush_time) {}

    TSAutoBufferStats& operator=(const TSAutoBufferStats& other) {
        if (this != &other) {
            points_buffered.store(other.points_buffered.load());
            points_flushed.store(other.points_flushed.load());
            flush_count.store(other.flush_count.load());
            auto_flush_count.store(other.auto_flush_count.load());
            manual_flush_count.store(other.manual_flush_count.load());
            size_triggered_flush.store(other.size_triggered_flush.load());
            time_triggered_flush.store(other.time_triggered_flush.load());
            buffer_overflow_count.store(other.buffer_overflow_count.load());
            dedup_dropped_count.store(other.dedup_dropped_count.load());
            memory_limit_rejected_count.store(other.memory_limit_rejected_count.load());
            backpressure_events.store(other.backpressure_events.load());
            current_buffer_size = other.current_buffer_size;
            current_buffer_memory = other.current_buffer_memory;
            current_ewma_latency_ms = other.current_ewma_latency_ms;
            current_adaptive_batch_size = other.current_adaptive_batch_size;
            last_flush_time = other.last_flush_time;
        }
        return *this;
    }
};

/**
 * @brief Auto-batching buffer for time series data points
 * 
 * Automatically buffers single data points and flushes them as compressed batches.
 * Thread-safe, with configurable size and time thresholds.
 * 
 * Usage:
 * @code
 * TSAutoBufferConfig config;
 * config.max_points_per_buffer = 500;
 * config.flush_interval = std::chrono::seconds(10);
 * 
 * TSAutoBuffer buffer(tsstore, config);
 * buffer.start();  // Start background flush thread
 * 
 * // Add points (will be buffered)
 * buffer.add(point1);
 * buffer.add(point2);
 * // ... points are automatically flushed in batches
 * 
 * buffer.stop();   // Stop and flush remaining points
 * @endcode
 */
class TSAutoBuffer {
public:
    /**
     * @brief Status returned by push() for non-blocking single-point inserts.
     */
    enum class PushStatus {
        OK,           ///< Point accepted and buffered successfully
        BUFFER_FULL,  ///< Total in-memory buffer bytes exceed config_.max_buffer_bytes; caller should back off
        INVALID_INPUT ///< Point has empty metric or entity; permanent error, do not retry
    };

    /**
     * @brief Construct auto-batching buffer
     * @param tsstore TSStore instance (not owned)
     * @param config Buffer configuration
     */
    explicit TSAutoBuffer(TSStore* tsstore, TSAutoBufferConfig config = TSAutoBufferConfig{});
    
    ~TSAutoBuffer();
    
    // Non-copyable, non-movable (contains threads)
    TSAutoBuffer(const TSAutoBuffer&) = delete;
    TSAutoBuffer& operator=(const TSAutoBuffer&) = delete;
    TSAutoBuffer(TSAutoBuffer&&) = delete;
    TSAutoBuffer& operator=(TSAutoBuffer&&) = delete;
    
    /**
     * @brief Start background flush thread
     */
    void start();
    
    /**
     * @brief Stop background flush thread and flush remaining points
     */
    void stop();
    
    /**
     * @brief Non-blocking single-point push for Gorilla single-point insert buffering.
     *
     * Routes single data points through the auto-buffer rather than writing directly
     * to RocksDB.  Points accumulate until gorilla_batch_size is reached, at which
     * point they are encoded with Gorilla and written as a single compressed chunk.
     *
     * Unlike add(), this method never blocks producers.  When the total in-memory
     * buffer size exceeds config_.max_buffer_bytes it returns BUFFER_FULL so the
     * caller can apply its own backpressure strategy.
     *
     * @param point Data point to buffer
     * @return PushStatus::OK on success, PushStatus::BUFFER_FULL when buffer is saturated
     */
    PushStatus push(const TSStore::DataPoint& point);

    /**
     * @brief Add a data point (will be buffered)
     * @param point Data point to buffer
     * @return Result<void> - success or error
     */
    Result<void> add(const TSStore::DataPoint& point);
    
    /**
     * @brief Force immediate flush of all buffered points
     * @return Number of points flushed
     */
    size_t flush();
    
    /**
     * @brief Flush buffered points for specific metric:entity
     * @param metric Metric name
     * @param entity Entity ID
     * @return Number of points flushed
     */
    size_t flushFor(const std::string& metric, const std::string& entity);
    
    /**
     * @brief Get current buffer statistics
     */
    TSAutoBufferStats getStats() const;
    
    /**
     * @brief Get current configuration
     */
    const TSAutoBufferConfig& getConfig() const { return config_; }
    
    /**
     * @brief Update configuration (takes effect on next flush)
     */
    void setConfig(const TSAutoBufferConfig& config);
    
    /**
     * @brief Check if buffer is running
     */
    bool isRunning() const { return running_.load(); }

    // ========== WAL Persistence ==========

    /**
     * Persist the current in-memory buffer state to a WAL file.
     * This allows crash-recovery: unflushed points can be replayed after restart.
     *
     * @param wal_path  File path where the WAL snapshot is written
     * @return Number of points persisted (0 if buffer is empty)
     */
    size_t persistToWAL(const std::string& wal_path);

    /**
     * Restore buffer state from a previously written WAL file.
     * Points are re-enqueued into the buffer; the caller must call flush()
     * or start() to replay them to TSStore.
     *
     * @param wal_path  File path of the WAL snapshot to restore
     * @return Number of points restored (-1 on error)
     */
    std::ptrdiff_t restoreFromWAL(const std::string& wal_path);

    /**
     * Delete a WAL file (call after a successful flush to avoid replaying
     * already-flushed data on next startup).
     *
     * @param wal_path  File to remove
     * @return true if file was deleted (or did not exist)
     */
    static bool removeWAL(const std::string& wal_path);

private:
    // Per-metric:entity buffer
    struct MetricBuffer {
        std::deque<TSStore::DataPoint> points;
        std::chrono::steady_clock::time_point first_point_time;
        size_t memory_bytes = 0;
        
        void add(const TSStore::DataPoint& point) {
            if (points.empty()) {
                first_point_time = std::chrono::steady_clock::now();
            }
            points.push_back(point);
            // Rough memory estimate: DataPoint overhead + JSON strings
            memory_bytes += sizeof(TSStore::DataPoint) + 
                           point.metric.size() + 
                           point.entity.size() + 
                           point.tags.dump().size() + 
                           point.metadata.dump().size();
        }
        
        void clear() {
            points.clear();
            memory_bytes = 0;
        }
    };

    /**
     * @brief Feedback-control loop that adapts flush batch size to TSStore write latency.
     *
     * Tracks an EWMA of observed write latencies and scales `current_batch_size`
     * inversely.  When latency exceeds the configured SLO, `isBackpressure()` returns
     * true so that the `add()` path can block producers at the high-watermark.
     */
    struct FlushController {
        double ewma_latency_ms{0.0};     ///< Exponentially-weighted moving average
        double alpha{0.1};               ///< EWMA smoothing factor (0 < alpha ≤ 1)
        double slo_ms{50.0};             ///< Write-latency SLO threshold (ms)
        size_t batch_min{100};           ///< Minimum batch size
        size_t batch_max{5000};          ///< Maximum batch size
        size_t current_batch_size{500};  ///< Current adaptive batch target

        explicit FlushController(double alpha_, double slo_ms_,
                                 size_t batch_min_, size_t batch_max_,
                                 size_t initial_batch)
            : ewma_latency_ms(slo_ms_ * 0.5)
            , alpha(alpha_)
            , slo_ms(slo_ms_)
            , batch_min(batch_min_)
            , batch_max(batch_max_)
            , current_batch_size(initial_batch) {}

        /// Feed a new TSStore write latency sample and recompute batch size.
        void updateLatency(double observed_ms) {
            ewma_latency_ms = alpha * observed_ms + (1.0 - alpha) * ewma_latency_ms;

            // Scale batch size inversely with latency relative to SLO.
            // ratio = slo / ewma: ratio > 1 means latency is below SLO (fast path),
            // ratio < 1 means latency exceeds SLO (slow path).
            // After clamping ratio to [0.1, 2.0]:
            //   - ratio = 2.0 (latency = slo/2, very fast) → target = batch_min + batch_max - batch_min = batch_max
            //   - ratio = 1.0 (latency = slo, at the threshold) → target = batch_min + (batch_max - batch_min) / 2
            //   - ratio = 0.1 (latency = 10×slo, very slow) → target ≈ batch_min
            // Division by 2.0 maps the [0.1, 2.0] ratio range onto [batch_min, batch_max]
            // such that ratio 2.0 reaches batch_max exactly.
            if (ewma_latency_ms <= 0.0) {
                current_batch_size = batch_max;
            } else {
                double ratio = slo_ms / ewma_latency_ms;
                // Clamp ratio to [0.1, 2.0] to avoid extreme sizes
                ratio = std::max(0.1, std::min(2.0, ratio));
                size_t target = static_cast<size_t>(batch_min + ratio * (batch_max - batch_min) / 2.0);
                current_batch_size = std::max(batch_min, std::min(batch_max, target));
            }
        }

        /// True when EWMA latency exceeds the SLO and backpressure should engage.
        bool isBackpressure() const { return ewma_latency_ms > slo_ms; }
    };
    
    TSStore* tsstore_;
    TSAutoBufferConfig config_;
    
    // Buffer storage: map[metric:entity -> buffer]
    std::map<std::string, MetricBuffer> buffers_;
    mutable std::mutex buffers_mutex_;
    
    // Background flush thread
    std::atomic<bool> running_{false};
    std::thread flush_thread_;
    std::condition_variable flush_cv_;
    std::mutex flush_mutex_;

    // Backpressure: producers block here when queue is above high_watermark
    std::condition_variable backpressure_cv_;
    std::mutex backpressure_mutex_;
    // Lock-free buffer-size counter for backpressure threshold checks (avoids
    // holding buffers_mutex_ in the wait predicate).
    std::atomic<size_t> bp_buffer_size_{0};

    // Optional adaptive flush controller (only active when enable_adaptive_flush=true)
    std::unique_ptr<FlushController> flush_controller_;
    
    // Statistics
    TSAutoBufferStats stats_;
    
    // Helper functions
    std::string makeBufferKey(const std::string& metric, const std::string& entity) const;
    void flushThread();
    size_t flushInternal(bool lock_held = false);
    size_t flushBuffer(const std::string& buffer_key, MetricBuffer& buffer);
    bool shouldFlushBuffer(const MetricBuffer& buffer) const;
    bool shouldFlushGlobal() const;
    /// Returns the effective per-metric flush size (adaptive or configured).
    size_t effectiveBatchSize() const;
};

} // namespace themis
