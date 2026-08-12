/**
 * @file vector_auto_buffer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 82/100
 * @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=1, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB Vector Index Auto-Batching Buffer
 * 
 * Automatic buffering of single vector inserts for batch processing.
 * Analogous to TSAutoBuffer for time series data.
 * 
 * Features:
 * - Configurable buffer size and flush intervals
 * - Per-namespace buffering
 * - Thread-safe operation
 * - Automatic flush on size/time thresholds
 * - Optional vector quantization for compression
 * - Integration with existing VectorIndexManager
 * 
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "index/vector_index.h"
#include "storage/base_entity.h"
#include <deque>
#include <map>
#include <mutex>
#include <atomic>
#include <thread>
#include <condition_variable>
#include <chrono>
#include <memory>
#include <string>

namespace themis {

/**
 * @brief Configuration for vector index auto-batching
 */
struct VectorAutoBufferConfig {
    // Buffer size thresholds
    size_t max_vectors_per_buffer = 1000;     // Max vectors per namespace before flush
    size_t max_total_vectors = 10000;         // Max total buffered vectors across all namespaces
    
    // Time-based flush
    std::chrono::milliseconds flush_interval{5000};  // Auto-flush every 5 seconds
    
    // Memory management
    size_t max_memory_bytes = 500 * 1024 * 1024;  // 500 MB max buffer memory
    
    // Performance tuning
    bool async_flush = true;                  // Flush in background thread
    size_t flush_batch_size = 500;           // Vectors per flush operation
    
    // Compression options
    enum class Compression {
        None,                  // No compression
        Quantization_Int8,     // Float32 → Int8 (4x reduction, ~1% accuracy loss)
        Quantization_Int16,    // Float32 → Int16 (2x reduction, ~0.01% accuracy loss)
        ProductQuantization    // PQ for HNSW (10-32x reduction, configurable accuracy)
    };
    Compression compression = Compression::None;

    // Product Quantization parameters (used when compression == ProductQuantization)
    int pq_num_subvectors = 8;    // Number of PQ sub-spaces (M); must divide vector dim
    int pq_num_centroids  = 256;  // Number of centroids per sub-space (k); ≤ 256 for uint8 codes

    // Vector field name (default: "embedding")
    std::string vector_field = "embedding";

    // Fallback embedding dimension used when extractVector() throws or returns
    // nullopt (instead of the old hardcoded constant 768).
    // Set this to the model's actual output dimension for accurate memory
    // accounting in VectorAutoBuffer.
    size_t fallback_dim = 768;
};

/**
 * @brief Statistics for vector auto-batching buffer
 */
struct VectorAutoBufferStats {
    std::atomic<uint64_t> vectors_buffered{0};
    std::atomic<uint64_t> vectors_flushed{0};
    std::atomic<uint64_t> flush_count{0};
    std::atomic<uint64_t> auto_flush_count{0};
    std::atomic<uint64_t> manual_flush_count{0};
    std::atomic<uint64_t> size_triggered_flush{0};
    std::atomic<uint64_t> time_triggered_flush{0};
    std::atomic<uint64_t> buffer_overflow_count{0};
    
    size_t current_buffer_size{0};
    size_t current_buffer_memory{0};
    
    std::chrono::steady_clock::time_point last_flush_time;

    VectorAutoBufferStats() = default;

    VectorAutoBufferStats(const VectorAutoBufferStats& other)
        : vectors_buffered(other.vectors_buffered.load())
        , vectors_flushed(other.vectors_flushed.load())
        , flush_count(other.flush_count.load())
        , auto_flush_count(other.auto_flush_count.load())
        , manual_flush_count(other.manual_flush_count.load())
        , size_triggered_flush(other.size_triggered_flush.load())
        , time_triggered_flush(other.time_triggered_flush.load())
        , buffer_overflow_count(other.buffer_overflow_count.load())
        , current_buffer_size(other.current_buffer_size)
        , current_buffer_memory(other.current_buffer_memory)
        , last_flush_time(other.last_flush_time) {}

