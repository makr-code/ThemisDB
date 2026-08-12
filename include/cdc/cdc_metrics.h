/**
 * @file cdc_metrics.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB CDC Metrics and Observability
 * 
 * Provides latency histograms, throughput tracking, and enhanced metrics
 * for Change Data Capture operations.
 * 
 * Copyright (c) 2025 ThemisDB Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <chrono>
#include <atomic>
#include <vector>
#include <algorithm>
#include <mutex>
#include <nlohmann/json.hpp>

namespace themis {
namespace cdc {

/**
 * @brief Latency histogram for tracking operation latencies
 * 
 * Uses fixed buckets for efficient percentile calculation.
 */
class LatencyHistogram {
public:
    LatencyHistogram() : count_(0), sum_micros_(0) {
        for (auto& bucket : buckets_) {
            bucket.store(0, std::memory_order_relaxed);
        }
    }
    
    /**
     * @brief Record a latency sample in microseconds
     */
    void record(uint64_t latency_micros) {
        count_++;
        sum_micros_ += latency_micros;
        
        // Find appropriate bucket
        size_t bucket = bucketIndex(latency_micros);
        buckets_[bucket]++;
    }
    
    /**
     * @brief Get count of samples
     */
    uint64_t count() const {
        return count_.load();
    }
    
    /**
     * @brief Get average latency in microseconds
     */
    double average() const {
        uint64_t cnt = count_.load();
        return cnt > 0 ? static_cast<double>(sum_micros_.load()) / cnt : 0.0;
    }
    
    /**
     * @brief Get P50 (median) latency in microseconds
     */
    uint64_t p50() const {
        return percentile(0.50);
    }
    
    /**
     * @brief Get P95 latency in microseconds
     */
    uint64_t p95() const {
        return percentile(0.95);
    }
    
    /**
     * @brief Get P99 latency in microseconds
     */
    uint64_t p99() const {
        return percentile(0.99);
    }
    
    /**
     * @brief Get percentile latency in microseconds
     */
    uint64_t percentile(double p) const {
        uint64_t total = count_.load();
        if (total == 0) return 0;
        
        uint64_t target = static_cast<uint64_t>(total * p);
        uint64_t accumulated = 0;
        
        for (size_t i = 0; i < buckets_.size(); i++) {
            accumulated += buckets_[i].load();
            if (accumulated >= target) {
                return bucketMidpoint(i);
            }
        }
        
        return bucketMidpoint(buckets_.size() - 1);
    }
    
    /**
     * @brief Convert to JSON for monitoring
     */
    nlohmann::json toJson() const {
        return {
            {"count", count_.load()},
            {"average_us", average()},
            {"p50_us", p50()},
            {"p95_us", p95()},
            {"p99_us", p99()},
            {"max_us", bucketMidpoint(buckets_.size() - 1)}
        };
    }
    
    /**
     * @brief Reset histogram
     */
    void reset() {
        count_.store(0, std::memory_order_relaxed);
        sum_micros_.store(0, std::memory_order_relaxed);
        for (auto& bucket : buckets_) {
            bucket.store(0, std::memory_order_relaxed);
        }
    }

private:
    // Histogram buckets in microseconds:
    // 0-100us, 100-250us, 250-500us, 500-1ms, 1-2.5ms, 2.5-5ms, 5-10ms, 
    // 10-25ms, 25-50ms, 50-100ms, 100-250ms, 250-500ms, 500ms-1s, 1s+
    static constexpr size_t NUM_BUCKETS = 14;
    std::array<std::atomic<uint64_t>, NUM_BUCKETS> buckets_;
    std::atomic<uint64_t> count_;
    std::atomic<uint64_t> sum_micros_;
    
    size_t bucketIndex(uint64_t latency_micros) const {
        if (latency_micros < 100) return 0;
        if (latency_micros < 250) return 1;
        if (latency_micros < 500) return 2;
        if (latency_micros < 1000) return 3;
        if (latency_micros < 2500) return 4;
        if (latency_micros < 5000) return 5;
        if (latency_micros < 10000) return 6;
        if (latency_micros < 25000) return 7;
        if (latency_micros < 50000) return 8;
        if (latency_micros < 100000) return 9;
        if (latency_micros < 250000) return 10;
        if (latency_micros < 500000) return 11;
        if (latency_micros < 1000000) return 12;
        return 13;  // 1s+
    }
    
    uint64_t bucketMidpoint(size_t bucket) const {
        constexpr uint64_t midpoints[] = {
            50, 175, 375, 750, 1750, 3750, 7500, 17500, 37500, 75000,
            175000, 375000, 750000, 1500000
        };
        return midpoints[bucket];
    }
};

/**
 * @brief Throughput tracker for events/bytes per second
 */
class ThroughputTracker {
public:
    ThroughputTracker() 
        : window_start_(std::chrono::steady_clock::now()),
          events_in_window_(0),
          bytes_in_window_(0) {}
    
    /**
     * @brief Record an event with payload size
     */
    void recordEvent(size_t bytes = 0) {
        resetWindowIfNeeded();
        events_in_window_++;
        bytes_in_window_ += bytes;
    }
    
