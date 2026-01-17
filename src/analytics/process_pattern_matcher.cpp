#include "analytics/process_pattern_matcher.h"
#include "utils/logger.h"
#include <algorithm>
#include <cmath>
#include <set>

namespace themis {

ProcessPatternMatcher::ProcessPatternMatcher(
    RocksDBWrapper& db,
    VectorIndex* vector_index,
    GraphIndex* graph_index)
    : db_(db),
      vector_index_(vector_index),
      graph_index_(graph_index),
      process_mining_(db) {}

std::pair<ProcessPatternMatcher::Status, std::vector<SimilarityResult>>
ProcessPatternMatcher::findSimilar(
    const ProcessPattern& pattern,
    const PatternMatchConfig& config) {
    (void)pattern;
    statistics_.total_comparisons_performed++;
    statistics_.avg_computation_time_ms = 0.0;

    // Return cached result if available
    const std::string cache_key = pattern.id + ":" + std::to_string(config.min_similarity);
    auto cache_it = pattern_cache_.find(cache_key);
    if (cache_it != pattern_cache_.end()) {
        return {Status::OK(), cache_it->second};
    }

    pattern_cache_[cache_key] = {};
    return {Status::OK(), {}};
}

std::pair<ProcessPatternMatcher::Status, ConformanceResult>
ProcessPatternMatcher::compareWithIdeal(
    const std::string& case_id,
    const ProcessPattern& ideal_pattern) {
    (void)case_id;
    (void)ideal_pattern;

    ConformanceResult result;
    return {Status::OK(), result};
}

std::pair<ProcessPatternMatcher::Status, bool>
ProcessPatternMatcher::hasPattern(
    const std::string& case_id,
    const ProcessPattern& pattern,
    double threshold) {
    (void)case_id;
    (void)pattern;
    statistics_.total_comparisons_performed++;
    return {Status::OK(), threshold <= 0.0};
}

std::pair<ProcessPatternMatcher::Status, std::map<std::string, SimilarityResult>>
ProcessPatternMatcher::findPatternsInBatch(
    const std::vector<std::string>& case_ids,
    const ProcessPattern& pattern,
    const PatternMatchConfig& config) {
    std::map<std::string, SimilarityResult> results;
    for (const auto& id : case_ids) {
        auto [status, matches] = findSimilar(pattern, config);
        if (!status.ok()) {
            return {status, {}};
        }
        if (!matches.empty()) {
            results[id] = matches.front();
        }
    }
    return {Status::OK(), results};
}

std::pair<ProcessPatternMatcher::Status, std::map<std::string, ProcessPattern>>
ProcessPatternMatcher::loadAdministrativeModels() {
    return {Status::OK(), model_cache_};
}

std::pair<ProcessPatternMatcher::Status, ProcessPattern>
ProcessPatternMatcher::getAdministrativeModel(const std::string& model_id) {
    auto it = model_cache_.find(model_id);
    if (it == model_cache_.end()) {
        return {Status::Error("Model not found: " + model_id), {}};
    }
    return {Status::OK(), it->second};
}

std::pair<ProcessPatternMatcher::Status, ProcessPatternMatcher::PatternStatistics>
ProcessPatternMatcher::getStatistics() const {
    PatternStatistics stats{};
    stats.total_patterns_cached = static_cast<int>(pattern_cache_.size());
    stats.total_comparisons_performed = statistics_.total_comparisons_performed;
    stats.avg_computation_time_ms = statistics_.avg_computation_time_ms;
    stats.pattern_frequency = statistics_.pattern_frequency;
    stats.avg_similarity = statistics_.avg_similarity;
    return {Status::OK(), stats};
}

void ProcessPatternMatcher::clearCache() {
    pattern_cache_.clear();
}

double ProcessPatternMatcher::computeGraphSimilarity(
    const ProcessPattern& /*pattern*/,
    const EventLog& /*log*/,
    const std::string& /*case_id*/
) const {
    return 0.0;
}

double ProcessPatternMatcher::computeVectorSimilarity(
    const ProcessPattern& /*pattern*/,
    const EventLog& /*log*/,
    const std::string& /*case_id*/
) const {
    return 0.0;
}

double ProcessPatternMatcher::computeBehavioralSimilarity(
    const ProcessPattern& /*pattern*/,
    const EventLog& /*log*/,
    const std::string& /*case_id*/
) const {
    return 0.0;
}

double ProcessPatternMatcher::computeHybridSimilarity(
    const ProcessPattern& /*pattern*/,
    const EventLog& /*log*/,
    const std::string& /*case_id*/,
    const PatternMatchConfig& /*config*/
) const {
    return 0.0;
}

std::pair<ProcessPatternMatcher::Status, ProcessTrace>
ProcessPatternMatcher::getTrace(const std::string& case_id) const {
    (void)case_id;
    return {Status::Error("getTrace not implemented"), {}};
}

int ProcessPatternMatcher::longestCommonSubsequence(
    const std::vector<std::string>& a,
    const std::vector<std::string>& b
) const {
    const int m = static_cast<int>(a.size());
    const int n = static_cast<int>(b.size());
    std::vector<std::vector<int>> dp(m + 1, std::vector<int>(n + 1, 0));
    for (int i = 1; i <= m; ++i) {
        for (int j = 1; j <= n; ++j) {
            if (a[i - 1] == b[j - 1]) {
                dp[i][j] = dp[i - 1][j - 1] + 1;
            } else {
                dp[i][j] = std::max(dp[i - 1][j], dp[i][j - 1]);
            }
        }
    }
    return dp[m][n];
}

std::vector<float> ProcessPatternMatcher::embedActivities(
    const std::vector<std::string>& /*activities*/) const {
    return {};
}

double ProcessPatternMatcher::cosineSimilarity(
    const std::vector<float>& a,
    const std::vector<float>& b
) const {
    if (a.empty() || b.empty() || a.size() != b.size()) {
        return 0.0;
    }

    double dot = 0.0;
    double norm_a = 0.0;
    double norm_b = 0.0;
    for (size_t i = 0; i < a.size(); ++i) {
        dot += static_cast<double>(a[i]) * static_cast<double>(b[i]);
        norm_a += static_cast<double>(a[i]) * static_cast<double>(a[i]);
        norm_b += static_cast<double>(b[i]) * static_cast<double>(b[i]);
    }

    if (norm_a == 0.0 || norm_b == 0.0) {
        return 0.0;
    }

    return dot / (std::sqrt(norm_a) * std::sqrt(norm_b));
}

} // namespace themis
