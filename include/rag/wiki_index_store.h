/**
 * @file wiki_index_store.h
 * @brief WikiIndexStore — BM25+, RRF fusion, HNSW stub, and persistent
 *        embedding cache interface for ThemisDB RAG Wave 5/7.
 *
 * @note Production-ready components: BM25+ scoring, RRF fusion,
 *       positional index, phrase queries, proximity queries (Wave 7).
 * @note STUB components: HNSW index backend, RocksDB persistent cache
 *       (see STUB/SIMULATION NOTEs in wiki_index_store.cpp).
 *
 * Thread-safety: All public methods are thread-safe via internal mutex.
 */

#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace themis::rag {

/// @brief Result of a BM25+ ranking or RRF fusion query.
struct IndexResult {
    std::string doc_id;
    float       score{0.0f};
};

// ─────────────────────────────────────────────────────────────────────────────
// BM25+ scoring (production-ready)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Compute BM25+ score for a single document against a set of query terms.
 *
 * BM25+ formula per term t:
 *   score(t,d) = IDF(t) * [ (tf(t,d) * (k1+1)) / (tf(t,d) + k1*(1-b+b*dl/avgdl)) + delta ]
 *
 * Parameters: k1 = 1.5, b = 0.75, delta = 0.5 (BM25+ lower-bound correction).
 *
 * @param query_terms   Tokenised query terms (duplicates are summed).
 * @param doc_text      Raw document text (whitespace-tokenised internally).
 * @param avg_doc_len   Corpus average document length in tokens.
 * @param idf_map       Pre-computed IDF values per term.  Missing terms → IDF 0.
 * @return              BM25+ relevance score (non-negative).
 */
float bm25PlusScore(const std::vector<std::string>&          query_terms,
                    const std::string&                       doc_text,
                    float                                    avg_doc_len,
                    const std::unordered_map<std::string, float>& idf_map);

// ─────────────────────────────────────────────────────────────────────────────
// Reciprocal Rank Fusion (production-ready)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Fuse multiple ranked lists using Reciprocal Rank Fusion (RRF).
 *
 * RRF(d) = Σ_r [ 1 / (k + rank_r(d)) ]  where k = 60 (default).
 *
 * @param ranked_lists  Each inner vector is an ordered list of doc_ids
 *                      (most relevant first).
 * @param k             RRF constant (higher k reduces the penalty for low ranks).
 * @return              Merged list sorted by descending RRF score.
 */
std::vector<IndexResult> rrfFusion(
    const std::vector<std::vector<std::string>>& ranked_lists,
    int k = 60);

// ─────────────────────────────────────────────────────────────────────────────
// WikiIndexStore — composite index façade
// ─────────────────────────────────────────────────────────────────────────────

/// @brief Configuration for WikiIndexStore.
struct WikiIndexStoreConfig {
    float avg_doc_len{128.0f}; ///< Corpus average document length.
    int   rrf_k{60};           ///< RRF constant.
};

/**
 * @brief Composite index store providing BM25+ lexical search and RRF fusion.
 *
 * @note HNSW vector search and RocksDB persistent embedding cache are
 *       architectural stubs in this release — see STUB/SIMULATION NOTEs in
 *       the implementation file.
 */
class WikiIndexStore {
public:
    using Config = WikiIndexStoreConfig;

    explicit WikiIndexStore(Config cfg = Config{});
    ~WikiIndexStore();

    // Non-copyable, movable.
    WikiIndexStore(const WikiIndexStore&)            = delete;
    WikiIndexStore& operator=(const WikiIndexStore&) = delete;
    WikiIndexStore(WikiIndexStore&&)                 = default;
    WikiIndexStore& operator=(WikiIndexStore&&)      = default;

    /// @brief Index a document for BM25+ retrieval.
    void addDocument(const std::string& doc_id, const std::string& text);

    /// @brief BM25+ ranked search.
    std::vector<IndexResult> searchBM25(
        const std::vector<std::string>& query_terms,
        size_t top_k = 10) const;

    /**
     * @brief Exact phrase search using positional index.
     *
     * Tokenises @p phrase with the same whitespace tokeniser used during
     * indexing, then returns documents where the terms appear as a
     * consecutive run (pos[i+1] == pos[i]+1 for every adjacent pair).
     * Results are BM25+-scored and sorted descending.
     *
     * Edge cases:
     *  - Single-term phrase → equivalent to searchBM25.
     *  - Empty phrase       → returns empty vector.
     *
     * @param phrase  Raw phrase string.
     * @param top_k   Maximum results to return.
     * @return        Ranked results, descending score.
     */
    std::vector<IndexResult> searchPhrase(
        const std::string& phrase,
        size_t top_k = 10) const;

    /**
     * @brief Proximity search: docs where term1 and term2 are ≤ distance apart.
     *
     * Uses the positional index to find the minimum token-distance between
     * any occurrence of term1 and any occurrence of term2 within the same
     * document.  Only documents satisfying the distance constraint are
     * returned.
     *
     * Edge cases:
     *  - term1 == term2   → requires ≥2 occurrences whose mutual distance
     *                        satisfies the constraint.
     *  - Either term absent in corpus → returns empty vector.
     *
     * @param term1     First term (pre-tokenised, lowercase).
     * @param term2     Second term (pre-tokenised, lowercase).
     * @param distance  Maximum allowed token distance (inclusive).
     * @param top_k     Maximum results to return.
     * @return          Ranked results, descending score.
     */
    std::vector<IndexResult> searchProximity(
        const std::string& term1,
        const std::string& term2,
        size_t distance,
        size_t top_k = 10) const;

    /// @brief Fuse the provided ranked lists with RRF.
    std::vector<IndexResult> fuseRRF(
        const std::vector<std::vector<std::string>>& ranked_lists) const;

    /// @brief Clear all indexed documents.
    void clear();

    /// @brief Number of indexed documents.
    size_t size() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace themis::rag
