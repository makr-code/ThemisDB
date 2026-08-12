/**
 * @file auth_metrics.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <memory>
#include <string>
#include <chrono>
#include <atomic>
#include <map>
#include <mutex>
#include <vector>

// ============================================================================
// Compilation Guard for Prometheus
// ============================================================================
#ifdef THEMIS_HAS_PROMETHEUS
#include <prometheus/registry.h>
#include <prometheus/counter.h>
#include <prometheus/gauge.h>
#include <prometheus/histogram.h>
#else
// Provide stub types when Prometheus is not available
namespace prometheus {
    /** @brief Registry for. */
    class Registry {};
    /** @brief Family. */
    template<typename T> class Family {};
    /** @brief Counter. */
    class Counter { public: void Increment(double = 1.0) {} };
    /** @brief Gauge. */
    class Gauge { public: void Set(double) {} void Increment(double = 1.0) {} void Decrement(double = 1.0) {} };
    /** @brief Histogram. */
    class Histogram { public: void Observe(double) {} };
}
#endif

namespace themis {
namespace auth {

/**
 * @brief Authentication method types for metrics
 */
enum class AuthMethod {
    JWT,
    GSSAPI,
    MFA,
    OAUTH_DEVICE,
    API_KEY,
    UNKNOWN
};

/**
 * @brief Prometheus metrics collector for authentication module
 * 
 * Thread-safe metrics collection for authentication operations.
 * Tracks success/failure rates, latency, cache performance, and security events.
 */
class AuthMetrics {
public:
    struct Config {
        std::string namespace_prefix = "themis_auth";
        bool enable_histograms = true;
        bool enable_detailed_metrics = true;
        
        // Histogram buckets for latency (in milliseconds)
        std::vector<double> latency_buckets = {1, 5, 10, 25, 50, 100, 250, 500, 1000, 2500};

        static Config defaults() { return {}; }
    };
    
#ifdef THEMIS_HAS_PROMETHEUS
    AuthMetrics();
    explicit AuthMetrics(const Config& config);
    explicit AuthMetrics(std::shared_ptr<prometheus::Registry> registry,
                        const Config& config = Config::defaults());
#else
    explicit AuthMetrics(const Config& config = Config::defaults());
#endif
    
    ~AuthMetrics() = default;
    
    // ========================================================================
    // Authentication Attempt Metrics
    // ========================================================================
    
    /**
     * @brief Record an authentication attempt
     * @param method Authentication method used
     * @param success Whether authentication succeeded
     * @param duration_ms Time taken for authentication
     */
    void recordAuthAttempt(AuthMethod method, bool success, double duration_ms = 0.0);
    
    /**
     * @brief Record authentication success
     * @param method Authentication method used
     * @param duration_ms Time taken for authentication
     */
    void recordAuthSuccess(AuthMethod method, double duration_ms);
    
    /**
     * @brief Record authentication failure
     * @param method Authentication method used
     * @param error_code Error code (from AuthErrorCode)
     * @param duration_ms Time taken before failure
     */
    void recordAuthFailure(AuthMethod method, int error_code, double duration_ms);
    
    // ========================================================================
    // JWKS Cache Metrics
    // ========================================================================
    
    /**
     * @brief Record JWKS cache hit
     */
    void recordJWKSCacheHit();
    
    /**
     * @brief Record JWKS cache miss (fetch required)
     */
    void recordJWKSCacheMiss();
    
    /**
     * @brief Record JWKS fetch duration
     * @param duration_ms Time taken to fetch JWKS
     * @param success Whether fetch succeeded
     */
    void recordJWKSFetch(double duration_ms, bool success);
    
    /**
     * @brief Record JWKS cache size
     * @param num_keys Number of keys in cache
     */
    void setJWKSCacheSize(int num_keys);
    
    // ========================================================================
    // Rate Limiting Metrics
    // ========================================================================
    
    /**
     * @brief Record rate limit exceeded event
     * @param type Type of rate limit (ip, user)
     */
    void recordRateLimitExceeded(const std::string& type);
    
    /**
     * @brief Record current rate limit token count
     * @param identifier Client identifier
     * @param tokens Current token count
     */
    void setRateLimitTokens(const std::string& identifier, double tokens);
    
