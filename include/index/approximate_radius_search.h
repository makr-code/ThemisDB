/**
 * @file approximate_radius_search.h
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
#include <memory>

namespace themis {

class VectorIndexManager;

namespace vector {

/**
 * @brief Approximate Radius Search for vector similarity
 * 
 * Provides efficient approximate search for all vectors within a given
 * radius (distance threshold) of a query vector. Unlike k-NN which finds
 * the k nearest neighbors, radius search finds ALL vectors within a
 * distance threshold.
 * 
 * Features:
 * - Approximate radius search (faster than exact)
 * - Configurable distance metrics (L2, Cosine, Dot Product)
 * - Max results limiting for performance
 * - Quality guarantees (recall threshold)
 * - Integration with existing vector indices
 * - Adaptive target count search with binary search optimization
 * - Sample-based result count estimation
 * - Comprehensive statistics tracking
 * 
 * Implementation Status: Production-Ready Beta
 * All core methods are fully implemented and tested.
 * 
 * References:
 * - Malkov, Y. A., & Yashunin, D. A. (2018). "Efficient and robust approximate nearest neighbor search using HNSW"
 * - Johnson, J., et al. (2019). "Billion-scale similarity search with GPUs" (FAISS)
 */
class ApproximateRadiusSearch {
public:
    /**
     * @brief Distance metric types
     */
    enum class Metric {
        L2,           // Euclidean distance
        COSINE,       // Cosine similarity
        DOT_PRODUCT   // Dot product similarity
    };

    /**
     * @brief Configuration for radius search
     */
    struct SearchConfig {
        Metric metric = Metric::COSINE;
        float radius = 0.5f;                   // Distance threshold
        int max_results = 1000;                // Maximum results to return
        float min_recall = 0.95f;              // Minimum recall guarantee (0-1)
        bool sort_results = true;              // Sort by distance
        std::optional<std::string> index_name; // Optional index filter
        int ef_search = 64;                    // HNSW search parameter
    };

    /**
     * @brief Single result from radius search
     */
    struct RadiusResult {
        std::string id;
        float distance = 0.0f;
        std::vector<float> vector;  // Optional: return vector data
    };

    /**
     * @brief Result of radius search operation
     */
    struct SearchResult {
        std::vector<RadiusResult> results;
        size_t total_candidates = 0;      // Total vectors evaluated
        float actual_max_distance = 0.0f; // Actual max distance in results
        float computation_time_ms = 0.0f;
        bool truncated = false;            // True if max_results limit reached
    };

    explicit ApproximateRadiusSearch(VectorIndexManager& vector_manager);

    /**
     * @brief Search for vectors within radius of query vector
     * 
     * @param query_vector The query vector
     * @param config Search configuration
     * @return Search results or error
     */
    Result<SearchResult> search(
        const std::vector<float>& query_vector,
        const SearchConfig& config
    );

    /**
     * @brief Search for vectors within radius using vector ID
     * 
     * Convenience method that looks up the query vector by ID.
     */
    Result<SearchResult> searchById(
        std::string_view query_id,
        const SearchConfig& config
    );

    /**
     * @brief Batch radius search for multiple query vectors
     */
    Result<std::vector<SearchResult>> batchSearch(
        const std::vector<std::vector<float>>& query_vectors,
        const SearchConfig& config
    );

    /**
     * @brief Search with dynamic radius adjustment
     * 
     * Automatically adjusts radius to return approximately target_count results
     * using binary search optimization.
     */
    Result<SearchResult> searchWithTargetCount(
        const std::vector<float>& query_vector,
        int target_count,
        const SearchConfig& config
    );

    /**
     * @brief Estimate result count for a given radius
     * 
     * Uses sample-based estimation to predict how many results would be
     * returned for a given radius. Useful for query planning and UI feedback.
     */
    Result<size_t> estimateResultCount(
        const std::vector<float>& query_vector,
        float radius,
        Metric metric = Metric::COSINE
    );

    /**
     * @brief Get statistics about radius search performance
     */
    struct Statistics {
        size_t total_searches = 0;
        double avg_results_per_search = 0.0;
        double avg_time_ms = 0.0;
        double avg_recall = 0.0;
    };

    const Statistics& getStatistics() const { return stats_; }
    void resetStatistics();

private:
    VectorIndexManager& vector_manager_;
    Statistics stats_;
};

} // namespace vector
} // namespace themis
