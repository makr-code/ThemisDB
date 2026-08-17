/**
 * @file changefeed_buffer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB Changefeed Auto-Batching Buffer
 * 
 * Automatic buffering of CDC events for batch processing with compression.
 * Analogous to TSAutoBuffer for time series and VectorAutoBuffer for vectors.
 * 
 * Features:
 * - Configurable buffer size and flush intervals
 * - Per-event-type buffering
 * - Thread-safe operation
 * - Automatic flush on size/time thresholds
 * - Optional Zstd compression for event payloads
 * - Integration with existing Changefeed
 * 
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "cdc/changefeed.h"
#include "cdc/cdc_metrics.h"
#include "cdc/dead_letter_queue.h"
#include <deque>
#include <map>
#include <mutex>
#include <atomic>
#include <thread>
#include <condition_variable>
#include <chrono>
#include <memory>

namespace themis {
using namespace themis::cdc;

/**
 * @brief Configuration for changefeed auto-batching
 */
struct ChangefeedBufferConfig {
    // Buffer size thresholds
    size_t max_events_per_buffer = 500;       // Max events per buffer before flush
    size_t max_total_events = 5000;           // Max total buffered events across all types
    
    // Time-based flush
    std::chrono::milliseconds flush_interval{1000};  // Auto-flush every 1 second (low latency for CDC)
    
    // Memory management
    size_t max_memory_bytes = 50 * 1024 * 1024;  // 50 MB max buffer memory
    
    // Performance tuning
    bool async_flush = true;                  // Flush in background thread
    size_t flush_batch_size = 250;           // Events per flush operation
    
    // Compression for event payloads
    bool compress_payloads = true;            // Zstd compression for large payloads
    size_t compression_threshold_bytes = 1024;  // Compress payloads > 1KB
    
    // Retry and error handling
    int max_retry_attempts = 3;               // Max retries for failed flushes
    std::chrono::milliseconds retry_backoff_ms{100};  // Initial backoff for retries
    bool exponential_backoff = true;          // Use exponential backoff for retries
    
    // Rate limiting
    bool enable_rate_limiting = false;        // Enable rate limiting
    size_t max_events_per_second = 10000;    // Max events per second (0 = unlimited)
    std::chrono::milliseconds rate_limit_window{1000};  // Rate limit window
};

/**
 * @brief Statistics for changefeed auto-batching buffer
 */
struct ChangefeedBufferStats {
    std::atomic<uint64_t> events_buffered{0};
    std::atomic<uint64_t> events_flushed{0};
    std::atomic<uint64_t> flush_count{0};
    std::atomic<uint64_t> auto_flush_count{0};
    std::atomic<uint64_t> manual_flush_count{0};
    std::atomic<uint64_t> size_triggered_flush{0};
    std::atomic<uint64_t> time_triggered_flush{0};
    std::atomic<uint64_t> buffer_overflow_count{0};
    std::atomic<uint64_t> compressed_payloads{0};
    std::atomic<uint64_t> retry_attempts{0};         // Total retry attempts
    std::atomic<uint64_t> retry_successes{0};        // Successful retries
    std::atomic<uint64_t> retry_failures{0};         // Failed retries (exhausted)
    std::atomic<uint64_t> flush_errors{0};           // Total flush errors
    std::atomic<uint64_t> rate_limited_events{0};    // Events delayed by rate limiting
    
    size_t current_buffer_size{0};
    size_t current_buffer_memory{0};
    double avg_compression_ratio{1.0};
    
    std::chrono::steady_clock::time_point last_flush_time;
    
    // Delete copy operations due to atomic members
    ChangefeedBufferStats(const ChangefeedBufferStats&) = delete;
    ChangefeedBufferStats& operator=(const ChangefeedBufferStats&) = delete;
    ChangefeedBufferStats(ChangefeedBufferStats&&) noexcept = default;
    ChangefeedBufferStats& operator=(ChangefeedBufferStats&&) noexcept = default;
    ChangefeedBufferStats() = default;
};

/**
 * @brief Auto-batching buffer for CDC events
 * 
 * Automatically buffers change events and flushes them as batches.
 * Thread-safe, with configurable size and time thresholds.
 * 
 * Usage:
 * @code
 * ChangefeedBufferConfig config;
 * config.max_events_per_buffer = 500;
 * config.flush_interval = std::chrono::seconds(1);
 * config.compress_payloads = true;
 * 
 * ChangefeedBuffer buffer(&changefeed, config);
 * buffer.start();  // Start background flush thread
 * 
 * // Add events (will be buffered)
 * buffer.recordEvent(event1);
 * buffer.recordEvent(event2);
 * // ... events are automatically flushed in batches
 * 
 * buffer.stop();   // Stop and flush remaining events
 * @endcode
 */
class ChangefeedBuffer {
public:
    /**
     * @brief Construct auto-batching buffer
     * @param changefeed Changefeed instance (not owned)
     * @param config Buffer configuration
     */
    explicit ChangefeedBuffer(Changefeed* changefeed, 
                              ChangefeedBufferConfig config = ChangefeedBufferConfig{});
    
    /**
     * @brief Destructor - stops the background flush thread and suppresses shutdown exceptions.
     */
    ~ChangefeedBuffer() noexcept;
    
