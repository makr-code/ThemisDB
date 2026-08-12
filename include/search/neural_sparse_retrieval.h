/**
 * @file neural_sparse_retrieval.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis {

/**
 * @brief Sparse term-weight vector produced by a neural encoder (e.g. SPLADE).
 *
 * Maps vocabulary term strings to their learned activation weights.  Only
 * non-zero terms are stored (sparse representation).  Weight values must be
 * non-negative; negative weights are silently clamped to zero.
 */
using SparseVector = std::unordered_map<std::string, float>;

/**
 * @brief Callable that encodes a text string into a SparseVector.
 *
 * Implementations should produce non-negative term weights.  The backend is
 * injected at runtime, keeping this class free of hard ML-library dependencies
 * (the same pattern used by LlmReranker::LlmBackend).
 *
 * Example (identity encoder for testing):
 * ```cpp
 * SparseEncoderBackend enc = [](const std::string& text) {
 *     SparseVector sv;
 *     for (auto& token : tokenize(text))
 *         sv[token] += 1.0f;
 *     return sv;
 * };
 * ```
 */
using SparseEncoderBackend = std::function<SparseVector(const std::string&)>;

/**
 * @brief SPLADE / BERT-based neural sparse retrieval engine.
 *
 * NeuralSparseRetrieval implements learned-sparse-representation search:
 *
 * 1. **Index**: documents are encoded into `SparseVector` representations via an
 *    injected `SparseEncoderBackend`.  Each document's non-zero terms are
 *    inserted into an in-memory inverted index (term → [(doc_id, weight)]).
 * 2. **Search**: the query is encoded into a `SparseVector` and matched against
 *    the inverted index using dot-product accumulation.  Only documents sharing
 *    at least one non-zero term with the query are scored, keeping the operation
 *    efficient for typical sparse representations.
 * 3. **Score**: `score(q, d) = Σ_t( q[t] * d[t] )` — standard inner product
 *    over shared terms, matching the scoring model used by SPLADE/uniCOIL.
 * 4. **Normalization**: optional linear rescaling of scores to [0, 1] (same
 *    pattern as `HybridSearch::normalizeScores`).
 *
 * ### Typical usage
 * ```cpp
 * NeuralSparseRetrieval::Config cfg;
 * cfg.k = 10;
 * NeuralSparseRetrieval nsr(cfg);
 *
 * // Attach a SPLADE-compatible encoder backend
 * nsr.setEncoder([&](const std::string& text) {
 *     return my_splade_model.encode(text);
 * });
 *
 * // Index documents
 * nsr.addDocumentText("doc1", "fast in-memory database engine");
 * nsr.addDocumentText("doc2", "neural sparse retrieval with SPLADE");
 *
 * // Query
 * auto results = nsr.searchText("database performance");
 * for (auto& r : results)
 *     std::cout << r.document_id << "  score=" << r.score << "\n";
 * ```
 *
 * Pre-computed sparse vectors can also be provided directly via `addDocument()`
 * and `search()`, allowing callers to encode outside the engine (e.g. for batch
 * processing or when reusing embeddings).
 *
 * @note Thread Safety: A single instance is NOT thread-safe.  Callers sharing
 *   an instance across threads must provide external synchronization.
 * @note Exception Safety: The constructor throws `std::invalid_argument` on
 *   invalid config.  `search()` and `searchText()` never throw; all exceptions
 *   from the encoder backend are caught and result in an empty result vector.
 *   `addDocumentText()` propagates exceptions from the encoder backend.
 */
class NeuralSparseRetrieval {
public:
    // -----------------------------------------------------------------------
    // Types
    // -----------------------------------------------------------------------

    /**
     * @brief Engine-level configuration.
     */
    struct Config {
        size_t k = 10;                 ///< Maximum results to return
        size_t max_terms_per_doc = 512;///< Soft cap on terms per document sparse vector
        float  score_threshold = 0.0f; ///< Minimum raw score for a result to be included
        bool   normalize_scores = true;///< Rescale result scores to [0, 1] before returning

        static Config defaults() { return {}; }
    };

    /**
     * @brief A single search result.
     */
    struct Result {
        std::string document_id;     ///< Primary key of the matching document
        float score = 0.0f;          ///< Final score (normalised if Config::normalize_scores)
        float raw_score = 0.0f;      ///< Inner-product score before normalisation
    };

    // -----------------------------------------------------------------------
    // Construction
    // -----------------------------------------------------------------------

    /**
     * @param config  Engine configuration.
     * @throws std::invalid_argument on invalid config (k == 0 or
     *   max_terms_per_doc == 0 or score_threshold < 0).
     */
    explicit NeuralSparseRetrieval(const Config& config = Config::defaults());

