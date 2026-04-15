/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            geval_evaluator.h                                  ║
  Version:         0.0.45                                             ║
  Last Modified:   2026-04-15 07:08:46                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     159                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file geval_evaluator.h
 * @brief G-Eval probabilistic scoring using token probabilities
 * 
 * Implements G-Eval style evaluation where the LLM's token probabilities
 * are used to compute continuous quality scores. Based on the G-Eval paper
 * (Liu et al., 2023).
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace themis::rag::judge {

/**
 * @brief Aggregation method for multiple samples
 */
enum class AggregationMethod {
    MEAN,      ///< Average of samples
    MEDIAN,    ///< Median of samples
    MODE       ///< Most frequent score
};

/// Number of discrete score levels used in G-Eval (1 through kNumScoreLevels)
static constexpr std::size_t kNumScoreLevels = 5;

/**
 * @brief G-Eval evaluation result
 */
struct GEvalResult {
    double geval_score;                  ///< Continuous score (0-1)
    std::vector<double> token_probabilities;  ///< Probabilities for levels 1-5
    double confidence;                   ///< Confidence in score (0-1)
    std::vector<double> sample_scores;   ///< Individual sample scores
    double variance;                     ///< Score variance across samples
    std::string dimension;               ///< Evaluated dimension
    std::string reasoning;               ///< Optional reasoning text
};

/**
 * @brief G-Eval evaluator using token probabilities
 * 
 * Implements probabilistic scoring where the LLM generates score tokens
 * (1-5) and we extract their probabilities from logits to compute a
 * continuous score: score = Σ(level × P(level))
 * 
 * This provides finer-grained evaluation than discrete rubric levels.
 */
class GEvalEvaluator {
public:
    /**
     * @brief Configuration for G-Eval
     */
    struct Config {
        int num_samples = 3;                ///< Number of evaluations for robustness
        AggregationMethod aggregation = AggregationMethod::MEAN;
        double temperature = 0.7;           ///< Sampling temperature
        bool extract_reasoning = true;      ///< Extract reasoning before score
        double confidence_threshold = 0.6;  ///< Minimum confidence for valid score
    };

    /**
     * @brief Construct evaluator with configuration
     */
    GEvalEvaluator();
    explicit GEvalEvaluator(const Config& config);
    
    /**
     * @brief Destructor
     */
    ~GEvalEvaluator();
    
    /**
     * @brief Evaluate answer using G-Eval probabilistic scoring
     * @param query Original query
     * @param answer Generated answer
     * @param documents Retrieved documents
     * @param dimension Dimension to evaluate (faithfulness, relevance, etc.)
     * @return G-Eval result with continuous score
     */
    GEvalResult evaluate(
        const std::string& query,
        const std::string& answer,
        const std::vector<std::pair<std::string, std::string>>& documents,
        const std::string& dimension = "overall"
    );
    
    /**
     * @brief Extract token probabilities from LLM response
     * 
     * This method interfaces with llama.cpp to get logits and compute
     * probabilities for score tokens (1-5).
     * 
     * @param prompt Evaluation prompt
     * @param score_tokens Token IDs for "1", "2", "3", "4", "5"
     * @return Probability distribution over score levels
     */
    std::vector<double> extractTokenProbabilities(
        const std::string& prompt,
        const std::vector<int>& score_tokens
    );
    
    /**
     * @brief Compute continuous G-Eval score from probabilities
     * @param probabilities P(level) for levels 1-5
     * @return Continuous score (0-1 normalized)
     */
    static double computeGEvalScore(const std::vector<double>& probabilities);
    
    /**
     * @brief Compute confidence from probability distribution
     * @param probabilities Token probabilities
     * @return Confidence score (0-1)
     */
    static double computeConfidence(const std::vector<double>& probabilities);
    
    /**
     * @brief Aggregate multiple sample scores
     * @param samples Individual scores from multiple evaluations
     * @param method Aggregation method
     * @return Aggregated score
     */
    static double aggregateScores(
        const std::vector<double>& samples,
        AggregationMethod method
    );

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace themis::rag::judge
