/**
 * @file wiki_index_store.h
 * @brief Dual secondary index (BM25 + vector) for LLM Wiki chunks.
 *
 * Provides read/write interfaces over a hybrid retrieval back-end that
 * combines a fulltext (BM25) secondary index from `SecondaryIndexManager`
 * with a dense vector HNSW index from `VectorIndexManager`, fused via
 * `HybridRetriever`.  A lightweight `JsonWikiIndexReader` (Phase A fallback)
 * reads the Python-MVP `index.json` format and performs in-memory BM25
 * scoring without a live RocksDB instance.
 *
 * ## Usage
 *
 * ### Phase A — JSON fallback (no RocksDB)
 * @code
 *   JsonWikiIndexReader reader("artifacts/llm-wiki-mvp/index.json");
 *   reader.load();
 *   auto chunks = reader.query("vector index design", 5, 0.0f);
 * @endcode
 *
 * ### Phase B — Full hybrid store
 * @code
 *   WikiIndexStore store(sim, vim, llm, WikiIndexConfig{});
 *   store.writeBatch(chunks);
 *   auto results = store.query("HNSW cosine similarity", 10, 0.05f);
 * @endcode
 *
 * ## Wiring into ModularRAGPipeline
 *
 * `WikiRagSource::retrieveFromWiki` implements `RAGStageHandler` and can be
 * wired directly into `ModularRAGPipelineConfig::retrieve_fn`:
 * @code
 *   WikiRagSource wrs(reader);
 *   cfg.retrieve_fn = [&wrs](ModularRAGContext& ctx) {
 *       return wrs.retrieveFromWiki(ctx);
 *   };
 * @endcode
 *
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 */

#pragma once

#include "index/secondary_index.h"
#include "index/vector_index.h"
#include "llm/embedded_llm.h"
#include "rag/hybrid_retriever.h"
#include "storage/base_entity.h"

#include <mutex>
#include <shared_mutex>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <atomic>

namespace themis {
namespace llm {

// ============================================================================
// WikiChunk — a single indexed text chunk from a wiki/markdown document
// ============================================================================

/**
 * @brief A single text chunk extracted from a wiki or Markdown file.
 *
 * Chunks are produced by `WikiChunkSplitter` and stored/queried via
 * `IWikiIndexWriter` / `IWikiIndexReader`.
 */
struct WikiChunk {
    std::string chunk_id;       ///< Stable deterministic ID (FNV-64 hex, 12 chars + "-" + seq)
    std::string doc_id;         ///< Source document identifier (normalised file path)
    std::string section_title;  ///< Heading of the containing section ("" for preamble)
    int         line_start = 0; ///< 1-based inclusive start line within the source file
    int         line_end   = 0; ///< 1-based inclusive end line within the source file
    std::string text;           ///< Raw chunk text
    std::vector<float> embedding; ///< Dense embedding (empty until indexed by WikiIndexStore)
    float       score = 0.0f;   ///< Retrieval score populated by query results
    std::string source_path;    ///< Absolute or relative path of the originating file
};

// ============================================================================
// WikiIndexConfig
// ============================================================================

/**
 * @brief Configuration for `WikiIndexStore`.
 */
struct WikiIndexConfig {
    std::string table_name    = "wiki_chunks"; ///< RocksDB table / namespace
    int         embedding_dim = 384;            ///< Expected embedding dimensionality
    int         top_k         = 10;             ///< Default retrieval limit
    float       min_score     = 0.0f;           ///< Minimum score threshold for results
    bool        enable_bm25   = true;           ///< Include BM25 candidates in fusion
    bool        enable_vector = true;           ///< Include KNN candidates in fusion
    double      rrf_k         = 60.0;           ///< Reciprocal Rank Fusion smoothing constant
};

// ============================================================================
// IWikiIndexReader
// ============================================================================

/**
 * @brief Abstract read interface for the wiki chunk index.
 *
 * Implementations must be thread-safe after construction/loading.
 */
class IWikiIndexReader {
public:
    virtual ~IWikiIndexReader() = default;

    /**
     * @brief Query the index for the most relevant chunks.
     *
     * @param query_text  Natural-language query string.
     * @param top_k       Maximum number of results to return.
     * @param min_score   Minimum score threshold; results below this are dropped.
     * @return            Up to `top_k` chunks sorted by descending score.
     */
    [[nodiscard]] virtual std::vector<WikiChunk> query(
        const std::string& query_text,
        int   top_k,
        float min_score) const = 0;

    /**
     * @brief Returns true when the reader is fully initialised and ready.
     * @return True if queries can be served.
     */
    [[nodiscard]] virtual bool isReady() const noexcept = 0;
};

// ============================================================================
// IWikiIndexWriter
// ============================================================================

/**
 * @brief Abstract write interface for the wiki chunk index.
 */
class IWikiIndexWriter {
public:
    virtual ~IWikiIndexWriter() = default;

