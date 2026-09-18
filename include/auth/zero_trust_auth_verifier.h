/**
 * @file zero_trust_auth_verifier.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <optional>
#include <memory>
#include <functional>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <thread>
#include <unordered_map>

#include "security/zero_trust_policy_enforcer.h"

namespace themis {
namespace utils { class AuditLogger; }
namespace auth {

class AuthWorkerThreadPool; ///< Forward-declared to reduce header coupling.
class SessionManager;

class ZeroTrustAuthVerifier {
public:
    using TokenVerifier = security::ZeroTrustPolicyEnforcer::TokenVerifier;

    struct Config {
        double min_trust_score = 0.7;

        bool device_id_expected = false;

        std::chrono::milliseconds re_evaluation_interval{std::chrono::seconds(300)};
    };

    struct Request {
        std::string request_id;               ///< Unique request identifier
        std::string user_id;                  ///< Claimed identity
        std::string token;                    ///< Bearer token / API key
        std::string client_ip;                ///< Source IPv4 address
        std::string resource;                 ///< Resource being accessed
        std::string action;                   ///< Action (read / write / delete …)
        std::optional<std::string> device_id; ///< Optional device identifier
    };

    struct MonitoredSession {
        std::string session_id;               ///< Session identifier (passed to terminateSession)
        std::string user_id;                  ///< Claimed identity
        std::string token;                    ///< Bearer token / API key
        std::string client_ip;                ///< Source IP at session creation
        std::string resource;                 ///< Primary resource (informational)
        std::string action;                   ///< Primary action (informational)
        std::optional<std::string> device_id; ///< Optional device identifier
    };

    struct Decision {
        bool allowed = false;          ///< Overall result
        double trust_score = 0.0;      ///< Composite trust score [0.0, 1.0]
        std::string reason;            ///< Human-readable explanation
        std::string request_id;        ///< Echo of Request::request_id
        bool identity_verified = false; ///< Token check passed
        bool network_ok = false;       ///< Network policy check passed
    };

    // ========================================================================
    // Construction
    // ========================================================================

    ZeroTrustAuthVerifier();
    /**
     * @brief Zero Trust Auth Verifier.
     * @param[in] token_verifier Input parameter.
     * @return Return value.
     */
    explicit ZeroTrustAuthVerifier(TokenVerifier token_verifier);
    ZeroTrustAuthVerifier(
        const Config& config,
        TokenVerifier token_verifier = nullptr);

    ~ZeroTrustAuthVerifier();

    // ========================================================================
    // Dependency injection
    // ========================================================================

    /**
     * @brief Set Audit Logger.
     * @param[in,out] logger Input/output parameter.
     * @details Implements setAuditLogger without additional internal calls.
     */
    void setAuditLogger(utils::AuditLogger* logger) { audit_logger_ = logger; }

    /**
     * @brief ======================================================================== Network policy management (delegated to the underlying enforcer) ========================================================================
     * @param[in] policy Network policy to add.
     */

    void addNetworkPolicy(const security::NetworkPolicy& policy);

    /**
     * @brief Remove a network policy by id.
     * @param[in] policy_id Identifier of the policy to remove.
     * @return True when a policy was removed.
     */
    bool removeNetworkPolicy(const std::string& policy_id);

    /**
     * @brief Return all currently registered network policies.
     * @return Snapshot of network policies.
     */
    std::vector<security::NetworkPolicy> getNetworkPolicies() const;

    /**
     * @brief ======================================================================== Core: continuous per-request verification ========================================================================
     * @param[in] req Input parameter.
     * @return Verification result.
     */

    Decision verify(const Request& req);

    /**
     * @brief ======================================================================== Background session monitoring (async policy re-evaluation) ========================================================================
     * @param[in] session Input parameter.
     * @param[in,out] session_manager Input/output parameter.
     */

    void startSessionMonitoring(const MonitoredSession& session,
                                SessionManager* session_manager);

    /**
     * @brief Stop Session Monitoring.
     * @param[in] session_id Identifier of the session.
     */
    void stopSessionMonitoring(const std::string& session_id);

    /**
     * @brief Monitored Session Count.
     * @return Return value.
     */
    size_t monitoredSessionCount() const;

    // ========================================================================
    // Metrics (read-only view of the underlying enforcer's counters)
    // ========================================================================

    const security::ZeroTrustPolicyEnforcer::Metrics& getMetrics() const {
        return enforcer_.getMetrics();
    }

private:
    Config config_;
    security::ZeroTrustPolicyEnforcer enforcer_;
    utils::AuditLogger* audit_logger_ = nullptr; ///< Non-owning; may be nullptr.

    // ========================================================================
    // Background re-evaluation state
    // ========================================================================

    struct MonitorEntry {
        MonitoredSession session;
        SessionManager*  session_manager;                          ///< Non-owning
        std::chrono::steady_clock::time_point next_eval;           ///< Deadline for next check (steady_clock)
    };

    mutable std::mutex                                  monitor_mutex_;
    std::condition_variable                             monitor_cv_;
    std::unordered_map<std::string, MonitorEntry>       monitored_sessions_; ///< session_id → entry
    std::unique_ptr<AuthWorkerThreadPool>               worker_pool_;
    std::thread                                         monitor_thread_;
    std::atomic<bool>                                   monitor_stop_{false};
    std::atomic<uint64_t>                               schedule_generation_{0};

    /**
     * @brief Monitor Loop.
     */
    void monitorLoop();

    /**
     * @brief Re Evaluate Session.
     * @param[in] entry Input parameter.
     */
    void reEvaluateSession(const MonitorEntry& entry);
};

} // namespace auth
} // namespace themis
