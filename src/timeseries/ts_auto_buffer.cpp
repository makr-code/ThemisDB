#include "timeseries/ts_auto_buffer.h"
#include "utils/logger.h"
#include "utils/tracing.h"
#include <algorithm>

namespace themis {

// ===== TSAutoBuffer Implementation =====

TSAutoBuffer::TSAutoBuffer(TSStore* tsstore, TSAutoBufferConfig config)
    : tsstore_(tsstore), config_(std::move(config)) {
    if (!tsstore_) {
        throw std::invalid_argument("TSAutoBuffer: tsstore cannot be null");
    }
    stats_.last_flush_time = std::chrono::steady_clock::now();
}

TSAutoBuffer::~TSAutoBuffer() {
    if (running_.load()) {
        stop();
    }
}

void TSAutoBuffer::start() {
    if (running_.exchange(true)) {
        THEMIS_WARN("TSAutoBuffer already running");
        return;
    }
    
    THEMIS_INFO("Starting TSAutoBuffer with max_points={}, flush_interval={}ms",
                config_.max_points_per_buffer,
                config_.flush_interval.count());
    
    if (config_.async_flush) {
        flush_thread_ = std::thread(&TSAutoBuffer::flushThread, this);
    }
}

void TSAutoBuffer::stop() {
    if (!running_.exchange(false)) {
        return;
    }
    
    THEMIS_INFO("Stopping TSAutoBuffer...");
    
    // Wake up flush thread
    flush_cv_.notify_all();
    
    // Wait for flush thread to finish
    if (flush_thread_.joinable()) {
        flush_thread_.join();
    }
    
    // Final flush of remaining points
    size_t flushed = flush();
    THEMIS_INFO("TSAutoBuffer stopped, final flush: {} points", flushed);
}

std::string TSAutoBuffer::makeBufferKey(const std::string& metric, 
                                        const std::string& entity) const {
    return metric + ":" + entity;
}

Result<void> TSAutoBuffer::add(const TSStore::DataPoint& point) {
    auto span = Tracer::startSpan("TSAutoBuffer.add");
    span.setAttribute("metric", point.metric);
    span.setAttribute("entity", point.entity);
    
    if (point.metric.empty()) {
        return ErrVoid(errors::ErrorCode::ERR_API_INVALID_REQUEST, "Metric name cannot be empty");
    }
    if (point.entity.empty()) {
        return ErrVoid(errors::ErrorCode::ERR_API_INVALID_REQUEST, "Entity ID cannot be empty");
    }
    
    std::string buffer_key = makeBufferKey(point.metric, point.entity);
    
    {
        std::unique_lock<std::mutex> lock(buffers_mutex_);
        
        // Check global memory limit
        if (stats_.current_buffer_memory >= config_.max_memory_bytes) {
            THEMIS_WARN("Buffer overflow: memory limit reached ({}MB), forcing flush",
                       config_.max_memory_bytes / 1024 / 1024);
            stats_.buffer_overflow_count++;
            
            // Flush without holding the lock
            lock.unlock();
            flushInternal(false);
            lock.lock();
        }
        
        // Add to buffer
        auto& buffer = buffers_[buffer_key];
        buffer.add(point);
        
        stats_.points_buffered++;
        stats_.current_buffer_size++;
        stats_.current_buffer_memory = buffer.memory_bytes;
        
        // Check if this buffer needs immediate flush
        if (buffer.points.size() >= config_.max_points_per_buffer) {
            THEMIS_DEBUG("Buffer size threshold reached for {}, flushing {} points",
                        buffer_key, buffer.points.size());
            
            size_t flushed = flushBuffer(buffer_key, buffer);
            stats_.size_triggered_flush++;
            
            THEMIS_DEBUG("Flushed {} points from {}", flushed, buffer_key);
        }
    }
    
    // Wake up flush thread if async
    if (config_.async_flush && running_.load()) {
        flush_cv_.notify_one();
    }
    
    return OkVoid();
}

size_t TSAutoBuffer::flush() {
    return flushInternal(false);
}

size_t TSAutoBuffer::flushFor(const std::string& metric, const std::string& entity) {
    std::string buffer_key = makeBufferKey(metric, entity);
    
    std::lock_guard<std::mutex> lock(buffers_mutex_);
    
    auto it = buffers_.find(buffer_key);
    if (it == buffers_.end() || it->second.points.empty()) {
        return 0;
    }
    
    return flushBuffer(buffer_key, it->second);
}

size_t TSAutoBuffer::flushInternal(bool lock_held) {
    auto span = Tracer::startSpan("TSAutoBuffer.flush");
    
    std::unique_lock<std::mutex> lock(buffers_mutex_, std::defer_lock);
    if (!lock_held) {
        lock.lock();
    }
    
    if (buffers_.empty()) {
        return 0;
    }
    
    size_t total_flushed = 0;
    
    // Flush all buffers
    for (auto& [buffer_key, buffer] : buffers_) {
        if (buffer.points.empty()) {
            continue;
        }
        
        size_t flushed = flushBuffer(buffer_key, buffer);
        total_flushed += flushed;
    }
    
    stats_.flush_count++;
    stats_.last_flush_time = std::chrono::steady_clock::now();
    
    THEMIS_DEBUG("Flushed {} total points from {} buffers", 
                 total_flushed, buffers_.size());
    
    return total_flushed;
}

size_t TSAutoBuffer::flushBuffer(const std::string& buffer_key, MetricBuffer& buffer) {
    if (buffer.points.empty()) {
        return 0;
    }
    
    auto span = Tracer::startSpan("TSAutoBuffer.flushBuffer");
    span.setAttribute("buffer_key", buffer_key);
    span.setAttribute("points", static_cast<int64_t>(buffer.points.size()));
    
    // Convert deque to vector for putDataPoints
    std::vector<TSStore::DataPoint> points(buffer.points.begin(), buffer.points.end());
    
    // Use putDataPoints for batch compression
    auto result = tsstore_->putDataPoints(points);
    
    if (!result) {
        THEMIS_ERROR("Failed to flush buffer {}: {}", buffer_key, result.error().message());
        return 0;
    }
    
    size_t flushed = points.size();
    stats_.points_flushed += flushed;
    stats_.current_buffer_size -= flushed;
    stats_.current_buffer_memory -= buffer.memory_bytes;
    
    // Clear buffer
    buffer.clear();
    
    return flushed;
}

bool TSAutoBuffer::shouldFlushBuffer(const MetricBuffer& buffer) const {
    // Size threshold
    if (buffer.points.size() >= config_.max_points_per_buffer) {
        return true;
    }
    
    // Time threshold
    auto age = std::chrono::steady_clock::now() - buffer.first_point_time;
    if (age >= config_.flush_interval) {
        return true;
    }
    
    return false;
}

bool TSAutoBuffer::shouldFlushGlobal() const {
    // Total points threshold
    if (stats_.current_buffer_size >= config_.max_total_points) {
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

void TSAutoBuffer::flushThread() {
    THEMIS_INFO("TSAutoBuffer flush thread started");
    
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
                THEMIS_DEBUG("Auto-flushed {} points", flushed);
            }
        }
    }
    
