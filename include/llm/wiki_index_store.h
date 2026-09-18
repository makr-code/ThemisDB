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
#include <optional>
#include <shared_mutex>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <atomic>
#include <cstddef>
#include <list>

namespace themis {
namespace llm {

// ============================================================================
// WikiEvalStats — retrieval evaluation metrics accumulated over query history
// ============================================================================

struct WikiEvalStats {
    double      recall_at_k1          = 0.0; ///< Mean Recall@1 over eval query window
    double      recall_at_k3          = 0.0; ///< Mean Recall@3 over eval query window
    double      recall_at_k5          = 0.0; ///< Mean Recall@5 over eval query window
    double      recall_at_k10         = 0.0; ///< Mean Recall@10 over eval query window
    double      mrr                   = 0.0; ///< Mean Reciprocal Rank over eval query window
    double      p95_query_latency_ms  = 0.0; ///< p95 query latency (all queries, ms)
    std::size_t query_count           = 0;   ///< Number of eval queries contributing to metrics
    std::size_t total_query_count     = 0;   ///< Total queries (eval + non-eval)
};

// ============================================================================
// WikiChunk — a single indexed text chunk from a wiki/markdown document
// ============================================================================

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

struct WikiIndexConfig {
#if defined(THEMIS_WIKI_PHASE_B)
    static constexpr bool kDefaultPhaseBEnabled = true;
#else
    static constexpr bool kDefaultPhaseBEnabled = false;
#endif

    bool        enable_phase_b = kDefaultPhaseBEnabled; ///< Gate for WikiIndexStore Phase B runtime
    std::string table_name    = "wiki_chunks"; ///< RocksDB table / namespace
    int         embedding_dim = 384;            ///< Expected embedding dimensionality (overridden when auto_probe_dim=true)
    int         top_k         = 10;             ///< Default retrieval limit
    float       min_score     = 0.0f;           ///< Minimum score threshold for results
    bool        enable_bm25   = true;           ///< Include BM25 candidates in fusion
    bool        enable_vector = true;           ///< Include KNN candidates in fusion
    double      rrf_k         = 60.0;           ///< Reciprocal Rank Fusion smoothing constant
    double      bm25_k1       = 1.5;            ///< BM25+ k1 parameter (Robertson & Zaragoza)
    double      bm25_b        = 0.75;           ///< BM25+ b parameter (Robertson & Zaragoza)
    double      bm25_delta    = 0.5;            ///< BM25+ lower-bound term frequency delta
    int         hnsw_m        = 16;             ///< HNSW graph degree (M)
    int         hnsw_ef_construction = 200;     ///< HNSW ef_construction
    int         hnsw_ef_search       = 64;      ///< HNSW ef_search

    // --- Phase 3 features (Target: Q3 2026) --------------------------------

    int         batch_size    = 32;

    bool        auto_probe_dim = false;

    bool        enable_persistent_cache = false;

    std::string embedding_cache_table = "embedding_cache";

    std::size_t embedding_cache_max_bytes = 0;

    bool        enable_phase_a_cache_migration = true;
};

// ============================================================================
// IWikiIndexReader
// ============================================================================

class IWikiIndexReader {
public:
    /**
     * @brief IWiki Index Reader.
     * @return Return value.
     */
    virtual ~IWikiIndexReader() = default;

    [[nodiscard]] virtual std::vector<WikiChunk> query(
        const std::string& query_text,
        int   top_k,
        float min_score) const = 0;

    [[nodiscard]] virtual bool isReady() const noexcept = 0;
};

// ============================================================================
// IWikiIndexWriter
// ============================================================================

class IWikiIndexWriter {
public:
    /**
     * @brief IWiki Index Writer.
     * @return Return value.
     */
    virtual ~IWikiIndexWriter() = default;

    /**
     * @brief Write Chunk.
     * @param[in] chunk Input parameter.
     */
    virtual void writeChunk(WikiChunk chunk) = 0;

    /**
     * @brief Write Batch.
     * @param[in] chunks Input parameter.
     */
    virtual void writeBatch(std::vector<WikiChunk> chunks) = 0;

    /**
     * @brief Flush.
     */
    virtual void flush() = 0;
};

// ============================================================================
// WikiIndexStore — production hybrid (BM25 + vector)
// ============================================================================

class WikiIndexStore : public IWikiIndexReader, public IWikiIndexWriter {
public:
    WikiIndexStore(SecondaryIndexManager& sim,
                   VectorIndexManager&    vim,
                   EmbeddedLLM&           llm,
                   WikiIndexConfig        config = {});

    // ─── IWikiIndexWriter ───────────────────────────────────────────────────

    void writeChunk(WikiChunk chunk) override;

    void writeBatch(std::vector<WikiChunk> chunks) override;

    void flush() override;

    // ─── IWikiIndexReader ───────────────────────────────────────────────────

    [[nodiscard]] std::vector<WikiChunk> query(
        const std::string& query_text,
        int   top_k,
        float min_score) const override;

    [[nodiscard]] bool isReady() const noexcept override;

    // -----------------------------------------------------------------------
    // Evaluation API (Recall@k / MRR / p95 latency)
    // -----------------------------------------------------------------------

    [[nodiscard]] std::vector<WikiChunk> evaluateQuery(
        const std::string&              query_text,
        int                             top_k,
        float                           min_score,
        const std::vector<std::string>& relevant_doc_ids) const;

    [[nodiscard]] WikiEvalStats getEvaluationStats() const;

