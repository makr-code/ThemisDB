/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            vector_auto_buffer.cpp                             ║
  Version:         0.0.41                                             ║
  Last Modified:   2026-04-14 11:34:12                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   92.0/100                                       ║
    • Total Lines:     451                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • a2d7c07202  2026-04-14  update after codefindings               ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "index/vector_auto_buffer.h"
#include "utils/logger.h"
#include "utils/tracing.h"
#include <algorithm>

namespace themis {

// ===== BufferedOp Helper =====

size_t VectorAutoBuffer::BufferedOp::estimateVectorSize(const BaseEntity& entity) {
    // Estimate size of vector data in entity
    // Assumes typical embedding field is a float array
    try {
        auto embedding = entity.extractVector("embedding");
        if (embedding.has_value()) {
            return embedding->size() * sizeof(float);
        }
    } catch (...) {
        // Fallback estimate
        return 768 * sizeof(float);  // Typical embedding size
    }
    return 0;
}

// ===== VectorAutoBuffer Implementation =====

VectorAutoBuffer::VectorAutoBuffer(VectorIndexManager* vectorIndex, 
                                   VectorAutoBufferConfig config)
    : vectorIndex_(vectorIndex), config_(std::move(config)) {
    if (!vectorIndex_) {
        throw std::invalid_argument("VectorAutoBuffer: vectorIndex cannot be null");
    }
    stats_.last_flush_time = std::chrono::steady_clock::now();
}

VectorAutoBuffer::~VectorAutoBuffer() {
    if (running_.load()) {
        stop();
    }
}

void VectorAutoBuffer::start() {
    if (running_.exchange(true)) {
        THEMIS_WARN("VectorAutoBuffer already running");
        return;
    }
    
    THEMIS_INFO("Starting VectorAutoBuffer with max_vectors={}, flush_interval={}ms",
                config_.max_vectors_per_buffer,
                config_.flush_interval.count());
    
    if (config_.async_flush) {
        flush_thread_ = std::thread(&VectorAutoBuffer::flushThread, this);
    }
}

void VectorAutoBuffer::stop() {
    if (!running_.exchange(false)) {
        return;
    }
    
    THEMIS_INFO("Stopping VectorAutoBuffer...");
    
    // Wake up flush thread
    flush_cv_.notify_all();
    
    // Wait for flush thread to finish
    if (flush_thread_.joinable()) {
        flush_thread_.join();
    }
    
    // Final flush of remaining vectors
    size_t flushed = flush();
    THEMIS_INFO("VectorAutoBuffer stopped, final flush: {} vectors", flushed);
}

std::string VectorAutoBuffer::makeBufferKey(const BaseEntity& /*entity*/) const {
    // Use entity type/namespace as buffer key
    // For now, use a simple "vectors" namespace
    // In production, this could be extracted from entity metadata
    return "vectors";
}

VectorIndexManager::Status VectorAutoBuffer::add(const BaseEntity& entity) {
    auto span = Tracer::startSpan("VectorAutoBuffer.add");
    span.setAttribute("pk", entity.getPrimaryKey());
    
    if (entity.getPrimaryKey().empty()) {
        return VectorIndexManager::Status::Error("Entity primary key cannot be empty");
    }
    
    std::string buffer_key = makeBufferKey(entity);
    
    {
        std::lock_guard<std::mutex> lock(buffers_mutex_);
        
        // Check global memory limit
        if (stats_.current_buffer_memory >= config_.max_memory_bytes) {
            THEMIS_WARN("Buffer overflow: memory limit reached ({}MB), forcing flush",
                       config_.max_memory_bytes / 1024 / 1024);
            stats_.buffer_overflow_count++;
            
            // Flush without lock (will re-acquire)
            buffers_mutex_.unlock();
            flushInternal(false);
            buffers_mutex_.lock();
        }
        
        // Add to buffer
        auto& buffer = buffers_[buffer_key];
        BufferedOp op(OpType::ADD, entity);
        size_t op_size = op.memory_bytes;
        buffer.add(std::move(op));
        
        stats_.vectors_buffered++;
        stats_.current_buffer_size++;
        stats_.current_buffer_memory += op_size;
        
        // Check if this buffer needs immediate flush
        if (buffer.operations.size() >= config_.max_vectors_per_buffer) {
            THEMIS_DEBUG("Buffer size threshold reached for {}, flushing {} vectors",
                        buffer_key, buffer.operations.size());
            
            size_t flushed = flushBuffer(buffer_key, buffer);
            stats_.size_triggered_flush++;
            
            THEMIS_DEBUG("Flushed {} vectors from {}", flushed, buffer_key);
        }
    }
    
    // Wake up flush thread if async
    if (config_.async_flush && running_.load()) {
        flush_cv_.notify_one();
    }
    
    return VectorIndexManager::Status::OK();
}

VectorIndexManager::Status VectorAutoBuffer::update(const BaseEntity& entity) {
    auto span = Tracer::startSpan("VectorAutoBuffer.update");
    span.setAttribute("pk", entity.getPrimaryKey());
    
    if (entity.getPrimaryKey().empty()) {
        return VectorIndexManager::Status::Error("Entity primary key cannot be empty");
    }
    
    std::string buffer_key = makeBufferKey(entity);
    
    {
        std::lock_guard<std::mutex> lock(buffers_mutex_);
        
        auto& buffer = buffers_[buffer_key];
        BufferedOp op(OpType::UPDATE, entity);
        size_t op_size = op.memory_bytes;
        buffer.add(std::move(op));
        
        stats_.vectors_buffered++;
        stats_.current_buffer_size++;
        stats_.current_buffer_memory += op_size;
        
        if (buffer.operations.size() >= config_.max_vectors_per_buffer) {
            size_t flushed = flushBuffer(buffer_key, buffer);
            stats_.size_triggered_flush++;
        }
    }
    
    if (config_.async_flush && running_.load()) {
        flush_cv_.notify_one();
    }
    
    return VectorIndexManager::Status::OK();
}

VectorIndexManager::Status VectorAutoBuffer::remove(const std::string& pk) {
    auto span = Tracer::startSpan("VectorAutoBuffer.remove");
    span.setAttribute("pk", pk);
    
    if (pk.empty()) {
        return VectorIndexManager::Status::Error("Primary key cannot be empty");
    }
    
    std::string buffer_key = "vectors";  // Same as makeBufferKey
    
    {
        std::lock_guard<std::mutex> lock(buffers_mutex_);
        
        auto& buffer = buffers_[buffer_key];
        BufferedOp op(OpType::REMOVE, pk);
        size_t op_size = op.memory_bytes;
        buffer.add(std::move(op));
        
        stats_.vectors_buffered++;
        stats_.current_buffer_size++;
        stats_.current_buffer_memory += op_size;
        
        if (buffer.operations.size() >= config_.max_vectors_per_buffer) {
            size_t flushed = flushBuffer(buffer_key, buffer);
            stats_.size_triggered_flush++;
        }
    }
    
    if (config_.async_flush && running_.load()) {
        flush_cv_.notify_one();
    }
    
    return VectorIndexManager::Status::OK();
}

size_t VectorAutoBuffer::flush() {
    return flushInternal(false);
}

size_t VectorAutoBuffer::flushFor(const std::string& namespace_key) {
    std::lock_guard<std::mutex> lock(buffers_mutex_);
    
    auto it = buffers_.find(namespace_key);
    if (it == buffers_.end() || it->second.operations.empty()) {
        return 0;
    }
    
    return flushBuffer(namespace_key, it->second);
}

size_t VectorAutoBuffer::flushInternal(bool lock_held) {
    auto span = Tracer::startSpan("VectorAutoBuffer.flush");
    
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
        if (buffer.operations.empty()) {
            continue;
        }
        
        size_t flushed = flushBuffer(buffer_key, buffer);
        total_flushed += flushed;
    }
    
