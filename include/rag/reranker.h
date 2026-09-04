/**
 * @file reranker.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.18
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "rag/rag_judge.h"

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <chrono>
#include <functional>
#include <unordered_map>

namespace themis::rag {

/**
 * @brief Configuration for the cross-encoder re-ranker
 */
struct CrossEncoderConfig {
    /// Path to the ONNX cross-encoder model file.
    /// Leave empty to use the built-in heuristic scorer.
    std::string model_path = "";

    /// Maximum sequence length accepted by the model (query + document tokens).
    size_t max_length = 512;

    /// Number of candidate documents to score per batch.
    size_t batch_size = 32;

    /// Number of top documents to return after re-ranking (0 = return all).
    size_t top_k = 10;

    /// Minimum relevance score threshold; documents below this are dropped.
    /// Set to 0.0 to disable filtering.
    double min_score_threshold = 0.0;

    /// Enable GPU inference when an ONNX model is loaded.
    bool use_gpu = false;

    /// Require model checksum verification before accepting loadModel().
    /// When enabled, loadModel() expects either expected_model_checksum or
    /// "<model_path>.sha256" to be present and matching.
    bool require_model_checksum = false;

    /// Optional expected checksum for the model file (hex FNV-1a 64-bit).
    /// If set, it takes precedence over a ".sha256" sidecar file.
    std::string expected_model_checksum = {};

    /// Cache re-ranking scores for (query, document-id) pairs to avoid
    /// re-scoring identical inputs within the same reranker instance.
    bool enable_score_cache = true;

    /// Maximum number of entries retained in the score cache.
    size_t max_cache_size = 1024;
};

/**
 * @brief Result from re-ranking a single query-document pair
 */
struct RerankScore {
    std::string document_id;   ///< Document identifier
    double relevance_score;    ///< Cross-encoder relevance score in [0, 1]
    double original_score;     ///< Bi-encoder / initial retrieval score
    size_t original_rank;      ///< Rank before re-ranking (0-based)
    size_t reranked_rank;      ///< Rank after re-ranking (0-based)
};

/**
 * @brief Aggregate result of a re-ranking pass
 */
struct RerankResult {
    /// Re-ranked documents in descending relevance order.
    std::vector<judge::RetrievedDocument> documents;

    /// Per-document scoring details.
    std::vector<RerankScore> scores;

    /// True when an actual cross-encoder model was used for scoring.
    bool used_model = false;

    /// Wall-clock time for the complete re-ranking pass.
    std::chrono::milliseconds rerank_time{0};
};

/**
 * @brief Re-ranking layer backed by a cross-encoder model
 *
 * Usage (with heuristic scorer, no model file needed):
 * @code
 *   CrossEncoderReranker reranker;
 *   auto result = reranker.rerank(query, candidates);
 * @endcode
 *
 * Usage (with ONNX cross-encoder model):
 * @code
 *   CrossEncoderConfig cfg;
 *   cfg.model_path = "models/cross-encoder-ms-marco.onnx";
 *   cfg.batch_size = 32;
 *   CrossEncoderReranker reranker(cfg);
 *   auto result = reranker.rerank(query, candidates);
 * @endcode
 *
 * Performance targets:
 *   - Heuristic mode: <5 ms for 100 candidates
 *   - ONNX model mode: +50-100 ms for 100 candidates (GPU: +20-50 ms)
 *   - Re-ranking Precision@10: ~85 % (cross-encoder model)
 */
class CrossEncoderReranker {
public:
    /**
     * @brief Construct reranker with default (heuristic) configuration
     */
    CrossEncoderReranker();

    /**
     * @brief Construct reranker with custom configuration
     * @param config Reranker configuration
     */
    explicit CrossEncoderReranker(const CrossEncoderConfig& config);

    /**
     * @brief Destructor
     */
    ~CrossEncoderReranker();

    /**
     * @brief Re-rank candidate documents for a given query.
     *
     * Scores every candidate against the query, optionally reuses the internal
     * score cache for repeated `(query, document-id)` pairs, and returns the
     * top-k results sorted in descending relevance order. The
     * @c similarity_score field of each returned document is replaced with the
     * cross-encoder relevance score.
     *
     * Empty input, oversized queries/documents, or excessive candidate counts
     * are rejected fail-closed and return an empty result instead of attempting
     * partial scoring.
     *
     * @param query        Natural-language query.
     * @param candidates   Initial retrieval results (any order, any count).
     * @param top_k        Number of documents to return; 0 uses config default.
     * @return             Re-rank result with sorted documents and score details.
     */
    RerankResult rerank(
        const std::string& query,
        const std::vector<judge::RetrievedDocument>& candidates,
        size_t top_k = 0
    ) const;

    /**
     * @brief Score a single query-document pair
     *
     * Returns a relevance score in [0, 1].  Higher is more relevant.
     *
     * @param query    Natural-language query
     * @param document Document text
     * @return         Relevance score
     */
    double score(const std::string& query, const std::string& document) const;

    /**
     * @brief Score multiple query-document pairs in one batch
     *
     * Equivalent to calling score() for each pair but may be more efficient
     * when a real batch-capable model is loaded.
     *
     * @param query       Natural-language query
     * @param documents   Document texts to score
     * @return            Relevance scores in the same order as @p documents
     */
    std::vector<double> scoreBatch(
        const std::string& query,
        const std::vector<std::string>& documents
    ) const;

    /**
     * @brief Load (or replace) the ONNX cross-encoder model.
     *
     * The loader canonicalises @p model_path, rejects symlinks, requires a
     * regular `.onnx` file, validates bounded file size and permissions, and
     * computes a SHA-256 digest before accepting the model. When a
     * `<model_path>.sha256` sidecar exists, the digest must match; when the
     * sidecar is absent, the loader emits a warning and proceeds.
     *
     * @param model_path Path to the ONNX model file.
     * @return           `true` when the model passed validation and was loaded;
     *                   `false` on invalid paths, checksum mismatches, or other
     *                   integrity/safety failures.
     */
    bool loadModel(const std::string& model_path);

    /**
     * @brief Check whether an ONNX model has been successfully loaded
     * @return true if a model is loaded and will be used for scoring
     */
    bool isModelLoaded() const;

    /**
     * @brief Invalidate the internal score cache
     */
    void clearCache();

    /**
     * @brief Return current configuration
     */
    const CrossEncoderConfig& getConfig() const;

    /**
     * @brief Update configuration
     *
     * Clears the score cache when cache-related settings change.
     *
     * @param config New configuration
     */
    void setConfig(const CrossEncoderConfig& config);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief Factory helpers for common cross-encoder configurations
 */
class CrossEncoderFactory {
public:
    /**
     * @brief Fast heuristic reranker (no model required, <5 ms / 100 docs)
     */
    static std::unique_ptr<CrossEncoderReranker> createFast();

    /**
     * @brief Balanced ONNX reranker (MiniLM-L6, good speed/accuracy)
     * @param model_path Path to MiniLM cross-encoder ONNX file
     */
    static std::unique_ptr<CrossEncoderReranker> createBalanced(
        const std::string& model_path = "");

    /**
     * @brief High-accuracy ONNX reranker (RoBERTa-base, best accuracy)
     * @param model_path Path to RoBERTa cross-encoder ONNX file
     */
    static std::unique_ptr<CrossEncoderReranker> createAccurate(
        const std::string& model_path = "");
};

} // namespace themis::rag
