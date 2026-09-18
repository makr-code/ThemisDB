/**
 * @file rate_limiting_middleware.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "server/rate_limiter_v2.h"
#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace themis {
namespace server {

class RateLimitingMiddleware {
public:
    struct EndpointLimit {
        std::string path_prefix;   ///< e.g. "/v2/documents", "/api/bulk"
        size_t      capacity;      ///< token bucket size (burst)
        double      refill_rate;   ///< tokens per second
    };

    struct Config {
        size_t default_capacity    = 100;

        double default_refill_rate = 100.0 / 60.0;

        std::vector<EndpointLimit> endpoint_overrides;

        std::vector<std::string> whitelist_ips;

        size_t max_clients = 10000;

        bool send_rate_limit_headers = true;

        Config() = default;
    };

    struct CheckResult {
        bool     allowed              = true;
        uint32_t retry_after_seconds  = 0;   ///< 0 when request is allowed
        size_t   remaining_tokens     = 0;   ///< tokens remaining after this request
        size_t   limit                = 0;   ///< effective bucket capacity for this endpoint

        std::unordered_map<std::string, std::string> headers;
    };

    RateLimitingMiddleware();

    /**
     * @brief Rate Limiting Middleware.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit RateLimitingMiddleware(const Config& config);

    CheckResult check(const std::string& client_key,
                      const std::string& path,
                      size_t cost = 1);

    /**
     * @brief Update the access control configuration.
     * @param[in] config New access control configuration.
     */
    void updateConfig(const Config& config);

    /**
     * @brief Get Config.
     * @return Return value.
     */
    Config getConfig() const;

    struct Stats {
        uint64_t total_requests    = 0;
        uint64_t allowed_requests  = 0;
        uint64_t rejected_requests = 0;
        size_t   active_clients    = 0;
    };

    /**
     * @brief Get Stats.
     * @return Return value.
     */
    Stats getStats() const;

    /**
     * @brief Reset the modification detection flag.
     */
    void reset();

private:
    /**
     * @brief Find Override Index.
     * @param[in] path Input parameter.
     * @return Return value.
     */
    std::size_t findOverrideIndex(const std::string& path) const;

    std::pair<size_t, double> limitForPath(const std::string& path) const;

    /**
     * @brief Rebuild Limiters.
     */
    void rebuildLimiters();

    // ── State ────────────────────────────────────────────────────────────
    mutable std::mutex config_mutex_;
    Config config_;

    // Whitelist set for O(1) lookup
    std::unordered_set<std::string> whitelist_set_;

    // Default per-client limiter (for requests that don't match any override).
    std::unique_ptr<PerClientRateLimiter> default_limiter_;

    // Per-endpoint override limiters, parallel to config_.endpoint_overrides.
    std::vector<std::unique_ptr<PerClientRateLimiter>> override_limiters_;

    // Aggregate counters (updated without the config_mutex_ held).
    std::atomic<uint64_t> total_requests_{0};
    std::atomic<uint64_t> allowed_requests_{0};
    std::atomic<uint64_t> rejected_requests_{0};
};

} // namespace server
} // namespace themis
