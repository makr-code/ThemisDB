/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            knowledge_graph_enricher.h                         ║
  Version:         0.0.27                                             ║
  Last Modified:   2026-02-22 08:56:04                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     195                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>

namespace themis {
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
     * @brief Find similar documents using semantic search
     * @param document_id Document ID
     * @param max_results Maximum number of results
     * @return Vector of similar document IDs with similarity scores
     */
    std::vector<std::pair<std::string, float>> findSimilarDocuments(
        const std::string& document_id,
        size_t max_results = 5);
    
    /**
     * @brief Set custom graph traversal query
     * @param query_name Query name (e.g., "find_provisions")
     * @param aql_query AQL query template with placeholders
     */
    void setCustomQuery(const std::string& query_name, const std::string& aql_query);

    /**
     * @brief Get AQL query template by name (Phase 6)
     * @param query_name Built-in name ("find_provisions", "find_case_law",
     *                   "find_similar", "update_context", "fetch_all") or custom name
     * @return AQL query template string, or empty string if not found
     */
    std::string getQueryTemplate(const std::string& query_name) const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace training
} // namespace themis
