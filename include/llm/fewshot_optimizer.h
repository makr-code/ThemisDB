/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            fewshot_optimizer.h                                ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:45:27                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     206                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file fewshot_optimizer.h
 * @brief Automatic few-shot example selection and optimization
 * 
 * Implements intelligent few-shot example selection based on:
 * - Diversity-based sampling
 * - Relevance scoring
 * - Performance-based caching
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <nlohmann/json.hpp>

namespace themis {
namespace llm {

/**
 * @brief Few-shot example with metadata
 */
struct FewShotExample {
    std::string input;                 ///< Example input
    std::string output;                ///< Example output
    nlohmann::json context;            ///< Additional context
    double relevance_score = 0.0;      ///< Relevance to query
    double diversity_score = 0.0;      ///< Diversity from other examples
    nlohmann::json metadata;           ///< Additional metadata
};

/**
 * @brief Configuration for few-shot optimization
 */
struct FewShotConfig {
    size_t max_examples = 5;           ///< Maximum number of examples
    size_t min_examples = 1;           ///< Minimum number of examples
    double diversity_weight = 0.4;     ///< Weight for diversity
    double relevance_weight = 0.6;     ///< Weight for relevance
    bool enable_caching = true;        ///< Enable example caching
    size_t cache_size = 1000;          ///< Maximum cache entries
};

/**
 * @brief Few-shot selection result
 */
struct SelectionResult {
    std::vector<FewShotExample> selected_examples; ///< Selected examples
    double avg_relevance = 0.0;        ///< Average relevance score
    double avg_diversity = 0.0;        ///< Average diversity score
    double selection_score = 0.0;      ///< Overall selection quality
    nlohmann::json metadata;           ///< Additional metadata
};

/**
 * @brief Few-shot optimizer class
 * 
 * Implements automatic example selection:
 * - Relevance-based ranking (similarity to query)
 * - Diversity-based sampling (coverage of input space)
 * - Performance-based caching (reuse successful selections)
 * - Dynamic example count (optimize for quality vs context length)
 */
class FewShotOptimizer {
public:
    /**
     * @brief Constructor
     * @param config Few-shot configuration
     */
    explicit FewShotOptimizer(const FewShotConfig& config = FewShotConfig{});
    
    /**
     * @brief Select optimal few-shot examples for a query
     * @param query Input query
     * @param candidate_examples Pool of candidate examples
     * @param num_examples Number of examples to select (optional)
     * @return Selection result with chosen examples
     */
    SelectionResult selectExamples(
        const std::string& query,
        const std::vector<FewShotExample>& candidate_examples,
        std::optional<size_t> num_examples = std::nullopt
    );
    
    /**
     * @brief Add examples to the cache
     * @param examples Examples to cache
     */
    void cacheExamples(const std::vector<FewShotExample>& examples);
    
    /**
     * @brief Get cached examples similar to query
     * @param query Input query
     * @param max_results Maximum number of results
     * @return Cached examples sorted by relevance
     */
    std::vector<FewShotExample> getCachedExamples(
        const std::string& query,
        size_t max_results = 10
    ) const;
    
    /**
     * @brief Clear the example cache
     */
    void clearCache();
    
    /**
     * @brief Compute relevance score between query and example
     * @param query Input query
     * @param example Candidate example
     * @return Relevance score (0.0-1.0)
     */
    static double computeRelevance(
        const std::string& query,
        const FewShotExample& example
    );
    
    /**
     * @brief Compute diversity score for a set of examples
     * @param examples Set of examples
     * @return Diversity score (0.0-1.0)
     */
    static double computeDiversity(
        const std::vector<FewShotExample>& examples
    );
    
    /**
     * @brief Format examples for prompt injection
     * @param examples Examples to format
     * @param format Format template (optional)
     * @return Formatted examples string
     */
    static std::string formatExamples(
        const std::vector<FewShotExample>& examples,
        const std::string& format = "Input: {input}\nOutput: {output}\n\n"
    );
    
    /**
     * @brief Get current configuration
     */
    const FewShotConfig& getConfig() const { return config_; }
    
    /**
     * @brief Update configuration
     */
    void setConfig(const FewShotConfig& config) { config_ = config; }
    
    /**
     * @brief Get cache statistics
     */
    nlohmann::json getCacheStats() const;

private:
    FewShotConfig config_;
    std::vector<FewShotExample> cache_;
    std::unordered_map<std::string, std::vector<size_t>> query_index_;
    
    /**
     * @brief Select examples using greedy diversity sampling
     */
    std::vector<FewShotExample> greedyDiversitySelection(
        const std::string& query,
        const std::vector<FewShotExample>& candidates,
        size_t num_examples
    );
    
    /**
     * @brief Compute pairwise similarity between examples
     */
    static double computeSimilarity(
        const FewShotExample& ex1,
        const FewShotExample& ex2
    );
    
    /**
     * @brief Update query index for faster lookup
     */
    void updateQueryIndex();
};

} // namespace llm
} // namespace themis
