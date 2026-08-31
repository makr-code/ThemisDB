/**
 * @file compression_selector.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "timeseries/compression_selector.h"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace themis {

// ============================================================================
// profileSeries
// ============================================================================

SeriesProfile profileSeries(const std::vector<TSStore::DataPoint>& points) {
    SeriesProfile p;
    p.sample_count = points.size();

    if (points.size() < 2) {
        return p;
    }

    // ----- value variance -----------------------------------------------
    {
        double sum = 0.0;
        for (const auto& dp : points) sum += dp.value;
        double mean = sum / static_cast<double>(points.size());

        double var = 0.0;
        for (const auto& dp : points) {
            double d = dp.value - mean;
            var += d * d;
        }
        p.value_variance = var / static_cast<double>(points.size());
    }

    // ----- timestamp deltas ---------------------------------------------
    {
        std::vector<int64_t> deltas;
        deltas.reserve(points.size() - 1);
        for (size_t i = 1; i < points.size(); ++i) {
            deltas.push_back(points[i].timestamp_ms - points[i - 1].timestamp_ms);
        }

        // timestamp regularity: fraction of deltas equal to the modal delta
        if (!deltas.empty()) {
            size_t  modal_count = 0;
            for (auto d : deltas) {
                size_t cnt = static_cast<size_t>(
                    std::count(deltas.begin(), deltas.end(), d));
                if (cnt > modal_count) { modal_count = cnt; }
            }
            p.timestamp_regularity =
                static_cast<double>(modal_count) / static_cast<double>(deltas.size());
        }

        // dod_mean_abs: mean absolute delta-of-delta
        if (deltas.size() >= 2) {
            double dod_sum = 0.0;
            for (size_t i = 1; i < deltas.size(); ++i) {
                dod_sum += std::abs(static_cast<double>(deltas[i] - deltas[i - 1]));
            }
            p.dod_mean_abs = dod_sum / static_cast<double>(deltas.size() - 1);
        }
    }

    // ----- run-length ratio ---------------------------------------------
    {
        size_t runs = 0;
        for (size_t i = 1; i < points.size(); ++i) {
            if (points[i].value == points[i - 1].value) ++runs;
        }
        p.run_length_ratio =
            static_cast<double>(runs) / static_cast<double>(points.size() - 1);
    }

    return p;
}

// ============================================================================
// HeuristicCompressionSelector
// ============================================================================

HeuristicCompressionSelector::HeuristicCompressionSelector()
    : config_{} {}

HeuristicCompressionSelector::HeuristicCompressionSelector(Config cfg)
    : config_(cfg) {}

CompressionStrategy HeuristicCompressionSelector::select(
    const SeriesProfile& profile) const {

    if (profile.sample_count < config_.min_samples) {
        return CompressionStrategy::None;
    }

    if (profile.run_length_ratio >= config_.rle_run_ratio_threshold) {
        return CompressionStrategy::RLE;
    }

    if (profile.dod_mean_abs <= config_.dod_mean_abs_threshold &&
        profile.timestamp_regularity >= config_.regularity_threshold) {
        return CompressionStrategy::DeltaOfDelta;
    }

    return CompressionStrategy::Gorilla;
}

CompressionStrategy HeuristicCompressionSelector::selectForPoints(
    const std::vector<TSStore::DataPoint>& points) const {

    return select(profileSeries(points));
}

// ============================================================================
// PerSeriesCompressionRegistry
// ============================================================================

PerSeriesCompressionRegistry::PerSeriesCompressionRegistry()
    : selector_(std::make_unique<HeuristicCompressionSelector>()) {}

PerSeriesCompressionRegistry::PerSeriesCompressionRegistry(
    std::unique_ptr<ICompressionSelector> selector)
    : selector_(std::move(selector)) {}

PerSeriesCompressionRegistry::SeriesKey
PerSeriesCompressionRegistry::makeKey(const std::string& metric,
                                       const std::string& entity) {
    return metric + ':' + entity;
}

void PerSeriesCompressionRegistry::setSelector(
    std::unique_ptr<ICompressionSelector> selector) {
    selector_ = std::move(selector);
    cached_.clear();
}

CompressionStrategy PerSeriesCompressionRegistry::strategyFor(
    const std::string& metric,
    const std::string& entity,
    const std::vector<TSStore::DataPoint>& sample) {

    const SeriesKey key = makeKey(metric, entity);

    // 1. Pinned entries take absolute priority
    {
        auto it = pinned_.find(key);
        if (it != pinned_.end()) return it->second;
    }

    // 2. Cached entries
    {
        auto it = cached_.find(key);
        if (it != cached_.end()) return it->second;
    }

    // 3. Fresh selection
    CompressionStrategy strategy = selector_
        ? selector_->selectForPoints(sample)
        : CompressionStrategy::Gorilla;

    cached_[key] = strategy;
    return strategy;
}

void PerSeriesCompressionRegistry::pinStrategy(
    const std::string& metric,
    const std::string& entity,
    CompressionStrategy strategy) {

    pinned_[makeKey(metric, entity)] = strategy;
}

void PerSeriesCompressionRegistry::clearPin(
    const std::string& metric,
    const std::string& entity) {

    pinned_.erase(makeKey(metric, entity));
}

void PerSeriesCompressionRegistry::clearCache() {
    cached_.clear();
}

size_t PerSeriesCompressionRegistry::registrySize() const {
    return pinned_.size() + cached_.size();
}

void PerSeriesCompressionRegistry::clear() {
    pinned_.clear();
    cached_.clear();
}

} // namespace themis
