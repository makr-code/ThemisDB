/**
 * @file prompt_evaluator.h
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
#include <nlohmann/json.hpp>

namespace themis {
namespace llm {

struct EvaluationMetrics {
    double semantic_similarity = 0.0;  ///< Semantic similarity score (0.0-1.0)
    double exact_match = 0.0;          ///< Exact match score (0.0 or 1.0)
    double partial_match = 0.0;        ///< Partial match score (0.0-1.0)
    double relevance = 0.0;            ///< Relevance score (0.0-1.0)
    nlohmann::json details;            ///< Additional metric details
};

struct AggregatedMetrics {
    /**
     * @brief Aggregated Metrics.
     * @return Return value.
     */
    virtual ~AggregatedMetrics() = default;
    double overall_score = 0.0;        ///< Overall weighted score
    double mean_similarity = 0.0;      ///< Mean semantic similarity
    double std_similarity = 0.0;       ///< Standard deviation of similarity
    size_t num_exact_matches = 0;      ///< Number of exact matches
    double pass_rate = 0.0;            ///< Percentage passing threshold
    nlohmann::json per_case_metrics;   ///< Per-test-case breakdown
};

struct EvaluatorConfig {
    double similarity_weight = 0.5;    ///< Weight for semantic similarity
    double exact_match_weight = 0.3;   ///< Weight for exact matches
    double relevance_weight = 0.2;     ///< Weight for relevance
    double pass_threshold = 0.7;       ///< Minimum score to pass
    bool enable_statistical_tests = true; ///< Enable significance testing
};

class PromptEvaluator {
public:
    explicit PromptEvaluator(const EvaluatorConfig& config = EvaluatorConfig{});
    
    /**
     * @brief Evaluate Single.
     * @param[in] output Input parameter.
     * @param[in] expected Input parameter.
     * @return Return value.
     */
    EvaluationMetrics evaluateSingle(
        const std::string& output,
        const std::string& expected
    ) const;
    
    /**
     * @brief Evaluate Batch.
     * @param[in] outputs Input parameter.
     * @param[in] expected Input parameter.
     * @return Return value.
     */
    AggregatedMetrics evaluateBatch(
        const std::vector<std::string>& outputs,
        const std::vector<std::string>& expected
    ) const;
    
    /**
     * @brief Compute Semantic Similarity.
     * @param[in] s1 Input parameter.
     * @param[in] s2 Input parameter.
     * @return Return value.
     */
    static double computeSemanticSimilarity(
        const std::string& s1,
        const std::string& s2
    );
    
    /**
     * @brief Compute Exact Match.
     * @param[in] output Input parameter.
     * @param[in] expected Input parameter.
     * @return Return value.
     */
    static double computeExactMatch(
        const std::string& output,
        const std::string& expected
    );
    
    /**
     * @brief Compute Partial Match.
     * @param[in] output Input parameter.
     * @param[in] expected Input parameter.
     * @return Return value.
     */
    static double computePartialMatch(
        const std::string& output,
        const std::string& expected
    );
    
    /**
     * @brief Compute Relevance.
     * @param[in] output Input parameter.
     * @param[in] expected Input parameter.
     * @return Return value.
     */
    static double computeRelevance(
        const std::string& output,
        const std::string& expected
    );
    
    static bool isStatisticallySignificant(
        const std::vector<double>& baseline_scores,
        const std::vector<double>& new_scores,
        double confidence_level = 0.95
    );
    
    const EvaluatorConfig& getConfig() const { return config_; }
    
    /**
     * @brief Set Config.
     * @param[in] config Input parameter.
     * @details Implements setConfig without additional internal calls.
     */
    void setConfig(const EvaluatorConfig& config) { config_ = config; }

private:
    EvaluatorConfig config_;
    
    /**
     * @brief Compute Weighted Score.
     * @param[in] metrics Input parameter.
     * @return Return value.
     */
    double computeWeightedScore(const EvaluationMetrics& metrics) const;
    
    /**
     * @brief Normalize String.
     * @param[in] s Input parameter.
     * @return Return value.
     */
    static std::string normalizeString(const std::string& s);
    
    /**
     * @brief Tokenize.
     * @param[in] s Input parameter.
     * @return Return value.
     */
    static std::vector<std::string> tokenize(const std::string& s);
    
    /**
     * @brief Levenshtein Distance.
     * @param[in] s1 Input parameter.
     * @param[in] s2 Input parameter.
     * @return Return value.
     */
    static size_t levenshteinDistance(const std::string& s1, const std::string& s2);
};

} // namespace llm
} // namespace themis