    // ========================================================================
    // Account Lockout Metrics
    // ========================================================================
    
    /**
     * @brief Record account lockout event
     * @param user_id User identifier
     */
    void recordAccountLockout(const std::string& user_id);
    
    /**
     * @brief Record account unlock event
     * @param user_id User identifier
     */
    void recordAccountUnlock(const std::string& user_id);
    
    /**
     * @brief Set current number of locked accounts
     * @param count Number of locked accounts
     */
    void setLockedAccountCount(int count);
    
    // ========================================================================
    // Error Metrics
    // ========================================================================
    
    /**
     * @brief Record error by error code
     * @param error_code Authentication error code
     */
    void recordError(int error_code);
    
    /**
     * @brief Record error by category
     * @param category Error category (jwt, gssapi, mfa, rate_limit)
     */
    void recordErrorByCategory(const std::string& category);
    
    // ========================================================================
    // Token Validation Metrics
    // ========================================================================
    
    /**
     * @brief Record token validation duration
     * @param method Authentication method
     * @param duration_ms Time taken for validation
     */
    void recordTokenValidation(AuthMethod method, double duration_ms);
    
    /**
     * @brief Record revoked token check
     * @param was_revoked Whether token was found to be revoked
     */
    void recordRevokedTokenCheck(bool was_revoked);

    // ========================================================================
    // Credential Stuffing Metrics
    // ========================================================================

    /**
     * @brief Record a credential-stuffing detection event.
     *
     * Increments the `credential_stuffing_attempts_total` counter with labels
     * `{user_id, ip, outcome}` where outcome is one of:
     *   "allowed", "captcha_required", "otp_required", "account_locked_24h"
     *
     * @param user_id  Targeted user account (may be empty for IP-only events)
     * @param ip       Source IP address of the stuffing attempt
     * @param outcome  Escalation outcome string
     */
    void recordCredentialStuffingAttempt(const std::string& user_id,
                                         const std::string& ip,
                                         const std::string& outcome);

    // ========================================================================
    // TOTP Drift Metrics
    // ========================================================================

    /**
     * @brief Record a TOTP validation that succeeded with a non-zero time step offset.
     *
     * Increments the `totp_drift_total` counter labelled with the signed step
     * offset value.  Sustained non-zero offsets indicate a device clock that is
     * drifting and should be investigated.
     *
     * @param step_offset The signed time step offset at which the code matched
     *                    (e.g., -1 means the previous 30-second window).
     */
    void recordTOTPDrift(int step_offset);

    /**
     * @brief Get the total number of TOTP drift events recorded (always available).
     */
    uint64_t getTOTPDriftCount() const;

    // ========================================================================
    // LDAP Connection Pool Metrics
    // ========================================================================

    /**
     * @brief Set the total LDAP connection pool size (idle + active).
     * @param count Current pool size
     */
    void setLDAPPoolSize(int count);

    /**
     * @brief Set the number of idle LDAP connections in the pool.
     * @param count Number of idle connections
     */
    void setLDAPIdleConnections(int count);

    /**
     * @brief Set the number of active (checked-out) LDAP connections.
     * @param count Number of active connections
     */
    void setLDAPActiveConnections(int count);
    
    // ========================================================================
    // Statistics Access
    // ========================================================================
    
    /**
     * @brief Get total authentication attempts
     */
    uint64_t getTotalAttempts() const;
    
    /**
     * @brief Get successful authentications
     */
    uint64_t getSuccessfulAuths() const;
    
    /**
     * @brief Get failed authentications
     */
    uint64_t getFailedAuths() const;
    
    /**
     * @brief Get success rate (0.0 to 1.0)
     */
    double getSuccessRate() const;

    /**
     * @brief Get total credential-stuffing detection events recorded.
     */
    uint64_t getCredentialStuffingTotal() const;

    /**
     * @brief Get current LDAP connection pool size.
     */
    int getLDAPPoolSize() const;

    /**
     * @brief Get number of idle LDAP connections.
     */
    int getLDAPIdleConnections() const;

