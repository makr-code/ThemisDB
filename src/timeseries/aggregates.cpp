/**
 * @file aggregates.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "timeseries/aggregates.h"
#include "utils/logger.h"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <map>

// Arrow Compute includes (conditional)
#ifdef ARROW_ENABLED
    #include <arrow/api.h>
    #include <arrow/compute/api.h>
#endif

namespace themis {

TimeSeriesAggregates::TimeSeriesAggregates(const Config& config)
    : config_(config) {
    
    THEMIS_INFO("TimeSeriesAggregates created: simd={}, batch_size={}, threads={}",
                config_.use_simd, config_.batch_size, config_.num_threads);
}

TimeSeriesAggregates::~TimeSeriesAggregates() = default;

TimeSeriesAggregates::AggregateResult TimeSeriesAggregates::aggregate(
    const int64_t* timestamps,
    const double* values,
    size_t count,
    const TimeWindow& window,
    AggregateFunction func
) {
    AggregateResult result = {};
    
    if (count == 0) {
        return result;
    }
    
    // Group data points into time windows
    std::map<int64_t, std::vector<double>> window_data;
    
    for (size_t i = 0; i < count; ++i) {
        // Calculate window start time
        int64_t window_start = (timestamps[i] / window.interval_seconds) * window.interval_seconds;
        
        if (window_start >= window.start_time && window_start < window.end_time) {
            window_data[window_start].push_back(values[i]);
        }
    }
    
    // Compute aggregates for each window
    for (const auto& [window_ts, window_values] : window_data) {
        result.timestamps.push_back(window_ts);
        result.values.push_back(
            applyAggregate(window_values.data(), window_values.size(), func)
        );
    }
    
    result.count = result.timestamps.size();
    
    THEMIS_INFO("Aggregated {} data points into {} windows",
                count, result.count);
    
    return result;
}

TimeSeriesAggregates::AggregateResult TimeSeriesAggregates::resample(
    const int64_t* timestamps,
    const double* values,
    size_t count,
    int64_t new_interval_seconds,
    AggregateFunction func
) {
    // Use aggregate with auto-detected time range
    if (count == 0) {
        return AggregateResult{};
    }
    
    int64_t min_time = *std::min_element(timestamps, timestamps + count);
    int64_t max_time = *std::max_element(timestamps, timestamps + count);
    
    TimeWindow window;
    window.start_time = min_time;
    window.end_time = max_time + new_interval_seconds;
    window.interval_seconds = new_interval_seconds;
    
    auto result = aggregate(timestamps, values, count, window, func);
    
    THEMIS_INFO("Resampled {} points to {} interval (func={})",
                count, new_interval_seconds, static_cast<int>(func));
    
    return result;
}

TimeSeriesAggregates::AggregateResult TimeSeriesAggregates::rollingWindow(
    const int64_t* timestamps,
    const double* values,
    size_t count,
    int64_t window_size_seconds,
    AggregateFunction func
) {
    AggregateResult result = {};
    
    if (count == 0) {
        return result;
    }
    
    // For each point, compute aggregate over preceding window
    for (size_t i = 0; i < count; ++i) {
        int64_t window_start = timestamps[i] - window_size_seconds;
        
        // Collect values in window
        std::vector<double> window_values = {};

        for (size_t j = 0; j <= i; ++j) {
            if (timestamps[j] >= window_start && timestamps[j] <= timestamps[i]) {
                window_values.push_back(values[j]);
            }
        }
        
        if (!window_values.empty()) {
            result.timestamps.push_back(timestamps[i]);
            result.values.push_back(
                applyAggregate(window_values.data(), window_values.size(), func)
            );
        }
    }
    
    result.count = result.timestamps.size();
    
    THEMIS_INFO("Rolling window: {} points, window={}s",
                result.count, window_size_seconds);
    
    return result;
}

double TimeSeriesAggregates::applyAggregate(
    const double* values,
    size_t count,
    AggregateFunction func
) {
    if (count == 0) {
        return 0.0;
    }
    
    switch (func) {
        case AggregateFunction::SUM:
            return std::accumulate(values, values + count, 0.0);
        
        case AggregateFunction::AVG: {
            double sum = std::accumulate(values, values + count, 0.0);
            return sum / count;
        }
        
        case AggregateFunction::MIN:
            return *std::min_element(values, values + count);
        
        case AggregateFunction::MAX:
            return *std::max_element(values, values + count);
        
        case AggregateFunction::COUNT:
            return static_cast<double>(count);
        
        case AggregateFunction::STDDEV: {
            double mean = std::accumulate(values, values + count, 0.0) / count;
            double variance = 0.0;
            for (size_t i = 0; i < count; ++i) {
                double diff = values[i] - mean;
                variance += diff * diff;
            }
            return std::sqrt(variance / count);
        }
        
        case AggregateFunction::VARIANCE: {
            double mean = std::accumulate(values, values + count, 0.0) / count;
            double variance = 0.0;
            for (size_t i = 0; i < count; ++i) {
                double diff = values[i] - mean;
                variance += diff * diff;
            }
            return variance / count;
        }
        
        case AggregateFunction::FIRST:
            return values[0];
        
        case AggregateFunction::LAST:
            return values[count - 1];
        
        case AggregateFunction::PERCENTILE_50:
            return computePercentile(values, count, 0.50);
        
        case AggregateFunction::PERCENTILE_95:
            return computePercentile(values, count, 0.95);
        
        case AggregateFunction::PERCENTILE_99:
            return computePercentile(values, count, 0.99);
        
        default:
            return 0.0;
    }
}

double TimeSeriesAggregates::computePercentile(
    const double* values,
    size_t count,
    double percentile
) {
    if (count == 0) {
        return 0.0;
    }
    
    // Copy and sort
    std::vector<double> sorted(values, values + count);
    std::sort(sorted.begin(), sorted.end());
    
    // Linear interpolation
    double index = percentile * (count - 1);
    size_t lower = static_cast<size_t>(std::floor(index));
    size_t upper = static_cast<size_t>(std::ceil(index));
    
    if (lower == upper) {
        return sorted[lower];
    }
    
    double weight = index - lower;
    return sorted[lower] * (1.0 - weight) + sorted[upper] * weight;
}

} // namespace themis

