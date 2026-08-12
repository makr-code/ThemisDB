/**
 * @file knowledge_graph_enricher.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=5; TODO=1, Stub=3, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once


#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include "training/training_error_codes.h"
#include "training/training_exceptions.h"

namespace themis {

// Forward declaration – keeps the training header free of heavy index dependencies
class VectorIndexManager;

namespace training {

/**
 * @brief Graph context extracted for a training sample
 */
struct GraphContext {
    std::vector<std::string> related_provisions;  ///< Related legal provisions
    std::vector<std::string> case_law;            ///< Related case law
    std::vector<std::string> internal_guidance;   ///< Internal administrative guidance
    std::vector<std::string> similar_documents;   ///< Semantically similar documents
    std::string context_summary;                  ///< Textual summary of context
    
    GraphContext() = default;
};

/**
 * @brief Enrichment statistics
 */
struct EnrichmentStats {
    size_t samples_processed = 0;
    size_t samples_enriched = 0;
    size_t graph_queries_executed = 0;
    size_t context_items_added = 0;
    double elapsed_seconds = 0.0;
    
    EnrichmentStats() = default;
};

/**
 * @brief Enrichment progress callback
 */
using EnrichmentCallback = std::function<void(size_t processed, 
                                              size_t total,
                                              const std::string& status)>;

/**
 * @brief Configuration for the enrichment LRU cache.
 */
struct EnrichmentCacheConfig {
    bool   enabled           = true;     ///< Enable or disable the cache
    size_t capacity          = 50000;    ///< Maximum number of cached entries
    size_t refresh_interval_seconds = 300; ///< Seconds between graph-version refreshes (0 = disable)

    EnrichmentCacheConfig() = default;
};

/**
 * @brief Runtime statistics for the enrichment LRU cache.
 */
struct EnrichmentCacheStats {
    size_t hits       = 0;  ///< Cache hits since last reset
    size_t misses     = 0;  ///< Cache misses since last reset
    size_t evictions  = 0;  ///< Entries evicted due to capacity or version change
    size_t size       = 0;  ///< Current number of cached entries

    EnrichmentCacheStats() = default;
};

/**
 * @brief Configuration for graph enrichment
 */
struct EnrichmentConfig {
    std::string target_collection;         ///< Collection with training samples
    std::string graph_name;                ///< Knowledge graph name
    size_t max_related_items = 5;          ///< Max related items per category
    size_t traversal_depth = 2;            ///< Graph traversal depth
    float similarity_threshold = 0.7f;     ///< Similarity threshold for semantic search
    bool include_provisions = true;        ///< Include related legal provisions
    bool include_case_law = true;          ///< Include related case law
    bool include_guidance = true;          ///< Include internal guidance
    bool include_similar_docs = true;      ///< Include similar documents
    size_t batch_size = 50;                ///< Samples per batch
    EnrichmentCacheConfig cache;           ///< LRU cache configuration

    EnrichmentConfig() = default;
};

/**
 * @brief Knowledge graph enricher for training samples
 * 
 * Enriches training samples by traversing the knowledge graph to find:
 * - Related legal provisions (cross-references)
 * - Relevant case law precedents
 * - Internal administrative guidance
 * - Semantically similar documents
 * 
 * The enriched context improves training quality by providing additional
 * semantic information about each training sample.
 * 
 * Example usage:
 * @code
 * EnrichmentConfig config;
 * config.target_collection = "legal_training_samples";
 * config.graph_name = "legal_knowledge_graph";
 * config.max_related_items = 5;
 * config.traversal_depth = 2;
 * 
 * KnowledgeGraphEnricher enricher(config, db);
 * auto stats = enricher.enrichAll();
 * std::cout << "Enriched " << stats.samples_enriched << " samples\n";
 * @endcode
 */
class KnowledgeGraphEnricher {
public:
    /**
     * @brief Construct graph enricher
     * @param config Enrichment configuration
     * @param db_connection Database connection string
     */
    explicit KnowledgeGraphEnricher(const EnrichmentConfig& config,
                                    const std::string& db_connection);
    
    ~KnowledgeGraphEnricher();
    
    // Delete copy
    KnowledgeGraphEnricher(const KnowledgeGraphEnricher&) = delete;
    KnowledgeGraphEnricher& operator=(const KnowledgeGraphEnricher&) = delete;
    
