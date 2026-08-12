/**
 * @file cross_lingual_search.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "index/vector_index.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace themis {

/**
 * @brief Cross-lingual semantic search using multilingual embeddings.
 *
 * CrossLingualSearch enables retrieving documents across language boundaries
 * by leveraging multilingual embedding models (e.g.
 * paraphrase-multilingual-mpnet-base-v2, LaBSE) that project text from
 * different languages into a shared vector space.  The class is
 * model-agnostic: callers are responsible for generating the embeddings and
 * supply pre-computed float vectors.
 *
 * ### Core operations
 *
 * 1. **Single-embedding search** (`search()`): issues a kNN query against the
 *    multilingual vector index and optionally applies per-language boost
 *    factors.
 *
 * 2. **Multi-embedding fusion** (`searchMultiEmbedding()`): issues kNN queries
 *    for several embeddings (e.g. query expressed in EN + DE + FR) and merges
 *    the ranked lists via Reciprocal Rank Fusion (RRF), enabling ensemble
 *    retrieval across language variants.
 *
 * ### Language enrichment
 *
 * When a `lang_map` (`doc_id → language_code`) is supplied via
 * `setLanguageMap()`, results are annotated with the document language.  The
 * same map is used to apply `LanguageHint` boosts: results whose language
 * matches a hint are multiplied by `hint.boost` after fusion.
 *
 * ### Typical usage
 * ```cpp
 * CrossLingualSearch::Config cfg;
 * cfg.k = 10;
 * CrossLingualSearch cls(&vec_index, cfg);
 *
 * // Optional: annotate results with per-document language information
 * cls.setLanguageMap({{"doc1", "en"}, {"doc2", "de"}, {"doc3", "fr"}});
 *
 * // Search with a multilingual embedding (model output for "machine learning")
 * std::vector<CrossLingualSearch::LanguageHint> hints = {
 *     {"en", 1.2},   // slight preference for English results
 * };
 * auto results = cls.search(query_embedding, hints);
 *
 * // Multi-embedding fusion across language variants
 * auto results2 = cls.searchMultiEmbedding({
 *     {en_embedding, 1.0},
 *     {de_embedding, 0.8},
 *     {fr_embedding, 0.8},
 * }, hints);
 * ```
 *
 * @note Thread Safety: A single CrossLingualSearch instance is NOT
 *   thread-safe.  Create separate instances per thread or provide external
 *   synchronisation.
 * @note Exception Safety: `search()` and `searchMultiEmbedding()` never
 *   throw.  All exceptions from the vector backend are caught internally
 *   and result in an empty result vector.
 */
class CrossLingualSearch {
public:
    // -----------------------------------------------------------------------
    // Types
    // -----------------------------------------------------------------------

    /**
     * @brief Engine-level configuration.
     */
    struct Config {
        size_t k = 10;                   ///< Maximum results to return
        size_t candidates = 100;         ///< kNN candidates to retrieve per query
        double score_threshold = 0.0;    ///< Minimum similarity score [0, 1]
        double rrf_k = 60.0;             ///< RRF smoothing constant (multi-embedding)
        size_t max_k = 10'000;           ///< Hard upper bound for k
        size_t max_candidates = 10'000;  ///< Hard upper bound for candidates
    };

    /**
     * @brief A per-language boost hint.
     *
     * Results whose document language matches `language_code` (from the
     * language map) have their final score multiplied by `boost`.
     */
    struct LanguageHint {
        std::string language_code;  ///< ISO 639-1 code (e.g. "en", "de", "fr")
        double boost = 1.0;         ///< Score multiplier (> 1.0 promotes, < 1.0 demotes)
    };

    /**
     * @brief A single query embedding with its fusion weight.
     */
    struct EmbeddingQuery {
        std::vector<float> embedding;  ///< Pre-computed multilingual embedding
        double weight = 1.0;           ///< Relative weight in RRF fusion
    };

