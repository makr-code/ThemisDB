/**
 * @file fewshot_optimizer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <nlohmann/json.hpp>

namespace themis {
namespace llm {

struct FewShotExample {
    std::string input;                 ///< Example input
    std::string output;                ///< Example output
    nlohmann::json context;            ///< Additional context
    double relevance_score = 0.0;      ///< Relevance to query
    double diversity_score = 0.0;      ///< Diversity from other examples
    nlohmann::json metadata;           ///< Additional metadata
};

struct FewShotConfig {
    size_t max_examples = 5;           ///< Maximum number of examples
    size_t min_examples = 1;           ///< Minimum number of examples
    double diversity_weight = 0.4;     ///< Weight for diversity
    double relevance_weight = 0.6;     ///< Weight for relevance
    bool enable_caching = true;        ///< Enable example caching
    size_t cache_size = 1000;          ///< Maximum cache entries
};

struct SelectionResult {
    std::vector<FewShotExample> selected_examples; ///< Selected examples
    double avg_relevance = 0.0;        ///< Average relevance score
    double avg_diversity = 0.0;        ///< Average diversity score
    double selection_score = 0.0;      ///< Overall selection quality
    nlohmann::json metadata;           ///< Additional metadata
};

class FewShotOptimizer {
public:
    explicit FewShotOptimizer(const FewShotConfig& config = FewShotConfig{});
    
    SelectionResult selectExamples(
        const std::string& query,
        const std::vector<FewShotExample>& candidate_examples,
        std::optional<size_t> num_examples = std::nullopt
    );
    
    /**
     * @brief Cache Examples.
     * @param[in] examples Input parameter.
     */
    void cacheExamples(const std::vector<FewShotExample>& examples);
    
    std::vector<FewShotExample> getCachedExamples(
        const std::string& query,
        size_t max_results = 10
    ) const;
    
    /**
     * @brief Clear Cache.
     */
    void clearCache();
    
    /**
     * @brief Compute Relevance.
     * @param[in] query Input parameter.
     * @param[in] example Input parameter.
     * @return Return value.
     */
    static double computeRelevance(
        const std::string& query,
        const FewShotExample& example
    );
    
    /**
     * @brief Compute Diversity.
     * @param[in] examples Input parameter.
     * @return Return value.
     */
    static double computeDiversity(
        const std::vector<FewShotExample>& examples
    );
    
    static std::string formatExamples(
        const std::vector<FewShotExample>& examples,
        const std::string& format = "Input: {input}\nOutput: {output}\n\n"
    );
    
    const FewShotConfig& getConfig() const { return config_; }
    
    /**
     * @brief Set Config.
     * @param[in] config Input parameter.
     * @details Implements setConfig without additional internal calls.
     */
    void setConfig(const FewShotConfig& config) { config_ = config; }
    
    /**
     * @brief Get Cache Stats.
     * @return Return value.
     */
    nlohmann::json getCacheStats() const;

private:
    FewShotConfig config_;
    std::vector<FewShotExample> cache_;
    std::unordered_map<std::string, std::vector<size_t>> query_index_;
    
    /**
     * @brief Greedy Diversity Selection.
     * @param[in] query Input parameter.
     * @param[in] candidates Input parameter.
     * @param[in] num_examples Input parameter.
     * @return Return value.
     */
    std::vector<FewShotExample> greedyDiversitySelection(
        const std::string& query,
        const std::vector<FewShotExample>& candidates,
        size_t num_examples
    );
    
    /**
     * @brief Compute Similarity.
     * @param[in] ex1 Input parameter.
     * @param[in] ex2 Input parameter.
     * @return Return value.
     */
    static double computeSimilarity(
        const FewShotExample& ex1,
        const FewShotExample& ex2
    );
    
    /**
     * @brief Update Query Index.
     */
    void updateQueryIndex();
};

} // namespace llm
} // namespace themis
