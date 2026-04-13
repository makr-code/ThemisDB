/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            pairwise_comparator.h                              ║
  Version:         0.0.38                                             ║
  Last Modified:   2026-04-13 04:19:03                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     176                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file pairwise_comparator.h
 * @brief Enhanced pairwise comparison for RAG answers with bias mitigation
 * 
 * Implements advanced pairwise comparison with position bias mitigation,
 * tie handling, and per-dimension analysis.
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace themis::rag::judge {

/**
 * @brief Comparison winner
 */
enum class ComparisonWinner {
    ANSWER_A,
    ANSWER_B,
    TIE
};

/**
 * @brief Bias mitigation strategy
 */
enum class BiasMitigationStrategy {
    NONE,              ///< No bias mitigation
    RANDOMIZE_ORDER,   ///< Random presentation order
    FLIP_AND_AVERAGE,  ///< Evaluate both orders and average
    MULTI_SAMPLE       ///< Multiple evaluations with different orders
};

/**
 * @brief Per-dimension comparison result
 */
struct DimensionComparison {
    std::string dimension_name;
    ComparisonWinner winner;
    double score_a;
    double score_b;
    double confidence;
    std::string reasoning;
};

/**
 * @brief Complete pairwise comparison result
 */
struct PairwiseComparisonResult {
    ComparisonWinner overall_winner;
    double overall_confidence;
    std::string overall_reasoning;
    
    // Per-dimension results
    std::vector<DimensionComparison> dimension_comparisons;
    
    // Scores
    double answer_a_overall_score;
    double answer_b_overall_score;
    
    // Bias detection
    bool position_bias_detected;
    double position_bias_magnitude;  ///< 0-1, higher = more bias
    
    // Metadata
    int num_evaluations;  ///< Number of evaluations performed
    bool flip_tested;     ///< Whether flip test was performed
};

/**
 * @brief Pairwise comparator with bias mitigation
 * 
 * Performs enhanced pairwise comparison of two answers with:
 * - Position bias mitigation
 * - Tie handling with confidence thresholds
 * - Per-dimension comparison
 * - Multiple evaluation strategies
 */
class PairwiseComparator {
public:
    /**
     * @brief Configuration for pairwise comparison
     */
    struct Config {
        BiasMitigationStrategy bias_strategy = BiasMitigationStrategy::FLIP_AND_AVERAGE;
        double tie_threshold = 0.05;        ///< Score difference for tie
        double confidence_threshold = 0.7;  ///< Minimum confidence for decision
        int num_samples = 3;                ///< For MULTI_SAMPLE strategy
        bool enable_per_dimension = true;   ///< Compare per dimension
    };

    /**
     * @brief Construct comparator with configuration
     */
    PairwiseComparator();
    explicit PairwiseComparator(const Config& config);
    
    /**
     * @brief Destructor
     */
    ~PairwiseComparator();
    
    /**
     * @brief Compare two answers pairwise
     * @param query Original query
     * @param documents Retrieved documents
     * @param answer_a First answer
     * @param answer_b Second answer
     * @return Pairwise comparison result
     */
    PairwiseComparisonResult compare(
        const std::string& query,
        const std::vector<std::pair<std::string, std::string>>& documents,
        const std::string& answer_a,
        const std::string& answer_b
    );
    
    /**
     * @brief Compare answers with LLM-based direct comparison
     * @param query Original query
     * @param documents Retrieved documents
     * @param answer_a First answer
     * @param answer_b Second answer
     * @param order_a_first If true, present A first; else B first
     * @return Comparison result
     */
    ComparisonWinner compareWithLLM(
        const std::string& query,
        const std::vector<std::pair<std::string, std::string>>& documents,
        const std::string& answer_a,
        const std::string& answer_b,
        bool order_a_first = true
    );
    
    /**
     * @brief Detect position bias in comparison
     * @param forward_result Result with A presented first
     * @param reverse_result Result with B presented first
     * @return Bias magnitude 0-1
     */
    double detectPositionBias(
        ComparisonWinner forward_result,
        ComparisonWinner reverse_result
    );

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace themis::rag::judge
