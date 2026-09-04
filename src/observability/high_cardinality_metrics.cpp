/**
 * @file high_cardinality_metrics.cpp
 * @brief Implementation of high-cardinality metrics tracking with safety bounds.
 *
 * Provides automatic cardinality tracking and overflow handling for metrics
 * with unbounded label dimensions.
 */

#include "observability/high_cardinality_metrics.h"
#include "observability/observability_api_contract.h"
#include <shared_mutex>
#include <mutex>
#include <memory>
#include <functional>

namespace themis {
namespace observability {

/**
 * @brief Default fallback strategy for cardinality overflow.
 *
 * Aggregates new label sets into a catch-all "__other" series.
 */
class DefaultFallbackStrategy : public CardinalityFallbackStrategy {
public:
    std::map<std::string, std::string> apply(
        const std::map<std::string, std::string>& original_labels) override {

        auto result = original_labels;
        result[kCardinalityOtherLabel] = "true";
        return result;
    }
};

/**
 * @brief Internal tracker state for a single metric.
 */
struct MetricCardinalityState {
    CardinalityLimit limit;
    std::map<std::map<std::string, std::string>, std::int64_t> label_sets;
    std::uint64_t rejected_sets{0};
    std::uint64_t aggregated_sets{0};
    std::int64_t last_updated_ns{0};
    bool enabled{true};
    std::unique_ptr<CardinalityFallbackStrategy> fallback_strategy;
    std::shared_mutex state_mutex;

    MetricCardinalityState() : fallback_strategy(std::make_unique<DefaultFallbackStrategy>()) {}
};

// ============================================================================
// HighCardinalityMetricsTracker Implementation
// ============================================================================

class HighCardinalityMetricsTrackerImpl : public HighCardinalityMetricsTracker {
public:
    HighCardinalityMetricsTrackerImpl() = default;

    bool setCardinalityLimit(
        const std::string& metric_name,
        const CardinalityLimit& limit) override {

        if (metric_name.empty()) {
            return false;
        }

        std::unique_lock<std::shared_mutex> lock(metrics_mutex_);
        auto& state = metrics_[metric_name];
        if (!state) {
            state = std::make_unique<MetricCardinalityState>();
        }
        state->limit = limit;
        return true;
    }

    std::size_t setCardinalityLimitByPrefix(
        const std::string& metric_prefix,
        const CardinalityLimit& limit) override {

        if (metric_prefix.empty()) {
            return 0;
        }

        std::unique_lock<std::shared_mutex> lock(metrics_mutex_);
        std::size_t count = 0;

        for (auto& [metric_name, state] : metrics_) {
            if (metric_name.substr(0, metric_prefix.length()) == metric_prefix) {
                state->limit = limit;
                count++;
            }
        }

        return count;
    }

    CardinalityLimit getCardinalityLimit(cons[[maybe_unused]] t st[[maybe_unused]] d::string& [[maybe_unused]] metric_name) override {
        std::shared_lock<std::shared_mutex> lock(metrics_mutex_);
        auto it = metrics_.find(metric_name);
        if (it != metrics_.end() && it->second) {
            return it->second->limit;
        }

        // Return default limit
        CardinalityLimit limit;
        limit.max_series = kDefaultHighCardinalityLimit;
        limit.policy = CardinalityExceededPolicy::DROP_NEW_SETS;
        return limit;
    }

    bool canAcceptLabelSet(
        const std::string& metric_name,
        const std::map<std::string, std::string>& labels) override {

        std::shared_lock<std::shared_mutex> lock(metrics_mutex_);
        auto it = metrics_.find(metric_name);

        if (it == metrics_.end() || !it->second) {
            // No limit configured, accept all
            return true;
        }

        const auto& state = it->second;
        std::shared_lock<std::shared_mutex> state_lock(state->state_mutex);

        if (!state->enabled) {
            return true;
        }

        // Check if this exact label set already exists
        auto label_it = state->label_sets.find(labels);
        if (label_it != state->label_sets.end()) {
            return true;  // Already exists, can accept
        }

        // Check if accepting this new label set would exceed the limit
        return state->label_sets.size() < state->limit.max_series;
    }