    stats_.flush_count++;
    stats_.last_flush_time = std::chrono::steady_clock::now();
    
    THEMIS_DEBUG("Flushed {} total vectors from {} buffers", 
                 total_flushed, buffers_.size());
    
    return total_flushed;
}

size_t VectorAutoBuffer::flushBuffer(const std::string& buffer_key, NamespaceBuffer& buffer) {
    if (buffer.operations.empty()) {
        return 0;
    }
    
    auto span = Tracer::startSpan("VectorAutoBuffer.flushBuffer");
    span.setAttribute("buffer_key", buffer_key);
    span.setAttribute("operations", static_cast<int64_t>(buffer.operations.size()));
    
    // Separate operations by type
    std::vector<BaseEntity> adds;
    std::vector<BaseEntity> updates;
    std::vector<std::string> removes;
    
    for (const auto& op : buffer.operations) {
        switch (op.type) {
            case OpType::ADD:
                adds.push_back(op.entity);
                break;
            case OpType::UPDATE:
                updates.push_back(op.entity);
                break;
            case OpType::REMOVE:
                removes.push_back(op.pk);
                break;
        }
    }
    
    size_t total_ops = adds.size() + updates.size() + removes.size();
    
    // Execute batched operations
    VectorIndexManager::Status status;
    
    if (!adds.empty()) {
        status = vectorIndex_->addBatch(adds, config_.vector_field);
        if (!status.ok) {
            THEMIS_ERROR("Failed to flush ADD batch for {}: {}", buffer_key, status.message);
            return 0;
        }
    }
    
    if (!updates.empty()) {
        status = vectorIndex_->updateBatch(updates, config_.vector_field);
        if (!status.ok) {
            THEMIS_ERROR("Failed to flush UPDATE batch for {}: {}", buffer_key, status.message);
            return 0;
        }
    }
    
    if (!removes.empty()) {
        status = vectorIndex_->removeBatch(removes);
        if (!status.ok) {
            THEMIS_ERROR("Failed to flush REMOVE batch for {}: {}", buffer_key, status.message);
            return 0;
        }
    }
    
    stats_.vectors_flushed += total_ops;
    stats_.current_buffer_size -= total_ops;
    stats_.current_buffer_memory -= buffer.memory_bytes;
    
    // Clear buffer
    buffer.clear();
    
    return total_ops;
}

