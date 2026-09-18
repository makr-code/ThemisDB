/**
 * @file metric_aggregator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "observability/metric_aggregator.h"

#include <algorithm>
#include <numeric>
#include <sstream>
#include <stdexcept>

namespace themis {
namespace observability {

// ============================================================================
// Internal helpers
// ============================================================================

std::string MetricAggregator::makeSeriesKey(
    const std::string& name,
    const std::map<std::string, std::string>& labels) {
    if (labels.empty()) {
      return name;
    }
    std::ostringstream oss = {};
    oss << name << "{";
    bool first = true;
    for (const auto& [k, v] : labels) {
        if (!first) {
          oss << ",";
        }
        oss << k << "=\"" << v << "\"";
        first = false;
    }
    oss << "}";
    return oss.str();
}

std::string MetricAggregator::makeLabelFingerprint(
    const std::map<std::string, std::string>& labels) {
    // Build a deterministic string from the sorted label map.
    return makeSeriesKey("", labels);
}

std::map<std::string, std::string> MetricAggregator::applyDropLabels(
    const std::map<std::string, std::string>& labels,
    const std::vector<std::string>& drop) {
    if (drop.empty()) {
      return labels;
    }
    std::map<std::string, std::string> result = {};

    for (const auto& [k, v] : labels) {
        if (std::find(drop.begin(), drop.end(), k) == drop.end()) {
            result[k] = v;
        }
    }
    return result;
}

/**
 * @brief Reduce.
 * @param[in] vals Input parameter.
 * @param[in] type Input parameter.
 * @return Return value.
 * @details Calls: empty(), std::accumulate(), begin(), end(), size(), std::max_element(), std::min_element(), std::sort().
 */
double MetricAggregator::reduce(std::vector<double> vals, AggregationType type) {
    if (vals.empty()) {
      return 0.0;
    }

    switch (type) {
        case AggregationType::SUM:
            return std::accumulate(vals.begin(), vals.end(), 0.0);

        case AggregationType::AVG:
            return std::accumulate(vals.begin(), vals.end(), 0.0) /
                   static_cast<double>(vals.size());

        case AggregationType::MAX:
            return *std::max_element(vals.begin(), vals.end());

        case AggregationType::MIN:
            return *std::min_element(vals.begin(), vals.end());

        case AggregationType::P50:
        [[fallthrough]];
        case AggregationType::P95:
        [[fallthrough]];
        case AggregationType::P99: {
            // Guard: already checked above, but be explicit for static analysis.
            if (vals.empty()) {
              return 0.0;
            }
            std::sort(vals.begin(), vals.end());
            double p = (type == AggregationType::P50) ? 0.50
                     : (type == AggregationType::P95) ? 0.95
                                                      : 0.99;
            // Nearest-rank method: clamp index to [0, size-1].
            size_t idx = static_cast<size_t>(p * static_cast<double>(vals.size() - 1));
            return vals[idx];
        }

        case AggregationType::RATE:
            // RATE is not valid for histogram reduction; callers must use
            // calculateRate() instead.  Return 0.0 as a safe fallback.
            return 0.0;
    }
    return 0.0;  // unreachable
}

/**
 * @brief Check Snapshot Cardinality.
 * @param[in] metric_name Input parameter.
 * @param[in] label_fp Input parameter.
 * @return True on success.
 * @details Calls: std::find(), begin(), end(), find(), size(), push_back().
 */
bool MetricAggregator::checkSnapshotCardinality(const std::string& metric_name,
                                                const std::string& label_fp) {
    // Called with mutex_ held.
    auto& known = known_series_[metric_name];
    auto it = std::find(known.begin(), known.end(), label_fp);
    if (it != known.end()) {
        // Already-known series — always allowed.
        return true;
    }

    auto limit_it = cardinality_limits_.find(metric_name);
    if (limit_it != cardinality_limits_.end() && limit_it->second > 0) {
        if (known.size() >= limit_it->second) {
            ++dropped_snapshots_;
            return false;
        }
    }
    known.push_back(label_fp);
    return true;
}

// ============================================================================
// Rate calculation
// ============================================================================

