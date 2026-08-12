/**
 * @file aggregates.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <cstdint>

namespace themis {

/**
 * @brief Time-Series Aggregates using Arrow Compute for SIMD performance
 * 
 * v1.2.0 Feature: 5-10x faster aggregations with Arrow's SIMD kernels
 * 
 * Benefits:
 * - SIMD vectorization (AVX2/AVX512)
 * - Zero-copy data processing
 * - Built-in time-series functions
 * 
 * Use Cases:
 * - Hypertable aggregations
 * - Real-time analytics dashboards
 * - Time-series downsampling
 */
class TimeSeriesAggregates {
public:
    struct Config {
        bool use_simd = true;           // Enable SIMD optimizations
        size_t batch_size = 10000;      // Batch size for processing
        int num_threads = 4;            // Parallel processing threads
    };
    
    enum class AggregateFunction {
        SUM,
        AVG,
        MIN,
        MAX,
        COUNT,
        STDDEV,
        VARIANCE,
        FIRST,
        LAST,
        PERCENTILE_50,  // Median
        PERCENTILE_95,
        PERCENTILE_99
    };
    
    struct TimeWindow {
        int64_t start_time;
        int64_t end_time;
        int64_t interval_seconds;  // Window size (e.g., 60 for 1-minute windows)
    };
    
    struct AggregateResult {
        std::vector<int64_t> timestamps;    // Window timestamps
        std::vector<double> values;          // Aggregated values
        size_t count = 0;                    // Number of windows
    };
    
    explicit TimeSeriesAggregates(const Config& config);
    ~TimeSeriesAggregates();
    
    TimeSeriesAggregates(const TimeSeriesAggregates&) = delete;
    TimeSeriesAggregates& operator=(const TimeSeriesAggregates&) = delete;
    TimeSeriesAggregates(TimeSeriesAggregates&&) = default;
    TimeSeriesAggregates& operator=(TimeSeriesAggregates&&) = default;
    
    /**
     * @brief Compute aggregates over time windows
     * 
     * @param timestamps Array of timestamps
     * @param values Array of values
     * @param count Number of data points
     * @param window Time window specification
     * @param func Aggregate function
     * @return Aggregated results per window
     */
    AggregateResult aggregate(
        const int64_t* timestamps,
        const double* values,
        size_t count,
        const TimeWindow& window,
        AggregateFunction func
    );
    
    /**
     * @brief Resample time series to different interval
     * 
     * Downsampling example: 1-second data → 1-minute aggregates
     */
    AggregateResult resample(
        const int64_t* timestamps,
        const double* values,
        size_t count,
        int64_t new_interval_seconds,
        AggregateFunction func
    );
    
    /**
     * @brief Rolling window aggregates
     * 
     * Example: 5-minute moving average
     */
    AggregateResult rollingWindow(
        const int64_t* timestamps,
        const double* values,
        size_t count,
        int64_t window_size_seconds,
        AggregateFunction func
    );
    
    /**
     * @brief Get configuration
     */
    const Config& getConfig() const { return config_; }

private:
    Config config_;
    
    /**
     * @brief Apply aggregate function to window
     */
    double applyAggregate(const double* values, size_t count, AggregateFunction func);
    
    /**
     * @brief Compute percentile
     */
    double computePercentile(const double* values, size_t count, double percentile);
};

} // namespace themis
