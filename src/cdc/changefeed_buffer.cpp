#include "cdc/changefeed_buffer.h"
#include "utils/logger.h"
#include "utils/tracing.h"
#include "utils/zstd_codec.h"
#include <algorithm>

namespace themis {

// ===== ChangefeedBuffer Implementation =====

ChangefeedBuffer::ChangefeedBuffer(Changefeed* changefeed, 
                                   ChangefeedBufferConfig config)
    : changefeed_(changefeed), config_(std::move(config)) {
    if (!changefeed_) {
        throw std::invalid_argument("ChangefeedBuffer: changefeed cannot be null");
    }
    stats_.last_flush_time = std::chrono::steady_clock::now();
}

ChangefeedBuffer::~ChangefeedBuffer() {
    if (running_.load()) {
        stop();
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
    
    // Wait for flush thread to finish
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
    auto span = Tracer::startSpan("ChangefeedBuffer.recordEvent");
    span.setAttribute("key", event.key);
    span.setAttribute("type", makeBufferKey(event));
    
    if (event.key.empty()) {
        THEMIS_WARN("ChangefeedBuffer: Event key cannot be empty");
        return event;  // Return as-is
    }
    
    // Compress large payloads if enabled
    if (config_.compress_payloads && event.value.has_value()) {
        size_t payload_size = event.value->size();
        if (payload_size > config_.compression_threshold_bytes) {
            try {
                auto compressed = utils::zstd_compress(*event.value, 3);
                if (!compressed.empty() && compressed.size() < payload_size) {
                    event.value = std::string(compressed.begin(), compressed.end());
                    event.metadata["_compressed"] = true;
                    stats_.compressed_payloads++;
                    
                    // Update compression ratio stats
                    double ratio = static_cast<double>(payload_size) / compressed.size();
                    stats_.avg_compression_ratio = 
                        (stats_.avg_compression_ratio * (stats_.compressed_payloads - 1) + ratio) / 
                        stats_.compressed_payloads;
                }
            } catch (const std::exception& e) {
                THEMIS_WARN("Compression failed for event (key={}): {}. Storing uncompressed.", 
                           event.key, e.what());
                // Continue with uncompressed payload
            }
        }
    }
    
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
        size_t event_size = buffered_event.memory_bytes;
        buffer.add(std::move(buffered_event));
        
        stats_.events_buffered++;
        stats_.current_buffer_size++;
        stats_.current_buffer_memory += event_size;
        
        // Check if this buffer needs immediate flush
        if (buffer.events.size() >= config_.max_events_per_buffer) {
            THEMIS_DEBUG("Buffer size threshold reached for {}, flushing {} events",
                        makeBufferKey(event), buffer.events.size());
            
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
                 total_flushed, buffers_.size());
    
    return total_flushed;
}

size_t ChangefeedBuffer::flushBuffer(Changefeed::ChangeEventType event_type, EventTypeBuffer& buffer) {
    if (buffer.events.empty()) {
        return 0;
    }
    
    auto span = Tracer::startSpan("ChangefeedBuffer.flushBuffer");
    span.setAttribute("event_type", static_cast<int64_t>(event_type));
    span.setAttribute("events", static_cast<int64_t>(buffer.events.size()));
    
    // Record all events to changefeed
    size_t count = 0;
    size_t failed = 0;
    for (auto& buffered_event : buffer.events) {
        try {
            // Decompress if needed
            Changefeed::ChangeEvent event = buffered_event.event;
            if (event.metadata.contains("_compressed") && event.metadata["_compressed"] == true) {
                if (event.value.has_value()) {
                    try {
                        std::vector<uint8_t> compressed_data(event.value->begin(), event.value->end());
                        auto decompressed = utils::zstd_decompress(compressed_data);
                        if (!decompressed.empty()) {
                            event.value = std::string(decompressed.begin(), decompressed.end());
                        } else {
                            THEMIS_ERROR("Decompression returned empty result for event key={}", event.key);
                            failed++;
                            continue;
                        }
                    } catch (const std::exception& e) {
                        THEMIS_ERROR("Decompression failed for event key={}: {}", event.key, e.what());
                        failed++;
                        continue;
                    }
                }
                event.metadata.erase("_compressed");
            }
            
            changefeed_->recordEvent(event);
            count++;
        } catch (const std::exception& e) {
            THEMIS_ERROR("Failed to record event: {}", e.what());
            failed++;
        }
    }
    
    if (failed > 0) {
        THEMIS_WARN("Failed to flush {} out of {} events", failed, buffer.events.size());
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
    if (buffer.events.size() >= config_.max_events_per_buffer) {
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

} // namespace themis
