/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            zero_trust_policy_enforcer.h                       ║
  Version:         0.0.7                                              ║
  Last Modified:   2026-04-13 20:26:11                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     272                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 72c5fa5ba9  2026-02-23  feat(security): implement zero-trust network policy enfor... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <unordered_map>
#include <chrono>
#include <mutex>
#include <atomic>
#include <functional>

namespace themis {
namespace security {

/**
 * @brief Per-request context for zero-trust identity verification
 *
 * Every inbound request must supply this context.  The enforcer uses
 * the fields to (a) re-verify the caller's identity and (b) check whether
 * the source network location is permitted under the active policies.
 */
struct ZeroTrustContext {
    std::string request_id;   ///< Unique request identifier (UUID or similar)
    std::string user_id;      ///< Claimed identity (from token/certificate)
    std::string client_ip;    ///< Source IP address (IPv4 or IPv6)
    std::string token;        ///< Bearer token, API key, or certificate fingerprint
    std::string resource;     ///< Resource being accessed
    std::string action;       ///< Action being performed (read / write / delete …)
    std::optional<std::string> device_id; ///< Optional device identifier
    std::chrono::system_clock::time_point timestamp = std::chrono::system_clock::now();
    std::unordered_map<std::string, std::string> attributes; ///< Extensible context
};

/**
 * @brief Network policy: defines which CIDRs are allowed/denied for an identity
 *
 * Zero-trust requires that access is granted only from explicitly permitted
 * network locations.  Policies can target a specific user or a role.
 */
struct NetworkPolicy {
    std::string policy_id;                    ///< Unique policy identifier
    std::string identity;                     ///< user_id or role name this policy applies to
    std::vector<std::string> allowed_cidrs;   ///< CIDRs from which access is permitted
    std::vector<std::string> denied_cidrs;    ///< CIDRs that are always blocked (takes precedence)
    bool default_deny = true;                 ///< Zero-trust: deny unless explicitly allowed
    std::optional<std::chrono::seconds> max_token_age; ///< Maximum token age for this identity
};

/**
 * @brief Result of per-request zero-trust verification
 */
struct VerificationResult {
    bool verified = false;                ///< Overall verification result
    bool identity_verified = false;       ///< Token/credential check passed
    bool network_policy_passed = false;   ///< Source IP is within allowed networks
    double trust_score = 0.0;            ///< Composite trust score [0.0, 1.0]
    std::string reason;                   ///< Human-readable explanation
    std::string request_id;              ///< Echo of ZeroTrustContext::request_id
    std::string policy_id;               ///< Policy that made the decision (if any)

    static VerificationResult Allow(
        const std::string& request_id,
        double trust_score = 1.0,
        const std::string& reason = "Zero-trust verification passed"
    ) {
        return {true, true, true, trust_score, reason, request_id, ""};
    }

    static VerificationResult Deny(
        const std::string& request_id,
        const std::string& reason,
        double trust_score = 0.0,
        const std::string& policy_id = ""
    ) {
        return {false, false, false, trust_score, reason, request_id, policy_id};
    }
};

/**
 * @brief Zero-trust network policy enforcer – per-request identity verification
 *
 * Implements the Phase 4 "Zero-trust network policy enforcement" roadmap item.
 *
 * Principles:
 *   - Never trust, always verify (every request is independently verified)
 *   - Deny by default unless explicitly permitted
 *   - Enforce network policies (IP/CIDR allowlists and denylists)
 *   - Emit a trust score combining identity, network, and device signals
 *
 * Integration:
 *   This class is designed to sit **in front of** AccessControlManager.
 *   Callers should:
 *     1. Call ZeroTrustPolicyEnforcer::verify() for every inbound request
 *     2. Only proceed to RBAC/ABAC evaluation if verify() returns verified=true
 *
 * Thread safety: all public methods are thread-safe.
 *
 * Example:
 * @code
 * ZeroTrustPolicyEnforcer zt;
 * zt.addNetworkPolicy({"admin-net", "admin", {"10.0.0.0/8"}, {}, true});
 *
 * ZeroTrustContext ctx;
 * ctx.request_id = "req-001";
 * ctx.user_id    = "alice";
 * ctx.client_ip  = "10.1.2.3";
 * ctx.token      = session_token;
 * ctx.resource   = "data";
 * ctx.action     = "read";
 *
 * auto result = zt.verify(ctx);
 * if (!result.verified) {
 *     return http_response(403, result.reason);
 * }
 * @endcode
 */
class ZeroTrustPolicyEnforcer {
public:
    /**
     * @brief Token verification callback type
     *
     * Callers may inject a custom token validator.  The callback receives
     * (token, user_id) and must return true when the token is authentic and
     * belongs to the supplied user_id.
     */
    using TokenVerifier = std::function<bool(const std::string& token,
                                             const std::string& user_id)>;

