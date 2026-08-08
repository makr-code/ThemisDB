/**
 * @file otlp_exemplar.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: otlp_exemplar.h | Version: 0.1.0 | Last Modified: 2026-05-20 17:13:04
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 60
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): #5056 docs(observability): update... (2026-05-13)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once
#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <cstdint>

namespace themis { namespace observability {

/**
 * @brief OTLP-specific trace context for metric exemplars.
 *
 * This struct carries trace information for OpenTelemetry metric exemplars.
 * It includes trace_id, span_id, and W3C trace flags, but NOT request_id.
 * For logging contexts that need request_id, use core::concerns::TraceContext instead.
 *
 * @note This struct is named OTLPTraceContext to avoid naming collision with
 *       core::concerns::TraceContext, which has a different field set.
 */
struct OTLPTraceContext {
    std::string trace_id;
    std::string span_id;
    uint8_t trace_flags = 1;
};

struct MetricExemplar {
    OTLPTraceContext trace_context;
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
    virtual bool shouldSample(const OTLPTraceContext& ctx) const = 0;
    virtual void recordMeasurement(double value, const OTLPTraceContext& ctx,
                                   const std::map<std::string, std::string>& attrs) = 0;
};

}} // namespace themis::observability