    // -----------------------------------------------------------------------
    // Encoder management
    // -----------------------------------------------------------------------

    /**
     * @brief Attach a sparse encoder backend.
     *
     * The backend is called by `addDocumentText()` and `searchText()` to
     * produce `SparseVector` representations from raw text.  Passing nullptr
     * removes any previously attached backend; subsequent calls to
     * `addDocumentText()` / `searchText()` will return empty without throwing.
     *
     * @param encoder  Callable conforming to `SparseEncoderBackend`.
     */
    void setEncoder(SparseEncoderBackend encoder);

    // -----------------------------------------------------------------------
    // Indexing
    // -----------------------------------------------------------------------

    /**
     * @brief Index a document with a pre-computed sparse vector.
     *
     * If a document with the same @p doc_id already exists it is removed first,
     * then re-indexed with the new vector.  Negative term weights in
     * @p sparse_vec are clamped to zero.  If the number of non-zero terms
     * exceeds `Config::max_terms_per_doc`, only the top-weighted terms are kept.
     *
     * @param doc_id      Unique document identifier (primary key).
     * @param sparse_vec  Pre-computed sparse representation of the document.
     */
    void addDocument(const std::string& doc_id, const SparseVector& sparse_vec);

    /**
     * @brief Encode @p text and index the resulting sparse vector.
     *
     * Calls the attached encoder backend.  If no encoder is set, the call is a
     * no-op (the document is not indexed).
     *
     * @param doc_id  Unique document identifier (primary key).
     * @param text    Raw document text passed to the encoder.
     * @throws Any exception propagated from the encoder backend.
     */
    void addDocumentText(const std::string& doc_id, const std::string& text);

    /**
     * @brief Remove a document from the index.
     *
     * If the document is not found this is a no-op.
     *
     * @param doc_id  Document identifier to remove.
     */
    void removeDocument(const std::string& doc_id);

    /**
     * @brief Remove all documents from the index.
     */
    void clear();

    // -----------------------------------------------------------------------
    // Search
    // -----------------------------------------------------------------------

    /**
     * @brief Search using a pre-computed query sparse vector.
     *
     * Computes `score(q, d) = Σ_t( q[t] * d[t] )` via inverted-index
     * accumulation and returns the top-k results above `score_threshold`.
     * Optionally normalizes scores to [0, 1] when `Config::normalize_scores`
     * is true.  Never throws.
     *
     * @param query_vec  Pre-computed sparse query representation.
     * @param k          Override for Config::k (0 means use Config::k).
     * @return Results sorted by score descending, limited to k.
     */
    std::vector<Result> search(const SparseVector& query_vec, size_t k = 0) const;

    /**
     * @brief Encode @p query_text and search.
     *
     * Calls the attached encoder backend to produce a sparse query vector, then
     * delegates to `search()`.  Returns empty when no encoder is set or when
     * the encoder throws (exception is caught and logged).  Never throws.
     *
     * @param query_text  Raw query string.
     * @param k           Override for Config::k (0 means use Config::k).
     * @return Results sorted by score descending, limited to k.
     */
    std::vector<Result> searchText(const std::string& query_text, size_t k = 0) const;

    // -----------------------------------------------------------------------
    // Utilities
    // -----------------------------------------------------------------------

    /**
     * @brief Number of documents currently indexed.
     */
    size_t size() const;

    const Config& getConfig() const { return config_; }
    void setConfig(const Config& config) { config_ = config; }

    /**
     * @brief Normalize result scores to [0, 1] in place.
     *
     * When all scores are equal (range == 0):
     *  - score > 0 → all normalized to 1.0
     *  - score == 0 → all normalized to 0.0
     *
     * Promoted to public static for direct unit testing (same pattern as
     * `HybridSearch::normalizeScores`).
     */
    static void normalizeScores(std::vector<Result>& results);

private:
    Config config_;
    SparseEncoderBackend encoder_;

    // Inverted index: term -> list of (doc_id, term_weight) pairs
    std::unordered_map<std::string, std::vector<std::pair<std::string, float>>> inverted_index_;

    // Forward index: doc_id -> sparse vector (needed for clean removal)
    std::unordered_map<std::string, SparseVector> forward_index_;

    // Internal helper: insert a validated + truncated sparse vector
    void insertVector(const std::string& doc_id, const SparseVector& vec);

    // Internal helper: remove a document from the inverted index
    void eraseFromIndex(const std::string& doc_id, const SparseVector& vec);

    // Internal helper: clamp and optionally truncate a sparse vector
    static SparseVector sanitize(const SparseVector& raw, size_t max_terms);
};

} // namespace themis
