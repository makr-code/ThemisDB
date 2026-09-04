/**
 * @file zero_trust_policy_enforcer.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=5; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=2, Debt=0, C=0, H=0, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "security/zero_trust_policy_enforcer.h"
#include "utils/logger.h"

#include <algorithm>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#endif
#include <chrono>
#include <cstdio>
#include <cstring>
#include <sstream>
#include <stdexcept>

namespace themis {
namespace security {

// ============================================================================
// Constructor
// ============================================================================

ZeroTrustPolicyEnforcer::ZeroTrustPolicyEnforcer(TokenVerifier token_verifier)
    : token_verifier_(std::move(token_verifier)) {}

// ============================================================================
// Policy management
// ============================================================================

void ZeroTrustPolicyEnforcer::addNetworkPolicy(const NetworkPolicy& policy) {
    std::lock_guard<std::mutex> lock(mutex_);
    policies_[policy.policy_id] = policy;
    THEMIS_INFO("ZeroTrust: added network policy '{}' for identity '{}'",
                policy.policy_id, policy.identity);
}

bool ZeroTrustPolicyEnforcer::removeNetworkPolicy(const std::string& policy_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = policies_.find(policy_id);
    if (it == policies_.end()) {
        return false;
    }
    THEMIS_INFO("ZeroTrust: removed network policy '{}'", policy_id);
    policies_.erase(it);
    return true;
}

std::vector<NetworkPolicy> ZeroTrustPolicyEnforcer::getNetworkPolicies() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<NetworkPolicy> result = {};

    result.reserve(policies_.size());
    for (const auto& kv : policies_) {
        result.push_back(kv.second);
    }
    return result;
}

// ============================================================================
// Core: per-request verification
// ============================================================================

VerificationResult ZeroTrustPolicyEnforcer::verify(const ZeroTrustContext& context) {
    metrics_.requests_total.fetch_add(1, std::memory_order_relaxed);

    // ── Continuous re-verification (Phase 3.1) ────────────────────────────
    // Check the active policy for this identity to determine the re-verification
    // interval and risk-score threshold before the token/network checks.
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const NetworkPolicy* policy = findPolicyForIdentity(context.user_id);

        if (policy) {
            // Risk-score revocation: if the accumulated session risk score
            // exceeds the threshold, immediately revoke the session.
            if (context.session_risk_score > policy->risk_score_threshold) {
                metrics_.requests_denied.fetch_add(1, std::memory_order_relaxed);
                THEMIS_WARN("ZeroTrust: session revoked — risk_score={:.3f} exceeds "
                            "threshold={:.3f} for user='{}' request='{}'",
                            context.session_risk_score, policy->risk_score_threshold,
                            context.user_id, context.request_id);
                auto result = VerificationResult::Deny(
                    context.request_id,
                    "Session revoked: risk score exceeded threshold",
                    0.0,
                    policy->policy_id
                );
                return result;
            }

            // Continuous re-verification interval: if the interval is non-zero
            // and the session has not been re-verified within that window,
            // fall through and perform a full token + network check now.
            if (policy->continuous_verification_interval_ms.count() > 0) {
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now() - context.last_verified_at);
                if (elapsed >= policy->continuous_verification_interval_ms) {
                    THEMIS_DEBUG("ZeroTrust: continuous re-verification triggered for "
                                 "user='{}' (elapsed={}ms interval={}ms)",
                                 context.user_id, elapsed.count(),
                                 policy->continuous_verification_interval_ms.count());
                    // Fall through to full token + network check below.
                }
            }
        }
    }

    // Step 1: identity (token) verification
    bool identity_ok = verifyToken(context.token, context.user_id);
    if (!identity_ok) {
        metrics_.identity_failures.fetch_add(1, std::memory_order_relaxed);
        metrics_.requests_denied.fetch_add(1, std::memory_order_relaxed);
        THEMIS_WARN("ZeroTrust: identity verification failed for user='{}' request='{}'",
                    context.user_id, context.request_id);
        auto result = VerificationResult::Deny(
            context.request_id,
            "Identity verification failed: invalid or missing token",
            0.0
        );
        result.identity_verified = false;
        return result;
    }

    // Step 2: network policy check
    bool network_ok = isIpAllowed(context.client_ip, context.user_id);
    if (!network_ok) {
        metrics_.network_policy_denials.fetch_add(1, std::memory_order_relaxed);
        metrics_.requests_denied.fetch_add(1, std::memory_order_relaxed);
        THEMIS_WARN("ZeroTrust: network policy denied user='{}' ip='{}' request='{}'",
                    context.user_id, context.client_ip, context.request_id);

        // Find policy_id for audit trail
        std::string pid = {};
        {
            std::lock_guard<std::mutex> lock(mutex_);
            const auto* p = findPolicyForIdentity(context.user_id);
            if (p) {
              pid = p->policy_id;
            }
        }
        auto result = VerificationResult::Deny(
            context.request_id,
            "Network policy denied access from IP: " + context.client_ip,
            0.0,
            pid
        );
        result.identity_verified = true;
        return result;
    }

    // Step 3: compute trust score
    double trust = computeTrustScore(context, identity_ok, network_ok);

    // Step 4: allow
    metrics_.requests_allowed.fetch_add(1, std::memory_order_relaxed);
    THEMIS_DEBUG("ZeroTrust: allowed user='{}' ip='{}' resource='{}' action='{}' "
                 "trust_score={:.2f} request='{}'",
                 context.user_id, context.client_ip, context.resource,
                 context.action, trust, context.request_id);

    auto result = VerificationResult::Allow(context.request_id, trust);
    return result;
}

// ============================================================================
// Individual checks
// ============================================================================

bool ZeroTrustPolicyEnforcer::verifyToken(const std::string& token,
                                          const std::string& user_id) const {
    if (!token_verifier_) {
        // No verifier configured — fail-closed by default.
        // Call setAllowUnverifiedToken(true) in test environments to override.
        if (!allow_unverified_token_) {
            THEMIS_WARN("ZeroTrust: verifyToken() called with no TokenVerifier set — denying (fail-closed)");
            return false;
        }
        return true;
    }
    try {
        return token_verifier_(token, user_id);
    } catch (const std::exception& e) {
        THEMIS_ERROR("ZeroTrust: token verifier threw: {}", e.what());
        return false;
    }
}

bool ZeroTrustPolicyEnforcer::isIpAllowed(const std::string& client_ip,
                                           const std::string& identity) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const NetworkPolicy* policy = findPolicyForIdentity(identity);

    if (!policy) {
        // No policy registered for this identity.
        if (policies_.empty()) {
            // No policies at all — fail-closed by default.
            // Call setAllowEmptyNetworkPolicies(true) for phased roll-out / migration.
            if (!allow_empty_network_policies_) {
                THEMIS_WARN("ZeroTrust: isIpAllowed() called with no network policies configured — denying (fail-closed)");
                return false;
            }
            return true;
        }
        // Policies exist but none match this identity → deny by default
        THEMIS_DEBUG("ZeroTrust: no network policy for identity '{}', default deny", identity);
        return false;
    }

    // Check denied CIDRs first (take precedence over allowed)
    for (const auto& cidr : policy->denied_cidrs) {
        if (ipMatchesCidrAny(client_ip, cidr)) {
            THEMIS_DEBUG("ZeroTrust: IP '{}' matched denied CIDR '{}'", client_ip, cidr);
            return false;
        }
    }

    // Check allowed CIDRs
    for (const auto& cidr : policy->allowed_cidrs) {
        if (ipMatchesCidrAny(client_ip, cidr)) {
            return true;
        }
    }

    // No allowed CIDR matched
    if (policy->default_deny) {
        THEMIS_DEBUG("ZeroTrust: IP '{}' not in allowed CIDRs for identity '{}', default deny",
                     client_ip, identity);
        return false;
    }
    return true;
}

double ZeroTrustPolicyEnforcer::computeTrustScore(const ZeroTrustContext& context,
                                                    bool identity_verified,
                                                    bool network_ok) const {
    double score = 0.0;

    if (identity_verified) {
      score += 0.4;
    }
    if (network_ok) {
      score += 0.4;
    }
    if (context.device_id.has_value() && !context.device_id->empty()) {
      score += 0.1;
    }

    // Request freshness: full credit if timestamp is within 60 seconds of now
    auto age = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now() - context.timestamp);
    if (age.count() <= 60) {
        score += 0.1;
    }

    // Deduct session risk score [0.0, 1.0] from the computed score.
    // A session_risk_score of 0 has no impact; a score of 1 collapses trust to 0.
    score -= context.session_risk_score;

    // Clamp to [0.0, 1.0]
    if (score < 0.0) {
      score = 0.0;
    }
    if (score > 1.0) {
      score = 1.0;
    }

    return score;
}

// ============================================================================
// Private helpers
// ============================================================================

const NetworkPolicy* ZeroTrustPolicyEnforcer::findPolicyForIdentity(
    const std::string& identity) const {
    // Linear scan – policy sets are typically small
    for (const auto& kv : policies_) {
        if (kv.second.identity == identity) {
            return &kv.second;
        }
    }
    return nullptr;
}

bool ZeroTrustPolicyEnforcer::parseIpv4(const std::string& ip, uint32_t& out) {
    // Parse dotted-decimal IPv4 (e.g. "192.168.1.100")
    unsigned int a = 0, b = 0, c = 0, d = 0;
    char extra = 0;
    // sscanf is acceptable here; ip is user-supplied but we validate the format
    int n = std::sscanf(ip.c_str(), "%u.%u.%u.%u%c", &a, &b, &c, &d, &extra);
    if (n != 4) {
      return false;
    }
    if (a > 255 || b > 255 || c > 255 || d > 255) {
      return false;
    }
    out = (a << 24) | (b << 16) | (c << 8) | d;
    return true;
}

bool ZeroTrustPolicyEnforcer::ipMatchesCidr(const std::string& ip,
                                              const std::string& cidr) {
    // Split CIDR into address and prefix length
    auto slash = cidr.find('/');
    if (slash == std::string::npos) {
        // Treat bare IP as /32
        return ip == cidr;
    }

    std::string cidr_addr = cidr.substr(0, slash);
    std::string prefix_str = cidr.substr(slash + 1);

    int prefix_len = 0;
    try {
        prefix_len = std::stoi(prefix_str);
    } catch (const std::exception& e) {
        THEMIS_WARN("ZeroTrust: invalid CIDR prefix '{}' in '{}': {}", prefix_str, cidr, e.what());
        return false;
    } catch (...) {
        THEMIS_WARN("ZeroTrust: invalid CIDR prefix '{}' in '{}'", prefix_str, cidr);
        return false;
    }
    if (prefix_len < 0 || prefix_len > 32) {
      return false;
    }

    uint32_t ip_int = 0;
    uint32_t cidr_int = 0;
    if (!parseIpv4(ip, ip_int)) {
      return false;
    }
    if (!parseIpv4(cidr_addr, cidr_int)) {
      return false;
    }

    if (prefix_len == 0) {
        // /0 matches everything
        return true;
    }

    uint32_t mask = (~uint32_t{0}) << (32 - prefix_len);
    return (ip_int & mask) == (cidr_int & mask);
}

// ============================================================================
// IPv6 helpers (Phase 3.2)
// ============================================================================

bool ZeroTrustPolicyEnforcer::parseIpv6(const std::string& ip,
                                         std::array<uint8_t, 16>& out) {
    // inet_pton(AF_INET6) handles all standard IPv6 representations including
    // :: abbreviation and IPv4-mapped form (::ffff:a.b.c.d).
    int rc = ::inet_pton(AF_INET6, ip.c_str(), out.data());
    return rc == 1;
}

bool ZeroTrustPolicyEnforcer::ipv6MatchesCidr(const std::string& ip,
                                                const std::string& cidr) {
    auto slash = cidr.find('/');
    if (slash == std::string::npos) {
        // Treat bare address as /128 exact match
        std::array<uint8_t, 16> a{}, b{};
        if (!parseIpv6(ip, a) || !parseIpv6(cidr, b)) {
          return false;
        }
        return a == b;
    }

    std::string cidr_addr = cidr.substr(0, slash);
    std::string prefix_str = cidr.substr(slash + 1);

    int prefix_len = 0;
    try {
        prefix_len = std::stoi(prefix_str);
    } catch (...) {
        THEMIS_WARN("ZeroTrust: invalid IPv6 CIDR prefix '{}' in '{}'", prefix_str, cidr);
        return false;
    }
    if (prefix_len < 0 || prefix_len > 128) {
      return false;
    }

    std::array<uint8_t, 16> ip_bytes{};
    std::array<uint8_t, 16> cidr_bytes{};
    if (!parseIpv6(ip, ip_bytes)) {
      return false;
    }
    if (!parseIpv6(cidr_addr, cidr_bytes)) {
      return false;
    }

    if (prefix_len == 0) return true; // /0 matches everything

    // Compare the leading prefix_len bits
    int full_bytes = prefix_len / 8;
    int remaining_bits = prefix_len % 8;

    for (int i = 0; i < full_bytes; ++i) {
        if (ip_bytes[i] != cidr_bytes[i]) {
          return false;
        }
    }
    if (remaining_bits > 0) {
        uint8_t mask = static_cast<uint8_t>(0xFFu << (8 - remaining_bits));
        if ((ip_bytes[full_bytes] & mask) != (cidr_bytes[full_bytes] & mask)) {
            return false;
        }
    }
    return true;
}

std::string ZeroTrustPolicyEnforcer::normaliseIpv4MappedIpv6(const std::string& ip) {
    // IPv4-mapped IPv6 addresses start with ::ffff: in text form
    constexpr std::string_view kPrefix = "::ffff:";
    if (static_cast<int>(ip.size()) > static_cast<int>(kPrefix.size()) &&
        ip.substr(0,static_cast<int>(kPrefix.size())) == kPrefix) {
        std::string candidate = ip.substr(kPrefix.size());
        uint32_t dummy = 0;
        if (parseIpv4(candidate, dummy)) {
            return candidate; // Return the IPv4 portion
        }
    }
    return ip;
}

bool ZeroTrustPolicyEnforcer::ipMatchesCidrAny(const std::string& ip,
                                                const std::string& cidr) {
    // Detect IPv6 by presence of ':' in either the IP or the CIDR base address.
    // If the IP is IPv4-mapped IPv6, normalise it first.
    std::string normalised_ip = normaliseIpv4MappedIpv6(ip);

    bool ip_is_v6 = (normalised_ip.find(':') != std::string::npos);
    bool cidr_is_v6 = (cidr.find(':') != std::string::npos);

    if (ip_is_v6 || cidr_is_v6) {
        if (!ip_is_v6 || !cidr_is_v6) {
            // Protocol mismatch after normalisation – cannot match
            return false;
        }
        return ipv6MatchesCidr(normalised_ip, cidr);
    }
    return ipMatchesCidr(normalised_ip, cidr);
}

} // namespace security
} // namespace themis


