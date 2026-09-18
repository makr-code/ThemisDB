/**
 * @file kv_cache_buffer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

    /**
     * @brief KVCache Buffer.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit KVCacheBuffer(const Config& config);
    ~KVCacheBuffer() noexcept;

    /**
     * @brief Append KV cache for a single token Returns: true if auto-flush triggered
     * @param[in] sequence_id Identifier of the sequence.
     * @param[in] key Input parameter.
     * @param[in] value Input parameter.
     * @return True when the operation succeeds.
     */
    bool appendToken(int sequence_id, const float* key, const float* value);

    /**
     * @brief Append multiple tokens at once
     * @param[in] sequence_id Identifier of the sequence.
     * @param[in] keys Input parameter.
     * @param[in] values Input parameter.
     * @param[in] n_tokens Input parameter.
     * @return True when the operation succeeds.
     */
    bool appendTokens(int sequence_id, const std::vector<float>& keys, 
                     const std::vector<float>& values, size_t n_tokens);

    /**
     * @brief Manual flush (e.
     * @details g., at end of sequence)
     */
    void flush();

    // Get current batch (for inspection)
    const std::vector<KVCache>& getCurrentBatch() const { return current_batch_; }

    /**
     * @brief Clear all cached data
     */
    void clear();

    // Get statistics
    /**
     * @brief Get Stats.
     * @return Return value.
     */
    Stats getStats() const;

    // Set flush callback (called when batch is flushed)
    using FlushCallback = std::function<void(const std::vector<KVCache>&)>;
    /**
     * @brief Set Flush Callback.
     * @param[in] callback Input parameter.
     * @details Implements setFlushCallback without additional internal calls.
     */
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
    
    /**
     * @brief Helper: Trigger flush if needed
     * @return True when the operation succeeds.
     */
    bool checkAndFlush();
    
    /**
     * @brief Helper: Get or create cache for sequence
     * @param[in] sequence_id Identifier of the sequence.
     * @return Return value.
     */
    KVCache& getCacheForSequence(int sequence_id);
};

class KVCacheBufferPool {
public:
    struct Config {
        size_t num_buffers = 8;                  // Pool size
        KVCacheBuffer::Config buffer_config;     // Config for each buffer
    };

    /**
     * @brief KVCache Buffer Pool.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit KVCacheBufferPool(const Config& config);
    ~KVCacheBufferPool();

    /**
     * @brief Acquire buffer for thread (thread-safe)
     * @return Return value.
     */
    std::shared_ptr<KVCacheBuffer> acquireBuffer();

    /**
     * @brief Release buffer back to pool
     * @param[in] buffer Input parameter.
     */
    void releaseBuffer(std::shared_ptr<KVCacheBuffer> buffer);

    // Get pool statistics
    struct PoolStats {
        size_t total_buffers = 0;
        size_t available_buffers = 0;
        size_t acquired_buffers = 0;
    };
    /**
     * @brief Get Pool Stats.
     * @return Return value.
     */
    PoolStats getPoolStats() const;

private:
    Config config_;
    std::vector<std::shared_ptr<KVCacheBuffer>> buffers_;
    std::vector<bool> buffer_available_;
    mutable std::mutex pool_mutex_;
};

} // namespace llm
} // namespace themis
