/**
 * @file hybrid_retriever.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=2, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "rag/rag_judge.h"
#include "rag/vectorizer_interface.h"

#include <memory>
#include <string>
#include <vector>

namespace themis::rag {

/**
 * @brief Configuration for HybridRetriever.
 */
struct HybridRetrieverConfig {
    /// Contribution weight for BM25 (sparse/keyword) results.
    /// Must be >= 0. Need not sum to 1 with vector_weight.
    double bm25_weight = 0.5;

    /// Contribution weight for vector (dense/semantic) results.
    /// Must be >= 0. Need not sum to 1 with bm25_weight.
    double vector_weight = 0.5;

    /// Use Reciprocal Rank Fusion (recommended).
    /// When false, a linear combination of normalised raw scores is used.
    bool use_rrf = true;

    /// RRF smoothing constant (default 60 per the original paper).
    /// Must be > 0.  Larger values reduce the rank-order sensitivity.
    double rrf_k = 60.0;

    /// Maximum number of documents returned after fusion (0 = return all).
    size_t top_k = 10;

    /// Normalise raw scores to [0, 1] before linear combination.
    /// Only relevant when use_rrf = false.
    bool normalize_scores = true;
};

/**
 * @brief Per-document score breakdown returned alongside each result.
 */
struct HybridScore {
    std::string document_id;  ///< Document identifier
    double bm25_score   = 0;  ///< Raw BM25 score (0 when not in BM25 results)
    double vector_score = 0;  ///< Raw vector score (0 when not in vector results)
    double hybrid_score = 0;  ///< Final fused score
    int    bm25_rank    = -1; ///< 1-based rank in BM25 list (-1 = absent)
    int    vector_rank  = -1; ///< 1-based rank in vector list (-1 = absent)
};

/**
 * @brief Result of a single HybridRetriever::fuse() call.
 */
struct HybridFusionResult {
    /// Fused documents sorted by descending hybrid_score.
    std::vector<judge::RetrievedDocument> documents;

    /// Per-document score details in the same order as @ref documents.
    std::vector<HybridScore> scores;

    /// Number of unique documents considered (union of both lists).
    size_t total_candidates = 0;

    /// True when RRF was used, false for linear combination.
    bool used_rrf = false;
};

/**
 * @brief RAG-level hybrid retriever fusing BM25 and vector candidate lists.
 *
 * Usage (RRF, default):
 * @code
 *   HybridRetrieverConfig cfg;
 *   cfg.bm25_weight   = 0.4;
 *   cfg.vector_weight = 0.6;
 *   cfg.top_k         = 10;
 *
 *   HybridRetriever retriever(cfg);
 *
 *   auto result = retriever.fuse(bm25_candidates, vector_candidates);
 *   for (const auto& doc : result.documents) {
 *       std::cout << doc.id << "  score=" << doc.similarity_score << "\n";
 *   }
 * @endcode
 *
 * Usage (linear combination):
 * @code
 *   HybridRetrieverConfig cfg;
 *   cfg.use_rrf        = false;
 *   cfg.bm25_weight    = 0.3;
 *   cfg.vector_weight  = 0.7;
 *   HybridRetriever retriever(cfg);
 *   auto result = retriever.fuse(bm25_candidates, vector_candidates);
 * @endcode
 *
 * Performance targets:
 *   - RRF mode: <1 ms for 100 candidates
 *   - Linear mode: <1 ms for 100 candidates
 *   - Recall@10 (RRF, balanced weights): ≥ 85 %
 */
class HybridRetriever {
public:
    /**
     * @brief Construct with default configuration (RRF, equal weights, top_k=10).
     */
    HybridRetriever();

    /**
     * @brief Construct with custom configuration.
     * @param config Retriever configuration.
     * @throws std::invalid_argument if any config parameter is invalid.
     */
    explicit HybridRetriever(const HybridRetrieverConfig& config);

    ~HybridRetriever() = default;

    HybridRetriever(const HybridRetriever&)            = default;
    HybridRetriever& operator=(const HybridRetriever&) = default;
    HybridRetriever(HybridRetriever&&)                 noexcept = default;
    HybridRetriever& operator=(HybridRetriever&&)      noexcept = default;

