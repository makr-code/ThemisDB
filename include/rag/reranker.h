/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            reranker.h                                         ║
  Version:         0.0.10                                             ║
  Last Modified:   2026-04-13 20:25:31                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     269                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a152677ace  2026-02-22  Code audit: fix header metadata inaccuracies (line counts... ║
    • 3987bc257e  2026-02-22  Add CrossEncoderReranker: header, implementation, tests, ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file reranker.h
 * @brief Re-ranking layer with cross-encoder model integration
 *
 * Two-stage retrieval: fast first-pass retrieval (bi-encoder / vector search)
 * followed by precise re-ranking using a cross-encoder model that jointly
 * encodes the query and each candidate document.
 *
 * Architecture:
 *   Query
 *     ↓
 *   Bi-Encoder Retrieval (fast, Top-100)
 *     ↓
 *   CrossEncoderReranker (accurate, Top-k)
 *     ↓
 *   RAG Generation
 *
 * When an ONNX cross-encoder model is available it is used for scoring;
 * otherwise the implementation falls back to a calibrated term-overlap
 * heuristic that preserves the same interface and produces usable scores
 * without requiring external model files.
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
     * @brief Re-rank candidate documents for a given query
     *
     * Scores every candidate against the query and returns the top-k results
     * sorted in descending relevance order.  The @c similarity_score field of
     * each returned document is replaced with the cross-encoder relevance score.
     *
     * @param query        Natural-language query
     * @param candidates   Initial retrieval results (any order, any count)
     * @param top_k        Number of documents to return; 0 uses config default
     * @return             Re-rank result with sorted documents and score details
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
     * @brief Load (or replace) the ONNX cross-encoder model
     * @param model_path Path to the ONNX model file
     * @return           true if loading succeeded
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