    // Non-copyable, non-movable (contains threads)
    ChangefeedBuffer(const ChangefeedBuffer&) = delete;
    ChangefeedBuffer& operator=(const ChangefeedBuffer&) = delete;
    ChangefeedBuffer(ChangefeedBuffer&&) = delete;
    ChangefeedBuffer& operator=(ChangefeedBuffer&&) = delete;
    
    /**
     * @brief Start background flush thread
     */
    void start();
    
    /**
     * @brief Stop background flush thread and flush remaining events
     */
    void stop();
    
    /**
     * @brief Record a change event (will be buffered)
     * @param event Event to record
     * @return Recorded event with assigned sequence (0 if buffered)
     */
    Changefeed::ChangeEvent recordEvent(Changefeed::ChangeEvent event);
    
    /**
     * @brief Force immediate flush of all buffered events
     * @return Number of events flushed
     */
    size_t flush();
    
    /**
     * @brief Flush buffered events for specific event type
     * @param event_type Event type to flush
     * @return Number of events flushed
     */
    size_t flushFor(Changefeed::ChangeEventType event_type);
    
    /**
     * @brief Get current buffer statistics
     */
    const ChangefeedBufferStats& getStats() const;
    
    /**
     * @brief Get enhanced metrics (latency, throughput, etc.)
     * @return CDCMetrics with histograms and counters
     */
    const CDCMetrics& getMetrics() const { return metrics_; }
    
    /**
     * @brief Reset metrics (for testing or periodic reset)
     */
    void resetMetrics() { metrics_.reset(); }
    
    /**
     * @brief Get current configuration
     */
    const ChangefeedBufferConfig& getConfig() const { return config_; }
    
    /**
     * @brief Update configuration (takes effect on next flush)
     */
    void setConfig(const ChangefeedBufferConfig& config);
    
    /**
     * @brief Check if buffer is running
     */
    bool isRunning() const { return running_.load(); }

    /**
     * @brief Attach a dead-letter queue to receive events that exhaust retries.
     *
     * When set, any event for which all delivery attempts fail is enqueued in
     * the provided DeadLetterQueue instead of being silently discarded.
     * The DeadLetterQueue is NOT owned by this buffer.
     *
     * @param dlq  Pointer to an existing DeadLetterQueue, or nullptr to detach.
     */
    void setDeadLetterQueue(cdc::DeadLetterQueue* dlq) { dlq_ = dlq; }

    /**
     * @brief Return the attached dead-letter queue (may be nullptr).
     */
    cdc::DeadLetterQueue* getDeadLetterQueue() const { return dlq_; }

private:
    // Dead-letter queue (optional, not owned)
    cdc::DeadLetterQueue* dlq_ = nullptr;
    // Buffered event wrapper
    struct BufferedEvent {
        Changefeed::ChangeEvent event;
        std::chrono::steady_clock::time_point timestamp;
        size_t memory_bytes = 0;
        bool compressed = false;
        
        BufferedEvent(const Changefeed::ChangeEvent& e) 
            : event(e), timestamp(std::chrono::steady_clock::now()) {
            // Rough memory estimate
            memory_bytes = sizeof(Changefeed::ChangeEvent) + 
                          event.key.size() + 
                          (event.value.has_value() ? event.value->size() : 0);
        }
    };
    
    // Per-event-type buffer
    struct EventTypeBuffer {
        std::deque<BufferedEvent> events;
        std::chrono::steady_clock::time_point first_event_time;
        size_t memory_bytes = 0;
        
        void add(BufferedEvent&& event) {
            if (events.empty()) {
                first_event_time = std::chrono::steady_clock::now();
            }
            memory_bytes += event.memory_bytes;
            events.push_back(std::move(event));
        }
        
        void clear() {
            events.clear();
            memory_bytes = 0;
        }
    };
    
    Changefeed* changefeed_;
    ChangefeedBufferConfig config_;
    
    // Buffer storage: map[event_type -> buffer]
    std::map<Changefeed::ChangeEventType, EventTypeBuffer> buffers_;
    mutable std::mutex buffers_mutex_;
    
    // Background flush thread
    std::atomic<bool> running_{false};
    std::thread flush_thread_;
    std::condition_variable flush_cv_;
    std::mutex flush_mutex_;
    
    // Statistics
    ChangefeedBufferStats stats_;
    
    // Enhanced metrics (P1 feature)
    CDCMetrics metrics_;
    
    // Helper functions
    std::string makeBufferKey(const Changefeed::ChangeEvent& event) const;
    void flushThread();
    size_t flushInternal(bool lock_held = false);
    size_t flushBuffer(Changefeed::ChangeEventType event_type, EventTypeBuffer& buffer);
    bool shouldFlushBuffer(const EventTypeBuffer& buffer) const;
    bool shouldFlushGlobal() const;
    
    // Rate limiting helper
    bool checkRateLimit();
    
    // Compression helpers
    std::string compressPayload(const std::string& payload);
    std::string decompressPayload(const std::string& compressed);
    
    // Rate limiting state
    std::chrono::steady_clock::time_point rate_limit_window_start_;
    std::atomic<size_t> events_in_current_window_{0};
    std::mutex rate_limit_mutex_;
};

} // namespace themis
