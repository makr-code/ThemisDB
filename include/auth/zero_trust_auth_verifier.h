/**
 * @file zero_trust_auth_verifier.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief Auth-layer bridge for zero-trust continuous verification.
 *
 * Implements the "Zero-trust access model with continuous verification"
 * roadmap item in the auth module.
 *
 * Unlike traditional session-based authentication that trusts a session
 * once established, this verifier re-validates every request independently:
 *   - Token/credential re-validation on every call (no cached auth state)
 *   - CIDR-based network policy enforcement via ZeroTrustPolicyEnforcer
 *   - Composite trust score computation
 *   - Audit logging for every verification decision
 *
 * Integration:
 *   Call verify() for every inbound request *before* RBAC/ABAC evaluation.
 *   Only proceed if the returned Decision::allowed is true.
 *
 * Thread safety: all public methods are thread-safe.
 *
 * Example:
 * @code
 * ZeroTrustAuthVerifier::Config cfg;
 * cfg.min_trust_score = 0.7;
 * ZeroTrustAuthVerifier verifier(cfg);
 * verifier.setAuditLogger(&my_logger);
 *
 * // Register a network policy for "alice"
 * security::NetworkPolicy p;
 * p.policy_id    = "corp-net";
 * p.identity     = "alice";
 * p.allowed_cidrs = {"10.0.0.0/8"};
 * p.default_deny  = true;
 * verifier.addNetworkPolicy(p);
 *
 * // For every inbound request:
 * ZeroTrustAuthVerifier::Request req;
 * req.request_id = generate_uuid();
 * req.user_id    = jwt_claims.sub;
 * req.token      = bearer_token;
 * req.client_ip  = peer_address;
 * req.resource   = "data";
 * req.action     = "read";
 *
 * auto decision = verifier.verify(req);
 * if (!decision.allowed) {
 *     return http_403(decision.reason);
 * }
 * @endcode
 */
class ZeroTrustAuthVerifier {
public:
    /**
     * @brief Token verifier callback type.
     *
     * Callers inject a validator that receives (token, user_id) and returns
     * true when the token is authentic and belongs to the given user_id.
     * Typically wraps JWTValidator::parseAndValidate.
     */
    using TokenVerifier = security::ZeroTrustPolicyEnforcer::TokenVerifier;

    /**
     * @brief Configuration for the verifier.
     */
    struct Config {
        /// Minimum composite trust score to allow a request [0.0, 1.0].
        /// Requests scoring below this threshold are denied even if token
        /// and network checks pass individually.
        double min_trust_score = 0.7;

        /// When true, a missing device_id causes an automatic score penalty
        /// (0.1 deducted by the underlying trust score computation).
        /// This flag documents the expectation; the penalty is always applied.
        bool device_id_expected = false;

        /// Interval between periodic background re-evaluations of long-lived
        /// sessions (WebSocket, gRPC streaming, DB connection pool).
        /// Re-evaluation runs on AuthWorkerThreadPool so it never blocks the
        /// data-plane thread.  Defaults to 300 000 ms (5 minutes).
        /// Use millisecond granularity so tests and short intervals are supported
        /// without narrowing conversions.
        std::chrono::milliseconds re_evaluation_interval{std::chrono::seconds(300)};
    };

    /**
     * @brief Per-request verification input.
     *
     * Callers must populate this for every request — there is no session
     * cache; continuous re-verification is the contract.
     */
    struct Request {
        std::string request_id;               ///< Unique request identifier
        std::string user_id;                  ///< Claimed identity
        std::string token;                    ///< Bearer token / API key
        std::string client_ip;                ///< Source IPv4 address
        std::string resource;                 ///< Resource being accessed
        std::string action;                   ///< Action (read / write / delete …)
        std::optional<std::string> device_id; ///< Optional device identifier
    };

    /**
     * @brief Identity snapshot used for background re-evaluation of a live session.
     *
     * Register a session via startSessionMonitoring() to have its zero-trust
     * posture periodically re-checked on a background worker thread.  If the
     * check fails, the session is automatically revoked via the supplied
     * SessionManager and an audit event is emitted.
     */
    struct MonitoredSession {
        std::string session_id;               ///< Session identifier (passed to terminateSession)
        std::string user_id;                  ///< Claimed identity
        std::string token;                    ///< Bearer token / API key
        std::string client_ip;                ///< Source IP at session creation
        std::string resource;                 ///< Primary resource (informational)
        std::string action;                   ///< Primary action (informational)
        std::optional<std::string> device_id; ///< Optional device identifier
    };

