/**
 * @file lifecycle.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <string>

namespace themis {
namespace core {
namespace concerns {

/**
 * @brief Result of a health or readiness probe for a single concern.
 *
 * Returned by isHealthy() / isReady() on each concern interface and
 * aggregated by ConcernsContext::healthCheck() and
 * ConcernsContext::readinessCheck().
 */
struct ProbeResult {
    bool ok = true;           ///< true = healthy / ready
    std::string message;      ///< Human-readable status detail

    /// Convenience: construct a healthy result.
    static ProbeResult healthy(const std::string& msg = "ok") {
        return {true, msg};
    }

    /// Convenience: construct an unhealthy result.
    static ProbeResult unhealthy(const std::string& msg) {
        return {false, msg};
    }
};

/**
 * @brief Aggregated health/readiness status for all core concerns.
 *
 * Returned by ConcernsContext::healthCheck() and
 * ConcernsContext::readinessCheck(). A single unhealthy concern marks
 * the whole context as unhealthy so that Kubernetes / load-balancer
 * probes can react accordingly.
 *
 * Fields default to healthy for optional concerns so legacy contexts that do
 * not wire every concern still report meaningful aggregate status.
 */
struct HealthStatus {
    ProbeResult logger;
    ProbeResult tracer;
    ProbeResult metrics;
    ProbeResult cache;
    ProbeResult secrets = ProbeResult::healthy();
        ProbeResult circuit_breaker = ProbeResult::healthy();
        ProbeResult featureFlags = ProbeResult::healthy();
    ProbeResult auditLog = ProbeResult::healthy();

    /// @return true only when every concern reports healthy/ready.
    bool isHealthy() const noexcept {
        return logger.ok && tracer.ok && metrics.ok && cache.ok &&
               secrets.ok && circuit_breaker.ok && featureFlags.ok &&
               auditLog.ok;
    }
};

} // namespace concerns
} // namespace core
} // namespace themis
