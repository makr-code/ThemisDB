/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            lifecycle.h                                        ║
  Version:         0.0.4                                              ║
  Last Modified:   2026-02-21 08:33:08                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     72                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f57cc26cc  2026-02-20  feat(core): lifecycle hooks, health/readiness probes, and... ║
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

    /// @return true only when every concern reports healthy/ready.
    bool isHealthy() const noexcept {
        return logger.ok && tracer.ok && metrics.ok && cache.ok;
    }
};

} // namespace concerns
} // namespace core
} // namespace themis