void MetricAggregator::recordCounterSample(
    const std::string& name, int64_t value,
    const std::map<std::string, std::string>& labels,
    std::chrono::seconds window) {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);

    std::string key = makeSeriesKey(name, labels);
    auto now = std::chrono::steady_clock::now();

    auto& deque = rate_samples_[key];
    deque.push_back({value, now});

    // Prune samples outside the retention window.
    auto cutoff = now - window;
    while (deque.size() > 1 && deque.front().timestamp < cutoff) {
        deque.pop_front();
    }
}

double MetricAggregator::calculateRate(
    const std::string& name,
    const std::map<std::string, std::string>& labels) const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);

    std::string key = makeSeriesKey(name, labels);
    auto it = rate_samples_.find(key);
    if (it == rate_samples_.end() || it->second.size() < 2) {
        return 0.0;
    }

    const auto& deque = it->second;
    const auto& oldest = deque.front();
    const auto& newest = deque.back();

    auto elapsed_s = std::chrono::duration<double>(
                         newest.timestamp - oldest.timestamp)
                         .count();
    if (elapsed_s <= 0.0) {
      return 0.0;
    }

    int64_t delta = newest.value - oldest.value;
    // Guard against counter resets (monotonically increasing counter that was
    // reset — treat as 0 rather than returning a negative rate).
    if (delta < 0) {
      return 0.0;
    }

    return static_cast<double>(delta) / elapsed_s;
}

/**
 * @brief ============================================================================ Histogram aggregation ============================================================================
 * @param[in] snapshot Input parameter.
 * @details Calls: lock(), makeLabelFingerprint(), checkSnapshotCardinality(), makeSeriesKey(), push_back().
 */

void MetricAggregator::addHistogramSnapshot(const HistogramSnapshot& snapshot) {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);

    std::string fp = makeLabelFingerprint(snapshot.labels);
    if (!checkSnapshotCardinality(snapshot.metric_name, fp)) {
        return;
    }

    // Store under a key that combines metric name and label fingerprint so that
    // snapshots from different shards with identical labels land in the same bucket.
    std::string key = makeSeriesKey(snapshot.metric_name, snapshot.labels);
    snapshots_[key].push_back(snapshot);
}

AggregatedMetric MetricAggregator::aggregateHistograms(
    const std::string& metric_name, AggregationType type,
    const std::map<std::string, std::string>& filter_labels) const {
    if (type == AggregationType::RATE) {
        throw std::invalid_argument(
            "AggregationType::RATE is not valid for histogram aggregation; "
            "use calculateRate() instead.");
    }

    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);

    // Collect all observations from all matching snapshot entries.
    std::vector<double> all_values = {};

    for (const auto& [key, snapshots] : snapshots_) {
        for (const auto& snap : snapshots) {
            if (snap.metric_name != metric_name) {
              continue;
            }

            // Apply label filter.
            bool match = true;
            for (const auto& [fk, fv] : filter_labels) {
                auto it = snap.labels.find(fk);
                if (it == snap.labels.end() || it->second != fv) {
                    match = false;
                    break;
                }
            }
            if (!match) {
              continue;
            }

            all_values.insert(all_values.end(),
                               snap.values.begin(), snap.values.end());
        }
    }

    if (all_values.empty()) {
        throw std::invalid_argument(
            "No snapshots available for metric '" + metric_name + "'.");
    }

    AggregatedMetric result;
    result.metric_name = metric_name;
    result.labels = filter_labels;
    result.type = type;
    result.value = reduce(std::move(all_values), type);
    result.timestamp = std::chrono::system_clock::now();
    return result;
}

/**
 * @brief ============================================================================ Rule-based aggregation ============================================================================
 * @param[in] rule Input parameter.
 * @details Calls: lock().
 */

void MetricAggregator::addAggregationRule(const AggregationRule& rule) {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);
    rules_[rule.metric_name] = rule;
}

/**
 * @brief Remove Aggregation Rule.
 * @param[in] metric_name Input parameter.
 * @return True on success.
 * @details Calls: lock(), erase().
 */
bool MetricAggregator::removeAggregationRule(const std::string& metric_name) {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);
    return rules_.erase(metric_name) > 0;
}

std::vector<AggregationRule> MetricAggregator::getRules() const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<AggregationRule> out = {};

    out.reserve(rules_.size());
    for (const auto& [name, rule] : rules_) {
        out.push_back(rule);
    }
    return out;
}

