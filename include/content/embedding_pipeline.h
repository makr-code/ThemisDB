/**
 * @file embedding_pipeline.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright (c) 2024 ThemisDB
// SPDX-License-Identifier: MIT

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <chrono>
#include <functional>

// Forward declaration — avoids pulling in the full content_metrics.h header
// from headers that only include embedding_pipeline.h.
namespace themis { namespace content { class ContentMetrics; } }

namespace themis {
namespace content {

/**
 * @brief Configuration for the embedding generation pipeline
 *
 * Controls which local model is used, batching behaviour, timeout, and the
 * output dimension.  The pipeline is activated on an ingest path only when
 * a non-empty `model_name` is supplied (matches FUTURE_ENHANCEMENTS.md
 * "activated when ContentPolicy::embeddingModel is set").
 */
struct EmbeddingPipelineConfig {
    /// Local model identifier forwarded to EmbeddedLLM (e.g. "default",
    /// "all-minilm-l6-v2").  Empty string disables the pipeline.
    std::string model_name;

    /// Maximum number of text chunks accumulated before a single batched
    /// inference call is issued to the model (performance target: ≤32).
    int batch_size = 32;

    /// Wall-clock timeout for a single embed() call in milliseconds.
    /// Calls exceeding this limit are treated as failures: content is stored
    /// without an embedding and the failure counter is incremented.
    int timeout_ms = 5000;

    /// Expected embedding dimension (0 = auto-detect from first response).
    int embedding_dim = 0;

    /// Optional metrics sink.  When non-null, every embedding failure
    /// (timeout or model error) calls ContentMetrics::recordEmbeddingFailure()
    /// so that the `content_embedding_failures_total` Prometheus counter is
    /// updated.  Follows the same optional-pointer pattern used by
    /// PDFProcessor::Config::metrics.
    ContentMetrics* metrics = nullptr;
};

/**
 * @brief Embedding generation pipeline for the content module
 *
 * Wraps the LLM embed() API with:
 *  - Batch accumulation up to `config.batch_size` texts
 *  - Per-call timeout enforcement (`config.timeout_ms`)
 *  - Failure tracking via an atomic counter
 *  - Optional ContentMetrics integration for Prometheus export
 *
 * Thread-safety: `generateEmbedding` and `generateEmbeddingBatch` are
 * individually thread-safe (each call is independent).  `getFailureCount`
 * may be called concurrently.
 *
 * Usage:
 * @code
 *   EmbeddingPipelineConfig cfg;
 *   cfg.model_name = "all-minilm-l6-v2";
 *   EmbeddingPipeline pipeline(cfg);
 *
 *   auto vec = pipeline.generateEmbedding("Hello world");
 *   auto batch = pipeline.generateEmbeddingBatch({"text1", "text2"});
 * @endcode
 */
class EmbeddingPipeline {
public:
    explicit EmbeddingPipeline(const EmbeddingPipelineConfig& config);
    ~EmbeddingPipeline() = default;

    // Non-copyable, movable
    EmbeddingPipeline(const EmbeddingPipeline&) = delete;
    EmbeddingPipeline& operator=(const EmbeddingPipeline&) = delete;
    EmbeddingPipeline(EmbeddingPipeline&&) = default;
    EmbeddingPipeline& operator=(EmbeddingPipeline&&) = default;

    /**
     * @brief Generate an embedding vector for a single text.
     *
     * Returns an empty vector on timeout or model failure; the internal
     * failure counter is incremented in that case, and if a ContentMetrics
     * sink was configured, ContentMetrics::recordEmbeddingFailure() is called.
     *
     * @param text  Input text (UTF-8).
     * @return Normalised float vector, or empty on failure.
     */
    std::vector<float> generateEmbedding(const std::string& text);

    /**
     * @brief Generate embeddings for a batch of texts.
     *
     * Internally splits the input into sub-batches of at most
     * `config.batch_size` texts to amortise model overhead.
     * Individual failures within a batch produce an empty vector for
     * the corresponding index while the rest are returned normally.
     *
     * @param texts  Input texts (UTF-8).
     * @return Vector of embedding vectors (same size as `texts`).
     */
    std::vector<std::vector<float>> generateEmbeddingBatch(
        const std::vector<std::string>& texts);

    /// True when the pipeline is active (non-empty model_name).
    bool isEnabled() const { return !config_.model_name.empty(); }

    /// Number of embedding calls that timed out or produced an error.
    uint64_t getFailureCount() const { return failure_count_.load(); }

    /// Returns the detected / configured embedding dimension (0 if unknown).
    int getEmbeddingDim() const { return embedding_dim_.load(); }

    /// Read-only access to the current configuration.
    const EmbeddingPipelineConfig& getConfig() const { return config_; }

private:
    EmbeddingPipelineConfig config_;
    mutable std::atomic<uint64_t> failure_count_{0};
    mutable std::atomic<int> embedding_dim_{0};

    /// Core embed call wrapped with timeout enforcement.
    /// Returns empty vector on timeout/error, increments failure_count_, and
    /// calls config_.metrics->recordEmbeddingFailure() if metrics is set.
    std::vector<float> embedWithTimeout(const std::string& text);

    /// Notify the optional metrics sink of a failure.
    void notifyFailure() const;
};

} // namespace content
} // namespace themis

