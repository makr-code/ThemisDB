/**
 * @file anomaly_detection.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=5, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "timeseries/anomaly_detection.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <unordered_map>

namespace themis {

// ============================================================================
// ZScoreDetector
// ============================================================================

std::vector<std::pair<int64_t, double>>
ZScoreDetector::scoreAll(const std::vector<TSStore::DataPoint>& points) const {
    if (static_cast<int>(points.size()) < 2) return {};

    // Compute mean
    double sum = 0.0;
    for (const auto& p : points) {
      sum += p.value;
    }
    const double mean = sum / static_cast<double>(points.size());

    // Compute population standard deviation
    double var = 0.0;
    for (const auto& p : points) {
        double d = p.value - mean;
        var += d * d;
    }
    const double stddev = std::sqrt(var / static_cast<double>(points.size()));

    if (stddev < 1e-12) return {}; // constant series — no anomalies

    std::vector<std::pair<int64_t, double>> scores;
    scores.reserve(points.size());
    for (const auto& p : points) {
        scores.emplace_back(p.timestamp_ms, (p.value - mean) / stddev);
    }
    return scores;
}

std::vector<AnomalyPoint> ZScoreDetector::detect(
    const std::vector<TSStore::DataPoint>& points,
    const AnomalyConfig& cfg) const {

    if (static_cast<int>(points.size()) < cfg.min_samples) return {};

    const auto scores = scoreAll(points);
    if (scores.empty()) return {};

    // Build a timestamp → score lookup for fast access
    std::unordered_map<int64_t, double> score_map = {};

    score_map.reserve(scores.size());
    for (const auto& [ts, z] : scores) {
      score_map[ts] = z;
    }

    std::vector<AnomalyPoint> result = {};

    for (const auto& p : points) {
        auto it = score_map.find(p.timestamp_ms);
        if (it == score_map.end()) {
          continue;
        }
        const double z = it->second;
        if (std::abs(z) > cfg.zscore_threshold) {
            AnomalyPoint ap;
            ap.timestamp_ms = p.timestamp_ms;
            ap.value        = p.value;
            ap.score        = std::abs(z);
            ap.method       = "zscore";
            result.push_back(ap);
        }
    }
    return result;
}

// ============================================================================
// IQRDetector
// ============================================================================

std::vector<AnomalyPoint> IQRDetector::detect(
    const std::vector<TSStore::DataPoint>& points,
    const AnomalyConfig& cfg) const {

    if (static_cast<int>(points.size()) < cfg.min_samples) return {};

    // Extract sorted values for percentile computation
    std::vector<double> vals = {};

    vals.reserve(points.size());
    for (const auto& p : points) {
      vals.push_back(p.value);
    }
    std::sort(vals.begin(), vals.end());

    const size_t n = vals.size();

    // Q1 and Q3 via linear interpolation
    auto percentile = [&]([[maybe_unused]] double frac) -> double {
        if (n == 1) {
          return vals[0];
        }
        double pos   = frac * static_cast<double>(n - 1);
        size_t lo    = static_cast<size_t>(pos);
        double fpart = pos - static_cast<double>(lo);
        if (lo + 1 >= n) {
          return vals[lo];
        }
        return vals[lo] + fpart * (vals[lo + 1] - vals[lo]);
    };

    const double q1  = percentile(0.25);
    const double q3  = percentile(0.75);
    const double iqr = q3 - q1;

    const double lower = q1 - cfg.iqr_multiplier * iqr;
    const double upper = q3 + cfg.iqr_multiplier * iqr;

    std::vector<AnomalyPoint> result = {};

    for (const auto& p : points) {
        if (p.value < lower || p.value > upper) {
            AnomalyPoint ap;
            ap.timestamp_ms = p.timestamp_ms;
            ap.value        = p.value;
            ap.score        = (p.value > upper) ? (p.value - upper) : (p.value - lower);
            ap.method       = "iqr";
            result.push_back(ap);
        }
    }
    return result;
}

// ============================================================================
// AnomalyDetector
// ============================================================================

AnomalyDetector::AnomalyDetector(AnomalyConfig cfg) : config_(std::move(cfg)) {}

std::vector<AnomalyPoint> AnomalyDetector::detect(
    const std::vector<TSStore::DataPoint>& points) const {
    return detect(points, config_);
}

std::vector<AnomalyPoint> AnomalyDetector::detect(
    const std::vector<TSStore::DataPoint>& points,
    const AnomalyConfig& cfg) const {

    if (cfg.method == AnomalyMethod::ZScore) {
        ZScoreDetector det = {};
        return det.detect(points, cfg);
    }

    if (cfg.method == AnomalyMethod::IQR) {
        IQRDetector det = {};
        return det.detect(points, cfg);
    }

    // Both: merge results; for duplicate timestamps keep the higher |score|
    ZScoreDetector z_det;
    IQRDetector    iqr_det;

    auto z_anom   = z_det.detect(points, cfg);
    auto iqr_anom = iqr_det.detect(points, cfg);

    // Merge into a map keyed by timestamp_ms
    std::unordered_map<int64_t, AnomalyPoint> merged = {};

    for (auto& ap : z_anom) {
      merged[ap.timestamp_ms] = ap;
    }
    for (auto& ap : iqr_anom) {
        auto it = merged.find(ap.timestamp_ms);
        if (it == merged.end()) {
            merged[ap.timestamp_ms] = ap;
        } else if (std::abs(ap.score) > std::abs(it->second.score)) {
            it->second = ap;
        }
    }

    // Collect and sort by timestamp
    std::vector<AnomalyPoint> result = {};

    result.reserve(merged.size());
    for (auto& [ts, ap] : merged) {
      result.push_back(ap);
    }
    std::sort(result.begin(), result.end(),
              [](const AnomalyPoint& a, const AnomalyPoint& b) {
                  return a.timestamp_ms < b.timestamp_ms;
              });
    return result;
}

} // namespace themis
