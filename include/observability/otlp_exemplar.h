/**
 * @file otlp_exemplar.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
 * This struct represents trace context information for OpenTelemetry Protocol (OTLP)
 * metric exemplars. It should not be confused with core::concerns::TraceContext,
 * which is used for log correlation and includes a request_id field.
 *
 * @see core::concerns::TraceContext for log/request correlation
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

/** @brief I exemplar reservoir. */
class IExemplarReservoir {
public:
    /**
     * @brief TBD: Describe ~IExemplarReservoir.
     * @return Return value.
     */
    virtual ~IExemplarReservoir() = default;
    /**
     * @brief TBD: Describe offer.
     * @param[in] exemplar Input parameter.
     */
    virtual void offer(const MetricExemplar& exemplar) = 0;
    /**
     * @brief TBD: Describe collect.
     * @return Return value.
     */
    virtual std::vector<MetricExemplar> collect() = 0;
    /**
     * @brief TBD: Describe size.
     * @return Return value.
     */
    virtual size_t size() const = 0;
    /**
     * @brief TBD: Describe reset.
     */
    virtual void reset() = 0;
};

/** @brief I exemplar sampler. */
class IExemplarSampler {
public:
    /**
     * @brief TBD: Describe ~IExemplarSampler.
     * @return Return value.
     */
    virtual ~IExemplarSampler() = default;
    /**
     * @brief TBD: Describe shouldSample.
     * @param[in] ctx Input parameter.
     * @return True on success.
     */
    virtual bool shouldSample(const OTLPTraceContext& ctx) const = 0;
    virtual void recordMeasurement(double value, const OTLPTraceContext& ctx,
                                   const std::map<std::string, std::string>& attrs) = 0;
};

}} // namespace themis::observability
