/**
 * @file downsampling.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "timeseries/downsampling.h"
#include "timeseries/tsstore.h"
#include "utils/logger.h"
#include <chrono>
#include <stdexcept>
#include <algorithm>

namespace themis {

// =========================================================================
// DownsamplingTier factory methods
// =========================================================================

DownsamplingTier DownsamplingTier::minutes(int n, std::chrono::seconds keep) {
    DownsamplingTier t;
    t.name      = std::to_string(n) + "m";
    t.resolution = std::chrono::milliseconds{static_cast<int64_t>(n) * 60 * 1000};
    t.retention  = keep;
    return t;
}

DownsamplingTier DownsamplingTier::hours(int n, std::chrono::seconds keep) {
    DownsamplingTier t;
    t.name      = std::to_string(n) + "h";
    t.resolution = std::chrono::milliseconds{static_cast<int64_t>(n) * 3600 * 1000};
    t.retention  = keep;
    return t;
}

DownsamplingTier DownsamplingTier::days(int n, std::chrono::seconds keep) {
    DownsamplingTier t;
    t.name      = std::to_string(n) + "d";
    t.resolution = std::chrono::milliseconds{static_cast<int64_t>(n) * 86400 * 1000};
    t.retention  = keep;
    return t;
}

// =========================================================================
// DownsamplingPolicy factory
// =========================================================================

DownsamplingPolicy DownsamplingPolicy::defaultPolicy(
    const std::string& metric,
    const std::optional<std::string>& entity)
{
    using namespace std::chrono_literals;
    DownsamplingPolicy p;
    p.metric = metric;
    p.entity = entity;
    // 1 min tier: keep 7 days
    p.tiers.push_back(DownsamplingTier::minutes(1, 7 * 24 * 3600s));
    // 1 hour tier: keep 90 days
    p.tiers.push_back(DownsamplingTier::hours(1, 90 * 24 * 3600s));
    // 1 day tier: keep 2 years
    p.tiers.push_back(DownsamplingTier::days(1, 2 * 365 * 24 * 3600s));
    return p;
}

// =========================================================================
// TierSelector
// =========================================================================

void TierSelector::registerPolicy(const DownsamplingPolicy& policy) {
    policies_[policy.metric] = policy;
}

std::optional<std::string> TierSelector::selectTier(
    const std::string& metric,
    std::chrono::milliseconds requested_resolution) const
{
    auto it = policies_.find(metric);
    if (it == policies_.end()) {
        return std::nullopt;
    }

    const auto& tiers = it->second.tiers;
    if (tiers.empty()) {
        return std::nullopt;
    }

    // Zero means "raw data requested" – skip tier selection
    if (requested_resolution.count() == 0) {
        return std::nullopt;
    }

    // Find the coarsest tier whose resolution <= requested_resolution.
    // Tiers are ordered finest→coarsest, so we iterate in reverse.
    std::optional<const DownsamplingTier*> best;
    for (auto& tier : tiers) {
        if (tier.resolution <= requested_resolution) {
            best = &tier;
        }
    }

    if (!best.has_value()) {
        return std::nullopt;
    }

    // Return the derived metric name as stored by ContinuousAggregateManager
    return ContinuousAggregateManager::derivedMetricName(metric, (*best)->resolution);
}

std::vector<DownsamplingTier> TierSelector::tiersFor(const std::string& metric) const {
    auto it = policies_.find(metric);
    if (it == policies_.end()) {
        return {};
    }
    return it->second.tiers;
}

// =========================================================================
// DownsamplingPipeline
// =========================================================================

DownsamplingPipeline::DownsamplingPipeline(TSStore* store)
    : store_(store)
    , agg_manager_(store)
{
    if (!store_) {
        throw std::invalid_argument("DownsamplingPipeline: store cannot be null");
    }
}

void DownsamplingPipeline::addPolicy([[maybe_unused]] const DownsamplingPolicy& policy) {
    if (policy.metric.empty()) {
        throw std::invalid_argument("DownsamplingPipeline::addPolicy: metric name cannot be empty");
    }
    if (policy.tiers.empty()) {
        throw std::invalid_argument("DownsamplingPipeline::addPolicy: tiers cannot be empty");
    }

    policies_[policy.metric] = policy;
    tier_selector_.registerPolicy(policy);

    THEMIS_INFO("DownsamplingPipeline: registered policy for '{}' with {} tier(s)",
                policy.metric, policy.tiers.size());
}

size_t DownsamplingPipeline::refresh(int64_t to_ms) {
    size_t total = 0;
    for (const auto& [metric, policy] : policies_) {
        // metric key is sufficient; policy is accessed in refreshMetric
        total += refreshMetric(metric, to_ms);
    }
    return total;
}

size_t DownsamplingPipeline::refreshMetric(const std::string& metric, int64_t to_ms) {
    auto it = policies_.find(metric);
    if (it == policies_.end()) {
        THEMIS_WARN("DownsamplingPipeline::refreshMetric: no policy for '{}'", metric);
        return 0;
    }

    const DownsamplingPolicy& policy = it->second;
    if (to_ms <= 0) {
        to_ms = nowMs();
    }

    size_t total = 0;
    std::string input_metric = metric;  // first tier reads raw data

    for (const auto& tier : policy.tiers) {
        const int64_t from_ms = getWatermark(metric, tier.name);

        if (from_ms >= to_ms) {
            THEMIS_DEBUG("DownsamplingPipeline: tier '{}' for '{}' is up-to-date (wm={})",
                         tier.name, metric, from_ms);
            // Advance input for next tier regardless
            input_metric = ContinuousAggregateManager::derivedMetricName(metric, tier.resolution);
            continue;
        }

        size_t written = refreshTier(policy, tier, input_metric, from_ms, to_ms);
        total += written;

        if (written > 0 || from_ms == 0) {
            // Advance watermark
            setWatermark(metric, tier.name, to_ms);
        }

        // Next tier reads this tier's output
        input_metric = ContinuousAggregateManager::derivedMetricName(metric, tier.resolution);
    }

    return total;
}

size_t DownsamplingPipeline::refreshTier(
    const DownsamplingPolicy& policy,
    const DownsamplingTier& tier,
    const std::string& input_metric,
    int64_t from_ms,
    int64_t to_ms)
{
    AggConfig cfg;
    cfg.metric = input_metric;
    cfg.entity = policy.entity;
    cfg.window.size = tier.resolution;

    THEMIS_DEBUG("DownsamplingPipeline: refreshing tier '{}' for '{}' [{}, {})",
                 tier.name, policy.metric, from_ms, to_ms);

    agg_manager_.refresh(cfg, from_ms, to_ms);

    // We don't have a direct "points written" count from ContinuousAggregateManager,
    // so return a positive sentinel to signal progress.
    return (to_ms > from_ms) ? 1u : 0u;
}

int64_t DownsamplingPipeline::getWatermark(
    const std::string& metric,
    const std::string& tier_name) const
{
    auto key = watermarkKey(metric, tier_name);
    auto it = watermarks_.find(key);
    return (it != watermarks_.end()) ? it->second : 0;
}

void DownsamplingPipeline::setWatermark(
    const std::string& metric,
    const std::string& tier_name,
    int64_t watermark_ms)
{
    watermarks_[watermarkKey(metric, tier_name)] = watermark_ms;
}

std::string DownsamplingPipeline::watermarkKey(
    const std::string& metric,
    const std::string& tier_name)
{
    return metric + ":" + tier_name;
}

int64_t DownsamplingPipeline::nowMs() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

} // namespace themis
