/**
 * @file graph_auto_buffer.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=1, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "index/graph_auto_buffer.h"
#include <stdexcept>
#include "utils/thread_join_utils.h"
#include "utils/logger.h"
#include "utils/tracing.h"
#include <algorithm>

namespace themis {

// ===== GraphAutoBuffer Implementation =====

size_t GraphAutoBuffer::BufferedOp::estimateEntitySize(const BaseEntity& entity) {
    try {
        if (entity.getFormat() == BaseEntity::Format::JSON) {
            return entity.toJson().size();
        }
        return entity.getBlobSize();
    } catch (...) {
        return 1024;
    }
}

GraphAutoBuffer::GraphAutoBuffer(PropertyGraphManager* graph, 
                                 GraphAutoBufferConfig config)
    : graph_(graph), config_(std::move(config)) {
    if (!graph_) {
        throw std::invalid_argument("GraphAutoBuffer: graph cannot be null");
    }
    stats_.last_flush_time = std::chrono::steady_clock::now();
}

GraphAutoBuffer::~GraphAutoBuffer() noexcept {
    // Gap: exception_in_destructor — wrap stop() to prevent exception propagation
    if (running_.load()) {
        try {
            stop();
        } catch (const std::exception& e) {
            // Destructor must never propagate exceptions; log and swallow
            THEMIS_ERROR("GraphAutoBuffer::~GraphAutoBuffer: exception during stop (ignored): {}",
                         e.what());
        } catch (...) {
            THEMIS_ERROR("GraphAutoBuffer::~GraphAutoBuffer: unknown exception during stop (ignored)");
        }
    }
}

void GraphAutoBuffer::start() {
    if (running_.exchange(true)) {
        THEMIS_WARN("GraphAutoBuffer already running");
        return;
    }
    
    THEMIS_INFO("Starting GraphAutoBuffer with max_nodes={}, max_edges={}, flush_interval={}ms",
                config_.max_nodes_per_buffer,
                config_.max_edges_per_buffer,
                config_.flush_interval.count());
    
    if (config_.async_flush) {
        flush_thread_ = std::thread(&GraphAutoBuffer::flushThread, this);
    }
}

void GraphAutoBuffer::stop() {
    if (!running_.exchange(false)) {
        return;
    }
    
    THEMIS_INFO("Stopping GraphAutoBuffer...");
    
    // Wake up flush thread
    flush_cv_.notify_all();
    
    // Wait for flush thread to finish
    if (flush_thread_.joinable() &&
        !utils::joinThreadWithin(flush_thread_)) {
        THEMIS_WARN("GraphAutoBuffer: flush thread exceeded shutdown timeout");
    }
    
    // Final flush of remaining operations
    size_t flushed = flush();
    THEMIS_INFO("GraphAutoBuffer stopped, final flush: {} operations", flushed);
}

PropertyGraphManager::Status GraphAutoBuffer::addNode(const BaseEntity& node, 
                                                       std::string_view graph_id) {
    auto span = Tracer::startSpan("GraphAutoBuffer.addNode");
    span.setAttribute("graph_id", std::string(graph_id));
    span.setAttribute("pk", node.getPrimaryKey());
    
    if (node.getPrimaryKey().empty()) {
        return PropertyGraphManager::Status::Error("Node primary key cannot be empty");
    }
    
    {
        std::unique_lock<std::timed_mutex> lock(buffers_mutex_);
        
        // Check global memory limit
        if (stats_.current_buffer_memory >= config_.max_memory_bytes) {
            THEMIS_WARN("Buffer overflow: memory limit reached ({}MB), forcing flush",
                       config_.max_memory_bytes / 1024 / 1024);
            stats_.buffer_overflow_count++;
            
            // Flush without lock (will re-acquire with timeout)
            lock.unlock();
            flushInternal(false);
            if (!lock.try_lock_for(std::chrono::seconds(30))) {
                THEMIS_ERROR("GraphAutoBuffer::addNode: timeout re-acquiring buffers_mutex_");
                return PropertyGraphManager::Status::Error("Buffer lock timeout");
            }
        }
        
        // Add to buffer
        std::string gid(graph_id);
        auto& buffer = buffers_[gid];
        BufferedOp op(OpType::ADD_NODE, node, graph_id);
        size_t op_size = op.memory_bytes;
        buffer.add(std::move(op));
        
        stats_.nodes_buffered++;
        stats_.current_buffer_size++;
        stats_.current_buffer_memory += op_size;
        
        // Check if this buffer needs immediate flush
        if (buffer.node_count >= config_.max_nodes_per_buffer) {
            THEMIS_DEBUG("Node buffer size threshold reached for {}, flushing {} operations",
                        gid,static_cast<int>(buffer.operations.size()));
            
            size_t flushed = flushBuffer(gid, buffer);
            stats_.size_triggered_flush++;
            
            THEMIS_DEBUG("Flushed {} operations from {}", flushed, gid);
        }
    }
    
    // Wake up flush thread if async
    if (config_.async_flush && running_.load()) {
        flush_cv_.notify_one();
    }
    
    return PropertyGraphManager::Status::OK();
}

PropertyGraphManager::Status GraphAutoBuffer::addEdge(const BaseEntity& edge, 
                                                       std::string_view graph_id) {
    auto span = Tracer::startSpan("GraphAutoBuffer.addEdge");
    span.setAttribute("graph_id", std::string(graph_id));
    span.setAttribute("pk", edge.getPrimaryKey());
    
    if (edge.getPrimaryKey().empty()) {
        return PropertyGraphManager::Status::Error("Edge primary key cannot be empty");
    }
    
    {
        std::unique_lock<std::timed_mutex> lock(buffers_mutex_);
        
        // Check global memory limit
        if (stats_.current_buffer_memory >= config_.max_memory_bytes) {
            THEMIS_WARN("Buffer overflow: memory limit reached ({}MB), forcing flush",
                       config_.max_memory_bytes / 1024 / 1024);
            stats_.buffer_overflow_count++;
            
            // Flush without lock (will re-acquire with timeout)
            lock.unlock();
            flushInternal(false);
            if (!lock.try_lock_for(std::chrono::seconds(30))) {
                THEMIS_ERROR("GraphAutoBuffer::addEdge: timeout re-acquiring buffers_mutex_");
                return PropertyGraphManager::Status::Error("Buffer lock timeout");
            }
        }
        
        // Add to buffer
        std::string gid(graph_id);
        auto& buffer = buffers_[gid];
        BufferedOp op(OpType::ADD_EDGE, edge, graph_id);
        size_t op_size = op.memory_bytes;
        buffer.add(std::move(op));
        
        stats_.edges_buffered++;
        stats_.current_buffer_size++;
        stats_.current_buffer_memory += op_size;
        
        // Check if this buffer needs immediate flush
        if (buffer.edge_count >= config_.max_edges_per_buffer) {
            THEMIS_DEBUG("Edge buffer size threshold reached for {}, flushing {} operations",
                        gid,static_cast<int>(buffer.operations.size()));
            
            size_t flushed = flushBuffer(gid, buffer);
            stats_.size_triggered_flush++;
            
            THEMIS_DEBUG("Flushed {} operations from {}", flushed, gid);
        }
    }
    
    // Wake up flush thread if async
    if (config_.async_flush && running_.load()) {
        flush_cv_.notify_one();
    }
    
    return PropertyGraphManager::Status::OK();
}

size_t GraphAutoBuffer::flush() {
    return flushInternal(false);
}

size_t GraphAutoBuffer::flushFor(const std::string& graph_id) {
    std::lock_guard<std::timed_mutex> lock(buffers_mutex_);
    
    auto it = buffers_.find(graph_id);
    if (it == buffers_.end() || it->second.operations.empty()) {
        return 0;
    }
    
    return flushBuffer(graph_id, it->second);
}

size_t GraphAutoBuffer::flushInternal(bool lock_held) {
    auto span = Tracer::startSpan("GraphAutoBuffer.flush");
    
    std::unique_lock<std::timed_mutex> lock(buffers_mutex_, std::defer_lock);
    if (!lock_held) {
        if (!lock.try_lock_for(std::chrono::seconds(30))) {
            THEMIS_WARN("GraphAutoBuffer::flushInternal: timeout acquiring buffers_mutex_");
            return 0;
        }
    }
    
    if (buffers_.empty()) {
        return 0;
    }
    
    size_t total_flushed = 0;
    
    // Flush all buffers
    for (auto& [graph_id, buffer] : buffers_) {
        if (buffer.operations.empty()) {
            continue;
        }
        
        size_t flushed = flushBuffer(graph_id, buffer);
        total_flushed += flushed;
    }
    
    stats_.flush_count++;
    stats_.last_flush_time = std::chrono::steady_clock::now();
    
    THEMIS_DEBUG("Flushed {} total operations from {} graphs", 
                 total_flushed,static_cast<int>(buffers_.size()));
    
    return total_flushed;
}

size_t GraphAutoBuffer::flushBuffer(const std::string& graph_id, GraphBuffer& buffer) {
    if (buffer.operations.empty()) {
        return 0;
    }
    
    auto span = Tracer::startSpan("GraphAutoBuffer.flushBuffer");
    span.setAttribute("graph_id", graph_id);
    span.setAttribute("operations", static_cast<int64_t>(buffer.operations.size()));
    
    // Execute all operations
    // Gap: RAII/variable-scope — declare count outside inner block so return value is valid
    size_t count = 0;
    {
        size_t nodes_flushed = 0;
        size_t edges_flushed = 0;
        
        for (auto& op : buffer.operations) {
            try {
                if (op.type == OpType::ADD_NODE) {
                    auto status = graph_->addNode(op.entity, op.graph_id);
                    if (status.ok) {
                        count++;
                        nodes_flushed++;
                    }
                } else if (op.type == OpType::ADD_EDGE) {
                    auto status = graph_->addEdge(op.entity, op.graph_id);
                    if (status.ok) {
                        count++;
                        edges_flushed++;
                    }
                }
            } catch (const std::exception& e) {
                THEMIS_ERROR("Failed to flush operation: {}", e.what());
            }
        }
        
        stats_.nodes_flushed += nodes_flushed;
        stats_.edges_flushed += edges_flushed;
        stats_.current_buffer_size -= count;
        stats_.current_buffer_memory -= buffer.memory_bytes;
    }
    
    // Clear buffer
    buffer.clear();
    
    return count;
}

bool GraphAutoBuffer::shouldFlushBuffer(const GraphBuffer& buffer) const {
    // Size threshold
    if (buffer.node_count >= config_.max_nodes_per_buffer || 
        buffer.edge_count >= config_.max_edges_per_buffer) {
        return true;
    }
    
    // Time threshold
    auto age = std::chrono::steady_clock::now() - buffer.first_op_time;
    if (age >= config_.flush_interval) {
        return true;
    }
    
    return false;
}

bool GraphAutoBuffer::shouldFlushGlobal() const {
    // Total operations threshold
    if (stats_.current_buffer_size >= config_.max_total_operations) {
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

void GraphAutoBuffer::flushThread() {
    THEMIS_INFO("GraphAutoBuffer flush thread started");
    
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
                THEMIS_DEBUG("Auto-flushed {} operations", flushed);
            }
        }
    }
    
    THEMIS_INFO("GraphAutoBuffer flush thread stopped");
}

GraphAutoBufferStats GraphAutoBuffer::getStats() const {
    std::lock_guard<std::timed_mutex> lock(buffers_mutex_);
    
    GraphAutoBufferStats stats;
    stats.nodes_buffered = stats_.nodes_buffered.load();
    stats.edges_buffered = stats_.edges_buffered.load();
    stats.nodes_flushed = stats_.nodes_flushed.load();
    stats.edges_flushed = stats_.edges_flushed.load();
    stats.flush_count = stats_.flush_count.load();
    stats.auto_flush_count = stats_.auto_flush_count.load();
    stats.manual_flush_count = stats_.manual_flush_count.load();
    stats.size_triggered_flush = stats_.size_triggered_flush.load();
    stats.time_triggered_flush = stats_.time_triggered_flush.load();
    stats.buffer_overflow_count = stats_.buffer_overflow_count.load();
    stats.current_buffer_size = stats_.current_buffer_size;
    stats.current_buffer_memory = stats_.current_buffer_memory;
    stats.last_flush_time = stats_.last_flush_time;
    
    return stats;
}

void GraphAutoBuffer::setConfig(const GraphAutoBufferConfig& config) {
    std::lock_guard<std::timed_mutex> lock(buffers_mutex_);
    config_ = config;
    
    THEMIS_INFO("GraphAutoBuffer config updated: max_nodes={}, max_edges={}, flush_interval={}ms",
                config_.max_nodes_per_buffer,
                config_.max_edges_per_buffer,
                config_.flush_interval.count());
}

} // namespace themis
