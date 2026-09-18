/**
 * @file health_probe.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <chrono>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace themis {
namespace core {

// ---------------------------------------------------------------------------
// HealthStatus — health state enum aligned with Kubernetes probe semantics
// ---------------------------------------------------------------------------

enum class HealthStatus {
    HEALTHY,    ///< Component is fully operational.
    DEGRADED,   ///< Component is partially impaired but still serving traffic.
    UNHEALTHY,  ///< Component cannot serve traffic; restart or intervention needed.
    UNKNOWN,    ///< Status has not been determined yet (e.g., startup in progress).
};

// ---------------------------------------------------------------------------
// HealthCheckResult — result from a single probe check
// ---------------------------------------------------------------------------

struct HealthCheckResult {
    std::string  component_name;
    HealthStatus status         = HealthStatus::UNKNOWN;
    std::string  message;
    double       latency_ms     = 0.0;
    std::chrono::system_clock::time_point checked_at;
    std::map<std::string, std::string> details; ///< Arbitrary key-value diagnostics.
};

// ---------------------------------------------------------------------------
// AggregateHealthReport — rolled-up view of all registered probes
// ---------------------------------------------------------------------------

struct AggregateHealthReport {
    HealthStatus overall_status = HealthStatus::UNKNOWN;
    std::vector<HealthCheckResult> components;
    std::chrono::system_clock::time_point generated_at;

    bool isHealthy() const { return overall_status == HealthStatus::HEALTHY; }

    bool isReady() const { return overall_status != HealthStatus::UNHEALTHY; }
};

// ---------------------------------------------------------------------------
// IHealthProbe — single-component health probe interface
// ---------------------------------------------------------------------------

class IHealthProbe {
public:
    /**
     * @brief IHealth Probe.
     * @return Return value.
     */
    virtual ~IHealthProbe() = default;

    /**
     * @brief Check Liveness.
     * @return Return value.
     */
    virtual HealthCheckResult checkLiveness() = 0;

    /**
     * @brief Check Readiness.
     * @return Return value.
     */
    virtual HealthCheckResult checkReadiness() = 0;

    /**
     * @brief Check Startup.
     * @return Return value.
     */
    virtual HealthCheckResult checkStartup() = 0;

    /**
     * @brief Component Name.
     * @return Return value.
     */
    virtual std::string componentName() const = 0;
};

// ---------------------------------------------------------------------------
// IHealthProbeRegistry — registry for multi-component health aggregation
// ---------------------------------------------------------------------------

class IHealthProbeRegistry {
public:
    /**
     * @brief IHealth Probe Registry.
     * @return Return value.
     */
    virtual ~IHealthProbeRegistry() = default;

    /**
     * @brief Register Probe.
     * @param[in] probe Input parameter.
     * @return True when the operation succeeds.
     */
    virtual bool registerProbe(std::shared_ptr<IHealthProbe> probe) = 0;

    /**
     * @brief Unregister Probe.
     * @param[in] component_name Name of the component.
     * @return True when the operation succeeds.
     */
    virtual bool unregisterProbe(const std::string& component_name) = 0;

    /**
     * @brief Check All.
     * @return Return value.
     */
    virtual AggregateHealthReport checkAll() = 0;

    /**
     * @brief Check Component.
     * @param[in] component_name Name of the component.
     * @return Return value.
     */
    virtual HealthCheckResult checkComponent(const std::string& component_name) = 0;
};

} // namespace core
} // namespace themis
