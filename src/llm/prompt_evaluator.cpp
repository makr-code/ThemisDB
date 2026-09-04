/**
 * @file prompt_evaluator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=5, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "llm/prompt_evaluator.h"
#include "utils/logger.h"
#include <algorithm>
#include <sstream>
#include <cmath>
#include <cctype>
#include <unordered_set>

namespace themis {
namespace llm {

PromptEvaluator::PromptEvaluator(const EvaluatorConfig& config)
    : config_(config) {
    THEMIS_DEBUG("Initialized PromptEvaluator with pass_threshold={}",
                 config_.pass_threshold);
}

EvaluationMetrics PromptEvaluator::evaluateSingle(
    const std::string& output,
    const std::string& expected
) const {
    EvaluationMetrics metrics;
    
    metrics.semantic_similarity = computeSemanticSimilarity(output, expected);
    metrics.exact_match = computeExactMatch(output, expected);
    metrics.partial_match = computePartialMatch(output, expected);
    metrics.relevance = computeRelevance(output, expected);
    
    // Store individual scores in details
    metrics.details["semantic_similarity"] = metrics.semantic_similarity;
    metrics.details["exact_match"] = metrics.exact_match;
    metrics.details["partial_match"] = metrics.partial_match;
    metrics.details["relevance"] = metrics.relevance;
    
    return metrics;
}

AggregatedMetrics PromptEvaluator::evaluateBatch(
    const std::vector<std::string>& outputs,
    const std::vector<std::string>& expected
) const {
    AggregatedMetrics agg;
    
    if (outputs.size() != expected.size()) {
        THEMIS_ERROR("Output and expected vectors must have same size");
        return agg;
    }
    
    if (outputs.empty()) {
        return agg;
    }
    
    std::vector<double> similarities;
    std::vector<double> weighted_scores;
    nlohmann::json per_case = nlohmann::json::array();
    
    for (size_t i = 0; i < outputs.size(); ++i) {
        auto metrics = evaluateSingle(outputs[i], expected[i]);
        double weighted = computeWeightedScore(metrics);
        
        similarities.push_back(metrics.semantic_similarity);
        weighted_scores.push_back(weighted);
        
        if (metrics.exact_match > 0.99) {
            agg.num_exact_matches++;
        }
        
        nlohmann::json case_metrics;
        case_metrics["index"] = i;
        case_metrics["weighted_score"] = weighted;
        case_metrics["metrics"] = metrics.details;
        per_case.push_back(case_metrics);
    }
    
    agg.per_case_metrics = per_case;
    
    // Compute mean
    double sum = 0.0;
    for (double s : similarities) {
        sum += s;
    }
    agg.mean_similarity = sum / similarities.size();
    
    // Compute standard deviation
    double sq_diff_sum = 0.0;
    for (double s : similarities) {
        double diff = s - agg.mean_similarity;
        sq_diff_sum += diff * diff;
    }
    agg.std_similarity = std::sqrt(sq_diff_sum / similarities.size());
    
    // Compute overall score
    double score_sum = 0.0;
    for (double s : weighted_scores) {
        score_sum += s;
    }
    agg.overall_score = score_sum / weighted_scores.size();
    
    // Compute pass rate
    size_t passed = 0;
    for (double s : weighted_scores) {
        if (s >= config_.pass_threshold) {
            passed++;
        }
    }
    agg.pass_rate = static_cast<double>(passed) / weighted_scores.size();
    
    THEMIS_DEBUG("Batch evaluation: overall_score={:.4f}, mean_similarity={:.4f}, pass_rate={:.2f}%",
                 agg.overall_score, agg.mean_similarity, agg.pass_rate * 100.0);
    
    return agg;
}

double PromptEvaluator::computeSemanticSimilarity(
    const std::string& s1,
    const std::string& s2
) {
    // Normalize strings
    auto norm1 = normalizeString(s1);
    auto norm2 = normalizeString(s2);
    
    // Tokenize
    auto tokens1 = tokenize(norm1);
    auto tokens2 = tokenize(norm2);
    
    if (tokens1.empty() || tokens2.empty()) {
        return 0.0;
    }
    
    // Compute word overlap (Jaccard similarity)
    std::unordered_set<std::string> set1(tokens1.begin(), tokens1.end());
    std::unordered_set<std::string> set2(tokens2.begin(), tokens2.end());
    
    size_t intersection = 0;
    for (const auto& token : set1) {
        if (set2.count(token) > 0) {
            intersection++;
        }
    }
    
    size_t union_size = set1.size() + set2.size() - intersection;
    
    if (union_size == 0) {
        return 1.0;
    }
    
    return static_cast<double>(intersection) / union_size;
}

double PromptEvaluator::computeExactMatch(
    const std::string& output,
    const std::string& expected
) {
    auto norm_output = normalizeString(output);
    auto norm_expected = normalizeString(expected);
    
    return (norm_output == norm_expected) ? 1.0 : 0.0;
}

double PromptEvaluator::computePartialMatch(
    const std::string& output,
    const std::string& expected
) {
    auto norm_output = normalizeString(output);
    auto norm_expected = normalizeString(expected);
    
    if (norm_output.empty() && norm_expected.empty()) {
        return 1.0;
    }
    
    if (norm_output.empty() || norm_expected.empty()) {
        return 0.0;
    }
    
    // Compute normalized Levenshtein distance
    size_t distance = levenshteinDistance(norm_output, norm_expected);
    size_t max_len = std::max(norm_output.length(), norm_expected.length());
    
    return 1.0 - (static_cast<double>(distance) / max_len);
}

double PromptEvaluator::computeRelevance(
    const std::string& output,
    const std::string& expected
) {
    auto tokens_output = tokenize(normalizeString(output));
    auto tokens_expected = tokenize(normalizeString(expected));
    
    if (tokens_expected.empty()) {
        return 1.0;
    }
    
    // Count how many expected tokens appear in output
    std::unordered_set<std::string> output_set(tokens_output.begin(), tokens_output.end());
    
    size_t found = 0;
    for (const auto& token : tokens_expected) {
        if (output_set.count(token) > 0) {
            found++;
        }
    }
    
    return static_cast<double>(found) / tokens_expected.size();
}

bool PromptEvaluator::isStatisticallySignificant(
    const std::vector<double>& baseline_scores,
    const std::vector<double>& new_scores,
    double /*confidence_level*/
) {
    if (baseline_scores.empty() || new_scores.empty()) {
        return false;
    }
    
    // Compute means
    double baseline_mean = 0.0;
    for (double s : baseline_scores) {
        baseline_mean += s;
    }
    baseline_mean /= baseline_scores.size();
    
    double new_mean = 0.0;
    for (double s : new_scores) {
        new_mean += s;
    }
    new_mean /= new_scores.size();
    
    // Simple check: new mean must be significantly higher
    // For a full implementation, use proper t-test
    double improvement = (new_mean - baseline_mean) / baseline_mean;
    
    // Require at least 5% improvement for statistical significance
    // (simplified approximation)
    return improvement > 0.05;
}