std::vector<AggregatedMetric> MetricAggregator::applyRules() const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<AggregatedMetric> results;

    for (const auto& [metric_name, rule] : rules_) {
        if (rule.type == AggregationType::RATE) {
            // For RATE rules we need to look at the rate_samples_ map.
            // Collect all keys that belong to this metric.
            for (const auto& [key, deque] : rate_samples_) {
                // Extract metric name from the key (up to first '{' or end)
                std::string kname = key;
                auto brace = key.find('{');
                if (brace != std::string::npos) {
                  kname = key.substr(0, brace);
                }
                if (kname != metric_name) {
                  continue;
                }
                if (deque.size() < 2) {
                  continue;
                }

                const auto& oldest = deque.front();
                const auto& newest = deque.back();
                auto elapsed_s = std::chrono::duration<double>(
                                     newest.timestamp - oldest.timestamp)
                                     .count();
                if (elapsed_s <= 0.0) {
                  continue;
                }
                int64_t delta = newest.value - oldest.value;
                if (delta < 0) {
                  continue;
                }

                AggregatedMetric r;
                r.metric_name = metric_name;
                r.type = AggregationType::RATE;
                r.value = static_cast<double>(delta) / elapsed_s;
                r.timestamp = std::chrono::system_clock::now();
                results.push_back(std::move(r));
            }
            continue;
        }

        // Collect observations for this metric, respecting drop_labels.
        std::map<std::string, std::vector<double>> grouped;               // group_key → values
        std::map<std::string, std::map<std::string, std::string>> glabels; // group_key → labels

        for (const auto& [key, snapshots] : snapshots_) {
            for (const auto& snap : snapshots) {
                if (snap.metric_name != metric_name) {
                  continue;
                }

                // Drop high-cardinality labels.
                auto effective_labels =
                    applyDropLabels(snap.labels, rule.drop_labels);

                // Build group key from group_by_labels.
                std::map<std::string, std::string> group_labels = {};

                for (const auto& gl : rule.group_by_labels) {
                    auto it = effective_labels.find(gl);
                    if (it != effective_labels.end()) {
                        group_labels[gl] = it->second;
                    }
                }

                std::string gk = makeLabelFingerprint(group_labels);
                glabels[gk] = group_labels;
                for (double v : snap.values) {
                    grouped[gk].push_back(v);
                }
            }
        }

        if (grouped.empty()) {
          continue;
        }

        for (auto& [gk, vals] : grouped) {
            AggregatedMetric r;
            r.metric_name = metric_name;
            r.type = rule.type;
            r.labels = glabels[gk];
            r.value = reduce(std::move(vals), rule.type);
            r.timestamp = std::chrono::system_clock::now();
            results.push_back(std::move(r));
        }
    }

    return results;
}

// ============================================================================
// Cross-shard aggregation and rollup
// ============================================================================

ShardAggregationSnapshot MetricAggregator::aggregateShardMetrics(
    const std::vector<ShardMetrics>& shard_metrics) const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);

    // Build a transient snapshot map from the supplied shard data.
    // Key: series key (metric_name + label fingerprint), value: list of snapshots.
    std::map<std::string, std::vector<HistogramSnapshot>> transient_snapshots;

    for (const auto& shard : shard_metrics) {
        // Merge shard-level labels with the per-metric labels.
        // The shard_id is always injected so rules can drop it via drop_labels.
        std::map<std::string, std::string> base_labels = shard.labels;
        base_labels["shard_id"] = shard.shard_id;

        for (const auto& [metric_name, values] : shard.metrics) {
            HistogramSnapshot snap;
            snap.metric_name = metric_name;
            snap.labels = base_labels;
            snap.values = values;
            snap.timestamp = shard.timestamp;

            std::string key = makeSeriesKey(metric_name, snap.labels);
            transient_snapshots[key].push_back(std::move(snap));
        }
    }

    // Apply registered rules against the transient data.
    std::vector<AggregatedMetric> results;

    for (const auto& [metric_name, rule] : rules_) {
        if (rule.type == AggregationType::RATE) {
            // RATE rules require time-series counter samples which are not
            // available in a one-shot ShardMetrics batch; skip silently.
            // Use addAggregationRule(RATE) with recordCounterSample() + applyRules()
            // for rate-based aggregation on buffered data.
            continue;
        }

        std::map<std::string, std::vector<double>> grouped;               // group_key → values
        std::map<std::string, std::map<std::string, std::string>> glabels; // group_key → labels

        for (const auto& [key, snapshots] : transient_snapshots) {
            for (const auto& snap : snapshots) {
                if (snap.metric_name != metric_name) {
                  continue;
                }

                auto effective_labels =
                    applyDropLabels(snap.labels, rule.drop_labels);

                std::map<std::string, std::string> group_labels = {};

                for (const auto& gl : rule.group_by_labels) {
                    auto it = effective_labels.find(gl);
                    if (it != effective_labels.end()) {
                        group_labels[gl] = it->second;
                    }
                }

                std::string gk = makeLabelFingerprint(group_labels);
                glabels[gk] = group_labels;
                for (double v : snap.values) {
                    grouped[gk].push_back(v);
                }
            }
        }

        if (grouped.empty()) {
          continue;
        }

        for (auto& [gk, vals] : grouped) {
            AggregatedMetric r;
            r.metric_name = metric_name;
            r.type = rule.type;
            r.labels = glabels[gk];
            r.value = reduce(std::move(vals), rule.type);
            r.timestamp = std::chrono::system_clock::now();
            results.push_back(std::move(r));
        }
    }

    ShardAggregationSnapshot snapshot;
    snapshot.metrics = std::move(results);
    snapshot.timestamp = std::chrono::system_clock::now();
    return snapshot;
}

