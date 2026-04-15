/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            hybrid_search.h                                    ║
  Version:         0.0.46                                             ║
  Last Modified:   2026-04-15 18:05:05                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     221                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "search/llm_reranker.h"

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include "index/vector_index.h"

namespace themis {

class SecondaryIndexManager;
class VectorIndexManager;

/**
 * @brief Hybrid Search combining BM25 (full-text) and Vector (semantic) search
 * 
 * v1.2.0 Feature: Reciprocal Rank Fusion (RRF) for RAG optimization
 * v1.3.0 Update: Real BM25 and Vector index integration
 * v1.4.0 Update: Configurable vector metric, config validation, normalization fixes,
 *                resource limits, exception safety, partial-result logging
 * v1.8.0 Update: Configurable LLM re-ranking via setReranker() (Phase 3)
 * 
 * Features:
 * - BM25 fulltext search with scoring
 * - Vector ANN search (HNSW)
 * - Reciprocal Rank Fusion (RRF) for result merging
 * - Linear combination fallback with pre-normalization
 * - Score normalization with edge-case handling
 * - Configurable vector distance metric (COSINE, DOT, L2)
 * - Bounded resource usage via max_k / max_candidates limits
 * - Optional LLM re-ranking via injected LlmBackend (setReranker())
 * 
 * @note Thread Safety: A single HybridSearch instance is NOT thread-safe.
 *   search() and setConfig() must not be called concurrently on the same
 *   instance. Callers that share an instance across threads must provide
 *   their own synchronization (e.g. a mutex around each call).
 *   Creating separate HybridSearch instances per thread is the preferred
 *   pattern, since the class is lightweight and holds no mutable state
 *   beyond Config and the two non-owning index pointers.
 * 
 * @note Exception Safety: The constructor offers strong exception safety
 *   (throws std::invalid_argument on invalid Config; the object is never
 *   partially constructed). search() is unconditionally noexcept at runtime:
 *   all exceptions from the index backends and from the fusion stage are
 *   caught internally, logged via THEMIS_ERROR, and search() returns an
 *   empty (or partial) result vector rather than propagating the exception.
 *   reciprocalRankFusion() and normalizeScores() may throw std::bad_alloc
 *   when called directly, but within search() this is caught and results in
 *   an empty return value.
 * 
 * Use Cases:
 * - RAG (Retrieval-Augmented Generation)
 * - Semantic + keyword search
 * - Document ranking
 * - Question answering systems
 * 
 * Performance:
 * - 85%+ recall@10 with RRF
 * - Combines lexical and semantic matching
 * - Configurable weights for BM25/vector balance
 */
class HybridSearch {
public:
    struct Config {
        double bm25_weight = 0.5;
        double vector_weight = 0.5;
        size_t k = 10;              // Final result count
        size_t k_bm25 = 50;         // BM25 candidate count
        size_t k_vector = 50;       // Vector candidate count
        bool use_rrf = true;        // Use RRF (recommended)
        double rrf_k = 60.0;        // RRF constant
        bool normalize_scores = true;

        // Resource limits: k and candidates are clamped to these bounds at
        // construction time to prevent unbounded memory / latency spikes.
        size_t max_k = 10'000;          // Hard upper bound for final result k
        size_t max_candidates = 10'000; // Hard upper bound for k_bm25 / k_vector
        
        // Configurable table/column for searches
        std::string default_table = "documents";
        std::string default_column = "content";
        
        // Vector distance metric used for similarity conversion
        VectorIndexManager::Metric vector_metric = VectorIndexManager::Metric::COSINE;
    };
    
    struct Result {
        std::string document_id;
        double bm25_score = 0.0;
        double vector_score = 0.0;
        double hybrid_score = 0.0;
        int bm25_rank = -1;
        int vector_rank = -1;
        std::string content;
        std::optional<double> geo_distance;
    };

    /**
     * @brief Diagnostic information returned alongside search results.
     *
     * Callers should check partial_result to detect degraded-mode responses.
     */
    struct SearchStats {
        bool bm25_ok = false;       ///< BM25 search ran without error
        bool vector_ok = false;     ///< Vector search ran without error
        bool partial_result = false;///< True when one source failed but the other succeeded
        size_t bm25_count = 0;      ///< Raw BM25 candidate count before fusion
        size_t vector_count = 0;    ///< Raw vector candidate count before fusion
    };

    explicit HybridSearch(
        SecondaryIndexManager* fulltext_index,
        VectorIndexManager* vector_index,
        const Config& config
    );
    ~HybridSearch();
    
    HybridSearch(const HybridSearch&) = delete;
    HybridSearch& operator=(const HybridSearch&) = delete;
    HybridSearch(HybridSearch&&) = default;
    HybridSearch& operator=(HybridSearch&&) = default;
    
    /**
     * @brief Perform hybrid search combining BM25 and vector search.
     *
     * Internally catches all exceptions from the index backends and returns
     * whatever partial results are available, logging errors via THEMIS_ERROR.
     * Never throws.
     *
     * @param text_query   Text query for BM25 search (pass "" to skip BM25)
     * @param vector_query Optional vector query for ANN search (nullptr to skip)
     * @param vector_dim   Dimension of vector_query; ignored when nullptr
     * @param stats        Optional output: filled with per-source diagnostics
     * @return Fused results ranked by hybrid score; may be partial if one
     *         source failed (indicated by stats.partial_result == true)
     */
    std::vector<Result> search(
        const std::string& text_query,
        const float* vector_query = nullptr,
        size_t vector_dim = 0,
        SearchStats* stats = nullptr
    );
    
    /**
     * @brief Fuse BM25 and vector results using Reciprocal Rank Fusion.
     *
     * RRF formula: score(d) = sum(1 / (k + rank_i(d)))
     * where k is a constant (default 60) and rank_i is the rank in result set i.
     * May throw std::bad_alloc; callers (search()) catch this.
     *
     * @param bm25_results   BM25 search results (ordered by score descending)
     * @param vector_results Vector search results (ordered by distance ascending)
     * @return Fused results sorted by hybrid score, limited to Config::k
     */
    std::vector<Result> reciprocalRankFusion(
        const std::vector<Result>& bm25_results,
        const std::vector<Result>& vector_results
    );
    
    const Config& getConfig() const { return config_; }
    void setConfig(const Config& config) { config_ = config; }

    /**
     * @brief Attach an LLM re-ranker to the search pipeline.
     *
     * When set, `search()` applies the re-ranker as a final step after RRF
     * fusion: the top-N fused results are scored by the LLM and returned in
     * the re-ranked order.  Calling with a null @p backend removes any
     * previously attached re-ranker (falls back to RRF order).
     *
     * @param backend  LLM callable (same signature as LlmReranker::LlmBackend).
     *                 Pass nullptr to disable.
     * @param config   Optional re-ranker configuration.
     */
    void setReranker(LlmReranker::LlmBackend backend,
                     const LlmReranker::Config& config = LlmReranker::Config{});

    /**
     * @brief Normalize scores in [min, max] to [0, 1].
     *
     * When all scores are equal (range == 0):
     *  - score > 0 → all normalized to 1.0
     *  - score == 0 → all normalized to 0.0
     *
     * @param results  Result list whose BM25 or vector scores are modified in place.
     * @param is_bm25  True to normalize bm25_score; false to normalize vector_score.
     */
    static void normalizeScores(std::vector<Result>& results, bool is_bm25);

private:
    SecondaryIndexManager* fulltext_index_;
    VectorIndexManager* vector_index_;
    Config config_;
    std::optional<LlmReranker> reranker_; ///< Optional LLM re-ranker (Phase 3)
};

} // namespace themis