double PromptEvaluator::computeWeightedScore(const EvaluationMetrics& metrics) const {
    return config_.similarity_weight * metrics.semantic_similarity +
           config_.exact_match_weight * metrics.exact_match +
           config_.relevance_weight * metrics.relevance;
}

std::string PromptEvaluator::normalizeString(const std::string& s) {
    std::string result;
    result.reserve(s.length());
    
    for (char c : s) {
        const bool is_space = (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v');
        if (is_space) {
            if (!result.empty() && result.back() != ' ') {
                result.push_back(' ');
            }
        } else {
            if (c >= 'A' && c <= 'Z') {
                result.push_back(static_cast<char>(c - 'A' + 'a'));
            } else {
                result.push_back(c);
            }
        }
    }
    
    // Trim trailing space
    if (!result.empty() && result.back() == ' ') {
        result.pop_back();
    }
    
    return result;
}

std::vector<std::string> PromptEvaluator::tokenize(const std::string& s) {
    std::vector<std::string> tokens;
    std::istringstream iss(s);
    std::string token;
    
    while (iss >> token) {
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }
    
    return tokens;
}

size_t PromptEvaluator::levenshteinDistance(
    const std::string& s1,
    const std::string& s2
) {
    const size_t m = s1.length();
    const size_t n = s2.length();
    
    if (m == 0) {
      return n;
    }
    if (n == 0) {
      return m;
    }
    
    std::vector<std::vector<size_t>> dp(m + 1, std::vector<size_t>(n + 1));
    
    for (size_t i = 0; i <= m; ++i) {
        dp[i][0] = i;
    }
    
    for (size_t j = 0; j <= n; ++j) {
        dp[0][j] = j;
    }
    
    for (size_t i = 1; i <= m; ++i) {
        for (size_t j = 1; j <= n; ++j) {
            if (s1[i - 1] == s2[j - 1]) {
                dp[i][j] = dp[i - 1][j - 1];
            } else {
                dp[i][j] = 1 + std::min({
                    dp[i - 1][j],     // deletion
                    dp[i][j - 1],     // insertion
                    dp[i - 1][j - 1]  // substitution
                });
            }
        }
    }
    
    return dp[m][n];
}

} // namespace llm
} // namespace themis
