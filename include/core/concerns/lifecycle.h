/*
 * ThemisDB | File: lifecycle.h | Version: 0.0.47 | Last Modified: 2026-05-20 17:13:04
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 67
 * Open Issues: TODOs=1, Stubs=1, Gaps=3, Unimpl=0, Mock=1, Sim=0, Debt=0
 * Gap Correlation: internal=3 | external_v3=n/a | delta=n/a | status=no_external_data
 * External Severity (v3): C=n/a, H=n/a, M=n/a
 * PR: #4481 feat(core): implement IHealthProbe, IConfigHotReloader, IDistribute... (2026-04-09T06:20:41Z)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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
