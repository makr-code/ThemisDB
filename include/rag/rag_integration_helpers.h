/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            rag_integration_helpers.h                          ║
  Version:         0.0.22                                             ║
  Last Modified:   2026-02-21 19:28:53                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     243                                            ║
    • Open Issues:     TODOs: 1, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 31e8b8df0  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file rag_integration_helpers.h
 * @brief Helper utilities for integrating RAG components with ThemisDB
 * 
 * This file provides utility functions for converting between different
 * component types in the RAG pipeline, particularly for integration between
 * VectorIndexManager and Knowledge Gap Detector.
 */

#pragma once

#include "rag/knowledge_gap_detector.h"
#include "index/vector_index.h"
#include "storage/base_entity.h"
#include <vector>
#include <string>

namespace themis::rag {

/**
 * @brief Convert VectorIndexManager search results to RetrievedDocuments
 * 
 * This helper converts search results from VectorIndexManager::searchKnn()
 * into the format expected by KnowledgeGapDetector.
 * 
 * @param search_results Vector of search results from VectorIndexManager
 * @param db Database wrapper to retrieve full entity content
 * @param metric Distance metric used (affects similarity conversion)
 * @return Vector of RetrievedDocument structures
 * 
 * @note For COSINE metric: similarity = 1 - distance
 * @note For L2 metric: similarity = 1 / (1 + distance)
 * @note For DOT metric: similarity = distance (assuming normalized vectors)
 */
inline std::vector<knowledge_gap::RetrievedDocument> convertToRetrievedDocuments(
    const std::vector<VectorIndexManager::Result>& search_results,
    RocksDBWrapper& db,
    VectorIndexManager::Metric metric = VectorIndexManager::Metric::COSINE
) {
    std::vector<knowledge_gap::RetrievedDocument> documents;
    documents.reserve(search_results.size());
    
    for (const auto& result : search_results) {
        knowledge_gap::RetrievedDocument doc;
        doc.id = result.pk;
        
        // Convert distance to similarity score (0.0-1.0 range)
        switch (metric) {
            case VectorIndexManager::Metric::COSINE:
                // Cosine distance = 1 - cosine_similarity
                // So: similarity = 1 - distance
                doc.similarity_score = std::max(0.0, std::min(1.0, 1.0 - result.distance));
                break;
                
            case VectorIndexManager::Metric::L2:
                // L2 distance: smaller is better
                // Convert to similarity: 1 / (1 + distance)
                doc.similarity_score = 1.0 / (1.0 + result.distance);
                break;
                
            case VectorIndexManager::Metric::DOT:
                // Dot product: larger is better (assuming normalized vectors)
                // Already in similarity form
                doc.similarity_score = std::max(0.0, std::min(1.0, result.distance));
                break;
        }
        
        // Retrieve full entity content from database
        auto entity_opt = db.getEntity(result.pk);
        if (entity_opt) {
            const BaseEntity& entity = entity_opt.value();
            
            // Extract content field (adjust field name as needed)
            if (entity.hasField("content")) {
                doc.content = entity.getFieldAsString("content");
            } else if (entity.hasField("text")) {
                doc.content = entity.getFieldAsString("text");
            } else if (entity.hasField("body")) {
                doc.content = entity.getFieldAsString("body");
            } else {
                // Fallback: use JSON representation
                doc.content = entity.toJson().dump();
            }
            
            // Extract metadata
            if (entity.hasField("timestamp")) {
                doc.metadata["timestamp"] = entity.getFieldAsString("timestamp");
            }
            if (entity.hasField("source")) {
                doc.metadata["source"] = entity.getFieldAsString("source");
            }
            if (entity.hasField("author")) {
                doc.metadata["author"] = entity.getFieldAsString("author");
            }
            if (entity.hasField("category")) {
                doc.metadata["category"] = entity.getFieldAsString("category");
            }
            if (entity.hasField("language")) {
                doc.metadata["language"] = entity.getFieldAsString("language");
            }
        } else {
            // Entity not found - use PK as content
            doc.content = "Entity not found: " + result.pk;
        }
        
        documents.push_back(std::move(doc));
    }
    
    return documents;
}

/**
 * @brief Example: Complete RAG pipeline with gap detection
 * 
 * @code
 * // 1. Initialize components
 * RocksDBWrapper db(config);
 * VectorIndexManager vector_mgr(db);
 * auto gap_detector = KnowledgeGapDetectorFactory::createBalanced();
 * 
 * // 2. Perform vector search
 * std::vector<float> query_embedding = embedQuery("What is machine learning?");
 * auto [status, search_results] = vector_mgr.searchKnn(query_embedding, 10);
 * 
 * // 3. Convert to RetrievedDocuments
 * auto documents = convertToRetrievedDocuments(
 *     search_results, 
 *     db, 
 *     VectorIndexManager::Metric::COSINE
 * );
 * 
 * // 4. Check for knowledge gaps
 * auto gap_result = gap_detector->detectPreGeneration(
 *     "What is machine learning?",
 *     documents
 * );
 * 
 * // 5. Handle gap or proceed with generation
 * if (gap_result.gap_detected) {
 *     // Apply fallback strategy
 *     switch (gap_result.recommendation) {
 *         case FallbackStrategy::EXPAND_SEARCH:
 *             // Retry search with more results
 *             break;
 *         case FallbackStrategy::REFORMULATE_QUERY:
 *             // Try alternative query formulation
 *             break;
 *         case FallbackStrategy::INSUFFICIENT_DATA_RESPONSE:
 *             return "I don't have enough information to answer this question.";
 *     }
 * } else {
 *     // Proceed with LLM generation
 *     std::string answer = llm->generate(query, documents);
 *     return answer;
 * }
 * @endcode
 */

/**
 * @brief Batch conversion for multiple queries (GPU-accelerated pipeline)
 * 
 * For high-throughput scenarios, this function processes multiple queries
 * in batch, leveraging GPU acceleration where available.
 * 
 * @param queries Vector of query strings
 * @param vector_mgr VectorIndexManager instance
 * @param db Database wrapper
 * @param k Number of results per query
 * @param metric Distance metric
 * @return Vector of document lists (one per query)
 * 
 * @note This is a placeholder implementation. Full batch processing
 *       requires integration with an embedding model and will be
 *       implemented in Phase 2.
 * 
 * @warning DO NOT USE IN PRODUCTION - Returns empty results
 */
inline std::vector<std::vector<knowledge_gap::RetrievedDocument>> 
batchConvertToRetrievedDocuments(
    const std::vector<std::string>& queries,
    VectorIndexManager& vector_mgr,
    RocksDBWrapper& db,
    size_t k = 10,
    VectorIndexManager::Metric metric = VectorIndexManager::Metric::COSINE
) {
    std::vector<std::vector<knowledge_gap::RetrievedDocument>> all_documents;
    all_documents.reserve(queries.size());
    
    // TODO Phase 2: Implement batch embedding and search
    // This requires:
    // 1. Batch embedding generation (e.g., via GPU-accelerated model)
    // 2. Batch vector search using VectorIndexManager
    // 3. Parallel document retrieval
    
    // Placeholder implementation - returns empty results
    // DO NOT USE IN PRODUCTION
    for (const auto& query : queries) {
        // Example of what the final implementation would look like:
        // std::vector<float> query_embedding = batchEmbedQueries({query})[0];
        // auto [status, results] = vector_mgr.searchKnn(query_embedding, k);
        // if (status.ok) {
        //     all_documents.push_back(convertToRetrievedDocuments(results, db, metric));
        // } else {
        //     all_documents.push_back({});
        // }
        
        (void)vector_mgr;  // Suppress unused parameter warning
        (void)db;
        (void)k;
        (void)metric;
        all_documents.push_back({});
    }
    
    return all_documents;
}

} // namespace themis::rag
