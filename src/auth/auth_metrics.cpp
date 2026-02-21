/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            auth_metrics.cpp                                   ║
  Version:         0.0.14                                             ║
  Last Modified:   2026-02-21 16:52:57                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     297                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "auth/auth_metrics.h"
#include "utils/logger.h"

namespace themis {
namespace auth {

// ============================================================================
// AuthMetrics Implementation
// ============================================================================

#ifdef THEMIS_HAS_PROMETHEUS

AuthMetrics::AuthMetrics()
    : AuthMetrics(std::make_shared<prometheus::Registry>(), Config()) {}

AuthMetrics::AuthMetrics(const Config& config)
    : AuthMetrics(std::make_shared<prometheus::Registry>(), config) {}

AuthMetrics::AuthMetrics(std::shared_ptr<prometheus::Registry> registry,
                        const Config& config)
    : config_(config)
    , registry_(registry)
    , auth_attempts_total_(prometheus::BuildCounter()
                          .Name(config.namespace_prefix + "_attempts_total")
                          .Help("Total number of authentication attempts")
                          .Register(*registry))
    , auth_successes_total_(prometheus::BuildCounter()
                           .Name(config.namespace_prefix + "_successes_total")
                           .Help("Total number of successful authentications")
                           .Register(*registry))
    , auth_failures_total_(prometheus::BuildCounter()
                          .Name(config.namespace_prefix + "_failures_total")
                          .Help("Total number of failed authentications")
                          .Register(*registry))
    , jwks_cache_hits_total_(prometheus::BuildCounter()
                            .Name(config.namespace_prefix + "_jwks_cache_hits_total")
                            .Help("Total JWKS cache hits")
                            .Register(*registry))
    , jwks_cache_misses_total_(prometheus::BuildCounter()
                              .Name(config.namespace_prefix + "_jwks_cache_misses_total")
                              .Help("Total JWKS cache misses")
                              .Register(*registry))
    , jwks_fetches_total_(prometheus::BuildCounter()
                         .Name(config.namespace_prefix + "_jwks_fetches_total")
                         .Help("Total JWKS fetch attempts")
                         .Register(*registry))
    , rate_limit_exceeded_total_(prometheus::BuildCounter()
                                .Name(config.namespace_prefix + "_rate_limit_exceeded_total")
                                .Help("Total rate limit exceeded events")
                                .Register(*registry))
    , account_lockouts_total_(prometheus::BuildCounter()
                             .Name(config.namespace_prefix + "_account_lockouts_total")
                             .Help("Total account lockout events")
                             .Register(*registry))
    , account_unlocks_total_(prometheus::BuildCounter()
                            .Name(config.namespace_prefix + "_account_unlocks_total")
                            .Help("Total account unlock events")
                            .Register(*registry))
    , errors_total_(prometheus::BuildCounter()
                   .Name(config.namespace_prefix + "_errors_total")
                   .Help("Total authentication errors")
                   .Register(*registry))
    , revoked_token_checks_total_(prometheus::BuildCounter()
                                 .Name(config.namespace_prefix + "_revoked_token_checks_total")
                                 .Help("Total revoked token checks")
                                 .Register(*registry))
    , jwks_cache_size_(prometheus::BuildGauge()
                      .Name(config.namespace_prefix + "_jwks_cache_size")
                      .Help("Current JWKS cache size (number of keys)")
                      .Register(*registry))
    , locked_accounts_current_(prometheus::BuildGauge()
                              .Name(config.namespace_prefix + "_locked_accounts_current")
                              .Help("Current number of locked accounts")
                              .Register(*registry))
    , auth_duration_ms_(prometheus::BuildHistogram()
                       .Name(config.namespace_prefix + "_duration_milliseconds")
                       .Help("Authentication duration in milliseconds")
                       .Register(*registry))
    , jwks_fetch_duration_ms_(prometheus::BuildHistogram()
                             .Name(config.namespace_prefix + "_jwks_fetch_duration_milliseconds")
                             .Help("JWKS fetch duration in milliseconds")
                             .Register(*registry))
    , token_validation_duration_ms_(prometheus::BuildHistogram()
                                   .Name(config.namespace_prefix + "_token_validation_duration_milliseconds")
                                   .Help("Token validation duration in milliseconds")
                                   .Register(*registry))
{
    utils::Logger::info("AuthMetrics initialized with Prometheus support");
}

#else

AuthMetrics::AuthMetrics(const Config& config)
    : config_(config)
{
    utils::Logger::warn("AuthMetrics initialized without Prometheus support (metrics disabled)");
}

#endif

void AuthMetrics::recordAuthAttempt(AuthMethod method, bool success, double duration_ms) {
    total_attempts_.fetch_add(1, std::memory_order_relaxed);
    
    if (success) {
        successful_auths_.fetch_add(1, std::memory_order_relaxed);
    } else {
        failed_auths_.fetch_add(1, std::memory_order_relaxed);
    }
    
#ifdef THEMIS_HAS_PROMETHEUS
    std::map<std::string, std::string> labels;
    labels["method"] = authMethodToString(method);
    
    auth_attempts_total_.Add(labels).Increment();
    
    if (success) {
        auth_successes_total_.Add(labels).Increment();
    } else {
        auth_failures_total_.Add(labels).Increment();
    }
    
    if (config_.enable_histograms && duration_ms > 0.0) {
        auth_duration_ms_.Add(labels, config_.latency_buckets).Observe(duration_ms);
    }
#endif
}

void AuthMetrics::recordAuthSuccess(AuthMethod method, double duration_ms) {
    recordAuthAttempt(method, true, duration_ms);
}

void AuthMetrics::recordAuthFailure(AuthMethod method, int error_code, double duration_ms) {
    recordAuthAttempt(method, false, duration_ms);
    recordError(error_code);
}

void AuthMetrics::recordJWKSCacheHit() {
#ifdef THEMIS_HAS_PROMETHEUS
    jwks_cache_hits_total_.Add({}).Increment();
#endif
}

void AuthMetrics::recordJWKSCacheMiss() {
#ifdef THEMIS_HAS_PROMETHEUS
    jwks_cache_misses_total_.Add({}).Increment();
#endif
}

void AuthMetrics::recordJWKSFetch(double duration_ms, bool success) {
#ifdef THEMIS_HAS_PROMETHEUS
    std::map<std::string, std::string> labels;
    labels["result"] = success ? "success" : "failure";
    
    jwks_fetches_total_.Add(labels).Increment();
    
    if (config_.enable_histograms) {
        jwks_fetch_duration_ms_.Add({}, config_.latency_buckets).Observe(duration_ms);
    }
#endif
}

void AuthMetrics::setJWKSCacheSize(int num_keys) {
#ifdef THEMIS_HAS_PROMETHEUS
    jwks_cache_size_.Add({}).Set(static_cast<double>(num_keys));
#endif
}

void AuthMetrics::recordRateLimitExceeded(const std::string& type) {
#ifdef THEMIS_HAS_PROMETHEUS
    std::map<std::string, std::string> labels;
    labels["type"] = type;
    rate_limit_exceeded_total_.Add(labels).Increment();
#endif
}

void AuthMetrics::setRateLimitTokens(const std::string& identifier, double tokens) {
    // This would create too many time series, so we skip it in the implementation
    // Instead, we rely on aggregate metrics
}

void AuthMetrics::recordAccountLockout(const std::string& user_id) {
#ifdef THEMIS_HAS_PROMETHEUS
    account_lockouts_total_.Add({}).Increment();
#endif
    utils::Logger::warn("Account locked: " + user_id);
}

void AuthMetrics::recordAccountUnlock(const std::string& user_id) {
#ifdef THEMIS_HAS_PROMETHEUS
    account_unlocks_total_.Add({}).Increment();
#endif
    utils::Logger::info("Account unlocked: " + user_id);
}

void AuthMetrics::setLockedAccountCount(int count) {
#ifdef THEMIS_HAS_PROMETHEUS
    locked_accounts_current_.Add({}).Set(static_cast<double>(count));
#endif
}

void AuthMetrics::recordError(int error_code) {
#ifdef THEMIS_HAS_PROMETHEUS
    std::map<std::string, std::string> labels;
    labels["error_code"] = std::to_string(error_code);
    errors_total_.Add(labels).Increment();
#endif
}

void AuthMetrics::recordErrorByCategory(const std::string& category) {
#ifdef THEMIS_HAS_PROMETHEUS
    std::map<std::string, std::string> labels;
    labels["category"] = category;
    errors_total_.Add(labels).Increment();
#endif
}

void AuthMetrics::recordTokenValidation(AuthMethod method, double duration_ms) {
#ifdef THEMIS_HAS_PROMETHEUS
    if (config_.enable_histograms) {
        std::map<std::string, std::string> labels;
        labels["method"] = authMethodToString(method);
        token_validation_duration_ms_.Add(labels, config_.latency_buckets).Observe(duration_ms);
    }
#endif
}

void AuthMetrics::recordRevokedTokenCheck(bool was_revoked) {
#ifdef THEMIS_HAS_PROMETHEUS
    std::map<std::string, std::string> labels;
    labels["result"] = was_revoked ? "revoked" : "valid";
    revoked_token_checks_total_.Add(labels).Increment();
#endif
}

uint64_t AuthMetrics::getTotalAttempts() const {
    return total_attempts_.load(std::memory_order_relaxed);
}

uint64_t AuthMetrics::getSuccessfulAuths() const {
    return successful_auths_.load(std::memory_order_relaxed);
}

uint64_t AuthMetrics::getFailedAuths() const {
    return failed_auths_.load(std::memory_order_relaxed);
}

double AuthMetrics::getSuccessRate() const {
    uint64_t total = getTotalAttempts();
    if (total == 0) {
        return 0.0;
    }
    uint64_t successes = getSuccessfulAuths();
    return static_cast<double>(successes) / static_cast<double>(total);
}

std::string AuthMetrics::authMethodToString(AuthMethod method) {
    switch (method) {
        case AuthMethod::JWT:
            return "jwt";
        case AuthMethod::GSSAPI:
            return "gssapi";
        case AuthMethod::MFA:
            return "mfa";
        case AuthMethod::UNKNOWN:
        default:
            return "unknown";
    }
}

} // namespace auth
} // namespace themis
