/**
 * @file multi_modal_search.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.43
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "index/secondary_index.h"
#include "index/vector_index.h"
#include <string>
#include <vector>

namespace themis {

/**
 * @brief Modality type for a multi-modal query.
 */
enum class Modality {
    TEXT,   ///< BM25 fulltext query
    IMAGE,  ///< Pre-computed image embedding (float vector)
    AUDIO,  ///< Pre-computed audio embedding (float vector)
    CUSTOM  ///< Any other modality expressed as a float vector
};

/**
 * @brief A single component of a multi-modal query.
 */
struct ModalQuery {
    Modality modality = Modality::TEXT;
    std::string text;                  ///< Used when modality == TEXT
    std::vector<float> embedding;      ///< Used for IMAGE / AUDIO / CUSTOM
    std::string embedding_namespace;   ///< VectorIndexManager object name for this modality
    double weight = 1.0;               ///< Contribution weight for score fusion
};

/**
 * @brief A multi-modal search result.
 */
struct MultiModalResult {
    std::string document_id;           ///< Primary key
    double score = 0.0;                ///< Fused relevance score
    std::string matched_modality;      ///< Which modality produced the best match
};

/**
 * @brief Unified search across text, image, and arbitrary-embedding modalities.
 *
 * MultiModalSearch accepts a list of `ModalQuery` components, executes each
 * query against the appropriate backend (fulltext index for TEXT modality,
 * VectorIndexManager for embedding modalities), and fuses the per-modality
 * result lists using Reciprocal Rank Fusion (RRF).
 *
 * This design keeps the module independent of any specific embedding model:
 * callers are responsible for generating the embeddings (e.g. via a CLIP
 * or Whisper model) and supplying the resulting float vectors.
 *
 * ### Usage
 * ```cpp
 * // Text + image query
 * MultiModalSearch::Config cfg;
 * cfg.k = 10;
 *
 * MultiModalSearch mms(&sec_index, &vec_index, cfg);
 *
 * std::vector<ModalQuery> queries = {
 *     { Modality::TEXT,  "sunset beach", {}, "text_ns",  0.6 },
 *     { Modality::IMAGE, "",    clip_vec, "image_ns", 1.0 },
 * };
 * auto results = mms.search(queries);
 * ```
 *
 * @note Thread Safety: A single MultiModalSearch instance is NOT thread-safe.
 * @note Exception Safety: `search()` never throws; errors are returned as an
 *   empty result list.
 */
class MultiModalSearch {
public:
    struct Config {
        size_t k = 10;                 ///< Results to return
        double rrf_k = 60.0;           ///< RRF smoothing constant
        size_t candidates_per_modal = 100; ///< How many candidates to fetch per modality
    };

    /**
     * @param sec_index  Non-owning pointer to a SecondaryIndexManager.  May be null
     *                   if no TEXT modality queries are used.
     * @param vec_index  Non-owning pointer to a VectorIndexManager.  May be null if
     *                   no embedding modalities are used.
     * @throws std::invalid_argument on invalid config.
     */
    explicit MultiModalSearch(SecondaryIndexManager* sec_index,
                              VectorIndexManager* vec_index);
    /**
     * @param sec_index  Non-owning pointer to a SecondaryIndexManager.  May be null
     *                   if no TEXT modality queries are used.
     * @param vec_index  Non-owning pointer to a VectorIndexManager.  May be null if
     *                   no embedding modalities are used.
     * @param config     Search configuration.
     * @throws std::invalid_argument on invalid config.
     */
    MultiModalSearch(SecondaryIndexManager* sec_index,
                     VectorIndexManager* vec_index,
                     const Config& config);

    // -----------------------------------------------------------------------
    // Search
    // -----------------------------------------------------------------------

    /**
     * @brief Execute a multi-modal search and return fused results.
     *
     * Each `ModalQuery` in `queries` is executed independently:
     * - TEXT queries use `SecondaryIndexManager::scanFulltextWithScores`.
     * - IMAGE / AUDIO / CUSTOM queries use `VectorIndexManager::searchKnn` on the
     *   specified `embedding_namespace`.
     *
     * Results from all modalities are fused via RRF and the top-k returned.
     *
     * @param queries   One or more modal query components.
     * @param table     Table name for TEXT modality fulltext lookup.
     * @param column    Column name for TEXT modality fulltext lookup.
     * @return Fused result list sorted by score descending.
     */
    std::vector<MultiModalResult> search(const std::vector<ModalQuery>& queries,
                                          const std::string& table = "",
                                          const std::string& column = "") const;

    /**
     * @brief Convenience: search with a single text + single image embedding.
     *
     * @param text_query       BM25 query string.
     * @param image_embedding  Pre-computed image embedding.
     * @param image_namespace  VectorIndexManager object name for images.
     * @param table            Table name for fulltext.
     * @param column           Column name for fulltext.
     * @param text_weight      Contribution weight for the text result list.
     * @param image_weight     Contribution weight for the image result list.
     */
    std::vector<MultiModalResult> searchTextAndImage(
        const std::string& text_query,
        const std::vector<float>& image_embedding,
        const std::string& image_namespace,
        const std::string& table,
        const std::string& column,
        double text_weight = 0.5,
        double image_weight = 1.0
    ) const;

    const Config& getConfig() const { return config_; }

private:
    SecondaryIndexManager* sec_index_;
    VectorIndexManager*    vec_index_;
    Config config_;

    // Execute one modal query, return (document_id, raw_score) pairs
    std::vector<std::pair<std::string, double>> executeModal(
        const ModalQuery& query,
        const std::string& table,
        const std::string& column
    ) const;

    // RRF fusion of multiple ranked lists with per-list weights and modality names
    std::vector<MultiModalResult> fuseRRF(
        const std::vector<std::vector<std::pair<std::string, double>>>& ranked_lists,
        const std::vector<double>& weights,
        const std::vector<std::string>& modality_names
    ) const;
};

} // namespace themis
