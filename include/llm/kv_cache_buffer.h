/**
 * @file kv_cache_buffer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "index/vector_auto_buffer.h"
#include <vector>
#include <memory>
#include <mutex>
#include <chrono>
#include <unordered_map>
#include <functional>

namespace themis {
namespace llm {

/**
 * @brief KV Cache Buffer based on VectorAutoBuffer
 * 
 * Reuses ThemisDB's VectorAutoBuffer for efficient KV cache batching:
 * - Auto-flush when batch size reached
 * - Memory-efficient buffer management
 * - Lock-free append operations
 * 
 * Phase 2.2: Cache Reuse Integration
 */
class KVCacheBuffer {
public:
    struct Config {
        size_t max_tokens_per_batch = 2048;      // Auto-flush threshold
        size_t embedding_dim = 4096;             // Model embedding dimension
        size_t num_layers = 32;                  // Number of transformer layers
        bool enable_auto_flush = true;           // Auto-flush when full
        std::chrono::milliseconds flush_interval{100};  // Periodic flush
    };

    struct KVCache {
        std::vector<float> keys;    // [n_tokens, embedding_dim]
        std::vector<float> values;  // [n_tokens, embedding_dim]
        size_t n_tokens = 0;
        int sequence_id = -1;       // Request/sequence identifier
    };

    struct Stats {
        size_t total_appends = 0;
        size_t total_flushes = 0;
        size_t total_tokens_cached = 0;
        size_t current_batch_size = 0;
        double avg_batch_utilization = 0.0;
    };

    explicit KVCacheBuffer(const Config& config);
    ~KVCacheBuffer() noexcept;

    // Append KV cache for a single token
    // Returns: true if auto-flush triggered
    bool appendToken(int sequence_id, const float* key, const float* value);

    // Append multiple tokens at once
    bool appendTokens(int sequence_id, const std::vector<float>& keys, 
                     const std::vector<float>& values, size_t n_tokens);

    // Manual flush (e.g., at end of sequence)
    void flush();

    // Get current batch (for inspection)
    const std::vector<KVCache>& getCurrentBatch() const { return current_batch_; }

    // Clear all cached data
    void clear();

    // Get statistics
    Stats getStats() const;

    // Set flush callback (called when batch is flushed)
    using FlushCallback = std::function<void(const std::vector<KVCache>&)>;
    void setFlushCallback(FlushCallback callback) { flush_callback_ = callback; }

private:
    Config config_;
    
    // Current batch being accumulated (analogous to VectorAutoBuffer)
    std::vector<KVCache> current_batch_;
    size_t current_batch_tokens_ = 0;
    
    // Buffer for each sequence in current batch
    std::unordered_map<int, size_t> sequence_to_index_;
    
    // Statistics
    mutable std::mutex stats_mutex_;
    Stats stats_;
    
    // Flush callback
    FlushCallback flush_callback_;
    
    // Auto-flush timer
    std::chrono::steady_clock::time_point last_flush_time_;
    
    // Helper: Trigger flush if needed
    bool checkAndFlush();
    
    // Helper: Get or create cache for sequence
    KVCache& getCacheForSequence(int sequence_id);
};

/**
 * @brief Shared KV Cache Buffer Pool
 * 
 * Manages multiple KVCacheBuffer instances for parallel inference.
 * Each worker thread gets its own buffer to avoid contention.
 * 
 * Thread-safe allocation from shared pool.
 */
class KVCacheBufferPool {
public:
    struct Config {
        size_t num_buffers = 8;                  // Pool size
        KVCacheBuffer::Config buffer_config;     // Config for each buffer
    };

    explicit KVCacheBufferPool(const Config& config);
    ~KVCacheBufferPool();

    // Acquire buffer for thread (thread-safe)
    std::shared_ptr<KVCacheBuffer> acquireBuffer();

    // Release buffer back to pool
    void releaseBuffer(std::shared_ptr<KVCacheBuffer> buffer);

    // Get pool statistics
    struct PoolStats {
        size_t total_buffers = 0;
        size_t available_buffers = 0;
        size_t acquired_buffers = 0;
    };
    PoolStats getPoolStats() const;

private:
    Config config_;
    std::vector<std::shared_ptr<KVCacheBuffer>> buffers_;
    std::vector<bool> buffer_available_;
    mutable std::mutex pool_mutex_;
};

} // namespace llm
} // namespace themis
