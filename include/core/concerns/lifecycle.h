/**
 * @file lifecycle.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <string>

namespace themis {
namespace core {
namespace concerns {

struct ProbeResult {
    bool ok = true;           ///< true = healthy / ready
    std::string message;      ///< Human-readable status detail

    static ProbeResult healthy(const std::string& msg = "ok") {
        return {true, msg};
    }

    /**
     * @brief Unhealthy.
     * @param[in] msg Input parameter.
     * @return Return value.
     * @details Implements unhealthy without additional internal calls.
     */
    static ProbeResult unhealthy(const std::string& msg) {
        return {false, msg};
    }
};

struct HealthStatus {
    ProbeResult logger;
    ProbeResult tracer;
    ProbeResult metrics;
    ProbeResult cache;
    ProbeResult secrets = ProbeResult::healthy();
        ProbeResult circuit_breaker = ProbeResult::healthy();
        ProbeResult featureFlags = ProbeResult::healthy();
    ProbeResult auditLog = ProbeResult::healthy();

    bool isHealthy() const noexcept {
        return logger.ok && tracer.ok && metrics.ok && cache.ok &&
               secrets.ok && circuit_breaker.ok && featureFlags.ok &&
               auditLog.ok;
    }
};

} // namespace concerns
} // namespace core
} // namespace themis