    /**
     * @brief Write a single chunk, computing its embedding if absent.
     * @param chunk  Chunk to store. `chunk.embedding` is populated if empty.
     * @throws std::runtime_error if the underlying store reports an error.
     */
    virtual void writeChunk(WikiChunk chunk) = 0;

    /**
     * @brief Write a batch of chunks using batch-embedding for efficiency.
     *
     * Chunks with a pre-populated `embedding` are written as-is.
     * The remainder are embedded in groups of 32 via `EmbeddedLLM::embedBatch`.
     *
     * @param chunks  Chunks to ingest. Mutated in-place (embeddings filled).
     */
    virtual void writeBatch(std::vector<WikiChunk> chunks) = 0;

    /**
     * @brief Flush any pending writes to durable storage.
     *
     * For RocksDB-backed stores this is effectively a no-op because WAL
     * provides durability; the method exists so callers can signal
     * intent and future implementations may honour it.
     */
    virtual void flush() = 0;
};

// ============================================================================
// WikiIndexStore — production hybrid (BM25 + vector)
// ============================================================================

/**
 * @brief Production-grade wiki chunk store using BM25 + dense vector fusion.
 *
 * Combines `SecondaryIndexManager` (fulltext BM25 on the `content` column and
 * a regular index on `doc_id`) with `VectorIndexManager` (COSINE HNSW) and
 * fuses results through `HybridRetriever` (RRF, configurable weights).
 *
 * Thread safety:
 *  - `query()` acquires a **shared** lock on `mutex_` (multiple concurrent
 *    readers are allowed).  Query embedding is computed through `llm_ptr_`
 *    (non-const pointer to `llm_`) under a separate `query_embed_mutex_`
 *    exclusive lock so that there is no race window on the query cache.
 *  - `writeChunk()`, `writeBatch()`, and `flush()` acquire an **exclusive**
 *    lock on `mutex_`; writes are fully serialised.
 *  - The chunk embedding cache (`embed_cache_`) is written only while holding
 *    the exclusive lock and read only while holding the shared lock — no
 *    separate per-cache lock is required.
 *  - The query embedding cache (`query_embed_cache_`) is independent of
 *    `embed_cache_` and is always accessed under `query_embed_mutex_`.
 */
class WikiIndexStore : public IWikiIndexReader, public IWikiIndexWriter {
public:
    /**
     * @brief Construct and initialise both index back-ends.
     *
     * Calls `SecondaryIndexManager::createFulltextIndex` for `content` and
     * `createIndex` for `doc_id`.  Calls `VectorIndexManager::init` with
     * `Metric::COSINE` and `config.embedding_dim`.
     *
     * @param sim     Initialised secondary index manager (RocksDB-backed).
     * @param vim     Initialised vector index manager (RocksDB-backed).
     * @param llm     EmbeddedLLM instance used for embedding text.
     * @param config  Index configuration; defaults are sensible for most uses.
     */
    WikiIndexStore(SecondaryIndexManager& sim,
                   VectorIndexManager&    vim,
                   EmbeddedLLM&           llm,
                   WikiIndexConfig        config = {});

    // ─── IWikiIndexWriter ───────────────────────────────────────────────────

    /**
     * @brief Write a single chunk into both the fulltext and vector indexes.
     *
     * If `chunk.embedding` is empty the method calls `EmbeddedLLM::embed` to
     * compute it.  The chunk is stored atomically: a `WriteBatchWrapper` is
     * used so either both indexes are updated or neither is.
     *
     * @param chunk  Chunk to store; `chunk.embedding` is populated if empty.
     * @throws std::runtime_error on index write failure.
     */
    void writeChunk(WikiChunk chunk) override;

    /**
     * @brief Batch-ingest chunks with grouped embedding.
     *
     * Chunks without embeddings are collected, embedded in groups of 32 via
     * `EmbeddedLLM::embedBatch`, then all chunks are written in a single
     * pass.
     *
     * @param chunks  Chunks to ingest. Embeddings are filled in-place.
     * @throws std::runtime_error on index write failure.
     */
    void writeBatch(std::vector<WikiChunk> chunks) override;

    /**
     * @brief No-op for RocksDB (WAL ensures durability); provided for API
     *        completeness and future override-ability.
     */
    void flush() override;

    // ─── IWikiIndexReader ───────────────────────────────────────────────────

    /**
     * @brief Hybrid BM25 + KNN query returning fused, ranked chunks.
     *
     * 1. BM25 candidates via `SecondaryIndexManager::scanFulltextWithScores`.
     * 2. KNN candidates via `VectorIndexManager::searchKnn` (query embedded
     *    on demand).
     * 3. Results fused by `HybridRetriever::fuse` (RRF).
     * 4. Chunks below `min_score` are filtered; at most `top_k` are returned.
     *
     * @param query_text  Natural-language query.
     * @param top_k       Maximum number of results.
     * @param min_score   Score threshold (post-fusion).
     * @return            Ranked `WikiChunk` list, descending score.
     */
    [[nodiscard]] std::vector<WikiChunk> query(
        const std::string& query_text,
        int   top_k,
        float min_score) const override;