    /**
     * @brief Reset Evaluation Stats.
     * @note Exception safety: noexcept.
     */
    void resetEvaluationStats() noexcept;

private:
    [[nodiscard]] static themis::BaseEntity toEntity(const WikiChunk& chunk);
    [[nodiscard]] static std::string makeEmbeddingCacheKey(const WikiChunk& chunk);
    /**
     * @brief Estimate Embedding Bytes.
     * @param[in] cache_key Input parameter.
     * @param[in] embedding Input parameter.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    static std::size_t estimateEmbeddingBytes(const std::string& cache_key,
                                              const std::vector<float>& embedding) noexcept;

    SecondaryIndexManager&      sim_;       ///< Fulltext + regular indexes
    VectorIndexManager&         vim_;       ///< HNSW vector index
    EmbeddedLLM&                llm_;       ///< Embedding provider
    WikiIndexConfig             config_;    ///< Operational configuration
    rag::HybridRetriever        retriever_; ///< RRF fusion engine
    std::atomic<bool>           ready_{false}; ///< Initialization flag

    mutable std::shared_mutex   mutex_;

    mutable std::unordered_map<std::string, std::vector<float>> embed_cache_;
    mutable std::list<std::string> embed_cache_lru_;
    mutable std::unordered_map<std::string, std::list<std::string>::iterator> embed_cache_lru_pos_;
    mutable std::unordered_map<std::string, std::size_t> embed_cache_entry_bytes_;
    mutable std::size_t embed_cache_bytes_{0};

    mutable EmbeddedLLM*        llm_ptr_;   ///< Raw non-owning pointer to llm_

    mutable std::mutex          query_embed_mutex_;

    mutable std::unordered_map<std::string, std::vector<float>> query_embed_cache_;

    std::atomic<bool>           dim_probed_{false};

    std::string                 emb_cache_table_;
    std::string                 legacy_emb_cache_table_;

    /**
     * @brief Load Persistent Embed Cache.
     */
    void loadPersistentEmbedCache();

    /**
     * @brief Persist Embedding.
     * @param[in] cache_key Input parameter.
     * @param[in] chunk_id Identifier of the chunk.
     * @param[in] embedding Input parameter.
     */
    void persistEmbedding(const std::string& cache_key,
                          const std::string& chunk_id,
                          const std::vector<float>& embedding);

    /**
     * @brief Fetch Persisted Embedding.
     * @param[in] cache_key Input parameter.
     * @return Return value.
     */
    std::optional<std::vector<float>> fetchPersistedEmbedding(
        const std::string& cache_key) const;
    /**
     * @brief Fetch Legacy Persisted Embedding By Chunk Id.
     * @param[in] chunk_id Identifier of the chunk.
     * @return Return value.
     */
    std::optional<std::vector<float>> fetchLegacyPersistedEmbeddingByChunkId(
        const std::string& chunk_id) const;

    /**
     * @brief Touch Embedding Cache Entry.
     * @param[in] cache_key Input parameter.
     */
    void touchEmbeddingCacheEntry(const std::string& cache_key) const;
    /**
     * @brief Upsert Embedding Cache Entry.
     * @param[in] cache_key Input parameter.
     * @param[in] embedding Input parameter.
     */
    void upsertEmbeddingCacheEntry(const std::string& cache_key,
                                   const std::vector<float>& embedding) const;
    /**
     * @brief Enforce Embedding Cache Limit.
     */
    void enforceEmbeddingCacheLimit() const;
    /**
     * @brief Try Resolve Embedding From Caches.
     * @param[in] chunk Input parameter.
     * @param[in,out] out_embedding Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool tryResolveEmbeddingFromCaches(const WikiChunk& chunk,
                                       std::vector<float>* out_embedding);
    /**
     * @brief Migrate Legacy Entry If Needed.
     * @param[in] chunk Input parameter.
     * @param[in] embedding Input parameter.
     */
    void migrateLegacyEntryIfNeeded(const WikiChunk& chunk,
                                    const std::vector<float>& embedding);

    /**
     * @brief Probe Embedding Dim.
     */
    void probeEmbeddingDim();

    // -----------------------------------------------------------------------
    // Evaluation metric accumulators (protected by eval_mutex_)
    // -----------------------------------------------------------------------

    static constexpr std::size_t kLatencyRingSize = 1024;

    mutable std::mutex          eval_mutex_;

    mutable std::vector<double> latency_ring_;       ///< Pre-allocated capacity kLatencyRingSize
    mutable std::size_t         latency_ring_head_{0}; ///< Next write position (mod kLatencyRingSize)
    mutable std::size_t         latency_ring_count_{0}; ///< Valid entries [0, kLatencyRingSize]

    mutable std::size_t         total_query_count_{0};

    mutable double              eval_recall_at_1_{0.0};
    mutable double              eval_recall_at_3_{0.0};
    mutable double              eval_recall_at_5_{0.0};
    mutable double              eval_recall_at_10_{0.0};
    mutable double              eval_mrr_{0.0};
    mutable std::size_t         eval_query_count_{0};

    /**
     * @brief Record Latency Locked.
     * @param[in] latency_ms Input parameter.
     * @note Exception safety: noexcept.
     */
    void recordLatencyLocked(double latency_ms) const noexcept;
};

// ============================================================================
// JsonWikiIndexReader — Phase A JSON fallback
// ============================================================================

class JsonWikiIndexReader : public IWikiIndexReader {
public:
    explicit JsonWikiIndexReader(std::string index_path, bool auto_load = false);

    /**
     * @brief Load.
     */
    void load();

    [[nodiscard]] std::vector<WikiChunk> query(
        const std::string& query_text,
        int   top_k,
        float min_score) const override;

    [[nodiscard]] bool isReady() const noexcept override;

    [[nodiscard]] std::size_t size() const noexcept;

private:
    std::string            index_path_;   ///< Path to JSON file
    std::vector<WikiChunk> chunks_;       ///< Loaded chunks
    bool                   loaded_{false}; ///< Load status flag
};

} // namespace llm
} // namespace themis
