/**
 * @file rag_integration_helpers.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "rag/knowledge_gap_detector.h"
#include "index/vector_index.h"
#include "storage/base_entity.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/key_schema.h"
#include "rag/rag_retrieval_helpers.h"
#include <functional>
#include <vector>
#include <string>
#include <stdexcept>

namespace themis::rag {

/**
 * @brief Pure conversion from common-provider evidence to knowledge-gap input.
 * @param candidates Owned evidence returned by RAGProvider.
 * @return Documents preserving identity, source collection and retrieval score.
 * @note Scores retain their retrieval semantics; they are not probabilities.
 */
[[nodiscard]] inline std::vector<knowledge_gap::RetrievedDocument> convertToRetrievedDocuments(
    const std::vector<RAGCandidate>& candidates) {
    std::vector<knowledge_gap::RetrievedDocument> documents;
    documents.reserve(candidates.size());
    for (const auto& candidate : candidates) {
        knowledge_gap::RetrievedDocument document;
        document.id = candidate.doc_id;
        document.content = candidate.content;
        document.similarity_score = candidate.score;
        document.metadata["source"] = candidate.doc_id;
        document.metadata["collection"] = candidate.source_namespace;
        documents.push_back(std::move(document));
    }
    return documents;
}

/**
 * @brief Legacy acquisition adapter; new consumers use RAGProvider and the pure overload.
 * 
 * This helper converts search results from VectorIndexManager::searchKnn()
 * into the format expected by KnowledgeGapDetector.
 * 
 * @param search_results Vector of search results from VectorIndexManager
 * @param db Database wrapper to retrieve full entity content
 * @param metric Distance metric used (affects similarity conversion)
 * @param collection Explicit source collection; empty requires fully qualified
 * storage keys in search_results. No collection is guessed.
 * @return Vector of RetrievedDocument structures
 * 
 * @note For COSINE metric: similarity = 1 - distance
 * @note For L2 metric: similarity = 1 / (1 + distance)
 * @note For DOT metric: similarity = distance (assuming normalized vectors)
 * @throws std::runtime_error if storage or usable content is unavailable.
 * @note This compatibility overload performs I/O and belongs to provider-side
 * acquisition, not to the pure helper contract.
 */
inline std::vector<knowledge_gap::RetrievedDocument> convertToRetrievedDocuments(
    const std::vector<VectorIndexManager::Result>& search_results,
    RocksDBWrapper& db,
    VectorIndexManager::Metric metric = VectorIndexManager::Metric::COSINE,
    const std::string& collection = {}
) {
    std::vector<RAGCandidate> candidates;
    candidates.reserve(search_results.size());
    
    for (const auto& result : search_results) {
        double similarity = 0.0;
        
        // Convert distance to similarity score (0.0-1.0 range)
        switch (metric) {
            case VectorIndexManager::Metric::COSINE:
                // Cosine distance = 1 - cosine_similarity
                // So: similarity = 1 - distance
                similarity = std::max(0.0, std::min(1.0, 1.0 - result.distance));
                break;
                
            case VectorIndexManager::Metric::L2:
                // L2 distance: smaller is better
                // Convert to similarity: 1 / (1 + distance)
                similarity = 1.0 / (1.0 + result.distance);
                break;
                
            case VectorIndexManager::Metric::DOT:
                // Dot product: larger is better (assuming normalized vectors)
                // Already in similarity form
                similarity = std::max(0.0, std::min(1.0, result.distance));
                break;
        }
        
        if (!std::isfinite(result.distance)) {
            throw std::runtime_error("RAG vector result has a non-finite distance");
        }
        const auto key = collection.empty()
            ? result.pk : KeySchema::makeRelationalKey(collection, result.pk);
        const auto blob = db.get(key);
        if (!blob) {
            throw std::runtime_error("RAG entity not found: " + key);
        }
        const auto entity = BaseEntity::deserialize(result.pk, *blob);
        auto candidate = makeRAGCandidate(result.pk, collection,
            nlohmann::json::parse(entity.toJson()), static_cast<float>(similarity), "retrieve:legacy_vector");
        if (!candidate) {
            throw std::runtime_error("RAG entity has no usable content: " + key);
        }
        candidates.push_back(std::move(*candidate));
    }
    
    return convertToRetrievedDocuments(candidates);
}

/**
 * @brief Example: Complete RAG pipeline with gap detection
 * 
 * @code
 * // Borrow an authorized QueryEngine with initialized indexes.
 * RAGProvider provider(engine);
 * auto gap_detector = KnowledgeGapDetectorFactory::createBalanced();
 * query::VectorGeoQuery vector_query;
 * vector_query.table = "docs";
 * vector_query.query_vector = embedQuery("What is machine learning?");
 * RetrievalRequest request;
 * request.query = vector_query;
 * auto evidence = provider.retrieve(request);
 * if (!evidence) {
 *     throw std::runtime_error(evidence.error().context());
 * }
 * auto documents = convertToRetrievedDocuments(evidence->candidates);
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
 * that embedding generation failed; the compatibility batch adapter throws.
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
 * @param collection Source collection, or fully qualified storage keys when empty.
 * @return Vector of document lists — one per input query (same order).
 *         An empty inner list indicates a successful search with no matches.
 * @throws std::runtime_error if embedding, search or materialization fails.
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
    VectorIndexManager::Metric metric = VectorIndexManager::Metric::COSINE,
    const std::string& collection = {}
) {
    std::vector<std::vector<knowledge_gap::RetrievedDocument>> all_documents;
    all_documents.reserve(queries.size());

    for (const auto& query : queries) {
        std::vector<float> embedding = embed_fn(query);

        if (embedding.empty()) {
            throw std::runtime_error("RAG query embedding unavailable");
        }

        auto [status, results] = vector_mgr.searchKnn(embedding, k);

        if (status.ok) {
            all_documents.push_back(
                convertToRetrievedDocuments(results, db, metric, collection)
            );
        } else {
            throw std::runtime_error("RAG vector search failed: " + status.message);
        }
    }

    return all_documents;
}

} // namespace themis::rag
