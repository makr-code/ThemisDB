/**
 * @file workload_predictor.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "performance/workload_predictor.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <sstream>

namespace themis {
namespace performance {

// ---------------------------------------------------------------------------
// Constructors
// ---------------------------------------------------------------------------

WorkloadPredictor::WorkloadPredictor()
    : config_(Config{}) {}

WorkloadPredictor::WorkloadPredictor(const Config& config)
    : config_(config) {}

// ---------------------------------------------------------------------------
// record
// ---------------------------------------------------------------------------

void WorkloadPredictor::record(const WorkloadSnapshot& snapshot) {
    std::unique_lock<std::shared_mutex> lk(mutex_);
    history_.push_back(snapshot);
    // Evict oldest observation when window is full
    while (static_cast<int>(history_.size()) > config_.history_window) {
        history_.pop_front();
    }
}

// ---------------------------------------------------------------------------
// predict
// ---------------------------------------------------------------------------

WorkloadForecast WorkloadPredictor::predict(uint64_t horizon_us) const {
    std::unique_lock<std::shared_mutex> lk(mutex_);

    WorkloadForecast result{};

    if (history_.empty()) {
        return result;
    }

    result.forecast_timestamp_us = history_.back().timestamp_us + horizon_us;

    if (static_cast<int>(history_.size()) < 2) {
        // Single observation: return it with zero confidence
        const auto& s = history_.back();
        result.predicted_qps               = s.qps;
        result.predicted_cpu_utilization   = s.cpu_utilization;
        result.predicted_memory_utilization = s.memory_utilization;
        result.predicted_avg_latency_us    = s.avg_latency_us;
        result.confidence                  = 0.0;
        return result;
    }

    // Build per-metric value series from the history window
    const size_t n = history_.size();
    std::vector<double> qps_vals(n), cpu_vals(n), mem_vals(n), lat_vals(n);
    for (size_t i = 0; i < n; ++i) {
        qps_vals[i] = history_[i].qps;
        cpu_vals[i] = history_[i].cpu_utilization;
        mem_vals[i] = history_[i].memory_utilization;
        lat_vals[i] = history_[i].avg_latency_us;
    }

    // EMA-smoothed last value
    const double ema_qps = compute_ema(qps_vals);
    const double ema_cpu = compute_ema(cpu_vals);
    const double ema_mem = compute_ema(mem_vals);
    const double ema_lat = compute_ema(lat_vals);

    // Linear regression slope and intercept
    const auto [qps_slope, qps_intercept] = linear_regression(qps_vals);
    const auto [cpu_slope, cpu_intercept] = linear_regression(cpu_vals);
    const auto [mem_slope, mem_intercept] = linear_regression(mem_vals);
    const auto [lat_slope, lat_intercept] = linear_regression(lat_vals);

    // Determine the number of observation-intervals represented by the horizon.
    // We use the average inter-sample interval derived from the history.
    double avg_interval_us = 0.0;
    if (n >= 2) {
        avg_interval_us = static_cast<double>(
            history_.back().timestamp_us - history_.front().timestamp_us) /
            static_cast<double>(n - 1);
    }

    // Steps into the future (fractional is fine for the linear projection).
    const double steps_ahead = (avg_interval_us > 0.0)
        ? (static_cast<double>(horizon_us) / avg_interval_us)
        : 1.0;

    // Project: blend regression trend with EMA smoothed baseline.
    // Using a 50/50 blend reduces over-extrapolation for short windows.
    const double regression_qps = qps_intercept + qps_slope * (static_cast<double>(n) - 1.0 + steps_ahead);
    const double regression_cpu = cpu_intercept + cpu_slope * (static_cast<double>(n) - 1.0 + steps_ahead);
    const double regression_mem = mem_intercept + mem_slope * (static_cast<double>(n) - 1.0 + steps_ahead);
    const double regression_lat = lat_intercept + lat_slope * (static_cast<double>(n) - 1.0 + steps_ahead);

    result.predicted_qps               = clamp(0.5 * ema_qps + 0.5 * regression_qps, 0.0, 1e9);
    result.predicted_cpu_utilization   = clamp(0.5 * ema_cpu + 0.5 * regression_cpu, 0.0, 1.0);
    result.predicted_memory_utilization = clamp(0.5 * ema_mem + 0.5 * regression_mem, 0.0, 1.0);
    result.predicted_avg_latency_us    = clamp(0.5 * ema_lat + 0.5 * regression_lat, 0.0, 1e12);

    // Confidence: take the minimum per-metric confidence (weakest link)
    const double conf_qps = compute_confidence(qps_vals);
    const double conf_cpu = compute_confidence(cpu_vals);
    const double conf_mem = compute_confidence(mem_vals);
    const double conf_lat = compute_confidence(lat_vals);
    result.confidence = std::min({conf_qps, conf_cpu, conf_mem, conf_lat});

    return result;
}

// ---------------------------------------------------------------------------
// recommend_scaling
// ---------------------------------------------------------------------------

ScaleRecommendation WorkloadPredictor::recommend_scaling(
    uint32_t current_thread_pool_size,
    uint64_t current_cache_size_mb) const
{
    // Predict 30 s into the future – a practical planning horizon
    constexpr uint64_t kHorizon30s = 30 * 1'000'000;
    const WorkloadForecast forecast = predict(kHorizon30s);

    ScaleRecommendation rec{};
    rec.recommended_thread_pool_size = current_thread_pool_size;
    rec.recommended_cache_size_mb    = current_cache_size_mb;
    rec.confidence                   = forecast.confidence;

    if (forecast.confidence < 0.1) {
        rec.direction = ScaleDirection::NONE;
        rec.reason    = "Insufficient data for a reliable recommendation";
        return rec;
    }

    // Determine dominant utilization signal
    const double util = std::max(forecast.predicted_cpu_utilization,
                                 forecast.predicted_memory_utilization);

    std::ostringstream reason_stream = {};

    if (util >= config_.scale_up_threshold) {
        rec.direction = ScaleDirection::UP;

        // Scale thread pool: increase by 50%, capped at max
        const uint32_t new_tp = static_cast<uint32_t>(
            std::min(static_cast<double>(config_.max_thread_pool_size),
                     std::ceil(static_cast<double>(current_thread_pool_size) * 1.5)));
        rec.recommended_thread_pool_size = std::max(new_tp, config_.min_thread_pool_size);

        // Scale cache: increase by 50%, capped at max
        const uint64_t new_cache = static_cast<uint64_t>(
            std::min(static_cast<double>(config_.max_cache_size_mb),
                     std::ceil(static_cast<double>(current_cache_size_mb) * 1.5)));
        rec.recommended_cache_size_mb = std::max(new_cache, config_.min_cache_size_mb);

        reason_stream << "Predicted utilization " << static_cast<int>(util * 100.0)
                      << "% exceeds scale-up threshold "
                      << static_cast<int>(config_.scale_up_threshold * 100.0) << "%";

    } else if (util <= config_.scale_down_threshold) {
        rec.direction = ScaleDirection::DOWN;

        // Scale thread pool: decrease by 25%, floored at min
        const uint32_t new_tp = static_cast<uint32_t>(
            std::max(static_cast<double>(config_.min_thread_pool_size),
                     std::floor(static_cast<double>(current_thread_pool_size) * 0.75)));
        rec.recommended_thread_pool_size = new_tp;

        // Scale cache: decrease by 25%, floored at min
        const uint64_t new_cache = static_cast<uint64_t>(
            std::max(static_cast<double>(config_.min_cache_size_mb),
                     std::floor(static_cast<double>(current_cache_size_mb) * 0.75)));
        rec.recommended_cache_size_mb = new_cache;

        reason_stream << "Predicted utilization " << static_cast<int>(util * 100.0)
                      << "% is below scale-down threshold "
                      << static_cast<int>(config_.scale_down_threshold * 100.0) << "%";

    } else {
        rec.direction = ScaleDirection::NONE;
        reason_stream << "Predicted utilization " << static_cast<int>(util * 100.0)
                      << "% is within acceptable range";
    }

    rec.reason = reason_stream.str();
    return rec;
}

// ---------------------------------------------------------------------------
// observation_count / reset
// ---------------------------------------------------------------------------

size_t WorkloadPredictor::observation_count() const noexcept {
    std::shared_lock<std::shared_mutex> lk(mutex_);
    return static_cast<int>(history_.size());
}

void WorkloadPredictor::reset() noexcept {
    std::unique_lock<std::shared_mutex> lk(mutex_);
    history_.clear();
}

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

double WorkloadPredictor::compute_ema(const std::vector<double>& values) const noexcept {
    if (values.empty()) {
      return 0.0;
    }
    double ema = values[0];
    for (size_t i = 1; i < values.size(); ++i) {
        ema = config_.ema_alpha * values[i] + (1.0 - config_.ema_alpha) * ema;
    }
    return ema;
}

std::pair<double, double> WorkloadPredictor::linear_regression(
    const std::vector<double>& values) const noexcept
{
    const size_t n = values.size();
    if (n < 2) {
        return {0.0, values.empty() ? 0.0 : values.back()};
    }

    // x = 0, 1, 2, …, n-1
    const double dn     = static_cast<double>(n);
    const double sum_x  = dn * (dn - 1.0) / 2.0;
    const double sum_x2 = dn * (dn - 1.0) * (2.0 * dn - 1.0) / 6.0;
    const double sum_y  = std::accumulate(values.begin(), values.end(), 0.0);

    double sum_xy = 0.0;
    for (size_t i = 0; i < n; ++i) {
        sum_xy += static_cast<double>(i) * values[i];
    }

    const double denom = dn * sum_x2 - sum_x * sum_x;
    if (std::abs(denom) < 1e-12) {
        return {0.0, sum_y / dn};
    }

    const double slope     = (dn * sum_xy - sum_x * sum_y) / denom;
    const double intercept = (sum_y - slope * sum_x) / dn;
    return {slope, intercept};
}

double WorkloadPredictor::compute_confidence(const std::vector<double>& values) const noexcept {
    const size_t n = values.size();
    if (n < 2) {
      return 0.0;
    }

    const double mean = std::accumulate(values.begin(), values.end(), 0.0) / static_cast<double>(n);
    if (std::abs(mean) < 1e-12) {
        // Near-zero mean: treat as perfectly stable
        return 1.0;
    }

    double variance = 0.0;
    for (const double v : values) {
        const double diff = v - mean;
        variance += diff * diff;
    }
    variance /= static_cast<double>(n);

    const double stddev = std::sqrt(variance);
    const double cv     = stddev / std::abs(mean); // Coefficient of variation

    // Map CV → confidence: CV=0 → 1.0, CV=1 → 0.0, CV>1 clamped to 0.
    return clamp(1.0 - cv, 0.0, 1.0);
}

double WorkloadPredictor::clamp(double v, double lo, double hi) noexcept {
    if (v < lo) {
      return lo;
    }
    if (v > hi) {
      return hi;
    }
    return v;
}

} // namespace performance
} // namespace themis

