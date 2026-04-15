/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            bias_detector.h                                    ║
  Version:         0.0.46                                             ║
  Last Modified:   2026-04-15 18:04:46                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     167                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file bias_detector.h
 * @brief Bias detection and mitigation for RAG Judge evaluations
 * 
 * Phase 5: Implements position bias, length bias, and self-enhancement bias detection
 */

#pragma once

#include "rag/rag_judge.h"
#include <vector>
#include <string>
#include <unordered_map>

namespace themis::rag::judge {

/**
 * @brief Type of bias detected in evaluation
 */
enum class BiasType {
    POSITION_BIAS,       ///< Preference for first or last position
    LENGTH_BIAS,         ///< Preference based on answer length
    SELF_ENHANCEMENT,    ///< Preference for own generated content
    NONE                 ///< No significant bias detected
};

/**
 * @brief Bias detection result
 */
struct BiasDetectionResult {
    BiasType type;
    double bias_magnitude;      ///< Magnitude of bias (0-1, higher = more bias)
    double p_value;              ///< Statistical significance
    std::string description;     ///< Human-readable description
    bool is_significant;         ///< Whether bias is statistically significant
    
    std::unordered_map<std::string, double> metrics; ///< Additional metrics
};

/**
 * @brief Configuration for bias detection
 */
struct BiasDetectorConfig {
    int min_samples_for_detection = 10;      ///< Minimum samples needed
    double significance_threshold = 0.05;     ///< P-value threshold
    double bias_threshold = 0.1;              ///< Minimum bias magnitude to report
    
    bool enable_position_bias_check = true;
    bool enable_length_bias_check = true;
    bool enable_self_enhancement_check = false; ///< Requires answer provenance
};

/**
 * @brief Bias detector for judge evaluations
 * 
 * Detects and quantifies various types of bias in judge evaluations
 * including position bias, length bias, and self-enhancement bias.
 */
class BiasDetector {
public:
    /**
     * @brief Construct bias detector with configuration
     * @param config Detector configuration
     */
    BiasDetector();
    explicit BiasDetector(const BiasDetectorConfig& config);
    
    /**
     * @brief Detect position bias in pairwise comparisons
     * 
     * Tests whether judge shows preference for first or second position
     * by comparing A>B vs B>A results.
     * 
     * @param comparisons Vector of comparison results with positions
     * @return Bias detection result
     */
    BiasDetectionResult detectPositionBias(
        const std::vector<std::pair<ComparisonResult, bool>>& comparisons
    );
    
    /**
     * @brief Detect length bias in evaluations
     * 
     * Tests correlation between answer length and evaluation scores.
     * 
     * @param evaluations Vector of (score, answer_length) pairs
     * @return Bias detection result
     */
    BiasDetectionResult detectLengthBias(
        const std::vector<std::pair<double, size_t>>& evaluations
    );
    
    /**
     * @brief Run comprehensive bias analysis
     * @param evaluation_history Historical evaluation data
     * @return Vector of detected biases
     */
    std::vector<BiasDetectionResult> analyzeAllBiases(
        const std::vector<EvaluationResult>& evaluation_history
    );
    
    /**
     * @brief Apply bias mitigation to comparison result
     * 
     * Adjusts comparison result based on detected biases.
     * 
     * @param result Original comparison result
     * @param detected_bias Detected bias information
     * @return Bias-adjusted result
     */
    ComparisonResult applyBiasMitigation(
        const ComparisonResult& result,
        const BiasDetectionResult& detected_bias
    );
    
    /**
     * @brief Update configuration
     * @param config New configuration
     */
    void setConfig(const BiasDetectorConfig& config);
    
    /**
     * @brief Get current configuration
     * @return Current configuration
     */
    BiasDetectorConfig getConfig() const;

private:
    BiasDetectorConfig config_;
    
    // Statistical helper functions
    double calculateCorrelation(
        const std::vector<double>& x,
        const std::vector<double>& y
    );
    
    double calculatePValue(double correlation, size_t sample_size);
    
    double calculateChiSquare(
        const std::vector<int>& observed,
        const std::vector<int>& expected
    );
};

} // namespace themis::rag::judge
