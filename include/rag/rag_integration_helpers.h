/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            rag_integration_helpers.h                          ║
  Version:         0.0.38                                             ║
  Last Modified:   2026-04-13 04:19:05                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     251                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 1573d95376  2026-03-09  feat(rag): implement batchConvert, add pipeline integrati... ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
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
#include <functional>
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
 * @brief Embedding function type for converting query strings to vectors
 *
 * The function receives a query string and returns a float vector suitable
 * for `VectorIndexManager::searchKnn()`.  Return an empty vector to signal
 * that embedding generation failed for the given query; the corresponding
 * result list will then be empty.
 */
using EmbeddingFunction = std::function<std::vector<float>(const std::string&)>;

/**
 * @brief Batch conversion for multiple queries
 *
 * For each query string the provided @p embed_fn generates a dense embedding,
 * which is then used for a K-NN search via @p vector_mgr.  Resulting
 * VectorIndexManager::Result lists are converted to RetrievedDocument vectors
 * using convertToRetrievedDocuments().
 *
 * @param queries     Vector of query strings to process.
 * @param vector_mgr  VectorIndexManager instance (must outlive this call).
 * @param db          Database wrapper for entity content retrieval.
 * @param embed_fn    Function that converts a query string to a float embedding.
 *                    Called sequentially from a single thread; no concurrency
 *                    guarantees are required of the provided callable.
 * @param k           Number of nearest neighbours to retrieve per query.
 * @param metric      Distance metric used for similarity conversion.
 * @return Vector of document lists — one per input query (same order).
 *         An empty inner list indicates that either embedding generation failed
 *         or the K-NN search returned no results for that query.
 *
 * @note This function processes queries sequentially.  Parallel execution
 *       requires a thread-safe VectorIndexManager implementation.
 */
inline std::vector<std::vector<knowledge_gap::RetrievedDocument>>
batchConvertToRetrievedDocuments(
    const std::vector<std::string>& queries,
    VectorIndexManager& vector_mgr,
    RocksDBWrapper& db,
    const EmbeddingFunction& embed_fn,
    size_t k = 10,
    VectorIndexManager::Metric metric = VectorIndexManager::Metric::COSINE
) {
    std::vector<std::vector<knowledge_gap::RetrievedDocument>> all_documents;
    all_documents.reserve(queries.size());

    for (const auto& query : queries) {
        std::vector<float> embedding = embed_fn(query);

        if (embedding.empty()) {
            // Embedding generation failed for this query — return empty list.
            all_documents.emplace_back();
            continue;
        }

        auto [status, results] = vector_mgr.searchKnn(embedding, k);

        if (status.ok) {
            all_documents.push_back(
                convertToRetrievedDocuments(results, db, metric)
            );
        } else {
            all_documents.emplace_back();
        }
    }

    return all_documents;
}

} // namespace themis::rag
