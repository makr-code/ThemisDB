/**
 * @file kv_cache_buffer.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 81/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/kv_cache_buffer.h"
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <spdlog/spdlog.h>

namespace themis {
namespace llm {

KVCacheBuffer::KVCacheBuffer(const Config& config)
    : config_(config)
    , last_flush_time_(std::chrono::steady_clock::now()) {
    
    // Pre-allocate space for typical batch
    current_batch_.reserve(config_.max_tokens_per_batch / 128);  // Estimate ~128 tokens per sequence
}

KVCacheBuffer::~KVCacheBuffer() noexcept {
    // Guard: flush() invokes the user-supplied flush_callback_ which is an
    // arbitrary std::function and could throw.  Destructors must not propagate
    // exceptions (ISO C++ §15.5.1), so we wrap the call and log any failure.
    if (!current_batch_.empty()) {
        try {
            flush();
        } catch (const std::exception& e) {
            // Cannot use spdlog safely inside a potentially-terminating context,
            // but the log call is best-effort; spdlog itself is noexcept on
            // common paths.
            spdlog::warn("KVCacheBuffer::~KVCacheBuffer: flush threw during destruction: {}",
                         e.what());
        } catch (...) {
            spdlog::warn("KVCacheBuffer::~KVCacheBuffer: flush threw unknown exception during "
                         "destruction — tokens may not have been persisted");
        }
    }
}

bool KVCacheBuffer::appendToken(int sequence_id, const float* key, const float* value) {
    if (key == nullptr || value == nullptr) {
        return false;
    }
    if (config_.embedding_dim == 0) {
        return false;
    }

    auto& cache = getCacheForSequence(sequence_id);
    
    // Append key and value
    cache.keys.insert(cache.keys.end(), key, key + config_.embedding_dim);
    cache.values.insert(cache.values.end(), value, value + config_.embedding_dim);
    cache.n_tokens++;
    
    current_batch_tokens_++;
    
    // Update stats
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.total_appends++;
        stats_.current_batch_size = current_batch_tokens_;
    }
    
    return checkAndFlush();
}

bool KVCacheBuffer::appendTokens(int sequence_id, const std::vector<float>& keys,
                                 const std::vector<float>& values, size_t n_tokens) {
    if (config_.embedding_dim == 0 && n_tokens > 0) {
        return false;
    }

    if (config_.embedding_dim != 0 &&
        n_tokens > (std::numeric_limits<size_t>::max() / config_.embedding_dim)) {
        throw std::invalid_argument("n_tokens * embedding_dim overflows size_t");
    }

    const size_t expected_elements = n_tokens * config_.embedding_dim;

    if (static_cast<int>(keys.size()) != expected_elements ||
        values.size() != expected_elements) {
        throw std::invalid_argument("Keys/values size mismatch with n_tokens and embedding_dim");
    }

    if (current_batch_tokens_ > (std::numeric_limits<size_t>::max() - n_tokens)) {
        throw std::overflow_error("current batch token count overflow");
    }
    
    auto& cache = getCacheForSequence(sequence_id);

    if (cache.n_tokens > (std::numeric_limits<size_t>::max() - n_tokens)) {
        throw std::overflow_error("sequence token count overflow");
    }
    
    // Append all keys and values
    cache.keys.insert(cache.keys.end(), keys.begin(), keys.end());
    cache.values.insert(cache.values.end(), values.begin(), values.end());
    cache.n_tokens += n_tokens;
    
    current_batch_tokens_ += n_tokens;
    
    // Update stats
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        if (stats_.total_appends > (std::numeric_limits<size_t>::max() - n_tokens)) {
            throw std::overflow_error("stats total_appends overflow");
        }
        stats_.total_appends += n_tokens;
        stats_.current_batch_size = current_batch_tokens_;
    }
    
    return checkAndFlush();
}

void KVCacheBuffer::flush() {
    if (current_batch_.empty()) {
        return;  // Nothing to flush
    }
    
    // Call flush callback if set
    if ([[maybe_unused]] flush_callback_) {
        flush_callback_([[maybe_unused]] current_batch_);
    }
    
    // Update stats
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.total_flushes++;
        stats_.total_tokens_cached += current_batch_tokens_;
        
        // Update average batch utilization
        double current_utilization = 0.0;
        if (config_.max_tokens_per_batch != 0) {
            current_utilization = static_cast<double>(current_batch_tokens_) /
                                  static_cast<double>(config_.max_tokens_per_batch);
        }
        stats_.avg_batch_utilization = 
            (stats_.avg_batch_utilization * (stats_.total_flushes - 1) + current_utilization) / stats_.total_flushes;
        
        stats_.current_batch_size = 0;
    }
    
    // Clear batch
    current_batch_.clear();
    sequence_to_index_.clear();
    current_batch_tokens_ = 0;
    last_flush_time_ = std::chrono::steady_clock::now();
}

void KVCacheBuffer::clear() {
    current_batch_.clear();
    sequence_to_index_.clear();
    current_batch_tokens_ = 0;
    
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_.current_batch_size = 0;
}

KVCacheBuffer::Stats KVCacheBuffer::getStats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_;
}

bool KVCacheBuffer::checkAndFlush() {
    bool flushed = false;
    
    if (!config_.enable_auto_flush) {
        return false;
    }
    
    // Check if batch size threshold reached
    if (current_batch_tokens_ >= config_.max_tokens_per_batch) {
        flush();
        flushed = true;
    }
    
    // Check if flush interval elapsed
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_flush_time_);
    if (elapsed >= config_.flush_interval && !current_batch_.empty()) {
        flush();
        flushed = true;
    }
    
    return flushed;
}

KVCacheBuffer::KVCache& KVCacheBuffer::getCacheForSequence([[maybe_unused]] int sequence_id) {
    auto it = sequence_to_index_.find(sequence_id);
    
    if (it != sequence_to_index_.end()) {
        // Sequence already in current batch
        return current_batch_[it->second];
    }
    
    // Create new cache for this sequence
    size_t index = current_batch_.size();
    sequence_to_index_[sequence_id] = index;
    
    current_batch_.emplace_back();
    auto& cache = current_batch_.back();
    cache.sequence_id = sequence_id;
    
    // Pre-allocate for typical sequence length
    cache.keys.reserve(128 * config_.embedding_dim);
    cache.values.reserve(128 * config_.embedding_dim);
    
    return cache;
}

// KVCacheBufferPool implementation

KVCacheBufferPool::KVCacheBufferPool(const Config& config)
    : config_(config) {
    
    // Create pool of buffers
    buffers_.reserve(config_.num_buffers);
    buffer_available_.resize(config_.num_buffers, true);
    
    for (size_t i = 0; i < config_.num_buffers; ++i) {
        buffers_.emplace_back(std::make_shared<KVCacheBuffer>(config_.buffer_config));
    }
}

KVCacheBufferPool::~KVCacheBufferPool() = default;

std::shared_ptr<KVCacheBuffer> KVCacheBufferPool::acquireBuffer() {
    std::lock_guard<std::mutex> lock(pool_mutex_);
    
    // Find first available buffer
    for (size_t i = 0; i < buffers_.size(); ++i) {
        if (buffer_available_[i]) {
            buffer_available_[i] = false;
            return buffers_[i];
        }
    }
    
    // No buffers available - create temporary one (pool exhausted)
    return std::make_shared<KVCacheBuffer>(config_.buffer_config);
}

void KVCacheBufferPool::releaseBuffer(std::shared_ptr<KVCacheBuffer> buffer) {
    std::lock_guard<std::mutex> lock(pool_mutex_);
    
    // Find buffer in pool and mark as available
    for (size_t i = 0; i < buffers_.size(); ++i) {
        if (buffers_[i] == buffer) {
            buffer->clear();  // Clear before returning to pool
            buffer_available_[i] = true;
            return;
        }
    }
    
    // Buffer not from pool (was temporary) - just let it be destroyed
}

KVCacheBufferPool::PoolStats KVCacheBufferPool::getPoolStats() const {
    std::lock_guard<std::mutex> lock(pool_mutex_);
    
    size_t available = std::count(buffer_available_.begin(), buffer_available_.end(), true);
    
    return PoolStats{
        .total_buffers = buffers_.size(),
        .available_buffers = available,
        .acquired_buffers = static_cast<int>(buffers_.size()) - available
    };
}

} // namespace llm
} // namespace themis