    THEMIS_INFO("TSAutoBuffer flush thread stopped");
}

TSAutoBufferStats TSAutoBuffer::getStats() const {
    std::lock_guard<std::mutex> lock(buffers_mutex_);

    TSAutoBufferStats stats;
    stats.points_buffered.store(stats_.points_buffered.load());
    stats.points_flushed.store(stats_.points_flushed.load());
    stats.flush_count.store(stats_.flush_count.load());
    stats.auto_flush_count.store(stats_.auto_flush_count.load());
    stats.manual_flush_count.store(stats_.manual_flush_count.load());
    stats.size_triggered_flush.store(stats_.size_triggered_flush.load());
    stats.time_triggered_flush.store(stats_.time_triggered_flush.load());
    stats.buffer_overflow_count.store(stats_.buffer_overflow_count.load());
    stats.current_buffer_size = stats_.current_buffer_size;
    stats.current_buffer_memory = stats_.current_buffer_memory;
    stats.last_flush_time = stats_.last_flush_time;
    
    return stats;
}

void TSAutoBuffer::setConfig(const TSAutoBufferConfig& config) {
    std::lock_guard<std::mutex> lock(buffers_mutex_);
    config_ = config;
    
    THEMIS_INFO("TSAutoBuffer config updated: max_points={}, flush_interval={}ms",
                config_.max_points_per_buffer,
                config_.flush_interval.count());
}

} // namespace themis