    /**
     * @brief Construct with optional token verifier
     * @param token_verifier Optional callback for token validation.
     *        When null, token verification is skipped (identity_verified = true).
     */
    explicit ZeroTrustPolicyEnforcer(TokenVerifier token_verifier = nullptr);

    ~ZeroTrustPolicyEnforcer() = default;

    // ========================================================================
    // Network policy management
    // ========================================================================

    /**
     * @brief Register a network policy
     * @param policy Policy to add.  If a policy with the same policy_id already
     *               exists it is replaced.
     */
    void addNetworkPolicy(const NetworkPolicy& policy);

    /**
     * @brief Remove a network policy by id
     * @return true if a policy was removed, false if not found
     */
    bool removeNetworkPolicy(const std::string& policy_id);

    /**
     * @brief Retrieve all currently registered policies (snapshot)
     */
    std::vector<NetworkPolicy> getNetworkPolicies() const;

    // ========================================================================
    // Core: per-request verification
    // ========================================================================

    /**
     * @brief Verify identity and enforce network policies for a single request
     *
     * Steps:
     *   1. Validate token/credential (via TokenVerifier callback if set)
     *   2. Look up network policies matching context.user_id
     *   3. Evaluate CIDR deny-list (blocked networks always denied)
     *   4. Evaluate CIDR allow-list (default_deny = true means deny if no match)
     *   5. Compute composite trust score
     *   6. Update metrics and return VerificationResult
     *
     * @param context Per-request context
     * @return VerificationResult with detailed pass/fail information
     */
    VerificationResult verify(const ZeroTrustContext& context);

    // ========================================================================
    // Individual checks (usable for testing or staged enforcement)
    // ========================================================================

    /**
     * @brief Verify a token/credential for the given user_id
     * @return true if no TokenVerifier is set (token check skipped) or
     *         if the TokenVerifier confirms the token
     */
    bool verifyToken(const std::string& token, const std::string& user_id) const;

    /**
     * @brief Check whether a source IP is allowed under the policies for identity
     *
     * @param client_ip IPv4 address (e.g. "192.168.1.1")
     * @param identity  user_id whose policies are evaluated
     * @return true if access is permitted from this IP
     */
    bool isIpAllowed(const std::string& client_ip, const std::string& identity) const;

    /**
     * @brief Compute a composite trust score for the given context
     *
     * Score contributions:
     *   - Identity verified: +0.4
     *   - Network policy passed: +0.4
     *   - Device ID present: +0.1
     *   - Request freshness (< 60 s): +0.1
     *
     * @return Score in [0.0, 1.0]
     */
    double computeTrustScore(const ZeroTrustContext& context,
                             bool identity_verified,
                             bool network_ok) const;

    // ========================================================================
    // Metrics
    // ========================================================================

    struct Metrics {
        std::atomic<uint64_t> requests_total{0};          ///< Total verify() calls
        std::atomic<uint64_t> identity_failures{0};       ///< Token verification failed
        std::atomic<uint64_t> network_policy_denials{0};  ///< Blocked by network policy
        std::atomic<uint64_t> requests_allowed{0};        ///< Passed all checks
        std::atomic<uint64_t> requests_denied{0};         ///< Failed any check
    };

    const Metrics& getMetrics() const { return metrics_; }

private:
    /// Check if a single IPv4 address falls within a CIDR block
    static bool ipMatchesCidr(const std::string& ip, const std::string& cidr);

    /// Convert dotted-decimal IPv4 string to 32-bit host-byte-order integer
    /// Returns false on parse error
    static bool parseIpv4(const std::string& ip, uint32_t& out);

    /// Find the first policy that applies to the given identity (by user_id).
    /// Returns nullptr if no policy is registered for this identity.
    const NetworkPolicy* findPolicyForIdentity(const std::string& identity) const;

    mutable std::mutex mutex_;
    std::unordered_map<std::string, NetworkPolicy> policies_; ///< Keyed by policy_id
    TokenVerifier token_verifier_;
    mutable Metrics metrics_;
};

} // namespace security
} // namespace themis
