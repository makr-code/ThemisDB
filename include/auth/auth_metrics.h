/**
 * @file auth_metrics.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
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
    class Registry {};
    template<typename T> class Family {};
    class Counter { public: void Increment(double = 1.0) {} };
    class Gauge { public: void Set(double) {} void Increment(double = 1.0) {} void Decrement(double = 1.0) {} };
    class Histogram { public: void Observe(double) {} };
}
#endif

namespace themis {
namespace auth {

enum class AuthMethod {
    JWT,
    GSSAPI,
    MFA,
    OAUTH_DEVICE,
    API_KEY,
    UNKNOWN
};

class AuthMetrics {
public:
    struct Config {
        std::string namespace_prefix = "themis_auth";
        bool enable_histograms = true;
        bool enable_detailed_metrics = true;
        
        // Histogram buckets for latency (in milliseconds)
        std::vector<double> latency_buckets = {1, 5, 10, 25, 50, 100, 250, 500, 1000, 2500};

        /**
         * @brief Defaults.
         * @return Return value.
         * @details Implements defaults without additional internal calls.
         */
        static Config defaults() { return {}; }
    };
    
#ifdef THEMIS_HAS_PROMETHEUS
    AuthMetrics();
    /**
     * @brief Auth Metrics.
     * @param[in] config Input parameter.
     * @return Return value.
     */
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
    
    void recordAuthAttempt(AuthMethod method, bool success, double duration_ms = 0.0);
    
    /**
     * @brief Record Auth Success.
     * @param[in] method Input parameter.
     * @param[in] duration_ms Input parameter.
     */
    void recordAuthSuccess(AuthMethod method, double duration_ms);
    
    /**
     * @brief Record Auth Failure.
     * @param[in] method Input parameter.
     * @param[in] error_code Input parameter.
     * @param[in] duration_ms Input parameter.
     */
    void recordAuthFailure(AuthMethod method, int error_code, double duration_ms);
    
    // ========================================================================
    // JWKS Cache Metrics
    // ========================================================================
    
    /**
     * @brief Record JWKSCache Hit.
     */
    void recordJWKSCacheHit();
    
    /**
     * @brief Record JWKSCache Miss.
     */
    void recordJWKSCacheMiss();
    
    /**
     * @brief Record JWKSFetch.
     * @param[in] duration_ms Input parameter.
     * @param[in] success Input parameter.
     */
    void recordJWKSFetch(double duration_ms, bool success);
    
    /**
     * @brief Set JWKSCache Size.
     * @param[in] num_keys Input parameter.
     */
    void setJWKSCacheSize(int num_keys);
    
    // ========================================================================
    // Rate Limiting Metrics
    // ========================================================================
    
    /**
     * @brief Record Rate Limit Exceeded.
     * @param[in] type Input parameter.
     */
    void recordRateLimitExceeded(const std::string& type);
    
    /**
     * @brief Set Rate Limit Tokens.
     * @param[in] identifier Input parameter.
     * @param[in] tokens Input parameter.
     */
    void setRateLimitTokens(const std::string& identifier, double tokens);
    
    // ========================================================================
    // Account Lockout Metrics
    // ========================================================================
    
    /**
     * @brief Record Account Lockout.
     * @param[in] user_id Identifier of the user.
     */
    void recordAccountLockout(const std::string& user_id);
    
    /**
     * @brief Record Account Unlock.
     * @param[in] user_id Identifier of the user.
     */
    void recordAccountUnlock(const std::string& user_id);
    
    /**
     * @brief Set Locked Account Count.
     * @param[in] count Input parameter.
     */
    void setLockedAccountCount(int count);
    
    // ========================================================================
    // Error Metrics
    // ========================================================================
    
    /**
     * @brief Record Error.
     * @param[in] error_code Input parameter.
     */
    void recordError(int error_code);
    
    /**
     * @brief Record Error By Category.
     * @param[in] category Input parameter.
     */
    void recordErrorByCategory(const std::string& category);
    
    // ========================================================================
    // Token Validation Metrics
    // ========================================================================
    
    /**
     * @brief Record Token Validation.
     * @param[in] method Input parameter.
     * @param[in] duration_ms Input parameter.
     */
    void recordTokenValidation(AuthMethod method, double duration_ms);
    
    /**
     * @brief Record Revoked Token Check.
     * @param[in] was_revoked Input parameter.
     */
    void recordRevokedTokenCheck(bool was_revoked);

    // ========================================================================
    // Credential Stuffing Metrics
    // ========================================================================

    /**
     * @brief Record Credential Stuffing Attempt.
     * @param[in] user_id Identifier of the user.
     * @param[in] ip Input parameter.
     * @param[in] outcome Input parameter.
     */
    void recordCredentialStuffingAttempt(const std::string& user_id,
                                         const std::string& ip,
                                         const std::string& outcome);

    // ========================================================================
    // TOTP Drift Metrics
    // ========================================================================

    /**
     * @brief Record TOTPDrift.
     * @param[in] step_offset Input parameter.
     */
    void recordTOTPDrift(int step_offset);

    /**
     * @brief Get TOTPDrift Count.
     * @return Return value.
     */
    uint64_t getTOTPDriftCount() const;

    // ========================================================================
    // LDAP Connection Pool Metrics
    // ========================================================================

    /**
     * @brief Set LDAPPool Size.
     * @param[in] count Input parameter.
     */
    void setLDAPPoolSize(int count);

    /**
     * @brief Set LDAPIdle Connections.
     * @param[in] count Input parameter.
     */
    void setLDAPIdleConnections(int count);

    /**
     * @brief Set LDAPActive Connections.
     * @param[in] count Input parameter.
     */
    void setLDAPActiveConnections(int count);
    
    // ========================================================================
    // Statistics Access
    // ========================================================================
    
    /**
     * @brief Get Total Attempts.
     * @return Return value.
     */
    uint64_t getTotalAttempts() const;
    
    /**
     * @brief Get Successful Auths.
     * @return Return value.
     */
    uint64_t getSuccessfulAuths() const;
    
    /**
     * @brief Get Failed Auths.
     * @return Return value.
     */
    uint64_t getFailedAuths() const;
    
    /**
     * @brief Get Success Rate.
     * @return Return value.
     */
    double getSuccessRate() const;

    /**
     * @brief Get Credential Stuffing Total.
     * @return Return value.
     */
    uint64_t getCredentialStuffingTotal() const;

    /**
     * @brief Get LDAPPool Size.
     * @return Return value.
     */
    int getLDAPPoolSize() const;

    /**
     * @brief Get LDAPIdle Connections.
     * @return Return value.
     */
    int getLDAPIdleConnections() const;

    /**
     * @brief Get LDAPActive Connections.
     * @return Return value.
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
    /**
     * @brief Auth Method To String.
     * @param[in] method Input parameter.
     * @return Return value.
     */
    static std::string authMethodToString(AuthMethod method);
};

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
    
    /**
     * @brief Record Success.
     * @details Calls: getDuration(), recordAuthSuccess().
     */
    void recordSuccess() {
        if (!recorded_) {
            auto duration = getDuration();
            metrics_.recordAuthSuccess(method_, duration);
            recorded_ = true;
        }
    }
    
    /**
     * @brief Record Failure.
     * @param[in] error_code Input parameter.
     * @details Calls: getDuration(), recordAuthFailure().
     */
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

