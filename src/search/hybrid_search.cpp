#include "search/hybrid_search.h"
#include "index/secondary_index.h"
#include "index/vector_index_manager.h"
#include "utils/logger.h"
#include <algorithm>
#include <unordered_map>
#include <cmath>

namespace themis {

HybridSearch::HybridSearch(
    SecondaryIndexManager* fulltext_index,
    VectorIndexManager* vector_index,
    const Config& config
) : fulltext_index_(fulltext_index),
    vector_index_(vector_index),
    config_(config) {
    
    THEMIS_INFO("HybridSearch initialized (RRF={}, k={})", 
                config_.use_rrf, config_.k);
}

HybridSearch::~HybridSearch() = default;

std::vector<HybridSearch::Result> HybridSearch::search(
    const std::string& text_query,
    const float* vector_query,
    size_t vector_dim
) {
    // Stub implementation - would integrate with actual indexes
    std::vector<Result> bm25_results;
    std::vector<Result> vector_results;
    
    // BM25 search
    if (!text_query.empty() && fulltext_index_) {
        // Simulated BM25 search
        for (size_t i = 0; i < config_.k_bm25 && i < 10; ++i) {
            Result r;
            r.document_id = "doc_" + std::to_string(i);
            r.bm25_score = 1.0 - (i * 0.1);
            r.bm25_rank = static_cast<int>(i + 1);
            bm25_results.push_back(r);
        }
    }
    
    // Vector search
    if (vector_query && vector_dim > 0 && vector_index_) {
        // Simulated vector search
        for (size_t i = 0; i < config_.k_vector && i < 10; ++i) {
            Result r;
            r.document_id = "doc_" + std::to_string(i + 5);
            r.vector_score = 1.0 - (i * 0.1);
            r.vector_rank = static_cast<int>(i + 1);
            vector_results.push_back(r);
        }
    }
    
    // Fuse results
    if (config_.use_rrf) {
        return reciprocalRankFusion(bm25_results, vector_results);
    } else {
        // Linear combination fallback
        std::vector<Result> combined;
        for (const auto& r : bm25_results) combined.push_back(r);
        for (const auto& r : vector_results) combined.push_back(r);
        return combined;
    }
}

std::vector<HybridSearch::Result> HybridSearch::reciprocalRankFusion(
    const std::vector<Result>& bm25_results,
    const std::vector<Result>& vector_results
) {
    // RRF: score(d) = sum(1 / (k + rank_i(d)))
    std::unordered_map<std::string, Result> doc_map;
    
    // Process BM25 results
    for (size_t i = 0; i < bm25_results.size(); ++i) {
        const auto& r = bm25_results[i];
        auto& doc = doc_map[r.document_id];
        doc.document_id = r.document_id;
        doc.bm25_score = r.bm25_score;
        doc.bm25_rank = static_cast<int>(i + 1);
        doc.content = r.content;
        
        // RRF contribution from BM25
        double rrf_score = 1.0 / (config_.rrf_k + (i + 1));
        doc.hybrid_score += config_.bm25_weight * rrf_score;
    }
    
    // Process vector results
    for (size_t i = 0; i < vector_results.size(); ++i) {
        const auto& r = vector_results[i];
        auto& doc = doc_map[r.document_id];
        doc.document_id = r.document_id;
        doc.vector_score = r.vector_score;
        doc.vector_rank = static_cast<int>(i + 1);
        if (doc.content.empty()) doc.content = r.content;
        
        // RRF contribution from vector search
        double rrf_score = 1.0 / (config_.rrf_k + (i + 1));
        doc.hybrid_score += config_.vector_weight * rrf_score;
    }
    
    // Convert map to vector and sort by hybrid score
    std::vector<Result> fused_results;
    for (const auto& [_, result] : doc_map) {
        fused_results.push_back(result);
    }
    
    std::sort(fused_results.begin(), fused_results.end(),
              [](const Result& a, const Result& b) {
                  return a.hybrid_score > b.hybrid_score;
              });
    
    // Limit to top-k
    if (fused_results.size() > config_.k) {
        fused_results.resize(config_.k);
    }
    
    THEMIS_INFO("RRF fusion: {} BM25 + {} vector -> {} results",
                bm25_results.size(), vector_results.size(), fused_results.size());
    
    return fused_results;
}

void HybridSearch::normalizeScores(std::vector<Result>& results, bool is_bm25) {
    if (results.empty()) return;
    
    // Find min/max scores
    double min_score = std::numeric_limits<double>::max();
    double max_score = std::numeric_limits<double>::lowest();
    
    for (const auto& r : results) {
        double score = is_bm25 ? r.bm25_score : r.vector_score;
        min_score = std::min(min_score, score);
        max_score = std::max(max_score, score);
    }
    
    // Normalize to [0, 1]
    double range = max_score - min_score;
    if (range > 0.0) {
        for (auto& r : results) {
            if (is_bm25) {
                r.bm25_score = (r.bm25_score - min_score) / range;
            } else {
                r.vector_score = (r.vector_score - min_score) / range;
            }
        }
    }
}

} // namespace themis
