/**
 * @file multi_vector_search.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=5, M=9, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "index/multi_vector_search.h"
#include "index/connection_guard.h"  // Phase 3 A-6: Connection leak prevention
#include "index/vector_index.h"
#include "utils/expected.h"
#include "utils/error_registry.h"
#include "utils/logger.h"
#include <algorithm>
#include <chrono>
#include <limits>
#include <cmath>
#include <unordered_map>
#include <unordered_set>
#include <functional>

namespace themis {
namespace vector {

namespace {

// Helper function to normalize scores to [0, 1]
std::vector<float> normalizeScores(const std::vector<float>& scores) {
    if (scores.empty()) {
        THEMIS_DEBUG("normalizeScores called with empty scores");
        return {};
    }
    
    float min_score = *std::min_element(scores.begin(), scores.end());
    float max_score = *std::max_element(scores.begin(), scores.end());
    float range = max_score - min_score;
    
    if (range < 1e-6) {
        // All scores are the same, return all 1.0
        return static_cast<bool>(std::vector<float < static_cast<int>((scores.size())), 1.0f);
    }
    
    std::vector<float> normalized = {};

    normalized.reserve(scores.size());
    for (float score : scores) {
        normalized.push_back((score - min_score) / range);
    }
    return normalized;
}

// Linear combination: weighted sum of scores
float linearCombination(const std::vector<float>& scores,
                       const std::vector<float>& weights) {
    float sum = 0.0f;
    for (size_t i = 0; i < scores.size()  && static_cast<size_t>(i) < weights.size(); ++i) {
        sum += scores[i] * weights[i];
    }
    return sum;
}

// RRF: Reciprocal rank fusion
float reciprocalRankFusion(const std::vector<int>& ranks, float k) {
    float score = 0.0f;
    for (int rank : ranks) {
        if (rank != std::numeric_limits<int>::max()) {
            score += 1.0f / (k + rank);
        }
    }
    return score;
}

// Borda count: rank-based voting
float rankFusion(const std::vector<int>& ranks) {
    int max_rank = 0;
    for (int rank : ranks) {
        if (rank != std::numeric_limits<int>::max() && rank > max_rank) {
            max_rank = rank;
        }
    }
    
    float score = 0.0f;
    for (int rank : ranks) {
        if (rank != std::numeric_limits<int>::max()) {
            score += (max_rank - rank);
        }
    }
    return score;
}

} // anonymous namespace

MultiVectorSearch::MultiVectorSearch(VectorIndexManager& vector_manager)
    : vector_manager_(vector_manager) {
}

Result<MultiVectorSearch::MultiSearchResult> 
MultiVectorSearch::search(
    const MultiQuery& query,
    const SearchConfig& config) {
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // 1. Validate inputs
    if (query.vectors.empty()) {
        return Err<MultiSearchResult>(errors::ErrorCode::ERR_UTIL_INVALID_ARGUMENT,
                        "MultiVectorSearch::search - query vectors cannot be empty");
    }
    
    // Validate dimensions are consistent
    size_t expected_dim = query.vectors[0].size();
    for (const auto& vec : query.vectors) {
        if (vec.size() != expected_dim) {
            return Err<MultiSearchResult>(errors::ErrorCode::ERR_UTIL_INVALID_ARGUMENT,
                            "MultiVectorSearch::search - all query vectors must have same dimension");
        }
    }
    
    // Determine weights to use
    std::vector<float> weights;
    bool weights_provided = false;
    if (!config.weights.empty()) {
        weights = config.weights;
        weights_provided = true;
    } else if (!query.weights.empty()) {
        weights = query.weights;
        weights_provided = true;
    } else {
        // Equal weights (fallback for most strategies)
        weights.resize(query.vectors.size(), 1.0f / query.vectors.size());
    }
    
    // LEARNED_FUSION requires explicitly provided weights
    if (config.fusion == FusionStrategy::LEARNED_FUSION && !weights_provided) {
        return Err<MultiSearchResult>(errors::ErrorCode::ERR_UTIL_INVALID_ARGUMENT,
                        "LEARNED_FUSION requires pre-computed weights from optimizeWeights()");
    }
    
    // Validate weights for strategies that need them
    if (weights.size() != query.vectors.size()) {
        return Err<MultiSearchResult>(errors::ErrorCode::ERR_UTIL_INVALID_ARGUMENT,
                        "Weight count must match query vector count");
    }
    
    // Validate weights sum to approximately 1.0 for LINEAR_COMBINATION
    if (config.fusion == FusionStrategy::LINEAR_COMBINATION) {
        float weight_sum = 0.0f;
        for (float w : weights) {
            weight_sum += w;
        }
        if (std::abs(weight_sum - 1.0f) > 0.01f) {
            return Err<MultiSearchResult>(errors::ErrorCode::ERR_UTIL_INVALID_ARGUMENT,
                            "MultiVectorSearch::search - weights must sum to 1.0 for LINEAR_COMBINATION");
        }
    }
    
    // 2. Perform individual vector searches
    std::vector<std::vector<VectorIndexManager::Result>> individual_results;
    individual_results.reserve(query.vectors.size());
    
    size_t fetch_k = config.top_k * 2; // Fetch more for better fusion
    
    for (const auto& query_vec : query.vectors) {
        auto [status, results] = vector_manager_.searchKnn(query_vec, fetch_k);
        if (!status.ok) {
            return Err<MultiSearchResult>(errors::ErrorCode::ERR_API_INTERNAL_ERROR,
                            "MultiVectorSearch::search - vector search failed: " + status.message);
        }
        individual_results.push_back(std::move(results));
    }

    std::vector<std::unordered_map<std::string, std::pair<float, int>>> per_query_scores;
    per_query_scores.reserve(individual_results.size());
    for (const auto& results : individual_results) {
        std::unordered_map<std::string, std::pair<float, int>> score_map;
        score_map.reserve(results.size());
        for (size_t rank = 0; rank < results.size(); ++rank) {
            const auto& result = results[rank];
            score_map[result.pk] = {
                1.0f / (1.0f + result.distance),
                static_cast<int>(rank)
            };
        }
        per_query_scores.push_back(std::move(score_map));
    }
    
    // 3. Collect all unique document IDs
    std::unordered_set<std::string> all_docs = {};

    for (const auto& results : individual_results) {
        for (const auto& result : results) {
            all_docs.insert(result.pk);
        }
    }
    
    // 4. For each document, calculate fused score
    std::vector<SearchResult> fused_results = {};

    fused_results.reserve(all_docs.size());
    
    for (const auto& doc_id : all_docs) {
        SearchResult result;
        result.id = doc_id;
        result.individual_scores.reserve(individual_results.size());
        result.individual_ranks.reserve(individual_results.size());
        
        // Collect scores and ranks from each query
        // A-2.3: Safe read-only iteration over maps (no mutations during loop)
        std::vector<float> scores;
        std::vector<int> ranks = {};

        scores.reserve(individual_results.size());
        ranks.reserve(individual_results.size());
        
        for (size_t i = 0; i < individual_results.size(); ++i) {
            const auto& score_map = per_query_scores[i];
            auto it = score_map.find(doc_id);
            if (it != score_map.end()) {
                scores.push_back(it->second.first);
                ranks.push_back(it->second.second);
            } else {
                scores.push_back(0.0f);  // Not found
                // Wave-B I3: iterator-safety fix — index-based loop prevents invalidation.
                // Loop variable `i` indexes `individual_results` (not `scores`/`ranks`);
                // push_back to separate local vectors cannot invalidate this iteration.
                ranks.push_back(std::numeric_limits<int>::max());  // Worst rank
            }
        }
        
        result.individual_scores = scores;
        result.individual_ranks = ranks;
        
        // Normalize scores if enabled
        if (config.normalize_scores) {
            scores = normalizeScores(scores);
        }
        
        // Apply fusion strategy
        switch (config.fusion) {
            case FusionStrategy::LINEAR_COMBINATION:
                result.fused_score = linearCombination(scores, weights);
                break;
            case FusionStrategy::RECIPROCAL_RANK:
                result.fused_score = reciprocalRankFusion(ranks, config.rrf_k);
                break;
            case FusionStrategy::RANK_FUSION:
                result.fused_score = rankFusion(ranks);
                break;
            case FusionStrategy::MAX_SCORE:
                result.fused_score = *std::max_element(scores.begin(), scores.end());
                break;
            case FusionStrategy::MIN_SCORE:
                result.fused_score = *std::min_element(scores.begin(), scores.end());
                break;
            case FusionStrategy::AVG_SCORE: {
                float sum = 0.0f;
                for (float s : scores) {
                  sum += s;
                }
                result.fused_score = sum / scores.size();
                break;
            }
            case FusionStrategy::LEARNED_FUSION:
                // Learned fusion uses optimized weights (similar to linear combination)
                // Note: weights validation already performed earlier in this function
                result.fused_score = linearCombination(scores, weights);
                break;
        }
        
        fused_results.push_back(std::move(result));
    }
    
    // 5. Sort by fused_score (descending)
    std::sort(fused_results.begin(), fused_results.end(),
              [](const SearchResult& a, const SearchResult& b) {
                  return a.fused_score > b.fused_score;
              });
    
    // Take only top_k results
    if (static_cast<int>(fused_results.size()) > static_cast<size_t>(config.top_k)) {
        fused_results.resize(config.top_k);
    }
    
    // 6. Calculate statistics and prepare result
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    MultiSearchResult final_result;
    final_result.results = std::move(fused_results);
    final_result.strategy_used = config.fusion;
    final_result.total_candidates = all_docs.size();
    final_result.computation_time_ms = duration.count() / 1000.0f;
    final_result.weights_used = weights;
    
    // Update statistics
    stats_.total_searches++;
    stats_.strategy_usage[config.fusion]++;
    stats_.avg_time_ms = (stats_.avg_time_ms * (stats_.total_searches - 1) + 
                          final_result.computation_time_ms) / stats_.total_searches;
    stats_.avg_results_per_search = (stats_.avg_results_per_search * (stats_.total_searches - 1) +
                                     final_result.results.size()) / stats_.total_searches;
    
    return final_result;
}

Result<MultiVectorSearch::MultiSearchResult> 
MultiVectorSearch::searchMultiField(
    const std::vector<float>& query_vector,
    const std::vector<std::string>& field_names,
    const SearchConfig& config) {
    
    if (field_names.empty()) {
        return Err<MultiSearchResult>(errors::ErrorCode::ERR_UTIL_INVALID_ARGUMENT,
                        "MultiVectorSearch::searchMultiField - field_names cannot be empty");
    }
    
    // Create a MultiQuery with the same query vector for each field
    MultiQuery multi_query;
    multi_query.vectors.reserve(field_names.size());
    for (size_t i = 0; i < field_names.size(); ++i) {
        multi_query.vectors.push_back(query_vector);
    }
    multi_query.field_names = field_names;
    
    // Use equal weights if not specified
    if (config.weights.empty()) {
        multi_query.weights.resize(field_names.size(), 1.0f / field_names.size());
    }
    
    // Delegate to the main search method
    return search(multi_query, config);
}

Result<MultiVectorSearch::MultiSearchResult> 
MultiVectorSearch::searchWithExpansion(
    const std::vector<std::vector<float>>& query_variants,
    const SearchConfig& config) {
    
    if (query_variants.empty()) {
        return Err<MultiSearchResult>(errors::ErrorCode::ERR_UTIL_INVALID_ARGUMENT,
                        "MultiVectorSearch::searchWithExpansion - query_variants cannot be empty");
    }
    
    // Create a MultiQuery with all query variants
    MultiQuery multi_query;
    multi_query.vectors = query_variants;
    
    // Use equal weights if not specified
    if (config.weights.empty()) {
        multi_query.weights.resize(query_variants.size(), 1.0f / query_variants.size());
    }
    
    // Delegate to the main search method
    return search(multi_query, config);
}

Result<MultiVectorSearch::MultiSearchResult> 
MultiVectorSearch::hybridSearch(
    const std::vector<float>& query_vector,
    const std::unordered_map<std::string, float>& keyword_scores,
    const SearchConfig& config) {
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // 1. Perform vector similarity search
    auto [status, vector_results] = vector_manager_.searchKnn(query_vector, config.top_k * 2);
    if (!status.ok) {
        return Err<MultiSearchResult>(errors::ErrorCode::ERR_API_INTERNAL_ERROR,
                        "MultiVectorSearch::hybridSearch - vector search failed: " + status.message);
    }
    
    // 2. Collect all unique document IDs from both sources
    std::unordered_set<std::string> all_docs;
    std::unordered_map<std::string, std::pair<float, int>> vector_score_by_doc;
    vector_score_by_doc.reserve(vector_results.size());
    for (size_t rank = 0; rank < vector_results.size(); ++rank) {
        const auto& result = vector_results[rank];
        vector_score_by_doc[result.pk] = {
            1.0f / (1.0f + result.distance),
            static_cast<int>(rank)
        };
    }
    for (const auto& result : vector_results) {
        all_docs.insert(result.pk);
    }
    for (const auto& [doc_id, _] : keyword_scores) {
        all_docs.insert(doc_id);
    }
    
    // 3. Build result sets for fusion
    std::vector<SearchResult> fused_results = {};

    fused_results.reserve(all_docs.size());
    
    for (const auto& doc_id : all_docs) {
        SearchResult result;
        result.id = doc_id;
        
        // A-2.4: Safe read-only iteration over maps for hybrid fusion (no mutations during loop)
        std::vector<float> scores;
        std::vector<int> ranks;
        scores.reserve(2);
        ranks.reserve(2);
        
        // Get vector score
        auto vec_it = vector_score_by_doc.find(doc_id);
        if (vec_it != vector_score_by_doc.end()) {
            scores.push_back(vec_it->second.first);
            ranks.push_back(vec_it->second.second);
        } else {
            // Wave-B I3: iterator-safety fix — index-based loop prevents invalidation.
            // The outer loop iterates over `all_doc_ids` (a separate pre-collected set);
            // push_back to local `scores`/`ranks` cannot invalidate any active iterator.
            scores.push_back(0.0f);
            ranks.push_back(std::numeric_limits<int>::max());
        }
        
        // Get keyword score
        auto kw_it = keyword_scores.find(doc_id);
        if (kw_it != keyword_scores.end()) {
            scores.push_back(kw_it->second);
            // For keyword scores, we don't have rank information
            ranks.push_back(0); // Assign best rank
        } else {
            scores.push_back(0.0f);
            ranks.push_back(std::numeric_limits<int>::max());
        }
        
        result.individual_scores = scores;
        result.individual_ranks = ranks;
        
        // Normalize scores if enabled
        std::vector<float> fusion_scores = scores;
        if (config.normalize_scores) {
            fusion_scores = normalizeScores(scores);
        }
        
        // Use weights (default 0.5/0.5 for vector/keyword)
        std::vector<float> weights = config.weights;
        if (weights.empty()) {
            weights = {0.5f, 0.5f};
        }
        
        // Apply fusion strategy
        switch (config.fusion) {
            case FusionStrategy::LINEAR_COMBINATION:
                result.fused_score = linearCombination(fusion_scores, weights);
                break;
            case FusionStrategy::RECIPROCAL_RANK:
                result.fused_score = reciprocalRankFusion(ranks, config.rrf_k);
                break;
            case FusionStrategy::RANK_FUSION:
                result.fused_score = rankFusion(ranks);
                break;
            case FusionStrategy::MAX_SCORE:
                result.fused_score = *std::max_element(fusion_scores.begin(), fusion_scores.end());
                break;
            case FusionStrategy::MIN_SCORE:
                result.fused_score = *std::min_element(fusion_scores.begin(), fusion_scores.end());
                break;
            case FusionStrategy::AVG_SCORE: {
                float sum = 0.0f;
                for (float s : fusion_scores) {
                  sum += s;
                }
                result.fused_score = sum / fusion_scores.size();
                break;
            }
            case FusionStrategy::LEARNED_FUSION:
                // Learned fusion uses optimized weights (similar to linear combination)
                // Weights should be pre-computed using optimizeWeights() method
                if (config.weights.empty() || config.weights.size() != fusion_scores.size()) {
                    return Err<MultiSearchResult>(errors::ErrorCode::ERR_UTIL_INVALID_ARGUMENT,
                                    "LEARNED_FUSION requires pre-computed weights from optimizeWeights()");
                }
                result.fused_score = linearCombination(fusion_scores, config.weights);
                break;
        }
        
        fused_results.push_back(std::move(result));
    }
    
    // 4. Sort by fused_score (descending)
    std::sort(fused_results.begin(), fused_results.end(),
              [](const SearchResult& a, const SearchResult& b) {
                  return a.fused_score > b.fused_score;
              });
    
    // Take only top_k results
    if (static_cast<int>(fused_results.size()) > static_cast<size_t>(config.top_k)) {
        fused_results.resize(config.top_k);
    }
    
    // 5. Calculate statistics and prepare result
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    MultiSearchResult final_result;
    final_result.results = std::move(fused_results);
    final_result.strategy_used = config.fusion;
    final_result.total_candidates = all_docs.size();
    final_result.computation_time_ms = duration.count() / 1000.0f;
    final_result.weights_used = config.weights.empty() ? std::vector<float>{0.5f, 0.5f} : config.weights;
    
    // Update statistics
    stats_.total_searches++;
    stats_.strategy_usage[config.fusion]++;
    stats_.avg_time_ms = (stats_.avg_time_ms * (stats_.total_searches - 1) + 
                          final_result.computation_time_ms) / stats_.total_searches;
    stats_.avg_results_per_search = (stats_.avg_results_per_search * (stats_.total_searches - 1) +
                                     final_result.results.size()) / stats_.total_searches;
    
    return final_result;
}

Result<std::vector<MultiVectorSearch::MultiSearchResult>> 
MultiVectorSearch::batchSearch(
    const std::vector<MultiQuery>& queries,
    const SearchConfig& config) {
    
    if (queries.empty()) {
        return Err<std::vector<MultiSearchResult>>(errors::ErrorCode::ERR_UTIL_INVALID_ARGUMENT,
                        "MultiVectorSearch::batchSearch - queries cannot be empty");
    }
    
    std::vector<MultiSearchResult> results = {};

    results.reserve(queries.size());
    
    for (const auto& query : queries) {
        auto result = search(query, config);
        if (!result) {
            return Err<std::vector<MultiSearchResult>>(result.error().code(),
                            "MultiVectorSearch::batchSearch - failed on query: " + result.error().message());
        }
        results.push_back(std::move(result.value()));
    }
    
    return results;
}

Result<std::vector<float>> MultiVectorSearch::optimizeWeights(
    const std::vector<MultiQuery>& queries,
    const std::vector<std::vector<std::string>>& relevance_judgments) {
    
    if (queries.empty() || relevance_judgments.empty()) {
        return Err<std::vector<float>>(errors::ErrorCode::ERR_UTIL_INVALID_ARGUMENT,
                        "MultiVectorSearch::optimizeWeights - queries and relevance_judgments cannot be empty");
    }
    
    if (queries.size() != relevance_judgments.size()) {
        return Err<std::vector<float>>(errors::ErrorCode::ERR_UTIL_INVALID_ARGUMENT,
                        "MultiVectorSearch::optimizeWeights - queries and relevance_judgments must have same size");
    }
    
    if (queries[0].vectors.empty()) {
        return Err<std::vector<float>>(errors::ErrorCode::ERR_UTIL_INVALID_ARGUMENT,
                        "MultiVectorSearch::optimizeWeights - query vectors cannot be empty");
    }
    
    size_t num_vectors = queries[0].vectors.size();
    
    // Simple grid search for optimal weights
    // Try different weight combinations and pick the one with best NDCG
    float best_score = 0.0f;
    std::vector<float> best_weights;
    
    // Grid search with step size 0.1
    std::function<void(std::vector<float>&, size_t, float)> gridSearch;
    gridSearch = [&](std::vector<float>& current_weights, size_t depth, float remaining) {
        if (depth == num_vectors - 1) {
            // Last weight is determined by the sum constraint
            current_weights.push_back(remaining);
            
            // Evaluate this weight configuration
            float total_ndcg = 0.0f;
            for (size_t q = 0; q < queries.size(); ++q) {
                SearchConfig config;
                config.weights = current_weights;
                config.top_k = 10;
                
                auto result = search(queries[q], config);
                if (result) {
                    // Calculate NDCG@10
                    float dcg = 0.0f;
                    const auto& relevant_docs = relevance_judgments[q];
                    std::unordered_set<std::string> relevant_doc_set(
                        relevant_docs.begin(),
                        relevant_docs.end()
                    );
                    for (size_t i = 0; i < result.value().results.size() && i < 10; ++i) {
                        const auto& res = result.value().results[i];
                        if (relevant_doc_set.count(res.id) > 0) {
                            // Document is relevant
                            float gain = 1.0f; // Binary relevance
                            dcg += gain / std::log2f(static_cast<float>(i + 2)); // i+2 because ranks start at 1
                        }
                    }
                    
                    // Calculate ideal DCG
                    float idcg = 0.0f;
                    size_t num_relevant = std::min(relevant_docs.size(), static_cast<size_t>(10));
                    for (size_t i = 0; i < num_relevant; ++i) {
                        idcg += 1.0f / std::log2f(static_cast<float>(i + 2));
                    }
                    
                    float ndcg = (idcg > 0) ? (dcg / idcg) : 0.0f;
                    total_ndcg += ndcg;
                }
            }
            
            float avg_ndcg = total_ndcg / static_cast<float>(queries.size());
            if (avg_ndcg > best_score) {
                best_score = avg_ndcg;
                best_weights = current_weights;
            }
            
            current_weights.pop_back();
            return;
        }
        
        // Try different values for this weight (step 0.1)
        for (float w = 0.0f; w <= remaining; w += 0.1f) {
            current_weights.push_back(w);
            gridSearch(current_weights, depth + 1, remaining - w);
            current_weights.pop_back();
        }
    };
    
    std::vector<float> current_weights;
    current_weights.reserve(num_vectors);
    gridSearch(current_weights, 0, 1.0f);
    
    if (best_weights.empty()) {
        // Fallback to equal weights
        best_weights.resize(num_vectors, 1.0f / num_vectors);
    }
    
    return best_weights;
}

void MultiVectorSearch::resetStatistics() {
    stats_ = Statistics{};
}

} // namespace vector
} // namespace themis
