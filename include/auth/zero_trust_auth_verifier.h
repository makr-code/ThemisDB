/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            zero_trust_auth_verifier.h                         ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-03-02 03:52:12                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     225                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • e8e02c9ec  2026-02-24  feat(auth): implement zero-trust continuous verification ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <optional>
#include <memory>
#include <functional>

#include "security/zero_trust_policy_enforcer.h"

namespace themis {
namespace utils { class AuditLogger; }
namespace auth {

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
     * @brief Construct with optional configuration.
     *
     * A default-constructed verifier has no token_verifier (all tokens pass)
     * and no network policies (all source IPs pass).  Inject both before
     * handling production traffic.
     *
     * @param config       Verifier configuration.
     * @param token_verifier Optional token validation callback.
     */
    explicit ZeroTrustAuthVerifier(
        const Config& config = Config(),
        TokenVerifier token_verifier = nullptr);

    ~ZeroTrustAuthVerifier() = default;

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
    // Metrics (read-only view of the underlying enforcer's counters)
    // ========================================================================

    const security::ZeroTrustPolicyEnforcer::Metrics& getMetrics() const {
        return enforcer_.getMetrics();
    }

private:
    Config config_;
    security::ZeroTrustPolicyEnforcer enforcer_;
    utils::AuditLogger* audit_logger_ = nullptr; ///< Non-owning; may be nullptr.
};

} // namespace auth
} // namespace themis
