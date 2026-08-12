/**
 * @file graph_auto_buffer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB Property Graph Auto-Batching Buffer
 * 
 * Automatic buffering of property graph operations (nodes/edges) for batch processing.
 * Analogous to TSAutoBuffer and VectorAutoBuffer.
 * 
 * Features:
 * - Configurable buffer size and flush intervals
 * - Separate buffering for nodes and edges
 * - Thread-safe operation
 * - Automatic flush on size/time thresholds
 * - Optional Zstd compression for property data
 * - Integration with PropertyGraphManager
 * 
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "index/property_graph.h"
#include "storage/base_entity.h"
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
 * @brief Configuration for property graph auto-batching
 */
struct GraphAutoBufferConfig {
    // Buffer size thresholds
    size_t max_nodes_per_buffer = 1000;       // Max nodes before flush
    size_t max_edges_per_buffer = 1000;       // Max edges before flush
    size_t max_total_operations = 10000;      // Max total buffered operations
    
    // Time-based flush
    std::chrono::milliseconds flush_interval{5000};  // Auto-flush every 5 seconds
    
    // Memory management
    size_t max_memory_bytes = 200 * 1024 * 1024;  // 200 MB max buffer memory
    
    // Performance tuning
    bool async_flush = true;                  // Flush in background thread
    size_t flush_batch_size = 500;           // Operations per flush
    
    // Compression for properties
    bool compress_properties = true;          // Zstd compression for large properties
    size_t compression_threshold_bytes = 1024;  // Compress properties > 1KB
    
    // Graph ID (default graph)
    std::string default_graph_id = "default";
};

/**
 * @brief Statistics for graph auto-batching buffer
 */
struct GraphAutoBufferStats {
    std::atomic<uint64_t> nodes_buffered{0};
    std::atomic<uint64_t> edges_buffered{0};
    std::atomic<uint64_t> nodes_flushed{0};
    std::atomic<uint64_t> edges_flushed{0};
    std::atomic<uint64_t> flush_count{0};
    std::atomic<uint64_t> auto_flush_count{0};
    std::atomic<uint64_t> manual_flush_count{0};
    std::atomic<uint64_t> size_triggered_flush{0};
    std::atomic<uint64_t> time_triggered_flush{0};
    std::atomic<uint64_t> buffer_overflow_count{0};
    
    size_t current_buffer_size{0};
    size_t current_buffer_memory{0};
    
    std::chrono::steady_clock::time_point last_flush_time;

