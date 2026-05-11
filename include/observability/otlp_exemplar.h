/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            otlp_exemplar.h                                    ║
  Version:         0.1.0                                              ║
  Last Modified:   2026-07-01 00:00:00                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: 🟡 Interface Header (Target: Q3 2026)                       ║
╚═════════════════════════════════════════════════════════════════════╝
 */
#pragma once
#include <string>
#include <vector>
#include &lt;map&gt;
#include <chrono>
#include <cstdint>

namespace themis { namespace observability {

struct TraceContext {
    std::string trace_id;
    std::string span_id;
    uint8_t trace_flags = 1;
};

struct MetricExemplar {
    TraceContext trace_context;
    std::map<std::string, std::string> filtered_attributes;
    double value;
    std::chrono::system_clock::time_point time_unix_nano;
};

enum class ExemplarReservoirStrategy {
    SIMPLE_FIXED_SIZE,
    ALIGNED_HISTOGRAM,
    TRACE_BASED,
};

struct ExemplarReservoirConfig {
    ExemplarReservoirStrategy strategy = ExemplarReservoirStrategy::ALIGNED_HISTOGRAM;
    size_t reservoir_size = 4;
    bool filter_sampled_only = false;
};

class IExemplarReservoir {
public:
    virtual ~IExemplarReservoir() = default;
    virtual void offer(const MetricExemplar& exemplar) = 0;
    virtual std::vector<MetricExemplar> collect() = 0;
    virtual size_t size() const = 0;
    virtual void reset() = 0;
};

class IExemplarSampler {
public:
    virtual ~IExemplarSampler() = default;
    virtual bool shouldSample(const TraceContext& ctx) const = 0;
    virtual void recordMeasurement(double value, const TraceContext& ctx,
                                   const std::map<std::string, std::string>& attrs) = 0;
};

}} // namespace themis::observability