    /**
     * @brief Get events per second in current window
     */
    double eventsPerSecond() const {
        auto elapsed = std::chrono::steady_clock::now() - window_start_;
        auto seconds = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() / 1000.0;
        return seconds > 0 ? events_in_window_.load() / seconds : 0.0;
    }
    
    /**
     * @brief Get bytes per second in current window
     */
    double bytesPerSecond() const {
        auto elapsed = std::chrono::steady_clock::now() - window_start_;
        auto seconds = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() / 1000.0;
        return seconds > 0 ? bytes_in_window_.load() / seconds : 0.0;
    }
    
    /**
     * @brief Convert to JSON
     */
    nlohmann::json toJson() const {
        return {
            {"events_per_second", eventsPerSecond()},
            {"bytes_per_second", bytesPerSecond()},
            {"events_in_window", events_in_window_.load()},
            {"bytes_in_window", bytes_in_window_.load()}
        };
    }
    
    /**
     * @brief Reset tracker
     */
    void reset() {
        window_start_ = std::chrono::steady_clock::now();
        events_in_window_ = 0;
        bytes_in_window_ = 0;
    }

private:
    static constexpr auto WINDOW_DURATION = std::chrono::seconds(60);
    
    std::chrono::steady_clock::time_point window_start_;
    std::atomic<uint64_t> events_in_window_;
    std::atomic<uint64_t> bytes_in_window_;
    
    void resetWindowIfNeeded() {
        auto elapsed = std::chrono::steady_clock::now() - window_start_;
        if (elapsed >= WINDOW_DURATION) {
            reset();
        }
    }
};

/**
 * @brief Enhanced CDC metrics
 */
struct CDCMetrics {
    // Latency histograms
    LatencyHistogram record_event_latency;
    LatencyHistogram flush_latency;
    LatencyHistogram compression_latency;
    LatencyHistogram decompression_latency;
    
    // Throughput tracking
    ThroughputTracker throughput;
    
    // Counters (existing metrics from previous implementation)
    std::atomic<uint64_t> events_recorded{0};
    std::atomic<uint64_t> events_flushed{0};
    std::atomic<uint64_t> flush_count{0};
    std::atomic<uint64_t> compression_count{0};
    std::atomic<uint64_t> decompression_count{0};
    std::atomic<uint64_t> errors{0};
    std::atomic<uint64_t> retries{0};

    // WebSocket transport counters (cdc_ws_* Prometheus metric names)
    std::atomic<uint64_t> ws_events_delivered{0};   ///< cdc_ws_events_delivered_total
    std::atomic<uint64_t> ws_overflow_total{0};     ///< cdc_ws_overflow_total

    // Kafka producer counters (cdc_kafka_* Prometheus metric names)
    std::atomic<uint64_t> kafka_delivered_total{0}; ///< cdc_kafka_delivered_total
    std::atomic<uint64_t> kafka_error_total{0};     ///< cdc_kafka_error_total
    
    /**
     * @brief Convert all metrics to JSON
     */
    nlohmann::json toJson() const {
        return {
            {"latency", {
                {"record_event", record_event_latency.toJson()},
                {"flush", flush_latency.toJson()},
                {"compression", compression_latency.toJson()},
                {"decompression", decompression_latency.toJson()}
            }},
            {"throughput", throughput.toJson()},
            {"counters", {
                {"events_recorded", events_recorded.load()},
                {"events_flushed", events_flushed.load()},
                {"flush_count", flush_count.load()},
                {"compression_count", compression_count.load()},
                {"decompression_count", decompression_count.load()},
                {"errors", errors.load()},
                {"retries", retries.load()},
                {"ws_events_delivered", ws_events_delivered.load()},
                {"ws_overflow_total", ws_overflow_total.load()},
                {"kafka_delivered_total", kafka_delivered_total.load()},
                {"kafka_error_total", kafka_error_total.load()}
            }}
        };
    }
    
    /**
     * @brief Reset all metrics
     */
    void reset() {
        record_event_latency.reset();
        flush_latency.reset();
        compression_latency.reset();
        decompression_latency.reset();
        throughput.reset();
        
        events_recorded = 0;
        events_flushed = 0;
        flush_count = 0;
        compression_count = 0;
        decompression_count = 0;
        errors = 0;
        retries = 0;
        ws_events_delivered = 0;
        ws_overflow_total = 0;
        kafka_delivered_total = 0;
        kafka_error_total = 0;
    }
};

/**
 * @brief RAII timer for automatic latency recording
 */
class ScopedTimer {
public:
    ScopedTimer(LatencyHistogram& histogram)
        : histogram_(histogram),
          start_(std::chrono::steady_clock::now()) {}
    
    ~ScopedTimer() {
        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start_);
        histogram_.record(duration.count());
    }

private:
    LatencyHistogram& histogram_;
    std::chrono::steady_clock::time_point start_;
};

/**
 * @brief Helper macro for easy latency recording
 */
#define CDC_MEASURE_LATENCY(histogram) \
    ScopedTimer _timer(histogram)

} // namespace cdc
} // namespace themis