    /**
     * @brief Result of a single continuous verification call.
     */
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

    /**
     * @brief Construct with default configuration.
     *
     * A default-constructed verifier has no token_verifier (all tokens pass)
     * and no network policies (all source IPs pass). Inject both before
     * handling production traffic.
     */
    ZeroTrustAuthVerifier();
    /**
     * @brief Construct with default configuration and custom token verifier.
     * @param token_verifier Optional token validation callback.
     */
    explicit ZeroTrustAuthVerifier(TokenVerifier token_verifier);
    /**
     * @brief Construct with explicit configuration.
     * @param config Verifier configuration.
     * @param token_verifier Optional token validation callback.
     */
    ZeroTrustAuthVerifier(
        const Config& config,
        TokenVerifier token_verifier = nullptr);

    /**
     * @brief Destructor — stops the background re-evaluation loop if running.
     */
    ~ZeroTrustAuthVerifier();

    // ========================================================================
    // Dependency injection
    // ========================================================================

    /**
     * @brief Attach an AuditLogger to receive zero-trust decision events.
     * Pass nullptr to detach.  The verifier does NOT take ownership.
     */
    void setAuditLogger(utils::AuditLogger* logger) { audit_logger_ = logger; }

    // ========================================================================
    // Network policy management (delegated to the underlying enforcer)
    // ========================================================================

    /**
     * @brief Register a network policy.
     * @see security::ZeroTrustPolicyEnforcer::addNetworkPolicy
     */
    void addNetworkPolicy(const security::NetworkPolicy& policy);

    /**
     * @brief Remove a network policy by id.
     * @return true if found and removed.
     */
    bool removeNetworkPolicy(const std::string& policy_id);

    /**
     * @brief Snapshot of all currently registered policies.
     */
    std::vector<security::NetworkPolicy> getNetworkPolicies() const;

    // ========================================================================
    // Core: continuous per-request verification
    // ========================================================================

    /**
     * @brief Verify a single request — the primary entry point.
     *
     * Always performs a full verification (token + network + trust score).
     * There is deliberately no session cache; callers must invoke this for
     * every request to satisfy the "continuous verification" contract.
     *
     * Steps:
     *   1. Build ZeroTrustContext from Request fields
     *   2. Delegate to ZeroTrustPolicyEnforcer::verify()
     *   3. Apply min_trust_score threshold
     *   4. Emit audit event
     *   5. Return Decision
     *
     * @param req Per-request input (must be freshly populated)
     * @return Decision with pass/fail and diagnostic details
     */
    Decision verify(const Request& req);

    // ========================================================================
    // Background session monitoring (async policy re-evaluation)
    // ========================================================================

    /**
     * @brief Register a long-lived session for periodic background re-evaluation.
     *
     * Starts an internal background worker thread (if not already running) and
     * adds the session to the re-evaluation schedule.  Every
     * Config::re_evaluation_interval seconds the session's zero-trust posture is
     * re-checked on a worker thread from AuthWorkerThreadPool — never on the
     * data-plane thread.
     *
     * If the re-evaluation fails the session is revoked via
     * session_manager->terminateSession() and an audit event
     * "zero_trust/re_evaluation_failed" is emitted.
     *
     * @param session        Snapshot of the session credentials to monitor.
     * @param session_manager Non-owning pointer to the session manager that owns
     *                       the session.  Must remain valid until the session is
     *                       stopped or the verifier is destroyed.
     */
    void startSessionMonitoring(const MonitoredSession& session,
                                SessionManager* session_manager);

    /**
     * @brief Unregister a session from background re-evaluation.
     *
     * No-op if the session is not currently monitored.  Safe to call from any
     * thread, including from within a re-evaluation callback.
     *
     * @param session_id Session identifier passed to startSessionMonitoring().
     */
    void stopSessionMonitoring(const std::string& session_id);

    /**
     * @brief Number of sessions currently registered for background monitoring.
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

    /// Internal representation of a monitored session.
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
    /// Incremented whenever the session schedule changes (new session added).
    /// The monitor loop predicate checks this to wake early on schedule updates.
    std::atomic<uint64_t>                               schedule_generation_{0};

    /// Background loop: wakes periodically and dispatches overdue sessions
    /// to the worker thread pool for policy re-evaluation.
    void monitorLoop();

    /// Executed on a worker thread: re-evaluates one session and terminates
    /// it (+ emits audit event) if the policy check fails.
    void reEvaluateSession(const MonitorEntry& entry);
};

} // namespace auth
} // namespace themis
