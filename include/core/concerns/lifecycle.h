/*
 * ThemisDB | File: lifecycle.h | Version: 0.0.47 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 68
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): #4481 feat(core): implement IHeal... (2026-04-09) | #3570 feat(core): dynamic log lev... (2026-03-12) | #2845 [core] Feature flag interfa... (2026-03-12) | #2722 [WIP] Add tracing and corre... (2026-03-12) | #2701 fix(core): repair syntax er... (2026-03-12)
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
