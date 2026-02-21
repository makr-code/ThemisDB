/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ts_auto_buffer.h                                   ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-02-21 07:18:11                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     249                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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

namespace themis {

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
    
    // Performance tuning
    bool async_flush = true;                  // Flush in background thread
    size_t flush_batch_size = 500;           // Points per flush operation
    
    // Compression (inherited from TSStore)
    TSStore::CompressionType compression = TSStore::CompressionType::Gorilla;
    int chunk_size_hours = 24;
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
    
    size_t current_buffer_size{0};
    size_t current_buffer_memory{0};
    
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
        , current_buffer_size(other.current_buffer_size)
        , current_buffer_memory(other.current_buffer_memory)
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
            current_buffer_size = other.current_buffer_size;
            current_buffer_memory = other.current_buffer_memory;
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
    
    // Statistics
    TSAutoBufferStats stats_;
    
    // Helper functions
    std::string makeBufferKey(const std::string& metric, const std::string& entity) const;
    void flushThread();
    size_t flushInternal(bool lock_held = false);
    size_t flushBuffer(const std::string& buffer_key, MetricBuffer& buffer);
    bool shouldFlushBuffer(const MetricBuffer& buffer) const;
    bool shouldFlushGlobal() const;
};

} // namespace themis
