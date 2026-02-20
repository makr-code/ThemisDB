#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include "index/vector_index.h"

namespace themis {

class SecondaryIndexManager;
class VectorIndexManager;

/**
 * @brief Hybrid Search combining BM25 (full-text) and Vector (semantic) search
 * 
 * v1.2.0 Feature: Reciprocal Rank Fusion (RRF) for RAG optimization
 * v1.3.0 Update: Real BM25 and Vector index integration
 * v1.4.0 Update: Configurable vector metric, config validation, normalization fixes
 * 
 * Features:
 * - BM25 fulltext search with scoring
 * - Vector ANN search (HNSW)
 * - Reciprocal Rank Fusion (RRF) for result merging
 * - Linear combination fallback with pre-normalization
 * - Score normalization with edge-case handling
 * - Configurable vector distance metric (COSINE, DOT, L2)
 * 
 * Use Cases:
 * - RAG (Retrieval-Augmented Generation)
 * - Semantic + keyword search
 * - Document ranking
 * - Question answering systems
 * 
 * Performance:
 * - 85%+ recall@10 with RRF
 * - Combines lexical and semantic matching
 * - Configurable weights for BM25/vector balance
 */
class HybridSearch {
public:
    struct Config {
        double bm25_weight = 0.5;
        double vector_weight = 0.5;
        size_t k = 10;              // Final result count
        size_t k_bm25 = 50;         // BM25 candidate count
        size_t k_vector = 50;       // Vector candidate count
        bool use_rrf = true;        // Use RRF (recommended)
        double rrf_k = 60.0;        // RRF constant
        bool normalize_scores = true;
        
        // Configurable table/column for searches
        std::string default_table = "documents";
        std::string default_column = "content";
        
        // Vector distance metric used for similarity conversion
        VectorIndexManager::Metric vector_metric = VectorIndexManager::Metric::COSINE;
    };
    
    struct Result {
        std::string document_id;
        double bm25_score = 0.0;
        double vector_score = 0.0;
        double hybrid_score = 0.0;
        int bm25_rank = -1;
        int vector_rank = -1;
        std::string content;
        std::optional<double> geo_distance;
    };
    
    explicit HybridSearch(
        SecondaryIndexManager* fulltext_index,
        VectorIndexManager* vector_index,
        const Config& config
    );
    ~HybridSearch();
    
    HybridSearch(const HybridSearch&) = delete;
    HybridSearch& operator=(const HybridSearch&) = delete;
    HybridSearch(HybridSearch&&) = default;
    HybridSearch& operator=(HybridSearch&&) = default;
    
    /**
     * @brief Perform hybrid search combining BM25 and vector search
     * 
     * @param text_query Text query for BM25 search
     * @param vector_query Optional vector query for ANN search
     * @param vector_dim Dimension of vector query
     * @return Fused results ranked by hybrid score
     */
    std::vector<Result> search(
        const std::string& text_query,
        const float* vector_query = nullptr,
        size_t vector_dim = 0
    );
    
    /**
     * @brief Fuse BM25 and vector results using Reciprocal Rank Fusion
     * 
     * RRF formula: score(d) = sum(1 / (k + rank_i(d)))
     * where k is a constant (default 60) and rank_i is the rank in result set i
     * 
     * @param bm25_results BM25 search results
     * @param vector_results Vector search results  
     * @return Fused results sorted by hybrid score
     */
    std::vector<Result> reciprocalRankFusion(
        const std::vector<Result>& bm25_results,
        const std::vector<Result>& vector_results
    );
    
    const Config& getConfig() const { return config_; }
    void setConfig(const Config& config) { config_ = config; }

private:
    SecondaryIndexManager* fulltext_index_;
    VectorIndexManager* vector_index_;
    Config config_;
    
    void normalizeScores(std::vector<Result>& results, bool is_bm25);
};

} // namespace themis
