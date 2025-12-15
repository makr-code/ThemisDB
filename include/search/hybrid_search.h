#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>

namespace themis {

class SecondaryIndexManager;
class VectorIndexManager;

/**
 * @brief Hybrid Search combining BM25 (full-text) and Vector (semantic) search
 * 
 * v1.2.0 Feature: Reciprocal Rank Fusion (RRF) for RAG optimization
 */
class HybridSearch {
public:
    struct Config {
        double bm25_weight = 0.5;
        double vector_weight = 0.5;
        size_t k = 10;
        size_t k_bm25 = 50;
        size_t k_vector = 50;
        bool use_rrf = true;
        double rrf_k = 60.0;
        bool normalize_scores = true;
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
        const Config& config = Config{}
    );
    ~HybridSearch();
    
    HybridSearch(const HybridSearch&) = delete;
    HybridSearch& operator=(const HybridSearch&) = delete;
    HybridSearch(HybridSearch&&) = default;
    HybridSearch& operator=(HybridSearch&&) = default;
    
    std::vector<Result> search(
        const std::string& text_query,
        const float* vector_query = nullptr,
        size_t vector_dim = 0
    );
    
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
