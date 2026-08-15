/**
 * @file vector_auto_buffer.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 81/100
 * @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=1, C=2, H=1, M=9, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "index/vector_auto_buffer.h"
#include "index/product_quantizer.h"
#include "utils/thread_join_utils.h"
#include "utils/logger.h"
#include "utils/tracing.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace themis {

// ===== BufferedOp Helper =====

size_t VectorAutoBuffer::BufferedOp::estimateVectorSize(const BaseEntity& entity,
                                                         size_t fallback_dim) {
    // Estimate size of vector data in entity.
    // Attempts to read the actual embedding dimension from the entity; falls back
    // to `fallback_dim` (configurable via VectorAutoBufferConfig::fallback_dim,
    // default 768) so callers with non-standard model dimensions get accurate
    // memory accounting.
    try {
        auto embedding = entity.extractVector("embedding");
        if (embedding.has_value()) {
            return embedding->size() * sizeof(float);
        }
    } catch (...) {
        // extractVector() threw (field absent or wrong type); use the caller-
        // supplied fallback dimension rather than a hardcoded constant.
    }
    return fallback_dim * sizeof(float);
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
    if (flush_thread_.joinable() &&
        !utils::joinThreadWithin(flush_thread_)) {
        THEMIS_WARN("VectorAutoBuffer: flush thread exceeded shutdown timeout");
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
                THEMIS_ERROR("VectorAutoBuffer::addBatch: timeout re-acquiring buffers_mutex_");
                return VectorIndexManager::Status::Error("Buffer lock timeout");
            }
        }
        
        // Add to buffer
        auto& buffer = buffers_[buffer_key];
        BufferedOp op(OpType::ADD, entity, config_.fallback_dim);
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
        std::lock_guard<std::timed_mutex> lock(buffers_mutex_);
        
        auto& buffer = buffers_[buffer_key];
        BufferedOp op(OpType::UPDATE, entity, config_.fallback_dim);
        size_t op_size = op.memory_bytes;
        buffer.add(std::move(op));
        
        stats_.vectors_buffered++;
        stats_.current_buffer_size++;
        stats_.current_buffer_memory += op_size;
        
        if (buffer.operations.size() >= config_.max_vectors_per_buffer) {
            flushBuffer(buffer_key, buffer);
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
        std::lock_guard<std::timed_mutex> lock(buffers_mutex_);
        
        auto& buffer = buffers_[buffer_key];
        BufferedOp op(OpType::REMOVE, pk);
        size_t op_size = op.memory_bytes;
        buffer.add(std::move(op));
        
        stats_.vectors_buffered++;
        stats_.current_buffer_size++;
        stats_.current_buffer_memory += op_size;
        
        if (buffer.operations.size() >= config_.max_vectors_per_buffer) {
            flushBuffer(buffer_key, buffer);
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
    std::lock_guard<std::timed_mutex> lock(buffers_mutex_);
    
    auto it = buffers_.find(namespace_key);
    if (it == buffers_.end() || it->second.operations.empty()) {
        return 0;
    }
    
    return flushBuffer(namespace_key, it->second);
}

size_t VectorAutoBuffer::flushInternal(bool lock_held) {
    auto span = Tracer::startSpan("VectorAutoBuffer.flush");
    
    std::unique_lock<std::timed_mutex> lock(buffers_mutex_, std::defer_lock);
    if (!lock_held) {
        if (!lock.try_lock_for(std::chrono::seconds(30))) {
            THEMIS_WARN("VectorAutoBuffer::flushInternal: timeout acquiring buffers_mutex_");
            return 0;
        }
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

    size_t add_count = 0;
    size_t update_count = 0;
    size_t remove_count = 0;
    for (const auto& op : buffer.operations) {
        switch (op.type) {
            case OpType::ADD:
                ++add_count;
                break;
            case OpType::UPDATE:
                ++update_count;
                break;
            case OpType::REMOVE:
                ++remove_count;
                break;
        }
    }
    adds.reserve(add_count);
    updates.reserve(update_count);
    removes.reserve(remove_count);
    
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
    VectorIndexManager::Status status = VectorIndexManager::Status::Error("No batched vector operation executed");
    
    if (!adds.empty()) {
        const auto compressed_adds = applyCompression(adds);
        status = vectorIndex_->addBatch(compressed_adds, config_.vector_field);
        if (!status.ok) {
            THEMIS_ERROR("Failed to flush ADD batch for {}: {}", buffer_key, status.message);
            return 0;
        }
    }
    
    if (!updates.empty()) {
        const auto compressed_updates = applyCompression(updates);
        status = vectorIndex_->updateBatch(compressed_updates, config_.vector_field);
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
    std::lock_guard<std::timed_mutex> lock(buffers_mutex_);

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
    std::lock_guard<std::timed_mutex> lock(buffers_mutex_);
    config_ = config;
    
    THEMIS_INFO("VectorAutoBuffer config updated: max_vectors={}, flush_interval={}ms",
                config_.max_vectors_per_buffer,
                config_.flush_interval.count());
}

std::vector<BaseEntity> VectorAutoBuffer::applyCompression(const std::vector<BaseEntity>& entities) {
    if (entities.empty()) {
        THEMIS_DEBUG("VectorAutoBuffer::applyCompression: called with empty entities");
        return {};
    }

    const auto compression = config_.compression;
    if (compression == VectorAutoBufferConfig::Compression::None) {
        return entities;
    }

    // Helper: scalar-quantize a float32 vector to Int8 or Int16 and store it
    // back as a float32 vector encoded inside a raw byte field so that the index
    // can reconstruct the approximated embeddings without changing BaseEntity's
    // public float-vector API.
    //
    // Encoding:
    //   Int8  – each float is linearly mapped to [-127, 127]:
    //             q = round(value / abs_max * 127),  stored as int8 packed in float
    //   Int16 – each float is linearly mapped to [-32767, 32767]:
    //             q = round(value / abs_max * 32767), stored as int16 packed in float
    //
    // The scale factor (abs_max) is appended as the very last float element so
    // that the decoder can reconstruct the original values as:
    //             value_approx = q * (abs_max / max_quant_value)

    if (compression == VectorAutoBufferConfig::Compression::ProductQuantization) {
        // Product Quantization: train a per-batch codebook and replace each
        // entity's embedding with the PQ-reconstructed (lossy) approximation.
        //
        // Pipeline:
        //   1. Collect all valid embeddings from the batch as training data.
        //   2. Validate that dim is divisible by pq_num_subvectors and that
        //      the batch is large enough to train (≥ pq_num_centroids vectors).
        //   3. Train a ProductQuantizer on the batch.
        //   4. For each entity: encode → decode → store reconstructed vector.
        //
        // Fallback: If preconditions are not met (empty batch, wrong dim, too
        // few training samples), the function logs a warning and returns the
        // original entities without modification, matching the behaviour of
        // the scalar-quantization path on zero-vectors.

        const std::string vec_field       = config_.vector_field;
        const int         num_subvectors  = std::max(1, config_.pq_num_subvectors);
        const int         num_centroids   = std::max(2, config_.pq_num_centroids);

        // Step 1: gather training vectors and determine dimensionality.
        std::vector<std::vector<float>> training_vecs;
        training_vecs.reserve(entities.size());
        for (const auto& entity : entities) {
            auto vec_opt = entity.extractVector(vec_field);
            if (vec_opt.has_value() && !vec_opt->empty()) {
                training_vecs.push_back(*vec_opt);
            }
        }

        if (training_vecs.empty()) {
            THEMIS_DEBUG("VectorAutoBuffer: PQ skipped — no valid training vectors in batch, returning entities unchanged");
            return entities;
        }

        const int dim = static_cast<int>(training_vecs[0].size());

        // Step 2: precondition checks.
        if (dim % num_subvectors != 0) {
            THEMIS_WARN("VectorAutoBuffer: PQ skipped — dim={} not divisible by "
                        "pq_num_subvectors={}; returning entities unchanged",
                        dim, num_subvectors);
            return entities;
        }
        if (static_cast<int>(training_vecs.size()) < num_centroids) {
            THEMIS_WARN("VectorAutoBuffer: PQ skipped — batch size={} < "
                        "pq_num_centroids={}; returning entities unchanged",
                        training_vecs.size(), num_centroids);
            return entities;
        }

        // Step 3: train.
        ProductQuantizer::Config pq_cfg;
        pq_cfg.num_subquantizers     = num_subvectors;
        pq_cfg.num_centroids         = num_centroids;
        pq_cfg.max_iterations        = 25;
        pq_cfg.convergence_threshold = 0.001f;

        ProductQuantizer pq(dim, pq_cfg);
        auto train_status = pq.train(training_vecs);
        if (!train_status.ok) {
            THEMIS_WARN("VectorAutoBuffer: PQ training failed — {}; "
                        "returning entities unchanged", train_status.message);
            return entities;
        }

        // Step 4: encode → decode each entity.
        std::vector<BaseEntity> result;
        result.reserve(entities.size());

        for (const auto& entity : entities) {
            auto vec_opt = entity.extractVector(vec_field);
            if (!vec_opt.has_value() || vec_opt->empty()) {
                result.push_back(entity);
                continue;
            }
            const auto codes        = pq.encode(*vec_opt);
            const auto reconstructed = pq.decode(codes);

            BaseEntity compressed = entity;
            compressed.setField(vec_field, Value{reconstructed});
            result.push_back(std::move(compressed));
        }

        THEMIS_DEBUG("VectorAutoBuffer: PQ compression applied — "
                     "entities={} dim={} subvectors={} centroids={}",
                     entities.size(), dim, num_subvectors, num_centroids);
        return result;
    }

    const bool use_int8 = (compression == VectorAutoBufferConfig::Compression::Quantization_Int8);
    const float max_quant_value = use_int8 ? 127.0f : 32767.0f;
    const std::string vec_field  = config_.vector_field;

    std::vector<BaseEntity> result;
    result.reserve(entities.size());

    for (const auto& entity : entities) {
        auto vec_opt = entity.extractVector(vec_field);
        if (!vec_opt.has_value() || vec_opt->empty()) {
            // No embedding field — pass through unchanged
            result.push_back(entity);
            continue;
        }

        const std::vector<float>& src = *vec_opt;
        const size_t dim = src.size();

        // Find per-vector absolute maximum for scale computation
        float abs_max = 0.0f;
        for (float v : src) {
            abs_max = std::max(abs_max, std::fabs(v));
        }

        if (abs_max < std::numeric_limits<float>::epsilon()) {
            // Zero vector – pass through unchanged
            result.push_back(entity);
            continue;
        }

        // Quantize: map float → integer → reconstruct approximated float
        // We store the reconstructed floats so that the downstream index code
        // that calls extractVector() receives the quantised values transparently.
        // The scale is embedded as the (dim+1)-th element.
        std::vector<float> quantised;
        quantised.reserve(dim + 1);
        const float scale = abs_max / max_quant_value;
        for (size_t i = 0; i < dim; ++i) {
            float q = std::round(src[i] / scale);
            // Clamp to [-max_quant_value, max_quant_value]
            q = std::max(-max_quant_value, std::min(max_quant_value, q));
            // Reconstruct approximate float (this IS the lossy compression)
            quantised.push_back(q * scale);
        }
        quantised.push_back(abs_max); // scale metadata for downstream decoders

        // Build a copy of the entity with the quantised vector
        BaseEntity compressed = entity;
        compressed.setField(vec_field, Value{quantised});

        THEMIS_DEBUG("VectorAutoBuffer: quantised {} dim={} abs_max={:.6f} type={}",
                     vec_field, dim, abs_max, use_int8 ? "Int8" : "Int16");

        result.push_back(std::move(compressed));
    }

    return result;
}

} // namespace themis