    VectorAutoBufferStats& operator=(const VectorAutoBufferStats& other) {
        if (this != &other) {
            vectors_buffered.store(other.vectors_buffered.load());
            vectors_flushed.store(other.vectors_flushed.load());
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
 * @brief Auto-batching buffer for vector index operations
 * 
 * Automatically buffers single vector inserts and flushes them as batches.
 * Thread-safe, with configurable size and time thresholds.
 * 
 * Usage:
 * @code
 * VectorAutoBufferConfig config;
 * config.max_vectors_per_buffer = 500;
 * config.flush_interval = std::chrono::seconds(10);
 * config.compression = VectorAutoBufferConfig::Compression::Quantization_Int8;
 * 
 * VectorAutoBuffer buffer(vectorIndex, config);
 * buffer.start();  // Start background flush thread
 * 
 * // Add vectors (will be buffered)
 * buffer.add(entity1);
 * buffer.add(entity2);
 * // ... vectors are automatically flushed in batches
 * 
 * buffer.stop();   // Stop and flush remaining vectors
 * @endcode
 */
class VectorAutoBuffer {
public:
    /**
     * @brief Construct auto-batching buffer
     * @param vectorIndex VectorIndexManager instance (not owned)
     * @param config Buffer configuration
     */
    explicit VectorAutoBuffer(VectorIndexManager* vectorIndex, 
                              VectorAutoBufferConfig config = VectorAutoBufferConfig{});
    
    ~VectorAutoBuffer();
    
    // Non-copyable, non-movable (contains threads)
    VectorAutoBuffer(const VectorAutoBuffer&) = delete;
    VectorAutoBuffer& operator=(const VectorAutoBuffer&) = delete;
    VectorAutoBuffer(VectorAutoBuffer&&) = delete;
    VectorAutoBuffer& operator=(VectorAutoBuffer&&) = delete;
    
    /**
     * @brief Start background flush thread
     */
    void start();
    
    /**
     * @brief Stop background flush thread and flush remaining vectors
     */
    void stop();
    
    /**
     * @brief Add a vector entity (will be buffered)
     * @param entity Entity containing vector data
     * @return Status
     */
    VectorIndexManager::Status add(const BaseEntity& entity);
    
    /**
     * @brief Update a vector entity (will be buffered)
     * @param entity Entity with updated vector data
     * @return Status
     */
    VectorIndexManager::Status update(const BaseEntity& entity);
    
    /**
     * @brief Remove a vector entity by PK (will be buffered)
     * @param pk Primary key of entity to remove
     * @return Status
     */
    VectorIndexManager::Status remove(const std::string& pk);
    
    /**
     * @brief Force immediate flush of all buffered vectors
     * @return Number of vectors flushed
     */
    size_t flush();
    
    /**
     * @brief Flush buffered vectors for specific namespace
     * @param namespace_key Namespace identifier
     * @return Number of vectors flushed
     */
    size_t flushFor(const std::string& namespace_key);
    
    /**
     * @brief Get current buffer statistics
     */
    VectorAutoBufferStats getStats() const;
    
    /**
     * @brief Get current configuration
     */
    const VectorAutoBufferConfig& getConfig() const { return config_; }
    
    /**
     * @brief Update configuration (takes effect on next flush)
     */
    void setConfig(const VectorAutoBufferConfig& config);
    
    /**
     * @brief Check if buffer is running
     */
    bool isRunning() const { return running_.load(); }

private:
    // Buffer for operations (add/update/remove)
    enum class OpType { ADD, UPDATE, REMOVE };
    
    struct BufferedOp {
        OpType type;
        BaseEntity entity;  // For ADD/UPDATE
        std::string pk;     // For REMOVE
        std::chrono::steady_clock::time_point timestamp;
        size_t memory_bytes = 0;
        
        BufferedOp(OpType t, const BaseEntity& e) 
            : type(t), entity(e), timestamp(std::chrono::steady_clock::now()) {
            // Rough memory estimate
            memory_bytes = sizeof(BaseEntity) + entity.getPrimaryKey().size() + 
                          estimateVectorSize(entity);
        }

        BufferedOp(OpType t, const BaseEntity& e, size_t fallback_dim)
            : type(t), entity(e), timestamp(std::chrono::steady_clock::now()) {
            memory_bytes = sizeof(BaseEntity) + entity.getPrimaryKey().size() +
                          estimateVectorSize(entity, fallback_dim);
        }
        
        BufferedOp(OpType t, const std::string& p)
            : type(t), pk(p), timestamp(std::chrono::steady_clock::now()) {
            memory_bytes = sizeof(std::string) + pk.size();
        }
        
        static size_t estimateVectorSize(const BaseEntity& entity,
                                         size_t fallback_dim = 768);
    };
    
    // Per-namespace buffer
    struct NamespaceBuffer {
        std::deque<BufferedOp> operations;
        std::chrono::steady_clock::time_point first_op_time;
        size_t memory_bytes = 0;
        
        void add(BufferedOp&& op) {
            if (operations.empty()) {
                first_op_time = std::chrono::steady_clock::now();
            }
            memory_bytes += op.memory_bytes;
            operations.push_back(std::move(op));
        }
        
        void clear() {
            operations.clear();
            memory_bytes = 0;
        }
    };
    
    VectorIndexManager* vectorIndex_;
    VectorAutoBufferConfig config_;
    
    // Buffer storage: map[namespace -> buffer]
    std::map<std::string, NamespaceBuffer> buffers_;
    mutable std::timed_mutex buffers_mutex_;
    
    // Background flush thread
    std::atomic<bool> running_{false};
    std::thread flush_thread_;
    std::condition_variable flush_cv_;
    std::mutex flush_mutex_;
    
    // Statistics
    VectorAutoBufferStats stats_;
    
    // Helper functions
    std::string makeBufferKey(const BaseEntity& entity) const;
    void flushThread();
    size_t flushInternal(bool lock_held = false);
    size_t flushBuffer(const std::string& buffer_key, NamespaceBuffer& buffer);
    bool shouldFlushBuffer(const NamespaceBuffer& buffer) const;
    bool shouldFlushGlobal() const;
    
    // Compression helpers (for future implementation)
    std::vector<BaseEntity> applyCompression(const std::vector<BaseEntity>& entities);
};

} // namespace themis