bool VectorAutoBuffer::shouldFlushBuffer(const NamespaceBuffer& buffer) const {
    // Size threshold
    if (buffer.operations.size() >= config_.max_vectors_per_buffer) {
        return true;
    }
    
    // Time threshold
    auto age = std::chrono::steady_clock::now() - buffer.first_op_time;
    if (age >= config_.flush_interval) {
        return true;
    }
    
    return false;
}

bool VectorAutoBuffer::shouldFlushGlobal() const {
    // Total vectors threshold
    if (stats_.current_buffer_size >= config_.max_total_vectors) {
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

void VectorAutoBuffer::flushThread() {
    THEMIS_INFO("VectorAutoBuffer flush thread started");
    
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
                THEMIS_DEBUG("Auto-flushed {} vectors", flushed);
            }
        }
    }
    
    THEMIS_INFO("VectorAutoBuffer flush thread stopped");
}

VectorAutoBufferStats VectorAutoBuffer::getStats() const {
    std::lock_guard<std::mutex> lock(buffers_mutex_);

    VectorAutoBufferStats stats;
    stats.vectors_buffered.store(stats_.vectors_buffered.load());
    stats.vectors_flushed.store(stats_.vectors_flushed.load());
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

void VectorAutoBuffer::setConfig(const VectorAutoBufferConfig& config) {
    std::lock_guard<std::mutex> lock(buffers_mutex_);
    config_ = config;
    
    THEMIS_INFO("VectorAutoBuffer config updated: max_vectors={}, flush_interval={}ms",
                config_.max_vectors_per_buffer,
                config_.flush_interval.count());
}

std::vector<BaseEntity> VectorAutoBuffer::applyCompression(const std::vector<BaseEntity>& entities) {
    // STUB/SIMULATION NOTE:
    // Purpose: Provide a stable non-compressing path until compression pipeline is implemented.
    // Activation: Always active; this function currently acts as a placeholder pass-through.
    // Production Delta: Returns original entities without quantization or PQ compression.
    // Removal Plan: Replace with configurable compression backends and remove pass-through-only behavior.
    return entities;
}

} // namespace themis
