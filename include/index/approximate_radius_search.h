/**
 * @file approximate_radius_search.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class ApproximateRadiusSearch {
public:
    enum class Metric {
        L2,           // Euclidean distance
        COSINE,       // Cosine similarity
        DOT_PRODUCT   // Dot product similarity
    };

    struct SearchConfig {
        Metric metric = Metric::COSINE;
        float radius = 0.5f;                   // Distance threshold
        int max_results = 1000;                // Maximum results to return
        float min_recall = 0.95f;              // Minimum recall guarantee (0-1)
        bool sort_results = true;              // Sort by distance
        std::optional<std::string> index_name; // Optional index filter
        int ef_search = 64;                    // HNSW search parameter
    };

    struct RadiusResult {
        std::string id;
        float distance = 0.0f;
        std::vector<float> vector;  // Optional: return vector data
    };

    struct SearchResult {
        std::vector<RadiusResult> results;
        size_t total_candidates = 0;      // Total vectors evaluated
        float actual_max_distance = 0.0f; // Actual max distance in results
        float computation_time_ms = 0.0f;
        bool truncated = false;            // True if max_results limit reached
    };

    /**
     * @brief Approximate Radius Search.
     * @param[in,out] vector_manager Input/output parameter.
     * @return Return value.
     */
    explicit ApproximateRadiusSearch(VectorIndexManager& vector_manager);

    /**
     * @brief Search.
     * @param[in] query_vector Input parameter.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    Result<SearchResult> search(
        const std::vector<float>& query_vector,
        const SearchConfig& config
    );

    /**
     * @brief Search By Id.
     * @param[in] query_id Identifier of the query.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    Result<SearchResult> searchById(
        std::string_view query_id,
        const SearchConfig& config
    );

    /**
     * @brief Batch Search.
     * @param[in] query_vectors Input parameter.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    Result<std::vector<SearchResult>> batchSearch(
        const std::vector<std::vector<float>>& query_vectors,
        const SearchConfig& config
    );

    /**
     * @brief Search With Target Count.
     * @param[in] query_vector Input parameter.
     * @param[in] target_count Input parameter.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    Result<SearchResult> searchWithTargetCount(
        const std::vector<float>& query_vector,
        int target_count,
        const SearchConfig& config
    );

    Result<size_t> estimateResultCount(
        const std::vector<float>& query_vector,
        float radius,
        Metric metric = Metric::COSINE
    );

    struct Statistics {
        size_t total_searches = 0;
        double avg_results_per_search = 0.0;
        double avg_time_ms = 0.0;
        double avg_recall = 0.0;
    };

    const Statistics& getStatistics() const { return stats_; }
    /**
     * @brief Reset Statistics.
     */
    void resetStatistics();

private:
    VectorIndexManager& vector_manager_;
    Statistics stats_;
};

} // namespace vector
} // namespace themis