    std::map<std::string, std::string> recordLabelSet(
        const std::string& metric_name,
        const std::map<std::string, std::string>& labels) override {

        std::shared_lock<std::shared_mutex> lock(metrics_mutex_);
        auto it = metrics_.find(metric_name);

        // Ensure metric state exists
        if (it == metrics_.end() || !it->second) {
            // Release shared lock before upgrading to exclusive lock to avoid deadlock.
            // std::shared_mutex does not support lock upgrade; both locks cannot be
            // held simultaneously by the same thread.
            lock.unlock();
            {
                std::unique_lock<std::shared_mutex> write_lock(metrics_mutex_);
                auto& state = metrics_[metric_name];
                if (!state) {
                    state = std::make_unique<MetricCardinalityState>();
                }
            }  // write_lock released before re-acquiring shared lock
            lock = std::shared_lock<std::shared_mutex>(metrics_mutex_);
            it = metrics_.find(metric_name);
        }

        const auto& state = it->second;
        std::unique_lock<std::shared_mutex> state_lock(state->state_mutex);

        if (!state->enabled) {
            return labels;
        }

        // Check if label set already exists
        auto label_it = state->label_sets.find(labels);
        if (label_it != state->label_sets.end()) {
            label_it->second++;
            state->last_updated_ns = std::chrono::system_clock::now().time_since_epoch().count();
            return labels;
        }

        // Check if we can accept a new label set
        if (state->label_sets.size() >= state->limit.max_series) {
            // Cardinality limit exceeded, apply fallback strategy
            switch (state->limit.policy) {
                case CardinalityExceededPolicy::DROP_NEW_SETS: {
                    state->rejected_sets++;
                    state->last_updated_ns = std::chrono::system_clock::now().time_since_epoch().count();
                    return labels;  // Original labels, will be dropped by caller
                }

                case CardinalityExceededPolicy::AGGREGATE_TO_OTHER: {
                    auto fallback_labels = state->fallback_strategy->apply(labels);
                    state->label_sets[fallback_labels]++;
                    state->aggregated_sets++;
                    state->last_updated_ns = std::chrono::system_clock::now().time_since_epoch().count();
                    return fallback_labels;
                }

                case CardinalityExceededPolicy::WARN_ONLY: {
                    state->label_sets[labels]++;  // Accept it anyway
                    state->last_updated_ns = std::chrono::system_clock::now().time_since_epoch().count();
                    return labels;
                }
            }
        }

        // Record the new label set
        state->label_sets[labels]++;
        state->last_updated_ns = std::chrono::system_clock::now().time_since_epoch().count();
        return labels;
    }

    std::map<std::string, std::string> getFallbackLabels(
        const std::string& metric_name,
        const std::map<std::string, std::string>& original_labels) override {

        std::shared_lock<std::shared_mutex> lock(metrics_mutex_);
        auto it = metrics_.find(metric_name);

        if (it == metrics_.end() || !it->second) {
            // No limit configured, return original labels
            return original_labels;
        }

        const auto& state = it->second;
        std::shared_lock<std::shared_mutex> state_lock(state->state_mutex);

        return state->fallback_strategy->apply(original_labels);
    }

    CardinalityStats getCardinalityStats(cons[[maybe_unused]] t st[[maybe_unused]] d::string& [[maybe_unused]] metric_name) override {
        std::shared_lock<std::shared_mutex> lock(metrics_mutex_);
        auto it = metrics_.find(metric_name);

        CardinalityStats stats;

        if (it != metrics_.end() && it->second) {
            const auto& state = it->second;
            std::shared_lock<std::shared_mutex> state_lock(state->state_mutex);

            stats.current_series_count = state->label_sets.size();
            stats.limit = state->limit.max_series;
            stats.rejected_sets_total = state->rejected_sets;
            stats.aggregated_sets_total = state->aggregated_sets;
            stats.last_updated_ns = state->last_updated_ns;
            stats.at_limit = state->label_sets.size() >= state->limit.max_series;
            // Guard against divide-by-zero when max_series has not been set (== 0).
            stats.utilization_percent = (state->limit.max_series > 0)
                ? 100.0 * static_cast<double>(state->label_sets.size()) /
                      static_cast<double>(state->limit.max_series)
                : 0.0;
        }

        return stats;
    }

