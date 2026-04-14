/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            prompt_evaluator.h                                 ║
  Version:         0.0.40                                             ║
  Last Modified:   2026-04-14 06:53:08                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     218                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file prompt_evaluator.h
 * @brief Metrics-based prompt evaluation
 * 
 * Provides evaluation metrics for prompt quality including:
 * - Semantic similarity
 * - Output quality metrics
 * - Statistical validation
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <nlohmann/json.hpp>

namespace themis {
namespace llm {

/**
 * @brief Evaluation metrics for a single test case
 */
struct EvaluationMetrics {
    double semantic_similarity = 0.0;  ///< Semantic similarity score (0.0-1.0)
    double exact_match = 0.0;          ///< Exact match score (0.0 or 1.0)
    double partial_match = 0.0;        ///< Partial match score (0.0-1.0)
    double relevance = 0.0;            ///< Relevance score (0.0-1.0)
    nlohmann::json details;            ///< Additional metric details
};

/**
 * @brief Aggregated evaluation results
 */
struct AggregatedMetrics {
    double overall_score = 0.0;        ///< Overall weighted score
    double mean_similarity = 0.0;      ///< Mean semantic similarity
    double std_similarity = 0.0;       ///< Standard deviation of similarity
    size_t num_exact_matches = 0;      ///< Number of exact matches
    double pass_rate = 0.0;            ///< Percentage passing threshold
    nlohmann::json per_case_metrics;   ///< Per-test-case breakdown
};

/**
 * @brief Configuration for evaluation
 */
struct EvaluatorConfig {
    double similarity_weight = 0.5;    ///< Weight for semantic similarity
    double exact_match_weight = 0.3;   ///< Weight for exact matches
    double relevance_weight = 0.2;     ///< Weight for relevance
    double pass_threshold = 0.7;       ///< Minimum score to pass
    bool enable_statistical_tests = true; ///< Enable significance testing
};

/**
 * @brief Prompt evaluator class
 * 
 * Evaluates prompt quality using multiple metrics:
 * - Semantic similarity (cosine similarity of embeddings)
 * - Exact and partial string matching
 * - Task-specific relevance scoring
 * - Statistical significance testing
 */
class PromptEvaluator {
public:
    /**
     * @brief Constructor
     * @param config Evaluator configuration
     */
    explicit PromptEvaluator(const EvaluatorConfig& config = EvaluatorConfig{});
    
    /**
     * @brief Evaluate a single output against expected output
     * @param output Actual output
     * @param expected Expected output
     * @return Evaluation metrics
     */
    EvaluationMetrics evaluateSingle(
        const std::string& output,
        const std::string& expected
    ) const;
    
    /**
     * @brief Evaluate multiple outputs
     * @param outputs Actual outputs
     * @param expected Expected outputs
     * @return Aggregated metrics
     */
    AggregatedMetrics evaluateBatch(
        const std::vector<std::string>& outputs,
        const std::vector<std::string>& expected
    ) const;
    
    /**
     * @brief Compute semantic similarity between two strings
     * 
     * Uses Jaccard similarity (word overlap) as a baseline implementation.
     * This is production-ready and efficient for most use cases.
     * 
     * For enhanced accuracy, this can be extended with:
     * - Embedding-based similarity (cosine similarity of sentence embeddings)
     * - BERT/transformer-based semantic models
     * - Domain-specific similarity metrics
     * 
     * @param s1 First string
     * @param s2 Second string
     * @return Similarity score (0.0-1.0)
     */
    static double computeSemanticSimilarity(
        const std::string& s1,
        const std::string& s2
    );
    
    /**
     * @brief Compute exact match score
     * @param output Actual output
     * @param expected Expected output
     * @return 1.0 if exact match, 0.0 otherwise
     */
    static double computeExactMatch(
        const std::string& output,
        const std::string& expected
    );
    
    /**
     * @brief Compute partial match score (normalized edit distance)
     * @param output Actual output
     * @param expected Expected output
     * @return Partial match score (0.0-1.0)
     */
    static double computePartialMatch(
        const std::string& output,
        const std::string& expected
    );
    
    /**
     * @brief Compute relevance score
     * Checks if key terms from expected output appear in actual output
     * @param output Actual output
     * @param expected Expected output
     * @return Relevance score (0.0-1.0)
     */
    static double computeRelevance(
        const std::string& output,
        const std::string& expected
    );
    
    /**
     * @brief Check if results are statistically significant
     * @param baseline_scores Baseline scores
     * @param new_scores New scores
     * @param confidence_level Confidence level (default 0.95)
     * @return true if improvement is statistically significant
     */
    static bool isStatisticallySignificant(
        const std::vector<double>& baseline_scores,
        const std::vector<double>& new_scores,
        double confidence_level = 0.95
    );
    
    /**
     * @brief Get current configuration
     */
    const EvaluatorConfig& getConfig() const { return config_; }
    
    /**
     * @brief Update configuration
     */
    void setConfig(const EvaluatorConfig& config) { config_ = config; }

private:
    EvaluatorConfig config_;
    
    /**
     * @brief Compute weighted score from individual metrics
     */
    double computeWeightedScore(const EvaluationMetrics& metrics) const;
    
    /**
     * @brief Normalize string for comparison
     */
    static std::string normalizeString(const std::string& s);
    
    /**
     * @brief Tokenize string into words
     */
    static std::vector<std::string> tokenize(const std::string& s);
    
    /**
     * @brief Compute Levenshtein distance
     */
    static size_t levenshteinDistance(const std::string& s1, const std::string& s2);
};

} // namespace llm
} // namespace themis
