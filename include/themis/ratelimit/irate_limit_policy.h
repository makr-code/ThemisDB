/**
 * @file irate_limit_policy.h
 * @brief Abstract global and adaptive rate-limit policy for ThemisDB.
 *
 * Governs the server-wide request rate across all tenants and clients.
 * Complements the per-tenant rate limit in ITenantQuotaPolicy with a
 * global admission guard and optional adaptive throttling based on observed
 * backend health.  Sits in Tier 2 of the four-tier resource-governance chain:
 *
 * @code
 *   compile-time constexpr (edition.h)   ← absolute ceiling, never overridable
 *   RuntimeLicenseGate                   ← edition-tier ceiling
 *   IRateLimitPolicy    (this file)      ← signed-plugin fine-tuning
 *   RateLimitConfig                      ← per-deployment operational tuning
 * @endcode
 *
 * All implementations must be individually thread-safe.
 *
 * @note This interface is part of the edition-policy plugin contract
 *       (IEditionPolicyPlugin::createRateLimitPolicy).  The claimed global
 *       RPS ceiling is validated against the compile-time constant
 *       edition::RATE_LIMIT_MAX_GLOBAL_RPS before EditionManager accepts the
 *       policy.
 *
 * @note Exception — WASM sandbox memory (Group 6) is a Security-Boundary and
 *       intentionally excluded from the plugin-policy override layer.  It is
 *       enforced separately and is not configurable via IEditionPolicyPlugin.
 */

#pragma once

#include <cstdint>
#include <string>

namespace themis {
namespace ratelimit {

/**
 * @brief Abstract global rate-limit policy.
 *
 * Controls two rate-limiting axes:
 *  - **Global RPS** — hard ceiling on total requests per second across all
 *    clients and tenants.
 *  - **Adaptive throttling** — feedback-driven rate reduction when backend
 *    health (latency, error rate) degrades.
 *
 * Implementations are installed into EditionManager via
 * `EditionManager::installRateLimitPolicy()` and sit above the existing
 * per-tenant AdaptiveRateLimiter / token-bucket layer.
 */
class IRateLimitPolicy {
public:
    virtual ~IRateLimitPolicy() = default;

    // Non-copyable, non-movable by default.
    IRateLimitPolicy(const IRateLimitPolicy&)            = delete;
    IRateLimitPolicy& operator=(const IRateLimitPolicy&) = delete;

    // -------------------------------------------------------------------------
    // Global admission
    // -------------------------------------------------------------------------

    /**
     * @brief Return true iff the next incoming request is admitted under the
     *        global rate limit.
     *
     * Uses token-bucket semantics for the global budget.  Callers must not
     * process the request if this returns false.
     *
     * @param client_id  Optional client/tenant identifier for per-client
     *                   accounting; empty string = no per-client tracking.
     * @return true when the request is within the global budget.
     */
    [[nodiscard]] virtual bool allowRequest(const std::string& client_id = "") = 0;

    /**
     * @brief Maximum global requests per second across all clients; 0 = unlimited.
     */
    [[nodiscard]] virtual uint64_t maxGlobalRequestsPerSecond() const noexcept = 0;

    // -------------------------------------------------------------------------
    // Adaptive throttling
    // -------------------------------------------------------------------------

    /**
     * @brief Record a completed backend request to feed the adaptive algorithm.
     *
     * The implementation uses the observed latency and success/failure outcome
     * to adjust the effective rate limit dynamically.  Must be called after
     * every dispatched backend call regardless of outcome.
     *
     * @param latency_ms  Round-trip latency of the backend call in milliseconds.
     * @param success     true when the backend call succeeded; false on error.
     * @param client_id   Optional client identifier for per-client health tracking.
     */
    virtual void recordSample(double latency_ms,
                              bool success,
                              const std::string& client_id = "") = 0;

    /**
     * @brief Return true when adaptive throttling is active for @p client_id.
     *
     * Adaptive throttling is considered active when the effective rate has
     * been reduced below the configured maximum due to observed health signals.
     *
     * @param client_id  Client/tenant identifier; empty = global check.
     */
    [[nodiscard]] virtual bool isThrottled(const std::string& client_id = "") const = 0;

    /**
     * @brief Return the current effective rate limit in requests per second.
     *
     * For non-adaptive policies this is identical to maxGlobalRequestsPerSecond().
     * For adaptive policies the value may be lower when backends are degraded.
     *
     * @param client_id  Client/tenant identifier; empty = global effective rate.
     * @return Effective RPS; 0 signals unlimited.
     */
    [[nodiscard]] virtual uint64_t effectiveRPS(const std::string& client_id = "") const = 0;

    // -------------------------------------------------------------------------
    // Status
    // -------------------------------------------------------------------------

    /**
     * @brief Return true when any rate-limit enforcement is active.
     *
     * Implementations should return false when the global limit is 0
     * (unlimited) and no adaptive throttling is triggered, so callers can
     * bypass the check on hot paths.
     */
    [[nodiscard]] virtual bool isEnforced() const noexcept = 0;

    /**
     * @brief Return the total number of requests rejected due to rate limiting.
     */
    [[nodiscard]] virtual uint64_t totalRejectedRequests() const noexcept = 0;

protected:
    IRateLimitPolicy() = default;
    IRateLimitPolicy(IRateLimitPolicy&&) = default;
    IRateLimitPolicy& operator=(IRateLimitPolicy&&) = default;
};

} // namespace ratelimit
} // namespace themis