    /**
     * @brief Fuse BM25 and vector candidate lists into a single ranked list.
     *
     * Either or both candidate lists may be empty; fusion degrades gracefully
     * to the non-empty list in that case.
     *
     * The @c similarity_score field of every returned document is set to the
     * fused hybrid score.
     *
     * @param bm25_candidates   BM25 / sparse-retrieval results in any order.
     *                          The @c similarity_score field is used as the
     *                          raw BM25 score when use_rrf = false.
     * @param vector_candidates Vector / dense-retrieval results in any order.
     *                          The @c similarity_score field is used as the
     *                          raw vector score when use_rrf = false.
     * @return Fused result with documents sorted by descending hybrid score.
     */
    HybridFusionResult fuse(
        const std::vector<judge::RetrievedDocument>& bm25_candidates,
        const std::vector<judge::RetrievedDocument>& vector_candidates
    ) const;

    /**
     * @brief Inject a dense vectorizer used by @ref retrieveWithVectorizer().
     *
     * Passing `nullptr` disables the integration path. The vectorizer must be
     * initialized before calling @ref retrieveWithVectorizer().
     *
     * @param vectorizer Shared vectorizer instance (may be null).
     */
    void setVectorizer(std::shared_ptr<IVectorizer> vectorizer);

    /**
     * @brief Return the currently configured vectorizer (may be null).
     */
    [[nodiscard]] std::shared_ptr<IVectorizer> getVectorizer() const;

    /**
     * @brief Build dense candidates via configured @ref IVectorizer and fuse them with BM25.
     *
     * This is an integration helper for DPR-style bi-encoders: query text is
     * encoded via `encodeQuery()`, each BM25 candidate content is encoded via
     * `encodePassage()`, cosine similarity is used as dense score, then standard
     * @ref fuse() is applied.
     *
     * @param query Query text to encode.
     * @param bm25_candidates BM25/sparse candidates (input pool for dense scoring).
     * @return Fused hybrid result.
     * @throws std::invalid_argument if query is empty.
     * @throws std::runtime_error if no vectorizer is configured or not initialized.
     */
    [[nodiscard]] HybridFusionResult retrieveWithVectorizer(
        const std::string& query,
        const std::vector<judge::RetrievedDocument>& bm25_candidates
    ) const;

    /**
     * @brief Return current configuration.
     */
    const HybridRetrieverConfig& getConfig() const;

    /**
     * @brief Update configuration.
     * @throws std::invalid_argument if any config parameter is invalid.
     */
    void setConfig(const HybridRetrieverConfig& config);

    /**
     * @brief Validate a configuration object.
     * @throws std::invalid_argument describing the first violation found.
     */
    static void validateConfig(const HybridRetrieverConfig& config);

private:
    HybridRetrieverConfig config_;
    std::shared_ptr<IVectorizer> vectorizer_;

    /// RRF fusion path (use_rrf = true).
    HybridFusionResult fuseRRF(
        const std::vector<judge::RetrievedDocument>& bm25_candidates,
        const std::vector<judge::RetrievedDocument>& vector_candidates
    ) const;

    /// Linear combination path (use_rrf = false).
    HybridFusionResult fuseLinear(
        const std::vector<judge::RetrievedDocument>& bm25_candidates,
        const std::vector<judge::RetrievedDocument>& vector_candidates
    ) const;
};

/**
 * @brief Factory helpers for common HybridRetriever configurations.
 */
class HybridRetrieverFactory {
public:
    /**
     * @brief Balanced RRF retriever: equal BM25 and vector weights (0.5/0.5).
     * @param top_k Number of documents to return.
     */
    static HybridRetriever createBalanced(size_t top_k = 10);

    /**
     * @brief Semantic-focused retriever: higher vector weight (0.3 BM25 / 0.7 vector).
     * @param top_k Number of documents to return.
     */
    static HybridRetriever createSemanticFocused(size_t top_k = 10);

    /**
     * @brief Keyword-focused retriever: higher BM25 weight (0.7 BM25 / 0.3 vector).
     * @param top_k Number of documents to return.
     */
    static HybridRetriever createKeywordFocused(size_t top_k = 10);
};

} // namespace themis::rag
