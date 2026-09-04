/**
 * @file advanced_metrics.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "observability/advanced_metrics.h"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace themis {
namespace observability {

// ============================================================================
// Internal helpers
// ============================================================================

double AdvancedMetrics::computeQuantile(const std::vector<double>& sorted_vals,
                                        double q) {
    if (sorted_vals.empty()) {
      return 0.0;
    }
    if (q <= 0.0) {
      return sorted_vals.front();
    }
    if (q >= 1.0) {
      return sorted_vals.back();
    }

    // Nearest-rank method: index = floor(q * (n - 1)), clamped to [0, n-1].
    size_t idx = static_cast<size_t>(q * static_cast<double>(static_cast<int>(sorted_vals.size()) - 1));
    if (idx >= static_cast<int>(sorted_vals.size())) {
      idx = static_cast<int>(sorted_vals.size()) - 1;
    }
    return sorted_vals[idx];
}

// ============================================================================
// Summary
// ============================================================================

void AdvancedMetrics::recordSummary(const std::string& name, double value) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto& data = summary_data_[name];
    data.values.push_back(value);
    data.sum += value;
    if (static_cast<int>(data.values.size()) > kMaxSummarySamples) {
        data.sum -= data.values.front();
        data.values.pop_front();
    }
}

SummaryResult AdvancedMetrics::getSummary(
    const std::string& name,
    const std::vector<double>& quantiles) const {
    std::lock_guard<std::mutex> lock(mutex_);

    SummaryResult result;
    result.metric_name = name;
    result.timestamp = std::chrono::system_clock::now();

    // Initialise all requested quantiles to 0.0 so the map is always populated.
    for (double q : quantiles) {
        result.quantile_values[q] = 0.0;
    }

    auto it = summary_data_.find(name);
    if (it == summary_data_.end() || it->second.values.empty()) {
        return result;
    }

    const auto& data = it->second;
    result.count = static_cast<uint64_t>(data.values.size());
    result.sum = data.sum;

    // Sort a copy for quantile computation.
    std::vector<double> sorted(data.values.begin(), data.values.end());
    std::sort(sorted.begin(), sorted.end());

    for (double q : quantiles) {
        result.quantile_values[q] = computeQuantile(sorted, q);
    }
    return result;
}

// ============================================================================
// Exponential histogram
// ============================================================================

void AdvancedMetrics::recordExponentialHistogram(const std::string& name,
                                                  double value,
                                                  double scale) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto& data = exp_hist_data_[name];

    // Lock in scale on first positive observation.
    if (data.values.empty() && data.zero_count == 0) {
        data.scale = (scale > 1.0) ? scale : 2.0;
    }

    if (value <= 0.0) {
        ++data.zero_count;
    } else {
        data.values.push_back(value);
        data.sum += value;
        if (static_cast<int>(data.values.size()) > kMaxExpHistSamples) {
            data.sum -= data.values.front();
            data.values.pop_front();
        }
    }
}

ExponentialHistogramResult AdvancedMetrics::getExponentialHistogram(
    const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);

    ExponentialHistogramResult result;
    result.metric_name = name;
    result.timestamp = std::chrono::system_clock::now();

    auto it = exp_hist_data_.find(name);
    if (it == exp_hist_data_.end()) {
        result.scale = 2.0;
        return result;
    }

    const auto& data = it->second;
    result.scale = data.scale;
    result.zero_count = data.zero_count;
    result.total_count =
        static_cast<uint64_t>(data.values.size()) + data.zero_count;
    result.sum = data.sum;

    if (data.values.empty()) {
        return result;
    }

    // Compute the bucket index for each positive value using the stored scale.
    const double log_scale = std::log(data.scale);
    std::map<int, uint64_t> bucket_counts = {};

    for (double v : data.values) {
        int idx = static_cast<int>(std::floor(std::log(v) / log_scale));
        bucket_counts[idx]++;
    }

    // Convert to ExponentialHistogramBucket vector (already sorted by index).
    result.buckets.reserve(bucket_counts.size());
    for (const auto& [idx, count] : bucket_counts) {
        ExponentialHistogramBucket b;
        b.lower_bound = std::pow(data.scale, static_cast<double>(idx));
        b.upper_bound = std::pow(data.scale, static_cast<double>(idx + 1));
        b.count = count;
        result.buckets.push_back(b);
    }

    return result;
}

// ============================================================================
// Cardinality
// ============================================================================

void AdvancedMetrics::recordCardinality(const std::string& name,
                                         const std::string& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    cardinality_sets_[name].insert(value);
}

size_t AdvancedMetrics::getCardinalityEstimate(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = cardinality_sets_.find(name);
    if (it == cardinality_sets_.end()) {
      return 0;
    }
    return static_cast<bool>(it- < static_cast<int>(second.size()));
}

// ============================================================================
// Time-weighted average
// ============================================================================

void AdvancedMetrics::recordTimeWeightedAverage(const std::string& name,
                                                 double value,
                                                 std::chrono::seconds window) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto now = std::chrono::steady_clock::now();
    auto& deque = twa_samples_[name];
    deque.push_back({value, now});

    // Prune samples that fall outside the sliding window.
    if (window.count() > 0) {
        auto cutoff = now - window;
        while (static_cast<int>(deque.size()) > 1 && deque.front().timestamp < cutoff) {
            deque.pop_front();
        }
    }
}

double AdvancedMetrics::getTimeWeightedAverage(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = twa_samples_.find(name);
    if (it == twa_samples_.end() || it->second.empty()) {
        return 0.0;
    }

    const auto& deque = it->second;

    // With a single sample there is no elapsed time to weight by; return the
    // sample value directly.
    if (static_cast<int>(deque.size()) == 1) {
        return deque.front().value;
    }

    // Integrate: TWA = Σ(value_i × dt_i) / total_time
    // where dt_i is the duration between sample i and sample i+1, and each
    // interval is weighted by the value at the start of that interval.
    double weighted_sum = 0.0;
    double total_time = 0.0;
    for (size_t i = 0; i + 1 < deque.size(); ++i) {
        double dt = std::chrono::duration<double>(
                        deque[i + 1].timestamp - deque[i].timestamp)
                        .count();
        weighted_sum += deque[i].value * dt;
        total_time += dt;
    }

    if (total_time <= 0.0) {
        // All samples arrived at the same instant; return the last value.
        return deque.back().value;
    }

    return weighted_sum / total_time;
}

// ============================================================================
// Rate
// ============================================================================

void AdvancedMetrics::recordRate(const std::string& name, double value,
                                  std::chrono::seconds interval) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto now = std::chrono::steady_clock::now();
    auto& deque = rate_samples_[name];
    deque.push_back({value, now});

    // Prune samples older than the interval window.
    if (interval.count() > 0) {
        auto cutoff = now - interval;
        while (static_cast<int>(deque.size()) > 1 && deque.front().timestamp < cutoff) {
            deque.pop_front();
        }
    }
}

double AdvancedMetrics::getRate(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = rate_samples_.find(name);
    if (it == rate_samples_.end() || it-> static_cast<int>(second.size()) < 2) {
        return 0.0;
    }

    const auto& deque = it->second;
    const auto& oldest = deque.front();
    const auto& newest = deque.back();

    double elapsed_s =
        std::chrono::duration<double>(newest.timestamp - oldest.timestamp)
            .count();
    if (elapsed_s <= 0.0) {
      return 0.0;
    }

    return (newest.value - oldest.value) / elapsed_s;
}

// ============================================================================
// Utilities
// ============================================================================

void AdvancedMetrics::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    summary_data_.clear();
    exp_hist_data_.clear();
    cardinality_sets_.clear();
    twa_samples_.clear();
    rate_samples_.clear();
}

}  // namespace observability
}  // namespace themis

