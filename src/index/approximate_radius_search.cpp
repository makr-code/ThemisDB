/**
 * @file approximate_radius_search.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "index/approximate_radius_search.h"
#include "index/vector_index.h"
#include "utils/error_registry.h"
#include "storage/base_entity.h"
#include <chrono>
#include <algorithm>

namespace themis {
namespace vector {

inline tl::unexpected<Error> makeError(errors::ErrorCode code, std::string message) {
    return tl::unexpected(Error(code, std::move(message)));
}

ApproximateRadiusSearch::ApproximateRadiusSearch(VectorIndexManager& vector_manager)
    : vector_manager_(vector_manager) {
}

// Helper function to convert metric types
static VectorIndexManager::Metric convertMetric(ApproximateRadiusSearch::Metric metric) {
    switch (metric) {
        case ApproximateRadiusSearch::Metric::L2:
            return VectorIndexManager::Metric::L2;
        case ApproximateRadiusSearch::Metric::COSINE:
            return VectorIndexManager::Metric::COSINE;
        case ApproximateRadiusSearch::Metric::DOT_PRODUCT:
            return VectorIndexManager::Metric::DOT;
    }
    return VectorIndexManager::Metric::COSINE;
}

Result<ApproximateRadiusSearch::SearchResult> 
ApproximateRadiusSearch::search(
    const std::vector<float>& query_vector,
    const SearchConfig& config) {
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Validate inputs
    if (query_vector.empty()) {
        return makeError(errors::ErrorCode::ERR_QUERY_INVALID_INPUT,
                        "Query vector cannot be empty");
    }
    
    if (static_cast<int>(query_vector.size()) != vector_manager_.getDimension()) {
        return makeError(errors::ErrorCode::ERR_QUERY_INVALID_INPUT,
                        "Query vector dimension mismatch. Expected " + 
                        std::to_string(vector_manager_.getDimension()) + 
                        ", got " + std::to_string(query_vector.size()));
    }
    
    if (config.radius <= 0.0f) {
        return makeError(errors::ErrorCode::ERR_QUERY_INVALID_INPUT,
                        "Radius must be positive");
    }
    
    if (config.max_results <= 0) {
        return makeError(errors::ErrorCode::ERR_QUERY_INVALID_INPUT,
                        "Max results must be positive");
    }
    
    // Check if metric matches index configuration
    VectorIndexManager::Metric current_metric = vector_manager_.getMetric();
    VectorIndexManager::Metric requested_metric = convertMetric(config.metric);
    
    if (current_metric != requested_metric) {
        return makeError(errors::ErrorCode::ERR_QUERY_INVALID_INPUT,
                        "Metric mismatch. Index is configured for different metric");
    }
    
    // Use VectorIndexManager's radius search
    size_t max_results = static_cast<size_t>(config.max_results);
    auto [status, results] = vector_manager_.searchKnnRadius(query_vector, config.radius, max_results, nullptr);
    
    if (!status.ok) {
        return makeError(errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED,
                        "Radius search failed: " + status.message);
    }
    
    // Convert results
    SearchResult search_result;
    search_result.results.reserve(results.size());
    
    float max_distance = 0.0f;
    for (const auto& r : results) {
        RadiusResult rr;
        rr.id = r.pk;
        rr.distance = r.distance;
        search_result.results.push_back(std::move(rr));
        max_distance = std::max(max_distance, r.distance);
    }
    
    // Sort if requested
    if (config.sort_results) {
        std::sort(search_result.results.begin(), search_result.results.end(),
                 [](const RadiusResult& a, const RadiusResult& b) {
                     return a.distance < b.distance;
                 });
    }
    
    // Set metadata
    search_result.total_candidates = results.size();
    search_result.actual_max_distance = max_distance;
    search_result.truncated = (static_cast<int>(results.size()) >= max_results && max_results > 0);
    
    auto end = std::chrono::high_resolution_clock::now();
    search_result.computation_time_ms = 
        std::chrono::duration<float, std::milli>(end - start).count();
    
    // Update statistics using numerically stable incremental mean
    stats_.total_searches++;
    double n = static_cast<double>(stats_.total_searches);
    stats_.avg_results_per_search += (static_cast<int>(results.size()) - stats_.avg_results_per_search) / n;
    stats_.avg_time_ms += (search_result.computation_time_ms - stats_.avg_time_ms) / n;
    
    return search_result;
}

Result<ApproximateRadiusSearch::SearchResult> 
ApproximateRadiusSearch::searchById(
    std::string_view query_id,
    const SearchConfig& config) {
    
    // Lookup vector from VectorIndexManager
    auto vectorOpt = vector_manager_.getVectorByPk(query_id);
    if (!vectorOpt) {
        return makeError(errors::ErrorCode::ERR_INDEX_NOT_FOUND,
                        "Vector with ID '" + std::string(query_id) + "' not found");
    }
    
    // Perform radius search with the retrieved vector
    return search(*vectorOpt, config);
}

Result<std::vector<ApproximateRadiusSearch::SearchResult>> 
ApproximateRadiusSearch::batchSearch(
    const std::vector<std::vector<float>>& query_vectors,
    const SearchConfig& config) {
    
    if (query_vectors.empty()) {
        return makeError(errors::ErrorCode::ERR_QUERY_INVALID_INPUT,
                        "Query vectors cannot be empty");
    }
    
    std::vector<SearchResult> batch_results = {};

    batch_results.reserve(query_vectors.size());
    
    // Process each query
    for (const auto& query : query_vectors) {
        auto result = search(query, config);
        if (!result.has_value()) {
            return makeError(result.error().code(), 
                           "Batch search failed on query: " + result.error().message());
        }
        batch_results.push_back(std::move(result.value()));
    }
    
    return batch_results;
}

Result<ApproximateRadiusSearch::SearchResult> 
ApproximateRadiusSearch::searchWithTargetCount(
    const std::vector<float>& query_vector,
    int target_count,
    const SearchConfig& config) {
    
    if (target_count <= 0) {
        return makeError(errors::ErrorCode::ERR_QUERY_INVALID_INPUT,
                        "Target count must be positive");
    }
    
    // Constants for binary search
    constexpr float MIN_SEARCH_RADIUS = 0.01f;
    constexpr float RADIUS_MULTIPLIER = 2.0f;
    constexpr int MAX_SEARCH_ITERATIONS = 10;
    constexpr float TARGET_TOLERANCE = 0.2f;  // 20% tolerance on target count
    
    // Binary search on radius to find the right value that gives ~target_count results
    float min_radius = MIN_SEARCH_RADIUS;
    float max_radius = config.radius * RADIUS_MULTIPLIER;
    SearchResult best_result;
    size_t best_count_diff = std::numeric_limits<size_t>::max();
    
    for (int iter = 0; iter < MAX_SEARCH_ITERATIONS; ++iter) {
        float test_radius = (min_radius + max_radius) / 2.0f;
        
        SearchConfig test_config = config;
        test_config.radius = test_radius;
        test_config.max_results = target_count * 3;  // Allow more to evaluate
        
        auto result = search(query_vector, test_config);
        if (!result.has_value()) {
            return result;
        }
        
        size_t actual_count = result.value().results.size();
        size_t count_diff = (actual_count > static_cast<size_t>(target_count)) 
                           ? (actual_count - target_count) 
                           : (target_count - actual_count);
        
        // Track best result
        if (count_diff < best_count_diff) {
            best_count_diff = count_diff;
            best_result = std::move(result.value());
        }
        
        // Check if we're within tolerance
        float ratio = static_cast<float>(actual_count) / static_cast<float>(target_count);
        if (ratio >= (1.0f - TARGET_TOLERANCE) && ratio <= (1.0f + TARGET_TOLERANCE)) {
            // Truncate to exact target count if needed
            if (static_cast<int>(best_result.results.size()) > static_cast<size_t>(target_count)) {
                best_result.results.resize(target_count);
                best_result.truncated = true;
            }
            return best_result;
        }
        
        // Adjust search range
        if (actual_count < static_cast<size_t>(target_count)) {
            min_radius = test_radius;
        } else {
            max_radius = test_radius;
        }
        
        // Prevent infinite loop with very small radius
        if (max_radius - min_radius < 0.001f) {
            break;
        }
    }
    
    // Return best result found
    if (static_cast<int>(best_result.results.size()) > static_cast<size_t>(target_count)) {
        best_result.results.resize(target_count);
        best_result.truncated = true;
    }
    
    return best_result;
}

Result<size_t> ApproximateRadiusSearch::estimateResultCount(
    const std::vector<float>& query_vector,
    float radius,
    Metric metric) {
    
    // Constants for sampling estimation
    constexpr size_t MAX_SAMPLE_SIZE = 100;
    constexpr size_t MIN_SAMPLE_SIZE = 10;
    constexpr size_t SAMPLE_PERCENTAGE = 10;  // 10% of total
    
    // Validate inputs
    if (query_vector.empty()) {
        return makeError(errors::ErrorCode::ERR_QUERY_INVALID_INPUT,
                        "Query vector cannot be empty");
    }
    
    if (radius <= 0.0f) {
        return makeError(errors::ErrorCode::ERR_QUERY_INVALID_INPUT,
                        "Radius must be positive");
    }
    
    // Check metric matches
    VectorIndexManager::Metric current_metric = vector_manager_.getMetric();
    VectorIndexManager::Metric requested_metric = convertMetric(metric);
    
    if (current_metric != requested_metric) {
        return makeError(errors::ErrorCode::ERR_QUERY_INVALID_INPUT,
                        "Metric mismatch. Index is configured for different metric");
    }
    
    // Get total vector count
    size_t total_vectors = vector_manager_.getVectorCount();
    
    if (total_vectors == 0) {
        return size_t(0);
    }
    
    // Sample-based estimation
    // Sample up to MAX_SAMPLE_SIZE vectors or SAMPLE_PERCENTAGE of total, whichever is smaller
    size_t sample_size = std::min(MAX_SAMPLE_SIZE, 
                                   std::max(MIN_SAMPLE_SIZE, total_vectors / SAMPLE_PERCENTAGE));
    
    // Perform a KNN search to get sample vectors
    auto [status, sample_results] = vector_manager_.searchKnn(query_vector, sample_size, nullptr);
    
    if (!status.ok || sample_results.empty()) {
        // Fallback: assume uniform distribution
        // Very rough estimate
        return size_t(0);
    }
    
    // Count how many samples fall within radius
    size_t within_radius = 0;
    for (const auto& result : sample_results) {
        if (result.distance <= radius) {
            within_radius++;
        }
    }

    // Extrapolate to full dataset
    // This is a rough estimate since we're sampling the k-nearest neighbors
    // which is biased towards closer vectors
    if (within_radius == static_cast<int>(sample_results.size())) {
        // All samples within radius, likely many more
        size_t estimate = (total_vectors * within_radius) / sample_size;
        return std::min(estimate, total_vectors);
    } else if (within_radius == 0) {
        return size_t(0);
    } else {
        // Extrapolate
        size_t estimate = (total_vectors * within_radius) / sample_size;
        return std::min(estimate, total_vectors);
    }
}

void ApproximateRadiusSearch::resetStatistics() {
    stats_ = Statistics{};
}

} // namespace vector
} // namespace themis

