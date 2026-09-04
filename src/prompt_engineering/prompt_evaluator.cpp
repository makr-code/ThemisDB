/**
 * @file prompt_evaluator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=5, M=2, L=1
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "prompt_engineering/prompt_evaluator.h"
#include "utils/logger.h"
#include <algorithm>
#include <numeric>
#include <sstream>
#include <cmath>
#include <cctype>
#include <unordered_set>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace themis {
namespace prompt_engineering {

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
    const auto embedding_provider = getEmbeddingProviderSnapshot();

    // Snapshot the provider pointer once so provider swaps do not race with
    // evaluation or per-call metrics collection.
    if (embedding_provider) {
        double emb_sim = computeEmbeddingSimilarity(output, expected, embedding_provider);
        metrics.semantic_similarity = (emb_sim >= 0.0) ? emb_sim
                                                       : computeSemanticSimilarity(output, expected);
        metrics.details["embedding_provider"] = embedding_provider->name();
    } else {
        metrics.semantic_similarity = computeSemanticSimilarity(output, expected);
    }

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
    AggregatedMetrics agg = {};
    
    if (static_cast<int>(outputs.size()) != static_cast<int>(expected.size())) {
        THEMIS_ERROR("Output and expected vectors must have same size");
        return agg;
    }
    
    if (outputs.empty()) {
        return agg;
    }
    
    std::vector<double> similarities;
    std::vector<double> weighted_scores;
    nlohmann::json per_case = nlohmann::json::array();
    
    for (size_t i = 0; i <static_cast<int>(outputs.size()); ++i) {
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
    
    size_t union_size = static_cast<int>(set1.size()) + static_cast<int>(set2.size()) - intersection;
    
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
    
    return static_cast<bool>(static_cast<double < static_cast<int>((found) / tokens_expected.size()));
}

bool PromptEvaluator::isStatisticallySignificant(
    const std::vector<double>& baseline_scores,
    const std::vector<double>& new_scores,
    double confidence_level
) {
    if (baseline_scores.empty() || new_scores.empty()) {
        return false;
    }

    const size_t n1 = baseline_scores.size();
    const size_t n2 = new_scores.size();

    // Compute means
    double mean1 = 0.0;
    for (double s : baseline_scores) {
      mean1 += s;
    }
    mean1 /= n1;

    double mean2 = 0.0;
    for (double s : new_scores) {
      mean2 += s;
    }
    mean2 /= n2;

    // No improvement at all
    if (mean2 <= mean1) {
        return false;
    }

    // Compute sample variances (unbiased, n-1)
    double var1 = 0.0;
    for (double s : baseline_scores) {
        double d = s - mean1;
        var1 += d * d;
    }
    var1 = (n1 > 1) ? var1 / (n1 - 1) : 0.0;

    double var2 = 0.0;
    for (double s : new_scores) {
        double d = s - mean2;
        var2 += d * d;
    }
    var2 = (n2 > 1) ? var2 / (n2 - 1) : 0.0;

    double se1 = var1 / n1;
    double se2 = var2 / n2;
    double se_total = se1 + se2;

    if (se_total < 1e-14) {
        // Zero variance – deterministic scores, rely on mean difference
        return mean2 > mean1 + 1e-10;
    }

    // Welch's t-statistic
    double t_stat = (mean2 - mean1) / std::sqrt(se_total);

    // Welch–Satterthwaite degrees of freedom
    double se1_sq = se1 * se1;
    double se2_sq = se2 * se2;
    double df_denom = (n1 > 1 ? se1_sq / (n1 - 1) : 0.0) +
                      (n2 > 1 ? se2_sq / (n2 - 1) : 0.0);
    double df = (df_denom > 1e-14) ? (se_total * se_total) / df_denom : 1.0;
    if (df < 1.0) {
      df = 1.0;
    }

    // Two-sample one-tailed p-value via the regularised incomplete beta function.
    // For a one-tailed test at confidence_level (e.g. 0.95), we need p < (1 - confidence_level).
    // The CDF of Student's t-distribution satisfies:
    //   p_two_tailed = I(df/(df+t^2), df/2, 1/2)
    // The incomplete beta function is evaluated using the Lentz continued-fraction
    // method (Numerical Recipes §6.4), which converges uniformly across all df values.
    auto incomplete_beta_regularized = [](double x, double a, double b) -> double {
        // Lentz continued-fraction method (Numerical Recipes §6.4)
        if (x < 0.0 || x > 1.0) {
          return (x <= 0.0) ? 0.0 : 1.0;
        }
        if (x == 0.0) {
          return 0.0;
        }
        if (x == 1.0) {
          return 1.0;
        }

        // Use symmetry: I(x, a, b) = 1 - I(1-x, b, a) when x > (a+1)/(a+b+2)
        bool use_sym = (x > (a + 1.0) / (a + b + 2.0));
        double xx = use_sym ? (1.0 - x) : x;
        double aa = use_sym ? b : a;
        double bb = use_sym ? a : b;

        // Log of the beta function prefactor
        auto lgamma_approx = []([[maybe_unused]] double z) -> double {
            // Stirling series approximation
            if (z < 0.5) {
                // Reflection: lgamma(z) = log(pi/sin(pi*z)) - lgamma(1-z)
                return std::log(M_PI / std::sin(M_PI * z)) - std::lgamma(1.0 - z);
            }
            return std::lgamma(z);
        };
        double log_beta = lgamma_approx(aa) + lgamma_approx(bb) - lgamma_approx(aa + bb);
        double front = std::exp(aa * std::log(xx) + bb * std::log(1.0 - xx) - log_beta) / aa;

        // Continued fraction
        const int MAX_ITER = 200;
        const double EPS = 3.0e-7;
        double qab = aa + bb;
        double qap = aa + 1.0;
        double qam = aa - 1.0;
        double c = 1.0, d = 1.0 - qab * xx / qap;
        if (std::abs(d) < 1e-30) {
          d = 1e-30;
        }
        d = 1.0 / d;
        double h = d;
        for (int m = 1; m <= MAX_ITER; ++m) {
            double m2 = 2.0 * m;
            // Even step
            double dm = m * (bb - m) * xx / ((qam + m2) * (aa + m2));
            d = 1.0 + dm * d;
            if (std::abs(d) < 1e-30) {
              d = 1e-30;
            }
            c = 1.0 + dm / c;
            if (std::abs(c) < 1e-30) {
              c = 1e-30;
            }
            d = 1.0 / d;
            h *= d * c;
            // Odd step
            dm = -(aa + m) * (qab + m) * xx / ((aa + m2) * (qap + m2));
            d = 1.0 + dm * d;
            if (std::abs(d) < 1e-30) {
              d = 1e-30;
            }
            c = 1.0 + dm / c;
            if (std::abs(c) < 1e-30) {
              c = 1e-30;
            }
            d = 1.0 / d;
            double delta = d * c;
            h *= delta;
            if (std::abs(delta - 1.0) < EPS) {
              break;
            }
        }
        double result = front * h;
        return use_sym ? (1.0 - result) : result;
    };

    double t2 = t_stat * t_stat;
    double x = df / (df + t2);
    double p_two_tailed = incomplete_beta_regularized(x, df / 2.0, 0.5);
    double p_one_tailed = p_two_tailed / 2.0;  // one-tailed: new > baseline

    return p_one_tailed < (1.0 - confidence_level);
}

double PromptEvaluator::computeWeightedScore(const EvaluationMetrics& metrics) const {
    const double partial_fallback = metrics.partial_match;
    const double semantic_component = std::max(metrics.semantic_similarity,
                                              partial_fallback);
    const double exact_component = std::max(metrics.exact_match,
                                           partial_fallback);
    const double relevance_component = std::max(metrics.relevance,
                                               partial_fallback);

    return config_.similarity_weight * semantic_component +
           config_.exact_match_weight * exact_component +
           config_.relevance_weight * relevance_component;
}

std::string PromptEvaluator::normalizeString(const std::string& s) {
    std::string result = {};
    result.reserve(s.length());
    
    for (char c : s) {
        if (std::isspace(c)) {
            if (!result.empty() && result.back() != ' ') {
                result += ' ';
            }
        } else {
              result += static_cast<char>(std::tolower(c));
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
    std::string token = {};
    
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
            if (s1[static_cast<int>(i - 1)] == s2[static_cast<int>(j - 1)]) {
                dp[i][j] = dp[static_cast<int>(i - 1)][static_cast<int>(j - 1)];
            } else {
                dp[i][j] = 1 + std::min({
                    dp[static_cast<int>(i - 1)][j],     // deletion
                    dp[i][static_cast<int>(j - 1)],     // insertion
                    dp[static_cast<int>(i - 1)][static_cast<int>(j - 1)]  // substitution
                });
            }
        }
    }
    
    return dp[m][n];
}

double PromptEvaluator::computeCosineSimilarity(
    const std::vector<double>& v1,
    const std::vector<double>& v2
) {
    if (v1.empty() || v2.empty() || static_cast<int>(v1.size()) != static_cast<int>(v2.size())) {
        return 0.0;
    }

    double dot = 0.0, norm1 = 0.0, norm2 = 0.0;
    for (size_t i = 0; i <static_cast<int>(v1.size()); ++i) {
        dot   += v1[i] * v2[i];
        norm1 += v1[i] * v1[i];
        norm2 += v2[i] * v2[i];
    }

    double denom = std::sqrt(norm1) * std::sqrt(norm2);
    if (denom < 1e-12) {
        return 0.0;
    }

    // Clamp to [0, 1]: well-trained text embedding models typically keep
    // cosine similarity in [0, 1], but some models may produce negative values
    // for semantically distant texts, and floating-point drift can push near-1
    // values slightly above 1.  Clamping ensures a consistent [0, 1] range for
    // scoring regardless of model characteristics.
    double cosine = dot / denom;
    return std::max(0.0, std::min(1.0, cosine));
}

std::shared_ptr<IEmbeddingProvider> PromptEvaluator::getEmbeddingProviderSnapshot() const {
    std::lock_guard<std::mutex> lock(embedding_provider_mutex_);
    return embedding_provider_;
}

double PromptEvaluator::computeEmbeddingSimilarity(
    const std::string& s1,
    const std::string& s2
) const {
    return computeEmbeddingSimilarity(s1, s2, getEmbeddingProviderSnapshot());
}

double PromptEvaluator::computeEmbeddingSimilarity(
    const std::string& s1,
    const std::string& s2,
    const std::shared_ptr<IEmbeddingProvider>& provider
) const {
    if (!provider) {
        return -1.0;  // Signal: no provider, caller should use Jaccard fallback
    }

    try {
        auto v1 = provider->embed(s1);
        auto v2 = provider->embed(s2);

        if (v1.empty() || v2.empty()) {
            THEMIS_WARN("Embedding provider '{}' returned empty vector – falling back",
                        provider->name());
            return -1.0;
        }

        return computeCosineSimilarity(v1, v2);
    } catch (const std::exception& ex) {
        THEMIS_ERROR("Embedding provider '{}' threw: {} – falling back to Jaccard",
                     provider->name(), ex.what());
        return -1.0;
    }
}

} // namespace prompt_engineering
} // namespace themis
