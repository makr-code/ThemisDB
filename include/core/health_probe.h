/*
 * ThemisDB | File: health_probe.h | Version: 0.1.0 | Last Modified: 2026-05-20 19:53:17
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 177
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): none
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

/**
 * @file health_probe.h
 * @brief Liveness, readiness, and startup health checking interfaces.
 *
 * Models the Kubernetes probe semantics (liveness, readiness, startupProbe)
 * extended with an aggregation registry for multi-component deployments.
 *
 * IHealthProbeRegistry::checkAll() produces an AggregateHealthReport whose
 * overall_status is UNHEALTHY if any component is UNHEALTHY, DEGRADED if any
 * component is DEGRADED (and none are UNHEALTHY), and HEALTHY otherwise.
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

/**
 * @brief Health state of a component or the whole system.
 */
enum class HealthStatus {
    HEALTHY,    ///< Component is fully operational.
    DEGRADED,   ///< Component is partially impaired but still serving traffic.
    UNHEALTHY,  ///< Component cannot serve traffic; restart or intervention needed.
    UNKNOWN,    ///< Status has not been determined yet (e.g., startup in progress).
};

// ---------------------------------------------------------------------------
// HealthCheckResult — result from a single probe check
// ---------------------------------------------------------------------------

/**
 * @brief Result produced by a single IHealthProbe check method.
 */
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

/**
 * @brief Aggregate health report produced by IHealthProbeRegistry::checkAll().
 *
 * `overall_status` follows the most-severe component status:
 *   UNHEALTHY > DEGRADED > UNKNOWN > HEALTHY.
 */
struct AggregateHealthReport {
    HealthStatus overall_status = HealthStatus::UNKNOWN;
    std::vector<HealthCheckResult> components;
    std::chrono::system_clock::time_point generated_at;

    /// Return `true` only if all components are HEALTHY.
    bool isHealthy() const { return overall_status == HealthStatus::HEALTHY; }

    /// Return `true` when traffic can be served (not UNHEALTHY).
    bool isReady() const { return overall_status != HealthStatus::UNHEALTHY; }
};

// ---------------------------------------------------------------------------
// IHealthProbe — single-component health probe interface
// ---------------------------------------------------------------------------

/**
 * @brief Pure-virtual interface for component-level health probes.
 *
 * Each subsystem (storage, cache, auth, etc.) registers one IHealthProbe with
 * the IHealthProbeRegistry.  The HTTP health endpoint delegates to
 * IHealthProbeRegistry::checkAll().
 *
 * ### Contract
 * - All three check methods must complete within the caller's timeout.
 * - Probe implementations must not throw; return UNHEALTHY with a message
 *   instead.
 * - `checkStartup()` returns HEALTHY once initialisation is complete and
 *   does not regress to UNHEALTHY or DEGRADED after that.
 */
class IHealthProbe {
public:
    virtual ~IHealthProbe() = default;

    /**
     * @brief Liveness check: is the component alive and not dead-locked?
     *
     * Maps to the Kubernetes `livenessProbe`.  A failed liveness check
     * signals that the process should be restarted.
     */
    virtual HealthCheckResult checkLiveness() = 0;

    /**
     * @brief Readiness check: is the component ready to handle traffic?
     *
     * Maps to the Kubernetes `readinessProbe`.  A failed readiness check
     * removes the pod from the service endpoint list.
     */
    virtual HealthCheckResult checkReadiness() = 0;

    /**
     * @brief Startup check: has initialisation completed?
     *
     * Maps to the Kubernetes `startupProbe`.  Returns UNKNOWN until
     * initialisation is done, then transitions to HEALTHY.
     */
    virtual HealthCheckResult checkStartup() = 0;

    /// Human-readable component name used as the key in AggregateHealthReport.
    virtual std::string componentName() const = 0;
};

// ---------------------------------------------------------------------------
// IHealthProbeRegistry — registry for multi-component health aggregation
// ---------------------------------------------------------------------------

/**
 * @brief Registry that aggregates IHealthProbe instances for a deployment.
 *
 * ### Thread safety
 * All methods must be safe to call concurrently.
 */
class IHealthProbeRegistry {
public:
    virtual ~IHealthProbeRegistry() = default;

    /**
     * @brief Register a health probe.
     *
     * @return `false` if a probe with the same `componentName()` is already registered.
     */
    virtual bool registerProbe(std::shared_ptr<IHealthProbe> probe) = 0;

    /**
     * @brief Unregister a probe by component name.
     *
     * @return `false` if no probe with @p component_name was found.
     */
    virtual bool unregisterProbe(const std::string& component_name) = 0;

    /**
     * @brief Run all registered probes and return the aggregate report.
     */
    virtual AggregateHealthReport checkAll() = 0;

    /**
     * @brief Run the readiness check for a specific component.
     *
     * Returns a result with status UNKNOWN if @p component_name is not registered.
     */
    virtual HealthCheckResult checkComponent(const std::string& component_name) = 0;
};

} // namespace core
} // namespace themis
