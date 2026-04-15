/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            lifecycle.h                                        ║
  Version:         0.0.46                                             ║
  Last Modified:   2026-04-15 18:02:25                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     82                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 50ae658f67  2026-03-09  feat(core): implement dynamic log level adjustment and au... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
