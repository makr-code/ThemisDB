/**
 * @file bias_detector.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "rag/bias_detector.h"
#include "utils/logger.h"
#include <cmath>
#include <algorithm>
#include <numeric>

namespace themis::rag::judge {

BiasDetector::BiasDetector()
    : BiasDetector(BiasDetectorConfig{}) {
}

BiasDetector::BiasDetector(const BiasDetectorConfig& config)
    : config_(config) {
    THEMIS_INFO("BiasDetector initialized");
}

BiasDetectionResult BiasDetector::detectPositionBias(
    const std::vector<std::pair<ComparisonResult, bool>>& comparisons
) {
    THEMIS_DEBUG("Detecting position bias with {} comparisons",static_cast<int>(comparisons.size()));
    
    BiasDetectionResult result;
    result.type = BiasType::POSITION_BIAS;
    result.is_significant = false;
    result.bias_magnitude = 0.0;
    result.p_value = 1.0;
    
    if (static_cast<int>(comparisons.size()) < config_.min_samples_for_detection) {
        result.description = "Insufficient samples for position bias detection";
        THEMIS_WARN("Not enough samples for position bias detection: {} < {}", 
                   comparisons.size(), config_.min_samples_for_detection);
        return result;
    }
    
    // Count wins for each position
    int first_position_wins = 0;
    int second_position_wins = 0;
    int ties = 0;
    
    for (const auto& [comp_result, was_flipped] : comparisons) {
        bool actual_first_won = (comp_result.winner == ComparisonResult::Winner::ANSWER_A);
        if (was_flipped) {
            actual_first_won = !actual_first_won;
        }
        
        if (comp_result.winner == ComparisonResult::Winner::TIE) {
            ties++;
        } else if (actual_first_won) {
            first_position_wins++;
        } else {
            second_position_wins++;
        }
    }
    
    // Chi-square test for position bias
    int total_decisions = first_position_wins + second_position_wins;
    if (total_decisions == 0) {
        result.description = "All comparisons resulted in ties";
        return result;
    }
    
    double expected = total_decisions / 2.0;
    std::vector<int> observed = {first_position_wins, second_position_wins};
    std::vector<int> expected_counts = {static_cast<int>(expected), static_cast<int>(expected)};
    
    double chi_square = calculateChiSquare(observed, expected_counts);
    result.p_value = calculatePValue(chi_square, 1); // 1 degree of freedom
    
    // Calculate bias magnitude (deviation from 50/50 split)
    double first_position_rate = static_cast<double>(first_position_wins) / total_decisions;
    result.bias_magnitude = std::abs(first_position_rate - 0.5);
    
    result.is_significant = (result.p_value < config_.significance_threshold) && 
                            (result.bias_magnitude > config_.bias_threshold);
    
    result.metrics["first_position_wins"] = first_position_wins;
    result.metrics["second_position_wins"] = second_position_wins;
    result.metrics["ties"] = ties;
    result.metrics["first_position_rate"] = first_position_rate;
    
    if (result.is_significant) {
        std::string preferred_position = (first_position_rate > 0.5) ? "first" : "second";
        result.description = "Significant position bias detected: preference for " + 
                            preferred_position + " position (" + 
                            std::to_string(static_cast<int>(result.bias_magnitude * 100)) + "%)";
        THEMIS_WARN("Position bias detected: {}", result.description);
    } else {
        result.description = "No significant position bias detected";
        THEMIS_DEBUG("No significant position bias");
    }
    
    return result;
}

BiasDetectionResult BiasDetector::detectLengthBias(
    const std::vector<std::pair<double, size_t>>& evaluations
) {
    THEMIS_DEBUG("Detecting length bias with {} evaluations",static_cast<int>(evaluations.size()));
    
    BiasDetectionResult result;
    result.type = BiasType::LENGTH_BIAS;
    result.is_significant = false;
    result.bias_magnitude = 0.0;
    result.p_value = 1.0;
    
    if (static_cast<int>(evaluations.size()) < config_.min_samples_for_detection) {
        result.description = "Insufficient samples for length bias detection";
        THEMIS_WARN("Not enough samples for length bias detection");
        return result;
    }
    
    // Extract scores and lengths
    std::vector<double> scores;
    std::vector<double> lengths;
    
    for (const auto& [score, length] : evaluations) {
        scores.push_back(score);
        lengths.push_back(static_cast<double>(length));
    }
    
    // Calculate Pearson correlation
    double correlation = calculateCorrelation(scores, lengths);
    result.bias_magnitude = std::abs(correlation);
    result.p_value = calculatePValue(correlation,static_cast<int>(evaluations.size()));
    
    result.is_significant = (result.p_value < config_.significance_threshold) && 
                            (result.bias_magnitude > config_.bias_threshold);
    
    result.metrics["correlation"] = correlation;
    result.metrics["mean_score"] = std::accumulate(scores.begin(), scores.end(), 0.0) / scores.size();
    result.metrics["mean_length"] = std::accumulate(lengths.begin(), lengths.end(), 0.0) / lengths.size();
    
    if (result.is_significant) {
        std::string direction = (correlation > 0) ? "positive" : "negative";
        result.description = "Significant length bias detected: " + direction + 
                            " correlation (" + std::to_string(correlation) + ")";
        THEMIS_WARN("Length bias detected: {}", result.description);
    } else {
        result.description = "No significant length bias detected";
        THEMIS_DEBUG("No significant length bias");
    }
    
    return result;
}

std::vector<BiasDetectionResult> BiasDetector::analyzeAllBiases(
    const std::vector<EvaluationResult>& evaluation_history
) {
    THEMIS_INFO("Running comprehensive bias analysis on {} evaluations",static_cast<int>(evaluation_history.size()));
    
    std::vector<BiasDetectionResult> results;
    
    // For length bias, we need to extract score-length pairs
    // This is a simplified version - in production, you'd need actual answer lengths
    std::vector<std::pair<double, size_t>> score_length_pairs;
    for (const auto& eval : evaluation_history) {
        // Estimate length from explanation length as proxy
        size_t estimated_length = eval.explanation.length();
        score_length_pairs.emplace_back(eval.overall_score, estimated_length);
    }
    
    if (config_.enable_length_bias_check && !score_length_pairs.empty()) {
        auto length_bias = detectLengthBias(score_length_pairs);
        if (length_bias.is_significant) {
            results.push_back(length_bias);
        }
    }
    
    // Position bias would require pairwise comparison history
    // Not available from evaluation history alone
    
    THEMIS_INFO("Bias analysis complete: {} significant biases detected",static_cast<int>(results.size()));
    return results;
}

ComparisonResult BiasDetector::applyBiasMitigation(
    const ComparisonResult& result,
    const BiasDetectionResult& detected_bias
) {
    ComparisonResult mitigated = result;
    
    if (!detected_bias.is_significant) {
        return mitigated; // No mitigation needed
    }
    
    if (detected_bias.type == BiasType::POSITION_BIAS) {
        // Reduce confidence when position bias is detected
        mitigated.confidence *= (1.0 - detected_bias.bias_magnitude);
        
        THEMIS_DEBUG("Applied position bias mitigation: confidence {} -> {}", 
                    result.confidence, mitigated.confidence);
    }
    
    return mitigated;
}

void BiasDetector::setConfig(const BiasDetectorConfig& config) {
    config_ = config;
    THEMIS_INFO("BiasDetector configuration updated");
}

BiasDetectorConfig BiasDetector::getConfig() const {
    return config_;
}

// Statistical helper functions

double BiasDetector::calculateCorrelation(
    const std::vector<double>& x,
    const std::vector<double>& y
) {
    if (static_cast<int>(x.size()) != static_cast<int>(y.size()) || x.empty()) {
        return 0.0;
    }
    
    size_t n = x.size();
    double sum_x = std::accumulate(x.begin(), x.end(), 0.0);
    double sum_y = std::accumulate(y.begin(), y.end(), 0.0);
    double mean_x = sum_x / n;
    double mean_y = sum_y / n;
    
    double numerator = 0.0;
    double sum_sq_x = 0.0;
    double sum_sq_y = 0.0;
    
    for (size_t i = 0; i < n; ++i) {
        double dx = x[i] - mean_x;
        double dy = y[i] - mean_y;
        numerator += dx * dy;
        sum_sq_x += dx * dx;
        sum_sq_y += dy * dy;
    }
    
    double denominator = std::sqrt(sum_sq_x * sum_sq_y);
    
    if (denominator < 1e-10) {
        return 0.0;
    }
    
    return numerator / denominator;
}

double BiasDetector::calculatePValue(double correlation, size_t sample_size) {
    if (sample_size < 3) {
        return 1.0;
    }
    
    // Approximate p-value using t-distribution
    // t = r * sqrt((n-2) / (1-r^2))
    double r_sq = correlation * correlation;
    if (r_sq >= 1.0) {
        return 0.0;
    }
    
    double t = correlation * std::sqrt((sample_size - 2) / (1.0 - r_sq));
    
    // Very rough approximation of p-value
    // For production, use a proper statistical library
    double p_value = 2.0 * (1.0 - std::erf(std::abs(t) / std::sqrt(2.0)));
    
    return std::max(0.0, std::min(1.0, p_value));
}

double BiasDetector::calculateChiSquare(
    const std::vector<int>& observed,
    const std::vector<int>& expected
) {
    if (static_cast<int>(observed.size()) != static_cast<int>(expected.size())) {
        return 0.0;
    }
    
    double chi_square = 0.0;
    for (size_t i = 0; i < observed.size(); ++i) {
        if (expected[i] > 0) {
            double diff = observed[i] - expected[i];
            chi_square += (diff * diff) / expected[i];
        }
    }
    
    return chi_square;
}

} // namespace themis::rag::judge