/**
 * @brief ============================================================================ Rollup / cardinality reduction ============================================================================
 * @param[in] window Input parameter.
 * @details Calls: lock(), std::chrono::system_clock::now(), erase(), std::remove_if(), begin(), end(), empty(), std::chrono::steady_clock::now().
 */

void MetricAggregator::rollupMetrics(std::chrono::minutes window) {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);

    auto now = std::chrono::system_clock::now();
    auto cutoff = now - window;

    // Remove histogram snapshots older than the window.
    for (auto& [key, snapshots] : snapshots_) {
        snapshots.erase(
            std::remove_if(snapshots.begin(), snapshots.end(),
                           [&cutoff](const HistogramSnapshot& s) {
                               return s.timestamp < cutoff;
                           }),
            snapshots.end());
    }

    // Remove empty series entries to keep the map tidy.
    for (auto it = snapshots_.begin(); it != snapshots_.end();) {
        if (it->second.empty()) {
            it = snapshots_.erase(it);
        } else {
            ++it;
        }
    }

    // Also prune rate samples using the same window (converted to seconds).
    auto steady_now = std::chrono::steady_clock::now();
    auto steady_cutoff =
        steady_now - std::chrono::duration_cast<std::chrono::seconds>(window);
    for (auto& [key, deque] : rate_samples_) {
        // Keep at least one sample even if it's older than the cutoff so that
        // the next call to recordCounterSample() can compute a valid delta.
        while (deque.size() > 1 && deque.front().timestamp < steady_cutoff) {
            deque.pop_front();
        }
    }
}

/**
 * @brief ============================================================================ Cardinality management ============================================================================
 * @param[in] metric_name Input parameter.
 * @param[in] limit Input parameter.
 * @details Calls: lock().
 */

void MetricAggregator::setMetricCardinalityLimit(const std::string& metric_name,
                                                  size_t limit) {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);
    cardinality_limits_[metric_name] = limit;
}

size_t MetricAggregator::getSeriesCount(const std::string& metric_name) const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = known_series_.find(metric_name);
    if (it == known_series_.end()) {
      return 0;
    }
    return it->second.size();
}

int64_t MetricAggregator::getDroppedSnapshotCount() const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);
    return dropped_snapshots_;
}

/**
 * @brief ============================================================================ Utilities ============================================================================
 * @details Calls: lock(), clear().
 */

void MetricAggregator::reset() {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);
    rate_samples_.clear();
    snapshots_.clear();
    known_series_.clear();
    dropped_snapshots_ = 0;
    // Rules are intentionally preserved across reset().
}

/**
 * @brief Prune Rate Samples.
 * @param[in] window Input parameter.
 * @details Calls: lock(), std::chrono::steady_clock::now(), size(), front(), pop_front().
 */
void MetricAggregator::pruneRateSamples(std::chrono::seconds window) {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);
    auto now = std::chrono::steady_clock::now();
    auto cutoff = now - window;
    for (auto& [key, deque] : rate_samples_) {
        while (deque.size() > 1 && deque.front().timestamp < cutoff) {
            deque.pop_front();
        }
    }
}

}  // namespace observability
}  // namespace themis
