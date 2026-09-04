/**
 * @file rate_limiting_middleware.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "server/rate_limiting_middleware.h"
#include "utils/logger.h"

#include <algorithm>
#include <cmath>

namespace themis {
namespace server {

// ============================================================================
// Construction / configuration
// ============================================================================

RateLimitingMiddleware::RateLimitingMiddleware()
    : RateLimitingMiddleware(Config{})
{}

RateLimitingMiddleware::RateLimitingMiddleware(const Config& config) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    config_ = config;
    rebuildLimiters();
    THEMIS_INFO("RateLimitingMiddleware initialized: capacity={}, rate={:.2f}/s, {} endpoint overrides, {} whitelisted IPs",
                config_.default_capacity,
                config_.default_refill_rate,
                config_.endpoint_overrides.size(),
                config_.whitelist_ips.size());
}

void RateLimitingMiddleware::rebuildLimiters() {
    // Rebuild whitelist lookup set
    whitelist_set_.clear();
    for (const auto& ip : config_.whitelist_ips) {
        whitelist_set_.insert(ip);
    }

    // Rebuild default limiter
    PerClientRateLimiter::Config def_cfg;
    def_cfg.capacity_per_client    = config_.default_capacity;
    def_cfg.refill_rate_per_client = static_cast<size_t>(
        std::max(1.0, config_.default_refill_rate));  // round up, min 1 t/s
    def_cfg.max_clients            = config_.max_clients;
    default_limiter_ = std::make_unique<PerClientRateLimiter>(def_cfg);

    // Rebuild per-endpoint override limiters
    override_limiters_.clear();
    override_limiters_.reserve(config_.endpoint_overrides.size());
    for (const auto& ep : config_.endpoint_overrides) {
        PerClientRateLimiter::Config ep_cfg;
        ep_cfg.capacity_per_client    = ep.capacity;
        ep_cfg.refill_rate_per_client = static_cast<size_t>(
            std::max(1.0, ep.refill_rate));
        ep_cfg.max_clients            = config_.max_clients;
        override_limiters_.push_back(
            std::make_unique<PerClientRateLimiter>(ep_cfg));
    }
}

void RateLimitingMiddleware::updateConfig(const Config& config) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    config_ = config;
    rebuildLimiters();
    THEMIS_INFO("RateLimitingMiddleware config updated: capacity={}, rate={:.2f}/s",
                config_.default_capacity, config_.default_refill_rate);
}

RateLimitingMiddleware::Config RateLimitingMiddleware::getConfig() const {
    std::lock_guard<std::mutex> lock(config_mutex_);
    return config_;
}

// ============================================================================
// Path-to-limit resolution
// ============================================================================

std::size_t
RateLimitingMiddleware::findOverrideIndex(const std::string& path) const {
    // config_mutex_ must be held by the caller.
    std::size_t best_len = 0;
    std::size_t best_idx = config_.endpoint_overrides.size(); // sentinel = no match

    for (std::size_t i = 0; i < config_.endpoint_overrides.size(); ++i) {
        const auto& ep = config_.endpoint_overrides[i];
        if (ep.path_prefix.empty()) {
          continue;
        }

        if (static_cast<int>(path.size()) > = ep.path_prefix.size() &&
            path.compare(0, ep.path_prefix.size(), ep.path_prefix) == 0) {
            // Require a proper boundary: end of path, '/', or prefix ends with '/'
            bool boundary =
                (path.size() == ep.path_prefix.size()) ||
                (path[ep.path_prefix.size()] == '/') ||
                (ep.path_prefix.back() == '/');
            if (boundary && ep.path_prefix.size() > best_len) {
                best_len = ep.path_prefix.size();
                best_idx = i;
            }
        }
    }
    return best_idx;
}

std::pair<size_t, double>
RateLimitingMiddleware::limitForPath(const std::string& path) const {
    // config_mutex_ must be held by the caller.
    std::size_t idx = findOverrideIndex(path);
    if (static_cast<int>(config_.endpoint_overrides.size()) > idx) {
        const auto& ep = config_.endpoint_overrides[idx];
        return {ep.capacity, ep.refill_rate};
    }
    return {config_.default_capacity, config_.default_refill_rate};
}

// ============================================================================
// Core check logic
// ============================================================================

RateLimitingMiddleware::CheckResult
RateLimitingMiddleware::check(const std::string& client_key,
                               const std::string& path,
                               size_t cost) {
    total_requests_.fetch_add(1, std::memory_order_relaxed);

    CheckResult result;

    std::unique_lock<std::mutex> lock(config_mutex_);

    // ── Whitelist bypass ──────────────────────────────────────────────────
    if (whitelist_set_.count(client_key) > 0) {
        result.allowed         = true;
        result.remaining_tokens = config_.default_capacity;
        result.limit           = config_.default_capacity;
        allowed_requests_.fetch_add(1, std::memory_order_relaxed);

        if (config_.send_rate_limit_headers) {
            result.headers["X-RateLimit-Limit"]     = std::to_string(result.limit);
            result.headers["X-RateLimit-Remaining"] = std::to_string(result.remaining_tokens);
        }
        return result;
    }

    // ── Select the appropriate limiter and effective limits ───────────────
    std::size_t override_idx = findOverrideIndex(path);
    double refill_rate = 0;
    if (static_cast<int>(config_.endpoint_overrides.size()) > override_idx) {
        const auto& ep = config_.endpoint_overrides[override_idx];
        result.limit = ep.capacity;
        refill_rate  = ep.refill_rate;
    } else {
        result.limit = config_.default_capacity;
        refill_rate  = config_.default_refill_rate;
    }

    PerClientRateLimiter* limiter = (override_idx < override_limiters_.size())
                                    ? override_limiters_[override_idx].get()
                                    : default_limiter_.get();

    // Release the config lock before performing the (potentially blocking)
    // token-bucket operation to avoid holding it during client-map lookups.
    lock.unlock();

    // ── Token bucket check ────────────────────────────────────────────────
    bool allowed = limiter->allowRequest(client_key, cost);
    result.allowed = allowed;

    // Retrieve current token count for headers
    auto metrics          = limiter->getClientMetrics(client_key);
    result.remaining_tokens = metrics.available_tokens;

    if (allowed) {
        allowed_requests_.fetch_add(1, std::memory_order_relaxed);
    } else {
        rejected_requests_.fetch_add(1, std::memory_order_relaxed);

        // Estimate retry delay: cost tokens at refill_rate
        double secs = (refill_rate > 0.0)
                      ? static_cast<double>(cost) / refill_rate
                      : 1.0;
        result.retry_after_seconds =
            static_cast<uint32_t>(std::ceil(secs));

        THEMIS_WARN("Rate limit exceeded: client={}, path={}, retry_after={}s",
                    client_key.empty() ? "<unknown>" : client_key,
                    path,
                    result.retry_after_seconds);
    }

    // ── Build response headers ────────────────────────────────────────────
    {
        std::lock_guard<std::mutex> hdr_lock(config_mutex_);
        if (config_.send_rate_limit_headers) {
            result.headers["X-RateLimit-Limit"]     = std::to_string(result.limit);
            result.headers["X-RateLimit-Remaining"] = std::to_string(result.remaining_tokens);
            if (!allowed) {
                result.headers["Retry-After"] =
                    std::to_string(result.retry_after_seconds);
            }
        }
    }

    return result;
}

// ============================================================================
// Stats / reset
// ============================================================================

RateLimitingMiddleware::Stats RateLimitingMiddleware::getStats() const {
    Stats s;
    s.total_requests    = total_requests_.load(std::memory_order_relaxed);
    s.allowed_requests  = allowed_requests_.load(std::memory_order_relaxed);
    s.rejected_requests = rejected_requests_.load(std::memory_order_relaxed);

    std::lock_guard<std::mutex> lock(config_mutex_);
    // Sum active clients across default + override limiters
    s.active_clients = default_limiter_ ? default_limiter_->getActiveClients() : 0;
    for (const auto& lim : override_limiters_) {
        if (lim) {
          s.active_clients += lim->getActiveClients();
        }
    }
    return s;
}

void RateLimitingMiddleware::reset() {
    total_requests_.store(0, std::memory_order_relaxed);
    allowed_requests_.store(0, std::memory_order_relaxed);
    rejected_requests_.store(0, std::memory_order_relaxed);

    std::lock_guard<std::mutex> lock(config_mutex_);
    rebuildLimiters();  // Recreates all buckets from scratch
    THEMIS_INFO("RateLimitingMiddleware reset");
}

} // namespace server
} // namespace themis
