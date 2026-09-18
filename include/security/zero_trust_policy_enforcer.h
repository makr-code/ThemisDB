/**
 * @file zero_trust_policy_enforcer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
#include <array>
#include <cstdint>

namespace themis {
namespace security {

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

    // ── Continuous re-verification fields (Phase 3.1) ─────────────────────
    std::chrono::system_clock::time_point last_verified_at{};

    double session_risk_score = 0.0;
};

struct NetworkPolicy {
    std::string policy_id;                    ///< Unique policy identifier
    std::string identity;                     ///< user_id or role name this policy applies to
    std::vector<std::string> allowed_cidrs;   ///< CIDRs from which access is permitted (IPv4 and IPv6)
    std::vector<std::string> denied_cidrs;    ///< CIDRs that are always blocked (takes precedence)
    bool default_deny = true;                 ///< Zero-trust: deny unless explicitly allowed
    std::optional<std::chrono::seconds> max_token_age; ///< Maximum token age for this identity

    // ── Continuous re-verification (Phase 3.1) ────────────────────────────
    std::chrono::milliseconds continuous_verification_interval_ms{0};

    double risk_score_threshold = 1.0;
};

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

class ZeroTrustPolicyEnforcer {
public:
    using TokenVerifier = std::function<bool(const std::string& token,
                                             const std::string& user_id)>;

    explicit ZeroTrustPolicyEnforcer(TokenVerifier token_verifier = nullptr);

    ~ZeroTrustPolicyEnforcer() = default;

    // ========================================================================
    // Network policy management
    // ========================================================================

    /**
     * @brief Register a network policy.
     * @param[in] policy Network policy to add.
     */
    void addNetworkPolicy(const NetworkPolicy& policy);

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
    std::vector<NetworkPolicy> getNetworkPolicies() const;


    /**
     * @brief Verify identity and enforce network policies for a request.
     * @param[in] context Zero-trust request context to verify.
     * @return Verification result.
     */
    VerificationResult verify(const ZeroTrustContext& context);

    // ========================================================================
    // Individual checks (usable for testing or staged enforcement)
    // ========================================================================

    // ========================================================================
    // Fail-closed configuration
    // ========================================================================

    void setAllowUnverifiedToken(bool allow) noexcept { allow_unverified_token_ = allow; }

    void setAllowEmptyNetworkPolicies(bool allow) noexcept { allow_empty_network_policies_ = allow; }


    /**
     * @brief Verify a bearer token or credential for a user.
     * @param[in] token Token to verify.
     * @param[in] user_id User identifier.
     * @return True when the token is valid.
     */
    bool verifyToken(const std::string& token, const std::string& user_id) const;

    /**
     * @brief Check whether an IP address is allowed for an identity.
     * @param[in] client_ip Client IP address.
     * @param[in] identity Identity associated with the request.
     * @return True when the IP is allowed.
     */
    bool isIpAllowed(const std::string& client_ip, const std::string& identity) const;

    /**
     * @brief Compute the composite zero-trust score.
     * @param[in] context Zero-trust request context.
     * @param[in] identity_verified True if identity verification succeeded.
     * @param[in] network_ok True if network policy checks passed.
     * @return Composite zero-trust score.
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
    /**
     * @brief Check whether an IP address matches a CIDR range.
     * @param[in] ip IP address to check.
     * @param[in] cidr CIDR range to compare against.
     * @return True when the IP matches the CIDR.
     */
    static bool ipMatchesCidr(const std::string& ip, const std::string& cidr);

    /**
     * @brief Parse an IPv4 address into an integer representation.
     * @param[in] ip IPv4 address string.
     * @param[in,out] out Output numeric IPv4 value.
     * @return True when the IPv4 address parsed successfully.
     */
    static bool parseIpv4(const std::string& ip, uint32_t& out);

    static bool parseIpv6(const std::string& ip, std::array<uint8_t, 16>& out);

    /**
     * @brief Check whether an IPv6 address matches a CIDR range.
     * @param[in] ip IPv6 address to check.
     * @param[in] cidr CIDR range to compare against.
     * @return True when the IP matches the CIDR.
     */
    static bool ipv6MatchesCidr(const std::string& ip, const std::string& cidr);

    /**
     * @brief Normalize an IPv4-mapped IPv6 address.
     * @param[in] ip IP address to normalize.
     * @return Normalized IP string.
     */
    static std::string normaliseIpv4MappedIpv6(const std::string& ip);

    /**
     * @brief Check whether an IP matches any CIDR in a policy.
     * @param[in] ip IP address to check.
     * @param[in] cidr CIDR range to compare against.
     * @return True when the IP matches at least one CIDR.
     */
    static bool ipMatchesCidrAny(const std::string& ip, const std::string& cidr);

    /**
     * @brief Find the policy that applies to an identity.
     * @param[in] identity Identity to look up.
     * @return Matching network policy or null if none.
     */
    const NetworkPolicy* findPolicyForIdentity(const std::string& identity) const;

    mutable std::mutex mutex_;
    std::unordered_map<std::string, NetworkPolicy> policies_; ///< Keyed by policy_id
    TokenVerifier token_verifier_;
    mutable Metrics metrics_;
    bool allow_unverified_token_{false};
    bool allow_empty_network_policies_{false};
};

} // namespace security
} // namespace themis