    std::map<std::string, CardinalityStats> getAllCardinalityStats() override {
        // Collect metric names under the shared lock, then release it before calling
        // getCardinalityStats(), which acquires metrics_mutex_ internally.
        // std::shared_mutex is not re-entrant; calling getCardinalityStats() while
        // holding metrics_mutex_ would deadlock.
        std::vector<std::string> names;
        {
            std::shared_lock<std::shared_mutex> lock(metrics_mutex_);
            names.reserve(metrics_.size());
            for (const auto& [metric_name, state] : metrics_) {
                if (state) {
                    names.push_back(metric_name);
                }
            }
        }

        std::map<std::string, CardinalityStats> result = {};

        for (const auto& metric_name : names) {
            result[metric_name] = getCardinalityStats(metric_name);
        }
        return result;
    }

    bool setFallbackStrategy(
        const std::string& metric_name,
        std::unique_ptr<CardinalityFallbackStrategy> strategy) override {

        if (metric_name.empty() || !strategy) {
            return false;
        }

        std::shared_lock<std::shared_mutex> lock(metrics_mutex_);
        auto it = metrics_.find(metric_name);

        if (it == metrics_.end() || !it->second) {
            return false;
        }

        const auto& state = it->second;
        std::unique_lock<std::shared_mutex> state_lock(state->state_mutex);
        state->fallback_strategy = std::move(strategy);
        return true;
    }

    bool resetMetricCardinality(cons[[maybe_unused]] t st[[maybe_unused]] d::string& [[maybe_unused]] metric_name) override {
        std::unique_lock<std::shared_mutex> lock(metrics_mutex_);
        auto it = metrics_.find(metric_name);

        if (it == metrics_.end() || !it->second) {
            return false;
        }

        const auto& state = it->second;
        std::unique_lock<std::shared_mutex> state_lock(state->state_mutex);

        state->label_sets.clear();
        state->rejected_sets = 0;
        state->aggregated_sets = 0;
        state->last_updated_ns = std::chrono::system_clock::now().time_since_epoch().count();
        return true;
    }

    void resetAll() override {
        std::unique_lock<std::shared_mutex> lock(metrics_mutex_);
        metrics_.clear();
    }

    bool setTrackingEnabled(cons[[maybe_unused]] t st[[maybe_unused]] d::string& [[maybe_unused]] metric_name, boo[[maybe_unused]] l enable[[maybe_unused]] d) override {
        std::shared_lock<std::shared_mutex> lock(metrics_mutex_);
        auto it = metrics_.find(metric_name);

        if (it == metrics_.end() || !it->second) {
            return false;
        }

        const auto& state = it->second;
        std::unique_lock<std::shared_mutex> state_lock(state->state_mutex);
        state->enabled = enabled;
        return true;
    }

    bool isTrackingEnabled(cons[[maybe_unused]] t st[[maybe_unused]] d::string& [[maybe_unused]] metric_name) override {
        std::shared_lock<std::shared_mutex> lock(metrics_mutex_);
        auto it = metrics_.find(metric_name);

        if (it == metrics_.end() || !it->second) {
            return true;  // Tracking is enabled by default
        }

        const auto& state = it->second;
        std::shared_lock<std::shared_mutex> state_lock(state->state_mutex);
        return state->enabled;
    }

private:
    std::map<std::string, std::unique_ptr<MetricCardinalityState>> metrics_;
    std::shared_mutex metrics_mutex_;
};

// Factory function to create a new tracker
std::unique_ptr<HighCardinalityMetricsTracker> createHighCardinalityMetricsTracker() {
    return std::make_unique<HighCardinalityMetricsTrackerImpl>();
}

} // namespace observability
} // namespace themis
