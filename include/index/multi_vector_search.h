/**
 * @file multi_vector_search.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "utils/expected.h"
#include <string>
#include <vector>
#include <optional>
#include <unordered_map>
#include <memory>

namespace themis {

class VectorIndexManager;

namespace vector {

/**
 * @brief Multi-Vector Search for complex similarity queries
 * 
 * Provides advanced search capabilities involving multiple vectors:
 * - Multiple query vectors (find items similar to ANY or ALL queries)
 * - Multiple vector fields per item (e.g., title + description embeddings)
 * - Weighted combination of multiple similarity scores
 * - Complex fusion strategies (linear, rank-based, reciprocal rank)
 * 
 * Use cases:
 * - Multi-modal search (text + image + audio)
 * - Multi-aspect similarity (title + content + tags)
 * - Query expansion with multiple reformulations
 * - Ensemble retrieval methods
 * - Hybrid vector + keyword search
 * 
 * Implementation Status: Production-Ready Beta
 * All core methods are fully implemented with multiple fusion strategies.
 * Supports: Linear Combination, Reciprocal Rank Fusion (RRF), Rank Fusion (Borda),
 * Max/Min/Avg Score fusion, and weight optimization via grid search.
 * 
 * References:
 * - Fox, E. A., & Shaw, J. A. (1994). "Combination of multiple searches" (CombSUM, CombMNZ)
 * - Cormack, G. V., et al. (2009). "Reciprocal rank fusion" (RRF)
 * - Dosovitskiy, A., et al. (2020). "An Image is Worth 16x16 Words" (Multi-modal embeddings)
 */
class MultiVectorSearch {
public:
    /**
     * @brief Fusion strategy for combining multiple scores
     */
    enum class FusionStrategy {
        LINEAR_COMBINATION,   // Weighted sum: w1*s1 + w2*s2 + ...
        MAX_SCORE,           // Take maximum score across vectors
        MIN_SCORE,           // Take minimum score across vectors
        AVG_SCORE,           // Average score across vectors
        RANK_FUSION,         // Combine based on ranks (Borda count)
        RECIPROCAL_RANK,     // Reciprocal rank fusion (RRF)
        LEARNED_FUSION       // Machine-learned fusion (future)
    };

    /**
     * @brief Configuration for multi-vector search
     */
    struct SearchConfig {
        FusionStrategy fusion = FusionStrategy::LINEAR_COMBINATION;
        std::vector<float> weights;           // Weights for linear combination
        int top_k = 10;                       // Number of results
        float rrf_k = 60.0f;                  // RRF constant (default: 60)
        bool normalize_scores = true;         // Normalize scores before fusion
        std::optional<std::string> index_name; // Optional index filter
        int ef_search = 64;                   // HNSW search parameter
    };

    /**
     * @brief Query specification for multi-vector search
     */
    struct MultiQuery {
        // Multiple query vectors with optional weights
        std::vector<std::vector<float>> vectors;
        std::vector<float> weights;           // Weight per query vector
        
        // Multiple field names if searching across fields
        std::vector<std::string> field_names;
        
        // Optional: require matching on all fields vs. any field
        bool require_all_fields = false;
    };

    /**
     * @brief Single result from multi-vector search
     */
    struct SearchResult {
        std::string id;
        float fused_score = 0.0f;              // Final combined score
        std::vector<float> individual_scores;   // Scores per query vector
        std::vector<int> individual_ranks;      // Ranks per query vector
        std::unordered_map<std::string, float> field_scores; // Scores per field
    };

    /**
     * @brief Result of multi-vector search operation
     */
    struct MultiSearchResult {
        std::vector<SearchResult> results;
        FusionStrategy strategy_used;
        size_t total_candidates = 0;
        float computation_time_ms = 0.0f;
        std::vector<float> weights_used;  // Actual weights applied
    };

    explicit MultiVectorSearch(VectorIndexManager& vector_manager);

    /**
     * @brief Search with multiple query vectors
     * 
     * Combines results from multiple query vectors using specified fusion strategy.
     */
    Result<MultiSearchResult> search(
        const MultiQuery& query,
        const SearchConfig& config
    );

    /**
     * @brief Search across multiple vector fields in items
     * 
     * Each item has multiple vector fields (e.g., title_embedding, content_embedding).
     * Combines scores across fields using the specified fusion strategy.
     */
    Result<MultiSearchResult> searchMultiField(
        const std::vector<float>& query_vector,
        const std::vector<std::string>& field_names,
        const SearchConfig& config
    );

    /**
     * @brief Query expansion: Search with multiple query reformulations
     * 
     * Useful for query expansion where multiple variants of the query are generated.
     * Combines results from all variants using the specified fusion strategy.
     */
    Result<MultiSearchResult> searchWithExpansion(
        const std::vector<std::vector<float>>& query_variants,
        const SearchConfig& config
    );

    /**
     * @brief Hybrid search: Combine vector search with keyword/filter scores
     * 
     * Fuses vector similarity with other scoring signals (e.g., BM25, TF-IDF).
     * Supports configurable weighting between vector and keyword scores.
     */
    Result<MultiSearchResult> hybridSearch(
        const std::vector<float>& query_vector,
        const std::unordered_map<std::string, float>& keyword_scores,
        const SearchConfig& config
    );

    /**
     * @brief Batch multi-vector search
     * 
     * Process multiple multi-vector queries efficiently in sequence.
     */
    Result<std::vector<MultiSearchResult>> batchSearch(
        const std::vector<MultiQuery>& queries,
        const SearchConfig& config
    );

    /**
     * @brief Optimize fusion weights using training data
     * 
     * Learn optimal weights for linear combination from labeled examples.
     * Uses grid search optimization with NDCG@10 as the objective function.
     */
    Result<std::vector<float>> optimizeWeights(
        const std::vector<MultiQuery>& queries,
        const std::vector<std::vector<std::string>>& relevance_judgments
    );

    /**
     * @brief Get statistics about multi-vector search performance
     */
    struct Statistics {
        size_t total_searches = 0;
        double avg_time_ms = 0.0;
        std::unordered_map<FusionStrategy, size_t> strategy_usage;
        double avg_results_per_search = 0.0;
    };

    const Statistics& getStatistics() const { return stats_; }
    void resetStatistics();

private:
    VectorIndexManager& vector_manager_;
    Statistics stats_;
};

} // namespace vector
} // namespace themis