    /**
     * @brief Returns true once both indexes are initialised.
     * @return True when queries can be served.
     */
    [[nodiscard]] bool isReady() const noexcept override;

private:
    /// @brief Convert a WikiChunk to a storage-compatible BaseEntity for index ingestion.
    [[nodiscard]] static themis::BaseEntity toEntity(const WikiChunk& chunk);

    SecondaryIndexManager&      sim_;       ///< Fulltext + regular indexes
    VectorIndexManager&         vim_;       ///< HNSW vector index
    EmbeddedLLM&                llm_;       ///< Embedding provider
    WikiIndexConfig             config_;    ///< Operational configuration
    rag::HybridRetriever        retriever_; ///< RRF fusion engine
    std::atomic<bool>           ready_{false}; ///< Initialization flag

    /// Guards write/read on the chunk embedding cache (exclusive for writes,
    /// shared for reads during `writeChunk`/`writeBatch`; writes are already
    /// serialised by `mutex_`).
    mutable std::shared_mutex   mutex_;

    /// In-memory embedding cache keyed by chunk_id (populated under exclusive lock).
    mutable std::unordered_map<std::string, std::vector<float>> embed_cache_;

    /// Non-owning pointer to `llm_` for use in `const` query contexts.
    ///
    /// `EmbeddedLLM::embed()` is not marked `const` (it updates an internal
    /// LRU cache protected by its own mutex), so a const reference cannot be
    /// used to call it inside a `const` method.  Storing a raw pointer avoids
    /// a `const_cast` at every call site.
    mutable EmbeddedLLM*        llm_ptr_;   ///< Raw non-owning pointer to llm_

    /// Mutex protecting the per-instance query embedding cache below.
    ///
    /// Held as an exclusive lock only during cache lookup/insert; both
    /// concurrent `query()` calls (shared `mutex_`) and concurrent writes
    /// (`unique_lock` on `mutex_`) are safe because this is an independent lock.
    mutable std::mutex          query_embed_mutex_;

    /// Cache for query-text embeddings, keyed by the raw query string.
    ///
    /// Populated on first `query()` call for a given text; subsequent calls
    /// return the cached vector without re-invoking the LLM.  Protected by
    /// `query_embed_mutex_` (not by `mutex_`).
    mutable std::unordered_map<std::string, std::vector<float>> query_embed_cache_;
};

// ============================================================================
// JsonWikiIndexReader — Phase A JSON fallback
// ============================================================================

/**
 * @brief Read-only wiki index backed by a JSON file produced by the Python MVP.
 *
 * The JSON format is an array of objects:
 * @code
 * [
 *   {
 *     "chunk_id":      "abc123",
 *     "file_path":     "docs/foo.md",
 *     "section_title": "Overview",
 *     "line_start":    1,
 *     "line_end":      20,
 *     "text":          "..."
 *   },
 *   ...
 * ]
 * @endcode
 *
 * Querying is performed via in-memory TF-weighted token-overlap BM25 because
 * no live embedding service is available in the fallback path.
 *
 * This class is self-contained: it has no RocksDB dependency and can be
 * instantiated in any test or offline context.
 */
class JsonWikiIndexReader : public IWikiIndexReader {
public:
    /**
     * @brief Construct, optionally loading immediately.
     *
     * @param index_path  Path to the `index.json` file.
     * @param auto_load   When true, calls `load()` during construction.
     *                    Set to false to defer loading (e.g. for testing).
     */
    explicit JsonWikiIndexReader(std::string index_path, bool auto_load = false);

    /**
     * @brief Parse the JSON index file into memory.
     *
     * Safe to call multiple times — subsequent calls replace the previous
     * state.
     *
     * @throws std::runtime_error if the file cannot be opened or parsed.
     */
    void load();

    /**
     * @brief BM25-style in-memory query using TF token-overlap scoring.
     *
     * Tokenises `query_text` with `[A-Za-z0-9_\-]+`, computes per-chunk TF
     * overlap scores, applies `min_score` threshold, and returns the top-k
     * results sorted by descending score.
     *
     * @param query_text  Natural-language query.
     * @param top_k       Maximum number of results (0 = return all matching).
     * @param min_score   Minimum score threshold.
     * @return            Ranked chunks, descending score.
     */
    [[nodiscard]] std::vector<WikiChunk> query(
        const std::string& query_text,
        int   top_k,
        float min_score) const override;

    /**
     * @brief Returns true once `load()` has succeeded.
     * @return True when chunks are available for querying.
     */
    [[nodiscard]] bool isReady() const noexcept override;

    /**
     * @brief Number of chunks currently held in memory.
     * @return Chunk count.
     */
    [[nodiscard]] std::size_t size() const noexcept;

private:
    std::string            index_path_;   ///< Path to JSON file
    std::vector<WikiChunk> chunks_;       ///< Loaded chunks
    bool                   loaded_{false}; ///< Load status flag
};

} // namespace llm
} // namespace themis