    // Custom copy to support returning by value (atomics are not copyable)
    GraphAutoBufferStats() = default;
    GraphAutoBufferStats(const GraphAutoBufferStats& other) {
        nodes_buffered.store(other.nodes_buffered.load());
        edges_buffered.store(other.edges_buffered.load());
        nodes_flushed.store(other.nodes_flushed.load());
        edges_flushed.store(other.edges_flushed.load());
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
    GraphAutoBufferStats& operator=(const GraphAutoBufferStats& other) {
        if (this != &other) {
            nodes_buffered.store(other.nodes_buffered.load());
            edges_buffered.store(other.edges_buffered.load());
            nodes_flushed.store(other.nodes_flushed.load());
            edges_flushed.store(other.edges_flushed.load());
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
 * @brief Auto-batching buffer for property graph operations
 * 
 * Automatically buffers node and edge operations and flushes them as batches.
 * Thread-safe, with configurable size and time thresholds.
 * 
 * Usage:
 * @code
 * GraphAutoBufferConfig config;
 * config.max_nodes_per_buffer = 500;
 * config.flush_interval = std::chrono::seconds(10);
 * 
 * GraphAutoBuffer buffer(&propertyGraph, config);
 * buffer.start();  // Start background flush thread
 * 
 * // Add nodes/edges (will be buffered)
 * buffer.addNode(node1, "social");
 * buffer.addEdge(edge1, "social");
 * // ... operations are automatically flushed in batches
 * 
 * buffer.stop();   // Stop and flush remaining operations
 * @endcode
 */
class GraphAutoBuffer {
public:
    /**
     * @brief Construct auto-batching buffer
     * @param graph PropertyGraphManager instance (not owned)
     * @param config Buffer configuration
     */
    explicit GraphAutoBuffer(PropertyGraphManager* graph, 
                             GraphAutoBufferConfig config = GraphAutoBufferConfig{});
    
    ~GraphAutoBuffer();
    
    // Non-copyable, non-movable (contains threads)
    GraphAutoBuffer(const GraphAutoBuffer&) = delete;
    GraphAutoBuffer& operator=(const GraphAutoBuffer&) = delete;
    GraphAutoBuffer(GraphAutoBuffer&&) = delete;
    GraphAutoBuffer& operator=(GraphAutoBuffer&&) = delete;
    
    /**
     * @brief Start background flush thread
     */
    void start();
    
    /**
     * @brief Stop background flush thread and flush remaining operations
     */
    void stop();
    
    /**
     * @brief Add a node (will be buffered)
     * @param node Node entity
     * @param graph_id Graph identifier
     * @return Status
     */
    PropertyGraphManager::Status addNode(const BaseEntity& node, 
                                         std::string_view graph_id = "default");
    
    /**
     * @brief Add an edge (will be buffered)
     * @param edge Edge entity
     * @param graph_id Graph identifier
     * @return Status
     */
    PropertyGraphManager::Status addEdge(const BaseEntity& edge, 
                                         std::string_view graph_id = "default");
    
    /**
     * @brief Force immediate flush of all buffered operations
     * @return Number of operations flushed
     */
    size_t flush();
    
    /**
     * @brief Flush buffered operations for specific graph
     * @param graph_id Graph identifier
     * @return Number of operations flushed
     */
    size_t flushFor(const std::string& graph_id);
    
    /**
     * @brief Get current buffer statistics
     */
    GraphAutoBufferStats getStats() const;
    
    /**
     * @brief Get current configuration
     */
    const GraphAutoBufferConfig& getConfig() const { return config_; }
    
    /**
     * @brief Update configuration (takes effect on next flush)
     */
    void setConfig(const GraphAutoBufferConfig& config);
    
    /**
     * @brief Check if buffer is running
     */
    bool isRunning() const { return running_.load(); }

private:
    // Buffer for operations
    enum class OpType { ADD_NODE, ADD_EDGE };
    
    struct BufferedOp {
        OpType type;
        BaseEntity entity;
        std::string graph_id;
        std::chrono::steady_clock::time_point timestamp;
        size_t memory_bytes = 0;
        
        BufferedOp(OpType t, const BaseEntity& e, std::string_view gid) 
            : type(t), entity(e), graph_id(gid), 
              timestamp(std::chrono::steady_clock::now()) {
            // Rough memory estimate
            memory_bytes = sizeof(BaseEntity) + entity.getPrimaryKey().size() + 
                          estimateEntitySize(entity) + graph_id.size();
        }
        
        static size_t estimateEntitySize(const BaseEntity& entity);
    };
    
    // Per-graph buffer
    struct GraphBuffer {
        std::deque<BufferedOp> operations;
        std::chrono::steady_clock::time_point first_op_time;
        size_t memory_bytes = 0;
        size_t node_count = 0;
        size_t edge_count = 0;
        
        void add(BufferedOp&& op) {
            if (operations.empty()) {
                first_op_time = std::chrono::steady_clock::now();
            }
            memory_bytes += op.memory_bytes;
            if (op.type == OpType::ADD_NODE) {
                node_count++;
            } else {
                edge_count++;
            }
            operations.push_back(std::move(op));
        }
        
        void clear() {
            operations.clear();
            memory_bytes = 0;
            node_count = 0;
            edge_count = 0;
        }
    };
    
    PropertyGraphManager* graph_;
    GraphAutoBufferConfig config_;
    
    // Buffer storage: map[graph_id -> buffer]
    std::map<std::string, GraphBuffer> buffers_;
    mutable std::timed_mutex buffers_mutex_;
    
    // Background flush thread
    std::atomic<bool> running_{false};
    std::thread flush_thread_;
    std::condition_variable flush_cv_;
    std::mutex flush_mutex_;
    
    // Statistics
    GraphAutoBufferStats stats_;
    
    // Helper functions
    void flushThread();
    size_t flushInternal(bool lock_held = false);
    size_t flushBuffer(const std::string& graph_id, GraphBuffer& buffer);
    bool shouldFlushBuffer(const GraphBuffer& buffer) const;
    bool shouldFlushGlobal() const;
};

} // namespace themis
