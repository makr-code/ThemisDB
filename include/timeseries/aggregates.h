/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            aggregates.h                                       ║
  Version:         0.0.46                                             ║
  Last Modified:   2026-04-15 18:05:58                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     154                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
