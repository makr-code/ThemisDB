/**
 * @file ts_auto_buffer.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=3, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "timeseries/ts_auto_buffer.h"
#include "timeseries/timeseries_metrics.h"
#include "utils/logger.h"
#include "utils/tracing.h"
#include <algorithm>
#include <fstream>
#include <filesystem>
#include <nlohmann/json.hpp>

namespace themis {

// ===== TSAutoBuffer Implementation =====

TSAutoBuffer::TSAutoBuffer(TSStore* tsstore, TSAutoBufferConfig config)
    : tsstore_(tsstore), config_(std::move(config)) {
    if (!tsstore_) {
        throw std::invalid_argument("TSAutoBuffer: tsstore cannot be null");
    }
    stats_.last_flush_time = std::chrono::steady_clock::now();

    if (config_.enable_adaptive_flush) {
        flush_controller_ = std::make_unique<FlushController>(
            config_.ewma_alpha,
            config_.backpressure_slo_ms,
            config_.adaptive_batch_min,
            config_.adaptive_batch_max,
            config_.flush_batch_size);
        stats_.current_adaptive_batch_size = config_.flush_batch_size;
    }
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

    // Unblock any producers waiting on backpressure
    backpressure_cv_.notify_all();
    
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

    // Backpressure: block producers when queue depth OR TSStore latency (EWMA) is above threshold.
    // Both depth_trigger and latency_trigger use backpressure_high_watermark so that the
    // onset of blocking is consistent regardless of the trigger source.
    if (config_.enable_adaptive_flush && flush_controller_) {
        const bool depth_trigger =
            bp_buffer_size_.load(std::memory_order_relaxed) >= config_.backpressure_high_watermark;
        // Latency trigger: EWMA exceeds the write-latency SLO AND queue is above high-watermark.
        // This prevents blocking at a near-empty queue just because of a transient slow write.
        const bool latency_trigger = flush_controller_->isBackpressure() &&
            bp_buffer_size_.load(std::memory_order_relaxed) >= config_.backpressure_high_watermark;

        if (depth_trigger || latency_trigger) {
            stats_.backpressure_events++;
            THEMIS_WARN("TSAutoBuffer backpressure: queue={}, ewma_latency={}ms (slo={}ms), blocking producer",
                        bp_buffer_size_.load(std::memory_order_relaxed),
                        flush_controller_->ewma_latency_ms,
                        config_.backpressure_slo_ms);

            // Emit metric via TimeSeriesMetrics if wired
            if (config_.metrics) {
                config_.metrics->recordBackpressure(point.metric);
            }

            // Wake up the flush thread immediately
            flush_cv_.notify_one();

            std::unique_lock<std::mutex> bp_lock(backpressure_mutex_);
            backpressure_cv_.wait(bp_lock, [this] {
                return !running_.load() ||
                       bp_buffer_size_.load(std::memory_order_relaxed) <
                           config_.backpressure_low_watermark;
            });

            // If the buffer was stopped while we were waiting, refuse to add more data
            if (!running_.load()) {
                return ErrVoid(errors::ErrorCode::ERR_API_RESOURCE_EXHAUSTED,
                               "TSAutoBuffer stopped while waiting for backpressure relief");
            }
        }
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
        
        auto& buffer = buffers_[buffer_key];

        // Check per-metric memory limit
        if (config_.max_memory_per_metric_bytes > 0 &&
            buffer.memory_bytes >= config_.max_memory_per_metric_bytes) {
            THEMIS_WARN("Per-metric memory limit reached for {}, rejecting point", buffer_key);
            stats_.memory_limit_rejected_count++;
            return ErrVoid(errors::ErrorCode::ERR_API_RESOURCE_EXHAUSTED,
                           "Per-metric memory limit reached for " + buffer_key);
        }

        // Deduplication: skip if a point with the same timestamp already exists
        if (config_.enable_dedup && !buffer.points.empty()) {
            bool duplicate = false;
            for (const auto& existing : buffer.points) {
                if (existing.timestamp_ms == point.timestamp_ms) {
                    duplicate = true;
                    break;
                }
            }
            if (duplicate) {
                stats_.dedup_dropped_count++;
                return OkVoid();
            }
        }

        // Add to buffer
        buffer.add(point);
        
        stats_.points_buffered++;
        stats_.current_buffer_size++;
        bp_buffer_size_.fetch_add(1, std::memory_order_relaxed);
        stats_.current_buffer_memory = buffer.memory_bytes;
        
        // Check if this buffer needs immediate flush (use effective batch size)
        if (buffer.points.size() >= effectiveBatchSize()) {
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

size_t TSAutoBuffer::flushInternal([[maybe_unused]] bool lock_held) {
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

    auto t_start = std::chrono::steady_clock::now();
    
    // Convert deque to vector for putDataPoints
    std::vector<TSStore::DataPoint> points(buffer.points.begin(), buffer.points.end());
    
    // Use putDataPoints for batch compression
    auto result = tsstore_->putDataPoints(points);

    auto t_end = std::chrono::steady_clock::now();
    double latency_ms = std::chrono::duration<double, std::milli>(t_end - t_start).count();
    
    // Feed latency sample to FlushController (adaptive mode)
    if (flush_controller_) {
        flush_controller_->updateLatency(latency_ms);
        stats_.current_ewma_latency_ms = flush_controller_->ewma_latency_ms;
        stats_.current_adaptive_batch_size = flush_controller_->current_batch_size;
    }

    if (!result) {
        THEMIS_ERROR("Failed to flush buffer {}: {}", buffer_key, result.error().message());
        return 0;
    }
    
    size_t flushed = points.size();
    stats_.points_flushed += flushed;
    stats_.current_buffer_size -= flushed;
    // Guard against underflow: only subtract if the counter is large enough
    size_t prev = bp_buffer_size_.load(std::memory_order_relaxed);
    bp_buffer_size_.store(prev >= flushed ? prev - flushed : 0, std::memory_order_relaxed);
    stats_.current_buffer_memory -= buffer.memory_bytes;
    
    // Clear buffer
    buffer.clear();

    // Wake any producers that were blocked on backpressure
    if (config_.enable_adaptive_flush &&
        bp_buffer_size_.load(std::memory_order_relaxed) < config_.backpressure_low_watermark) {
        backpressure_cv_.notify_all();
    }
    
    return flushed;
}

bool TSAutoBuffer::shouldFlushBuffer(const MetricBuffer& buffer) const {
    // Size threshold (use adaptive batch size when enabled)
    if (buffer.points.size() >= effectiveBatchSize()) {
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
            
            // Before flushing, detect overdue buffers (data older than flush_interval × multiplier).
            // This can happen if a previous flush attempt failed and data is stuck.
            if (config_.metrics) {
                auto now = std::chrono::steady_clock::now();
                auto overdue_threshold =
                    config_.flush_interval * static_cast<int>(config_.overdue_flush_multiplier);
                std::lock_guard<std::mutex> buf_lock(buffers_mutex_);
                for (const auto& [key, buf] : buffers_) {
                    if (buf.points.empty()) {
                      continue;
                    }
                    auto age = now - buf.first_point_time;
                    if (age >= overdue_threshold) {
                        double age_ms = std::chrono::duration<double, std::milli>(age).count();
                        THEMIS_WARN("TSAutoBuffer overdue flush detected for '{}': age={}ms",
                                    key, age_ms);
                        config_.metrics->recordOverdueFlush(key, age_ms);
                    }
                }
            }

            size_t flushed = flushInternal(false);
            if (flushed > 0) {
                stats_.auto_flush_count++;
                stats_.time_triggered_flush++;
                THEMIS_DEBUG("Auto-flushed {} points", flushed);

                // After flushing, wake any producers blocked on backpressure
                if (config_.enable_adaptive_flush) {
                    backpressure_cv_.notify_all();
                }
            }
        }
    }
    
    THEMIS_INFO("TSAutoBuffer flush thread stopped");
}

size_t TSAutoBuffer::effectiveBatchSize() const {
    if (flush_controller_) {
        return flush_controller_->current_batch_size;
    }
    return config_.max_points_per_buffer;
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
    stats.dedup_dropped_count.store(stats_.dedup_dropped_count.load());
    stats.memory_limit_rejected_count.store(stats_.memory_limit_rejected_count.load());
    stats.backpressure_events.store(stats_.backpressure_events.load());
    stats.current_buffer_size = stats_.current_buffer_size;
    stats.current_buffer_memory = stats_.current_buffer_memory;
    stats.current_ewma_latency_ms = stats_.current_ewma_latency_ms;
    stats.current_adaptive_batch_size = stats_.current_adaptive_batch_size;
    stats.last_flush_time = stats_.last_flush_time;
    
    return stats;
}

void TSAutoBuffer::setConfig(const TSAutoBufferConfig& config) {
    std::lock_guard<std::mutex> lock(buffers_mutex_);
    config_ = config;

    if (config_.enable_adaptive_flush) {
        flush_controller_ = std::make_unique<FlushController>(
            config_.ewma_alpha,
            config_.backpressure_slo_ms,
            config_.adaptive_batch_min,
            config_.adaptive_batch_max,
            config_.flush_batch_size);
        stats_.current_adaptive_batch_size = config_.flush_batch_size;
    } else {
        flush_controller_.reset();
        stats_.current_adaptive_batch_size = 0;
    }
    
    THEMIS_INFO("TSAutoBuffer config updated: max_points={}, flush_interval={}ms, adaptive={}",
                config_.max_points_per_buffer,
                config_.flush_interval.count(),
                config_.enable_adaptive_flush);
}

// ========== WAL Persistence ==========

size_t TSAutoBuffer::persistToWAL(const std::string& wal_path) {
    std::lock_guard<std::mutex> lock(buffers_mutex_);

    // Serialize all buffered points as newline-delimited JSON
    std::ofstream ofs(wal_path, std::ios::trunc);
    if (!ofs.is_open()) {
        THEMIS_ERROR("TSAutoBuffer::persistToWAL: cannot open '{}'", wal_path);
        return 0;
    }

    size_t count = 0;
    for (const auto& [key, buf] : buffers_) {
        for (const auto& pt : buf.points) {
            nlohmann::json entry;
            entry["metric"]        = pt.metric;
            entry["entity"]        = pt.entity;
            entry["timestamp_ms"]  = pt.timestamp_ms;
            entry["value"]         = pt.value;
            entry["tags"]          = pt.tags;
            entry["metadata"]      = pt.metadata;
            ofs << entry.dump() << "\n";
            ++count;
        }
    }
    THEMIS_INFO("TSAutoBuffer::persistToWAL: wrote {} points to '{}'", count, wal_path);
    return count;
}

std::ptrdiff_t TSAutoBuffer::restoreFromWAL(const std::string& wal_path) {
    std::ifstream ifs(wal_path);
    if (!ifs.is_open()) {
        THEMIS_WARN("TSAutoBuffer::restoreFromWAL: file '{}' not found", wal_path);
        return -1;
    }

    std::vector<TSStore::DataPoint> restored;
    std::string line = {};
    while (std::getline(ifs, line)) {
        if (line.empty()) {
          continue;
        }
        try {
            auto j = nlohmann::json::parse(line);
            TSStore::DataPoint pt;
            pt.metric       = j.at("metric").get<std::string>();
            pt.entity       = j.at("entity").get<std::string>();
            pt.timestamp_ms = j.at("timestamp_ms").get<int64_t>();
            pt.value        = j.at("value").get<double>();
            if (j.contains("tags")) {
              pt.tags     = j["tags"];
            }
            if (j.contains("metadata")) {
              pt.metadata = j["metadata"];
            }
            restored.push_back(std::move(pt));
        } catch (const std::exception& e) {
            THEMIS_WARN("TSAutoBuffer::restoreFromWAL: skipping malformed line: {}", e.what());
        }
    }

    // Re-enqueue into buffers (bypassing dedup / memory checks intentionally)
    {
        std::lock_guard<std::mutex> lock(buffers_mutex_);
        for (auto& pt : restored) {
            auto key = makeBufferKey(pt.metric, pt.entity);
            buffers_[key].add(pt);
        }
        // Synchronise bp_buffer_size_ with the newly added points
        bp_buffer_size_.fetch_add(restored.size(), std::memory_order_relaxed);
        stats_.current_buffer_size += restored.size();
    }

    THEMIS_INFO("TSAutoBuffer::restoreFromWAL: restored {} points from '{}'",
                restored.size(), wal_path);
    return static_cast<std::ptrdiff_t>(restored.size());
}

bool TSAutoBuffer::removeWAL(const std::string& wal_path) {
    if (wal_path.empty()) {
      return true;
    }
    // Return true if file doesn't exist (already gone = success)
    if (!std::filesystem::exists(wal_path)) {
      return true;
    }
    return std::filesystem::remove(wal_path);
}

TSAutoBuffer::PushStatus TSAutoBuffer::push(const TSStore::DataPoint& point) {
    if (point.metric.empty() || point.entity.empty()) {
        return PushStatus::INVALID_INPUT;
    }

    std::string buffer_key = makeBufferKey(point.metric, point.entity);

    {
        std::lock_guard<std::mutex> lock(buffers_mutex_);

        // Non-blocking backpressure: compute total in-memory bytes across all buffers
        // and return BUFFER_FULL when max_buffer_bytes is configured and exceeded.
        if (config_.max_buffer_bytes > 0) {
            size_t total_bytes = 0;
            for (const auto& [key, buf] : buffers_) {
                total_bytes += buf.memory_bytes;
            }
            if (total_bytes >= config_.max_buffer_bytes) {
                stats_.buffer_overflow_count++;
                THEMIS_WARN("TSAutoBuffer::push backpressure: total_mem={}B exceeds max_buffer_bytes={}B",
                            total_bytes, config_.max_buffer_bytes);
                return PushStatus::BUFFER_FULL;
            }
        }

        auto& buffer = buffers_[buffer_key];
        buffer.add(point);

        stats_.points_buffered++;
        stats_.current_buffer_size++;
        bp_buffer_size_.fetch_add(1, std::memory_order_relaxed);
        stats_.current_buffer_memory = buffer.memory_bytes; // same pattern as add()

        // Flush once gorilla_batch_size points have accumulated for this series
        const size_t batch_trigger = (config_.gorilla_batch_size > 0)
                                         ? config_.gorilla_batch_size
                                         : effectiveBatchSize();
        if (buffer.points.size() >= batch_trigger) {
            size_t flushed = flushBuffer(buffer_key, buffer);
            stats_.size_triggered_flush++;
            THEMIS_DEBUG("TSAutoBuffer::push gorilla batch flush: {} points for {}",
                         flushed, buffer_key);
        }
    }

    // Notify background flush thread
    if (config_.async_flush && running_.load()) {
        flush_cv_.notify_one();
    }

    return PushStatus::OK;
}

} // namespace themis

