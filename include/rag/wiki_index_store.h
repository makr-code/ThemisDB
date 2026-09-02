/**
 * @file wiki_index_store.h
 * @brief WikiIndexStore — BM25+, RRF fusion, HNSW (hnswlib or fallback),
 *        and RocksDB-backed persistent embedding cache.
 *
 * @note Production-ready components: BM25+ scoring, RRF fusion,
 *       positional index, phrase queries, proximity queries (Wave 7).
 * @note HNSW: wired against hnswlib when THEMIS_HNSW_ENABLED is set by the
 *       build system; falls back to exhaustive cosine scan otherwise.
 * @note Persistent cache: RocksDB CF "embedding_cache" used when
 *       THEMIS_ROCKSDB_AVAILABLE and WikiIndexStoreConfig::cache_dir is set;
 *       in-memory LRU always active when max_cache_size > 0.
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

    // ─── W8-20: HNSW parameters ───────────────────────────────────────────
    /// Enable HNSW approximate nearest-neighbour search.
    /// Requires @c addVector() calls and a wired backend (Wave B).
    bool   enable_hnsw{false};
    /// ef search parameter (query-time accuracy/speed trade-off).
    size_t hnsw_ef{200};
    /// M connections per new node (graph connectivity).
    size_t hnsw_m{16};
    /// Maximum connections per node in layer 0 (= 2 × M by default).
    size_t hnsw_max_m0{32};
    /// ef_construction — candidate list size during index build.
    size_t hnsw_ef_construction{200};

    // ─── W8-20: Persistent embedding cache parameters ─────────────────────
    /// Filesystem path for the RocksDB embedding cache; empty = disabled.
    std::string cache_dir{};
    /// Per-entry TTL in seconds (0 = no expiry).
    int    cache_ttl_seconds{3600};
    /// Maximum number of in-memory cached embeddings (LRU eviction above cap).
    size_t max_cache_size{10000};
};

/**
 * @brief Composite index store providing BM25+ lexical search and RRF fusion.
 *
 * @note HNSW vector search is wired against hnswlib (when THEMIS_HNSW_ENABLED)
 *       or falls back to an exhaustive cosine scan.  RocksDB persistent
 *       embedding cache is wired when THEMIS_ROCKSDB_AVAILABLE and
 *       WikiIndexStoreConfig::cache_dir is non-empty.
 */
class WikiIndexStore {
public:
    using Config = WikiIndexStoreConfig;

    explicit WikiIndexStore(Config cfg = Config{});
    ~WikiIndexStore();

    // Non-copyable, movable.
    WikiIndexStore(const WikiIndexStore&)            = delete;
    WikiIndexStore& operator=(const WikiIndexStore&) = delete;
    WikiIndexStore(WikiIndexStore&&) noexcept;
    WikiIndexStore& operator=(WikiIndexStore&&) noexcept;

    /**
     * @brief Index a document for BM25+ retrieval.
     * @param doc_id  Stable document identifier.
     * @param text    Raw document text to tokenize and index.
     */
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

    /**
     * @brief Fuse the provided ranked lists with reciprocal-rank fusion.
     * @param ranked_lists  Ranked document-id lists to combine.
     * @return RRF-ranked results derived from the supplied lists.
     */
    std::vector<IndexResult> fuseRRF(
        const std::vector<std::vector<std::string>>& ranked_lists) const;

    /// @brief Clear all indexed documents.
    void clear();

    /**
     * @brief Return the number of indexed documents.
     * @return Count of documents currently stored in the index.
     */
    size_t size() const;

    // ─── W8-18: HNSW vector index ─────────────────────────────────────────

    /**
     * @brief Add a dense embedding vector for @p doc_id.
     *
     * [W8-18] Stores the vector in the in-memory HNSW injection-bridge
     * backend if @c WikiIndexStoreConfig::enable_hnsw is true.  When the
     * backend is disabled (compile-time flag @c THEMIS_HNSW_BACKEND not set
     * or @c enable_hnsw is false) the call is a no-op and logs a warning.
     *
     * @param doc_id     Document identifier (must match an already-indexed
     *                   document to allow hybrid fusion).
     * @param embedding  Dense vector; all calls must use the same dimension.
     * @throws std::invalid_argument if @p embedding is empty.
     */
    void addVector(const std::string& doc_id,
                   const std::vector<float>& embedding);

    /**
     * @brief Approximate nearest-neighbour search over stored vectors.
     *
     * [W8-18] Returns up to @p top_k doc_ids ranked by cosine similarity.
     * When HNSW backend is disabled this returns an empty vector and logs a
     * warning.
     *
     * @param query_embedding  Query dense vector (same dimension as indexed).
     * @param top_k            Maximum results.
     * @return                 Ranked results (score = cosine similarity ∈ [0,1]).
     */
    std::vector<IndexResult> searchHNSW(
        const std::vector<float>& query_embedding,
        size_t top_k = 10) const;

    // ─── W8-19: Persistent embedding cache ────────────────────────────────

    /**
     * @brief Persist an embedding vector for @p key in the RocksDB cache.
     *
     * [W8-19] Writes the embedding to the RocksDB column family under
     * SHA-256(@p key + model_id).  No-op when @c cache_dir is empty or
     * @c THEMIS_ROCKSDB_CACHE is not defined.
     *
     * @param key        Cache key (typically doc_id or content hash).
     * @param embedding  Dense vector to persist.
     */
    void cacheEmbedding(const std::string& key,
                        const std::vector<float>& embedding);

    /**
     * @brief Retrieve a cached embedding for @p key.
     *
     * [W8-19] Returns the stored vector on cache hit, or an empty vector on
     * miss/expiry.  No-op (returns {}) when @c cache_dir is empty.
     *
     * @param key  Cache key (same key used in @c cacheEmbedding).
     * @return     Stored embedding or empty vector on miss.
     */
    std::vector<float> retrieveEmbedding(const std::string& key) const;

    // ─── W8-21: Hybrid retrieval ───────────────────────────────────────────

    /**
     * @brief Hybrid BM25+ + HNSW search fused with RRF.
     *
     * [W8-21] Runs BM25+ lexical search on @p query_terms and HNSW
     * approximate nearest-neighbour search on @p query_embedding in
     * parallel, then fuses both ranked lists with @c fuseRRF().
     *
     * When HNSW is disabled (config or compile flag) only the BM25+ list is
     * used — the result is identical to @c searchBM25().
     *
     * @param query_terms      Tokenised BM25+ query.
     * @param query_embedding  Dense query vector for HNSW (may be empty when
     *                         HNSW is disabled; caller must not mix empty and
     *                         populated embeddings across add/search calls).
     * @param top_k            Maximum results.
     * @return                 Fused results, descending score.
     */
    std::vector<IndexResult> searchHybrid(
        const std::vector<std::string>& query_terms,
        const std::vector<float>&       query_embedding,
        size_t                          top_k = 10) const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace themis::rag