    /**
     * @brief Enrich all samples in target collection
     * @param callback Optional progress callback
     * @return Enrichment statistics
     */
    EnrichmentStats enrichAll(EnrichmentCallback callback = nullptr);
    
    /**
     * @brief Enrich a specific sample
     * @param sample_id Sample ID
     * @return Graph context for this sample
     */
    GraphContext enrichSample(const std::string& sample_id);
    
    /**
     * @brief Enrich samples matching a query
     * @param aql_query AQL query to select samples
     * @param callback Optional progress callback
     * @return Enrichment statistics
     */
    EnrichmentStats enrichQuery(const std::string& aql_query,
                               EnrichmentCallback callback = nullptr);
    
    /**
     * @brief Find related legal provisions for a document
     * @param document_id Document ID
     * @param max_results Maximum number of results
     * @return Vector of related provision IDs
     */
    std::vector<std::string> findRelatedProvisions(const std::string& document_id,
                                                   size_t max_results = 5);
    
    /**
     * @brief Find related case law for a document
     * @param document_id Document ID
     * @param max_results Maximum number of results
     * @return Vector of case law document IDs
     */
    std::vector<std::string> findRelatedCaseLaw(const std::string& document_id,
                                                size_t max_results = 5);

    /**
     * @brief Find internal administrative guidance documents for a document
     * @param document_id Document ID
     * @param max_results Maximum number of results
     * @return Vector of guidance document IDs
     */
    std::vector<std::string> findRelatedGuidance(const std::string& document_id,
                                                 size_t max_results = 5);
    
    /**
     * @brief Find similar documents using semantic search
     * @param document_id Document ID
     * @param max_results Maximum number of results
     * @return Vector of similar document IDs with similarity scores
     */
    std::vector<std::pair<std::string, float>> findSimilarDocuments(
        const std::string& document_id,
        size_t max_results = 5);
    
    /**
     * @brief Wire a vector index for semantic similarity search.
     *
     * When set, `findSimilarDocuments()` uses this index for cosine-similarity
     * queries instead of returning an empty stub result.  The index must already
     * be initialised (i.e. `init()` called) and contain document embeddings
     * stored under the key equal to the document ID.  Ownership is NOT
     * transferred; the caller must ensure the index outlives the enricher.
     *
     * @param vim Pointer to an initialised VectorIndexManager, or nullptr to
     *            disable vector search and revert to the offline stub.
     */
    void setVectorIndex(VectorIndexManager* vim);

    /**
     * @brief Set the graph schema version used for cache-key generation.
     *
     * The version string is appended to every cache key so that schema changes
     * can be reflected immediately without clearing the entire cache.  Defaults
     * to `"v0"` (deterministic for offline/test builds).  In production, call
     * this with the current schema version after connecting to the graph DB.
     *
     * @param version Non-empty version string (e.g. `"v3"`, `"2026-05-05"`).
     *                Ignored if empty.
     */
    void setGraphVersion(const std::string& version);

    /**
     * @brief Register a sample → source-document mapping for offline use.
     *
     * When no AQL query engine is wired, `enrichSample()` resolves the source
     * document ID of a sample by looking up this in-process registry.  Entries
     * must be registered before calling `enrichSample()`.
     *
     * @param sample_id   Training sample key.
     * @param document_id Corresponding source document ID / URN.
     */
    void registerSourceDocument(const std::string& sample_id,
                                const std::string& document_id);

    /**
     * @brief Set custom graph traversal query
     * @param query_name Query name (e.g., "find_provisions")
     * @param aql_query AQL query template with placeholders
     */
    void setCustomQuery(const std::string& query_name, const std::string& aql_query);

    /**
     * @brief Get AQL query template by name (Phase 6)
     * @param query_name Built-in name ("find_provisions", "find_case_law",
     *                   "find_guidance", "find_similar", "update_context",
     *                   "fetch_all") or custom name
     * @return AQL query template string, or empty string if not found
     */
    std::string getQueryTemplate(const std::string& query_name) const;

    /**
     * @brief Enable the enrichment LRU cache (Phase 9).
     * @param config Cache configuration (capacity, refresh interval).
     */
    void enableCache(const EnrichmentCacheConfig& config = {});

    /**
     * @brief Disable the enrichment LRU cache and evict all entries.
     */
    void disableCache();

    /**
     * @brief Return current cache hit/miss/eviction statistics.
     */
    EnrichmentCacheStats getCacheStats() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace training
} // namespace themis