    /**
     * @brief A single cross-lingual search result.
     */
    struct Result {
        std::string document_id;   ///< Primary key of the matching document
        double score = 0.0;        ///< Relevance score (language boosts applied)
        std::string language;      ///< ISO 639-1 code from language map (may be empty)
    };

    // -----------------------------------------------------------------------
    // Construction
    // -----------------------------------------------------------------------

    /**
     * @brief Construct a CrossLingualSearch engine.
     *
     * @param vec_index  Non-owning pointer to a VectorIndexManager.  May be
     *                   null; all searches will return empty results.
     * @throws std::invalid_argument when config contains invalid values.
     */
    explicit CrossLingualSearch(VectorIndexManager* vec_index);
    /**
     * @brief Construct a CrossLingualSearch engine.
     *
     * @param vec_index  Non-owning pointer to a VectorIndexManager.  May be
     *                   null; all searches will return empty results.
     * @param config     Engine configuration.
     * @throws std::invalid_argument when config contains invalid values.
     */
    CrossLingualSearch(VectorIndexManager* vec_index,
                       const Config& config);

    // -----------------------------------------------------------------------
    // Language map
    // -----------------------------------------------------------------------

    /**
     * @brief Supply a document-language map for result annotation and boosts.
     *
     * Maps document primary keys to ISO 639-1 language codes (e.g. "en",
     * "de", "fr").  Replaces any previously set map.
     *
     * @param lang_map  Map of doc_id → language_code.
     */
    void setLanguageMap(std::unordered_map<std::string, std::string> lang_map);

    // -----------------------------------------------------------------------
    // Search
    // -----------------------------------------------------------------------

    /**
     * @brief Search with a single multilingual embedding.
     *
     * Issues a kNN query on the vector index, converts distances to
     * similarity scores via `1 / (1 + distance)`, applies language boost
     * factors from @p language_hints, filters candidates below
     * `Config::score_threshold`, and returns the top-k results.
     *
     * @param query_embedding  Pre-computed multilingual embedding vector.
     *                         Empty vector returns empty results immediately.
     * @param language_hints   Optional per-language boost factors.
     * @return Results sorted by score descending, capped at Config::k.
     */
    std::vector<Result> search(
        const std::vector<float>& query_embedding,
        const std::vector<LanguageHint>& language_hints = {}
    ) const;

    /**
     * @brief Fuse results from multiple embeddings via Reciprocal Rank Fusion.
     *
     * Executes an independent kNN query for each `EmbeddingQuery`, merges the
     * ranked lists using weighted RRF, then applies language boosts and score
     * threshold filtering.  Queries with empty embeddings are skipped silently.
     *
     * RRF formula (per query list i):
     * ```
     * score(doc) += weight_i / (rrf_k + rank_i(doc))
     * ```
     *
     * @param queries         One or more (embedding, weight) pairs.
     * @param language_hints  Optional per-language boost factors.
     * @return Fused results sorted by score descending, capped at Config::k.
     */
    std::vector<Result> searchMultiEmbedding(
        const std::vector<EmbeddingQuery>& queries,
        const std::vector<LanguageHint>& language_hints = {}
    ) const;

    // -----------------------------------------------------------------------
    // Accessors
    // -----------------------------------------------------------------------

    const Config& getConfig() const { return config_; }
    void setConfig(const Config& config) { config_ = config; }

private:
    VectorIndexManager* vec_index_;
    Config config_;
    std::unordered_map<std::string, std::string> lang_map_;  ///< doc_id → language_code

    // Execute a single kNN query; returns (doc_id, similarity_score) sorted desc
    std::vector<std::pair<std::string, double>> executeKnn(
        const std::vector<float>& embedding
    ) const;

    // Apply language boosts, score threshold, and populate language field
    std::vector<Result> applyHintsAndFinalize(
        std::vector<std::pair<std::string, double>> scored,
        const std::vector<LanguageHint>& hints
    ) const;
};

} // namespace themis
