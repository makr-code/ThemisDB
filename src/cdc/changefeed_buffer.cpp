/**
 * @file changefeed_buffer.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=1, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "cdc/changefeed_buffer.h"
#include "cdc/cdc_error.h"
#include "utils/logger.h"
#include "utils/tracing.h"
#include "utils/zstd_codec.h"
#include <algorithm>

namespace themis {
using namespace themis::cdc;

// ===== ChangefeedBuffer Implementation =====

ChangefeedBuffer::ChangefeedBuffer(Changefeed* changefeed, 
                                   ChangefeedBufferConfig config)
    : changefeed_(changefeed), config_(std::move(config)) {
    if (!changefeed_) {
        throw std::invalid_argument("ChangefeedBuffer: changefeed cannot be null");
    }
    stats_.last_flush_time = std::chrono::steady_clock::now();
    rate_limit_window_start_ = std::chrono::steady_clock::now();
}

ChangefeedBuffer::~ChangefeedBuffer() noexcept {
    try {
        if (running_.load()) {
            stop();
        }
    } catch (...) {
        // Destructors must not throw while draining the background flush thread.
    }
}

void ChangefeedBuffer::start() {
    if (running_.exchange(true)) {
        THEMIS_WARN("ChangefeedBuffer already running");
        return;
    }
    
    THEMIS_INFO("Starting ChangefeedBuffer with max_events={}, flush_interval={}ms",
                config_.max_events_per_buffer,
                config_.flush_interval.count());
    
    if (config_.async_flush) {
        flush_thread_ = std::thread(&ChangefeedBuffer::flushThread, this);
    }
}

void ChangefeedBuffer::stop() {
    if (!running_.exchange(false)) {
        return;
    }
    
    THEMIS_INFO("Stopping ChangefeedBuffer...");
    
    // Wake up flush thread
    flush_cv_.notify_all();
    
    // Safe blocking join: the worker only waits on flush_cv_. After clearing
    // running_ and notifying above, the thread wakes promptly and exits.
    if (flush_thread_.joinable()) {
        flush_thread_.join();
    }
    
    // Final flush of remaining events
    size_t flushed = flush();
    THEMIS_INFO("ChangefeedBuffer stopped, final flush: {} events", flushed);
}

std::string ChangefeedBuffer::makeBufferKey(const Changefeed::ChangeEvent& event) const {
    // Use event type as buffer key
    switch (event.type) {
        case Changefeed::ChangeEventType::EVENT_PUT: return "PUT";
        case Changefeed::ChangeEventType::EVENT_DELETE: return "DELETE";
        case Changefeed::ChangeEventType::EVENT_TRANSACTION_COMMIT: return "TX_COMMIT";
        case Changefeed::ChangeEventType::EVENT_TRANSACTION_ROLLBACK: return "TX_ROLLBACK";
        default: return "UNKNOWN";
    }
}

Changefeed::ChangeEvent ChangefeedBuffer::recordEvent(Changefeed::ChangeEvent event) {
    CDC_MEASURE_LATENCY(metrics_.record_event_latency);
    
    auto span = Tracer::startSpan("ChangefeedBuffer.recordEvent");
    span.setAttribute("key", event.key);
    span.setAttribute("type", makeBufferKey(event));
    
    if (event.key.empty()) {
        THEMIS_WARN("ChangefeedBuffer: Event key cannot be empty");
        metrics_.errors++;
        return event;  // Return as-is
    }
    
    // Apply rate limiting if enabled
    if (config_.enable_rate_limiting) {
        checkRateLimit();
    }
    
    // Compress large payloads if enabled
    if (config_.compress_payloads && event.value.has_value()) {
        size_t payload_size = event.value->size();
        if (payload_size > config_.compression_threshold_bytes) {
            CDC_MEASURE_LATENCY(metrics_.compression_latency);
            try {
                auto compressed = utils::zstd_compress(*event.value, 3);
                if (!compressed.empty() && static_cast<int>(compressed.size()) < payload_size) {
                    event.value = std::string(compressed.begin(), compressed.end());
                    event.metadata["_compressed"] = true;
                    stats_.compressed_payloads++;
                    metrics_.compression_count++;
                    
                    // Update compression ratio stats
                    double ratio = static_cast<double>(payload_size) / compressed.size();
                    stats_.avg_compression_ratio = 
                        (stats_.avg_compression_ratio * (stats_.compressed_payloads - 1) + ratio) / 
                        stats_.compressed_payloads;
                }
            } catch (const std::exception& e) {
                THEMIS_WARN("Compression failed for event (key={}): {}. Storing uncompressed.", 
                           event.key, e.what());
                metrics_.errors++;
                // Continue with uncompressed payload
            }
        }
    }
    
    // Track throughput
    size_t event_size = event.value.has_value() ? event.value->size() : 0;
    metrics_.throughput.recordEvent(event_size);
    metrics_.events_recorded++;
    
    {
        std::unique_lock<std::mutex> lock(buffers_mutex_);
        
        // Check global memory limit
        if (stats_.current_buffer_memory >= config_.max_memory_bytes) {
            THEMIS_WARN("Buffer overflow: memory limit reached ({}MB), forcing flush",
                       config_.max_memory_bytes / 1024 / 1024);
            stats_.buffer_overflow_count++;
            
            // Unlock before flush to avoid deadlock, flush will re-acquire lock
            lock.unlock();
            flushInternal(false);
            lock.lock();
        }
        
        // Add to buffer
        auto& buffer = buffers_[event.type];
        BufferedEvent buffered_event(event);
        size_t buffered_size = buffered_event.memory_bytes;
        buffer.add(std::move(buffered_event));
        
        stats_.events_buffered++;
        stats_.current_buffer_size++;
        stats_.current_buffer_memory += buffered_size;
        
        // Check if this buffer needs immediate flush
        if (static_cast<int>(buffer.events.size()) >= config_.max_events_per_buffer) {
            THEMIS_DEBUG("Buffer size threshold reached for {}, flushing {} events",
                        makeBufferKey(event),static_cast<int>(buffer.events.size()));
            
            size_t flushed = flushBuffer(event.type, buffer);
            stats_.size_triggered_flush++;
            
            THEMIS_DEBUG("Flushed {} events from {}", flushed, makeBufferKey(event));
        }
    }
    
    // Wake up flush thread if async
    if (config_.async_flush && running_.load()) {
        flush_cv_.notify_one();
    }
    
    // Return event with sequence=0 to indicate it's buffered
    event.sequence = 0;
    return event;
}

size_t ChangefeedBuffer::flush() {
    return flushInternal(false);
}

size_t ChangefeedBuffer::flushFor(Changefeed::ChangeEventType event_type) {
    std::lock_guard<std::mutex> lock(buffers_mutex_);
    
    auto it = buffers_.find(event_type);
    if (it == buffers_.end() || it->second.events.empty()) {
        return 0;
    }
    
    return flushBuffer(event_type, it->second);
}

size_t ChangefeedBuffer::flushInternal(bool lock_held) {
    auto span = Tracer::startSpan("ChangefeedBuffer.flush");
    
    std::unique_lock<std::mutex> lock(buffers_mutex_, std::defer_lock);
    if (!lock_held) {
        lock.lock();
    }
    
    if (buffers_.empty()) {
        return 0;
    }
    
    size_t total_flushed = 0;
    
    // Flush all buffers
    for (auto& [event_type, buffer] : buffers_) {
        if (buffer.events.empty()) {
            continue;
        }
        
        size_t flushed = flushBuffer(event_type, buffer);
        total_flushed += flushed;
    }
    
    stats_.flush_count++;
    stats_.last_flush_time = std::chrono::steady_clock::now();
    
    THEMIS_DEBUG("Flushed {} total events from {} buffers", 
                 total_flushed,static_cast<int>(buffers_.size()));
    
    return total_flushed;
}

size_t ChangefeedBuffer::flushBuffer(Changefeed::ChangeEventType event_type, EventTypeBuffer& buffer) {
    CDC_MEASURE_LATENCY(metrics_.flush_latency);
    
    if (buffer.events.empty()) {
        return 0;
    }
    
    auto span = Tracer::startSpan("ChangefeedBuffer.flushBuffer");
    span.setAttribute("event_type", static_cast<int64_t>(event_type));
    span.setAttribute("events", static_cast<int64_t>(buffer.events.size()));
    
    // Record all events to changefeed with retry logic
    size_t count = 0;
    size_t failed = 0;
    
    for (auto& buffered_event : buffer.events) {
        bool recorded = false;
        int retry_count = 0;
        std::string last_error = {};
        
        while (!recorded && retry_count <= config_.max_retry_attempts) {
            try {
                // Decompress if needed
                Changefeed::ChangeEvent event = buffered_event.event;
                if (event.metadata.contains("_compressed") && event.metadata["_compressed"] == true) {
                    if (event.value.has_value()) {
                        {
                            CDC_MEASURE_LATENCY(metrics_.decompression_latency);
                            try {
                                std::vector<uint8_t> compressed_data(event.value->begin(), event.value->end());
                                auto decompressed = utils::zstd_decompress(compressed_data);
                                if (!decompressed.empty()) {
                                    event.value = std::string(decompressed.begin(), decompressed.end());
                                    metrics_.decompression_count++;
                                } else {
                                    THEMIS_ERROR("Decompression returned empty result for event key={}", event.key);
                                    stats_.flush_errors++;
                                    metrics_.errors++;
                                    last_error = "decompression returned empty result";
                                    break;  // Don't retry decompression errors
                                }
                            } catch (const std::exception& e) {
                                THEMIS_ERROR("Decompression failed for event key={}: {}", event.key, e.what());
                                stats_.flush_errors++;
                                metrics_.errors++;
                                last_error = e.what();
                                break;  // Don't retry decompression errors
                            }
                        }
                    }
                    event.metadata.erase("_compressed");
                }
                
                // Try to record the event
                changefeed_->recordEvent(event);
                count++;
                recorded = true;
                metrics_.events_flushed++;
                
                if (retry_count > 0) {
                    stats_.retry_successes++;
                    metrics_.retries++;
                    THEMIS_DEBUG("Successfully recorded event after {} retries", retry_count);
                }
            } catch (const std::exception& e) {
                retry_count++;
                stats_.retry_attempts++;
                metrics_.retries++;
                last_error = e.what();
                
                if (retry_count <= config_.max_retry_attempts) {
                    // Calculate backoff duration with maximum cap
                    auto backoff = config_.retry_backoff_ms;
                    if (config_.exponential_backoff && retry_count > 1) {
                        // Cap exponential backoff to prevent overflow (max 30 seconds)
                        int exponent = std::min(retry_count - 1, 8);  // 2^8 = 256
                        long long exponential_backoff_ms = static_cast<long long>(config_.retry_backoff_ms.count()) * (1LL << exponent);
                        backoff = std::chrono::milliseconds(
                            std::min(exponential_backoff_ms, 30000LL)
                        );
                    }
                    
                    THEMIS_WARN("Failed to record event (attempt {}/{}): {}. Retrying after {}ms",
                               retry_count, config_.max_retry_attempts + 1, e.what(), backoff.count());
                    
                    std::this_thread::sleep_for(backoff);
                } else {
                    THEMIS_ERROR("Failed to record event after {} attempts: {}", retry_count, e.what());
                    stats_.retry_failures++;
                    stats_.flush_errors++;
                    metrics_.errors++;
                    // Note: failed++ will be done below if !recorded
                }
            }
        }
        
        if (!recorded && retry_count > config_.max_retry_attempts) {
            failed++;
            // Route to dead-letter queue if configured
            if (dlq_) {
                try {
                    dlq_->enqueue(buffered_event.event, last_error, retry_count);
                } catch (const std::exception& dlq_ex) {
                    THEMIS_ERROR("DeadLetterQueue enqueue failed for event key={}: {}",
                                 buffered_event.event.key, dlq_ex.what());
                }
            }
        }
    }
    
    if (failed > 0) {
        THEMIS_WARN("Failed to flush {} out of {} events (exhausted retries)", failed,static_cast<int>(buffer.events.size()));
    }
    
    stats_.events_flushed += count;
    stats_.current_buffer_size -= count;
    stats_.current_buffer_memory -= buffer.memory_bytes;
    
    // Clear buffer
    buffer.clear();
    
    return count;
}

bool ChangefeedBuffer::shouldFlushBuffer(const EventTypeBuffer& buffer) const {
    // Size threshold
    if (static_cast<int>(buffer.events.size()) >= config_.max_events_per_buffer) {
        return true;
    }
    
    // Time threshold
    auto age = std::chrono::steady_clock::now() - buffer.first_event_time;
    if (age >= config_.flush_interval) {
        return true;
    }
    
    return false;
}

bool ChangefeedBuffer::shouldFlushGlobal() const {
    // Total events threshold
    if (stats_.current_buffer_size >= config_.max_total_events) {
        return true;
    }
    
    // Memory threshold
    if (stats_.current_buffer_memory >= config_.max_memory_bytes) {
        return true;
    }
    
    // Time-based: check if oldest buffer is ready
    auto now = std::chrono::steady_clock::now();
    auto time_since_flush = now - stats_.last_flush_time;
    if (time_since_flush >= config_.flush_interval) {
        return true;
    }
    
    return false;
}

void ChangefeedBuffer::flushThread() {
    THEMIS_INFO("ChangefeedBuffer flush thread started");
    
    while (running_.load()) {
        std::unique_lock<std::mutex> lock(flush_mutex_);
        
        // Wait for flush interval or notification
        flush_cv_.wait_for(lock, config_.flush_interval, [this] {
            return !running_.load() || shouldFlushGlobal();
        });
        
        if (!running_.load()) {
            break;
        }
        
        // Check if we need to flush
        if (shouldFlushGlobal()) {
            lock.unlock();  // Release before flushing
            
            size_t flushed = flushInternal(false);
            if (flushed > 0) {
                stats_.auto_flush_count++;
                stats_.time_triggered_flush++;
                THEMIS_DEBUG("Auto-flushed {} events", flushed);
            }
        }
    }
    
    THEMIS_INFO("ChangefeedBuffer flush thread stopped");
}

const ChangefeedBufferStats& ChangefeedBuffer::getStats() const {
    std::lock_guard<std::mutex> lock(buffers_mutex_);
    return stats_;
}

void ChangefeedBuffer::setConfig(const ChangefeedBufferConfig& config) {
    std::lock_guard<std::mutex> lock(buffers_mutex_);
    config_ = config;
    
    THEMIS_INFO("ChangefeedBuffer config updated: max_events={}, flush_interval={}ms",
                config_.max_events_per_buffer,
                config_.flush_interval.count());
}

std::string ChangefeedBuffer::compressPayload(const std::string& payload) {
    auto compressed = utils::zstd_compress(payload, 3);
    if (compressed.empty()) {
        return payload;  // Return original if compression fails
    }
    return std::string(compressed.begin(), compressed.end());
}

std::string ChangefeedBuffer::decompressPayload(const std::string& compressed) {
    std::vector<uint8_t> compressed_data(compressed.begin(), compressed.end());
    auto decompressed = utils::zstd_decompress(compressed_data);
    if (decompressed.empty()) {
        return compressed;  // Return original if decompression fails
    }
    return std::string(decompressed.begin(), decompressed.end());
}

bool ChangefeedBuffer::checkRateLimit() {
    if (!config_.enable_rate_limiting || config_.max_events_per_second == 0) {
        return true;  // Rate limiting disabled
    }
    
    std::lock_guard<std::mutex> lock(rate_limit_mutex_);
    
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - rate_limit_window_start_
    );
    
    // Reset window if it has expired
    if (elapsed >= config_.rate_limit_window) {
        rate_limit_window_start_ = now;
        events_in_current_window_.store(0);
    }
    
    // Check if we're within the limit
    size_t current_count = events_in_current_window_.load();
    if (current_count >= config_.max_events_per_second) {
        stats_.rate_limited_events++;
        
        // Calculate how long to wait
        auto wait_time = config_.rate_limit_window - elapsed;
        if (wait_time.count() > 0) {
            THEMIS_DEBUG("Rate limit reached, waiting {}ms", wait_time.count());
            std::this_thread::sleep_for(wait_time);
            
            // Reset window after waiting
            rate_limit_window_start_ = std::chrono::steady_clock::now();
            events_in_current_window_.store(0);
        }
    }
    
    events_in_current_window_++;
    return true;
}

} // namespace themis