    /**
     * @brief Get number of active LDAP connections.
     */
    int getLDAPActiveConnections() const;

private:
    Config config_;
    
#ifdef THEMIS_HAS_PROMETHEUS
    std::shared_ptr<prometheus::Registry> registry_;
    
    // Counter families
    prometheus::Family<prometheus::Counter>& auth_attempts_total_;
    prometheus::Family<prometheus::Counter>& auth_successes_total_;
    prometheus::Family<prometheus::Counter>& auth_failures_total_;
    prometheus::Family<prometheus::Counter>& jwks_cache_hits_total_;
    prometheus::Family<prometheus::Counter>& jwks_cache_misses_total_;
    prometheus::Family<prometheus::Counter>& jwks_fetches_total_;
    prometheus::Family<prometheus::Counter>& rate_limit_exceeded_total_;
    prometheus::Family<prometheus::Counter>& account_lockouts_total_;
    prometheus::Family<prometheus::Counter>& account_unlocks_total_;
    prometheus::Family<prometheus::Counter>& errors_total_;
    prometheus::Family<prometheus::Counter>& revoked_token_checks_total_;
    prometheus::Family<prometheus::Counter>& totp_drift_total_;
    prometheus::Family<prometheus::Counter>& credential_stuffing_attempts_total_;
    
    // Gauge families
    prometheus::Family<prometheus::Gauge>& jwks_cache_size_;
    prometheus::Family<prometheus::Gauge>& locked_accounts_current_;
    prometheus::Family<prometheus::Gauge>& ldap_pool_size_;
    prometheus::Family<prometheus::Gauge>& ldap_idle_connections_;
    prometheus::Family<prometheus::Gauge>& ldap_active_connections_;
    
    // Histogram families
    prometheus::Family<prometheus::Histogram>& auth_duration_ms_;
    prometheus::Family<prometheus::Histogram>& jwks_fetch_duration_ms_;
    prometheus::Family<prometheus::Histogram>& token_validation_duration_ms_;
#endif
    
    // Local counters (always available, even without Prometheus)
    std::atomic<uint64_t> total_attempts_{0};
    std::atomic<uint64_t> successful_auths_{0};
    std::atomic<uint64_t> failed_auths_{0};
    std::atomic<uint64_t> totp_drift_count_{0};
    std::atomic<uint64_t> credential_stuffing_total_{0};

    // LDAP connection pool gauges (always available)
    std::atomic<int> ldap_pool_size_count_{0};
    std::atomic<int> ldap_idle_connections_count_{0};
    std::atomic<int> ldap_active_connections_count_{0};
    
    // Helper methods
    static std::string authMethodToString(AuthMethod method);
};

/**
 * @brief RAII helper for automatic auth duration measurement
 * 
 * Example usage:
 *   AuthDurationTimer timer(metrics, AuthMethod::JWT);
 *   // ... perform authentication ...
 *   timer.recordSuccess();  // or timer.recordFailure(error_code)
 */
class AuthDurationTimer {
public:
    AuthDurationTimer(AuthMetrics& metrics, AuthMethod method)
        : metrics_(metrics)
        , method_(method)
        , start_(std::chrono::steady_clock::now())
        , recorded_(false)
    {}
    
    ~AuthDurationTimer() {
        if (!recorded_) {
            // Record as attempt without success/failure
            auto duration = getDuration();
            metrics_.recordAuthAttempt(method_, false, duration);
        }
    }
    
    void recordSuccess() {
        if (!recorded_) {
            auto duration = getDuration();
            metrics_.recordAuthSuccess(method_, duration);
            recorded_ = true;
        }
    }
    
    void recordFailure(int error_code) {
        if (!recorded_) {
            auto duration = getDuration();
            metrics_.recordAuthFailure(method_, error_code, duration);
            recorded_ = true;
        }
    }
    
    double getDuration() const {
        auto end = std::chrono::steady_clock::now();
        return std::chrono::duration<double, std::milli>(end - start_).count();
    }

private:
    AuthMetrics& metrics_;
    AuthMethod method_;
    std::chrono::steady_clock::time_point start_;
    bool recorded_;
};

} // namespace auth
} // namespace themis

