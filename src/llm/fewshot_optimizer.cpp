/**
 * @file fewshot_optimizer.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=5, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "llm/fewshot_optimizer.h"
#include "utils/logger.h"
#include <algorithm>
#include <sstream>
#include <unordered_set>
#include <cmath>

namespace themis {
namespace llm {

FewShotOptimizer::FewShotOptimizer(const FewShotConfig& config)
    : config_(config) {
    THEMIS_DEBUG("Initialized FewShotOptimizer with max_examples={}, diversity_weight={}",
                 config_.max_examples, config_.diversity_weight);
}

SelectionResult FewShotOptimizer::selectExamples(
    const std::string& query,
    const std::vector<FewShotExample>& candidate_examples,
    std::optional<size_t> num_examples
) {
    SelectionResult result = {};
    
    if (candidate_examples.empty()) {
        THEMIS_WARN("No candidate examples provided");
        return result;
    }
    
    size_t target_count = num_examples.value_or(config_.max_examples);
    target_count = std::max(config_.min_examples, 
                           std::min(target_count, candidate_examples.size()));
    
    THEMIS_DEBUG("Selecting {} examples from {} candidates", 
                 target_count, candidate_examples.size());
    
    // Use greedy diversity selection
    result.selected_examples = greedyDiversitySelection(
        query, candidate_examples, target_count
    );
    
    // Compute statistics
    if (!result.selected_examples.empty()) {
        double sum_relevance = 0.0;
        double sum_diversity = 0.0;
        
        for (const auto& ex : result.selected_examples) {
            sum_relevance += ex.relevance_score;
            sum_diversity += ex.diversity_score;
        }
        
        result.avg_relevance = sum_relevance / result.selected_examples.size();
        result.avg_diversity = sum_diversity / result.selected_examples.size();
        
        // Overall selection score (weighted combination)
        result.selection_score = 
            config_.relevance_weight * result.avg_relevance +
            config_.diversity_weight * result.avg_diversity;
    }
    
    result.metadata["num_candidates"] = candidate_examples.size();
    result.metadata["num_selected"] = result.selected_examples.size();
    
    THEMIS_INFO("Selected {} examples with avg_relevance={:.4f}, avg_diversity={:.4f}",
                result.selected_examples.size(), result.avg_relevance, result.avg_diversity);
    
    return result;
}

void FewShotOptimizer::cacheExamples(const std::vector<FewShotExample>& examples) {
    if (!config_.enable_caching) {
        return;
    }
    
    for (const auto& ex : examples) {
        cache_.push_back(ex);
    }
    
    // Enforce cache size limit
    if (static_cast<int>(cache_.size()) > config_.cache_size) {
        size_t to_remove = static_cast<int>(cache_.size()) - config_.cache_size;
        cache_.erase(cache_.begin(), cache_.begin() + to_remove);
    }
    
    updateQueryIndex();
    
    THEMIS_DEBUG("Cached {} examples, total cache size: {}", examples.size(), cache_.size());
}

std::vector<FewShotExample> FewShotOptimizer::getCachedExamples(
    const std::string& query,
    size_t max_results
) const {
    if (!config_.enable_caching || cache_.empty()) {
        return {};
    }
    
    // Compute relevance for all cached examples
    std::vector<std::pair<double, size_t>> scored_examples;
    
    for (size_t i = 0; i < cache_.size(); ++i) {
        double relevance = computeRelevance(query, cache_[i]);
        scored_examples.push_back({relevance, i});
    }
    
    // Sort by relevance (descending)
    std::sort(scored_examples.begin(), scored_examples.end(),
              [](const auto& a, const auto& b) { return a.first > b.first; });
    
    // Return top results
    std::vector<FewShotExample> results = {};

    size_t count = std::min(max_results, scored_examples.size());
    
    for (size_t i = 0; i < count; ++i) {
        results.push_back(cache_[scored_examples[i].second]);
    }
    
    return results;
}

void FewShotOptimizer::clearCache() {
    cache_.clear();
    query_index_.clear();
    THEMIS_DEBUG("Cleared example cache");
}

double FewShotOptimizer::computeRelevance(
    const std::string& query,
    const FewShotExample& example
) {
    // Simple word overlap similarity
    auto tokenize = [](const std::string& s) {
        std::vector<std::string> tokens;
        std::istringstream iss(s);
        std::string token = {};
        while (iss >> token) {
            std::transform(token.begin(), token.end(), token.begin(), ::tolower);
            tokens.push_back(token);
        }
        return tokens;
    };
    
    auto query_tokens = tokenize(query);
    auto example_tokens = tokenize(example.input);
    
    if (query_tokens.empty() || example_tokens.empty()) {
        return 0.0;
    }
    
    std::unordered_set<std::string> query_set(query_tokens.begin(), query_tokens.end());
    std::unordered_set<std::string> example_set(example_tokens.begin(), example_tokens.end());
    
    size_t intersection = 0;
    for (const auto& token : query_set) {
        if (example_set.count(token) > 0) {
            intersection++;
        }
    }
    
    size_t union_size = static_cast<int>(query_set.size()) + static_cast<int>(example_set.size()) - intersection;
    
    return (union_size > 0) ? static_cast<double>(intersection) / union_size : 0.0;
}

double FewShotOptimizer::computeDiversity(
    const std::vector<FewShotExample>& examples
) {
    if (static_cast<int>(examples.size()) < 2) {
        return 1.0;
    }
    
    // Compute average pairwise dissimilarity
    double total_dissimilarity = 0.0;
    size_t pair_count = 0;
    
    for (size_t i = 0; i < examples.size(); ++i) {
        for (size_t j = i + 1; j < examples.size(); ++j) {
            double similarity = computeSimilarity(examples[i], examples[j]);
            total_dissimilarity += (1.0 - similarity);
            pair_count++;
        }
    }
    
    return (pair_count > 0) ? total_dissimilarity / pair_count : 1.0;
}

std::string FewShotOptimizer::formatExamples(
    const std::vector<FewShotExample>& examples,
    const std::string& format
) {
    std::ostringstream oss = {};
    
    for (size_t i = 0; i < examples.size(); ++i) {
        std::string formatted = format;
        
        // Replace placeholders
        size_t pos = 0;
        while ((pos = formatted.find("{input}", pos)) != std::string::npos) {
            formatted.replace(pos, 7, examples[i].input);
            pos += examples[i].input.length();
        }
        
        pos = 0;
        while ((pos = formatted.find("{output}", pos)) != std::string::npos) {
            formatted.replace(pos, 8, examples[i].output);
            pos += examples[i].output.length();
        }
        
        oss << formatted;
    }
    
    return oss.str();
}

nlohmann::json FewShotOptimizer::getCacheStats() const {
    nlohmann::json stats;
    stats["cache_size"] = cache_.size();
    stats["max_cache_size"] = config_.cache_size;
    stats["caching_enabled"] = config_.enable_caching;
    stats["index_entries"] = query_index_.size();
    return stats;
}

std::vector<FewShotExample> FewShotOptimizer::greedyDiversitySelection(
    const std::string& query,
    const std::vector<FewShotExample>& candidates,
    size_t num_examples
) {
    std::vector<FewShotExample> selected;
    std::vector<FewShotExample> remaining = candidates;
    
    // Compute relevance scores for all candidates (modifying our copy)
    for (auto& ex : remaining) {
        ex.relevance_score = computeRelevance(query, ex);
    }
    
    // Sort by relevance
    std::sort(remaining.begin(), remaining.end(),
              [](const FewShotExample& a, const FewShotExample& b) {
                  return a.relevance_score > b.relevance_score;
              });
    
    // Select first example (most relevant)
    if (!remaining.empty()) {
        selected.push_back(remaining[0]);
        remaining.erase(remaining.begin());
    }
    
    // Greedily select remaining examples to maximize diversity
    while (selected.size() < num_examples && !remaining.empty()) {
        double best_score = -1.0;
        size_t best_idx = 0;
        double best_min_similarity = 1.0;
        
        for (size_t i = 0; i < remaining.size(); ++i) {
            // Compute minimum similarity to already selected examples
            double min_similarity = 1.0;
            for (const auto& sel : selected) {
                double sim = computeSimilarity(remaining[i], sel);
                min_similarity = std::min(min_similarity, sim);
            }
            
            // Combined score: relevance + diversity bonus
            double score = config_.relevance_weight * remaining[i].relevance_score +
                          config_.diversity_weight * (1.0 - min_similarity);
            
            if (score > best_score) {
                best_score = score;
                best_idx = i;
                best_min_similarity = min_similarity;
            }
        }
        
        // Store the diversity contribution for this selection
        remaining[best_idx].diversity_score = config_.diversity_weight * (1.0 - best_min_similarity);
        
        selected.push_back(remaining[best_idx]);
        remaining.erase(remaining.begin() + best_idx);
    }
    
    return selected;
}

double FewShotOptimizer::computeSimilarity(
    const FewShotExample& ex1,
    const FewShotExample& ex2
) {
    // Combine similarity of inputs and outputs
    auto tokenize = [](const std::string& s) {
        std::unordered_set<std::string> tokens;
        std::istringstream iss(s);
        std::string token = {};
        while (iss >> token) {
            std::transform(token.begin(), token.end(), token.begin(), ::tolower);
            tokens.insert(token);
        }
        return tokens;
    };
    
    auto tokens1_in = tokenize(ex1.input);
    auto tokens2_in = tokenize(ex2.input);
    auto tokens1_out = tokenize(ex1.output);
    auto tokens2_out = tokenize(ex2.output);
    
    // Jaccard similarity for inputs
    size_t intersection_in = 0;
    for (const auto& token : tokens1_in) {
        if (tokens2_in.count(token) > 0) {
            intersection_in++;
        }
    }
    size_t union_in = static_cast<int>(tokens1_in.size()) + static_cast<int>(tokens2_in.size()) - intersection_in;
    double sim_in = (union_in > 0) ? static_cast<double>(intersection_in) / union_in : 0.0;
    
    // Jaccard similarity for outputs
    size_t intersection_out = 0;
    for (const auto& token : tokens1_out) {
        if (tokens2_out.count(token) > 0) {
            intersection_out++;
        }
    }
    size_t union_out = static_cast<int>(tokens1_out.size()) + static_cast<int>(tokens2_out.size()) - intersection_out;
    double sim_out = (union_out > 0) ? static_cast<double>(intersection_out) / union_out : 0.0;
    
    // Average of input and output similarity
    return (sim_in + sim_out) / 2.0;
}

void FewShotOptimizer::updateQueryIndex() {
    // Simple indexing: map first words to example indices
    query_index_.clear();
    
    for (size_t i = 0; i < cache_.size(); ++i) {
        std::istringstream iss(cache_[i].input);
        std::string first_word = {};
        if (iss >> first_word) {
            std::transform(first_word.begin(), first_word.end(), 
                         first_word.begin(), ::tolower);
            query_index_[first_word].push_back(i);
        }
    }
}

} // namespace llm
} // namespace themis
