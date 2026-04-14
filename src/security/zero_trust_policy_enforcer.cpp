/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            zero_trust_policy_enforcer.cpp                     ║
  Version:         0.0.10                                             ║
  Last Modified:   2026-04-14 18:51:36                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     286                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 72c5fa5ba9  2026-02-23  feat(security): implement zero-trust network policy enfor... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "security/zero_trust_policy_enforcer.h"
#include "utils/logger.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
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
    std::vector<NetworkPolicy> result;
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
        std::string pid;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            const auto* p = findPolicyForIdentity(context.user_id);
            if (p) pid = p->policy_id;
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
        // No verifier configured – treat as pass-through (integration callers
        // are expected to supply a verifier in production).
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
        // Honour global zero-trust default: deny unless a policy explicitly permits.
        // However, if there are NO policies at all, allow (unconfigured system).
        if (policies_.empty()) {
            return true;
        }
        // Policies exist but none match this identity → deny by default
        THEMIS_DEBUG("ZeroTrust: no network policy for identity '{}', default deny", identity);
        return false;
    }

    // Check denied CIDRs first (take precedence over allowed)
    for (const auto& cidr : policy->denied_cidrs) {
        if (ipMatchesCidr(client_ip, cidr)) {
            THEMIS_DEBUG("ZeroTrust: IP '{}' matched denied CIDR '{}'", client_ip, cidr);
            return false;
        }
    }

    // Check allowed CIDRs
    for (const auto& cidr : policy->allowed_cidrs) {
        if (ipMatchesCidr(client_ip, cidr)) {
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

    if (identity_verified)  score += 0.4;
    if (network_ok)         score += 0.4;
    if (context.device_id.has_value() && !context.device_id->empty()) score += 0.1;

    // Request freshness: full credit if timestamp is within 60 seconds of now
    auto age = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now() - context.timestamp);
    if (age.count() <= 60) {
        score += 0.1;
    }

    // Clamp to [0.0, 1.0]
    if (score < 0.0) score = 0.0;
    if (score > 1.0) score = 1.0;

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
    if (n != 4) return false;
    if (a > 255 || b > 255 || c > 255 || d > 255) return false;
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
    if (prefix_len < 0 || prefix_len > 32) return false;

    uint32_t ip_int = 0;
    uint32_t cidr_int = 0;
    if (!parseIpv4(ip, ip_int)) return false;
    if (!parseIpv4(cidr_addr, cidr_int)) return false;

    if (prefix_len == 0) {
        // /0 matches everything
        return true;
    }

    uint32_t mask = (~uint32_t{0}) << (32 - prefix_len);
    return (ip_int & mask) == (cidr_int & mask);
}

} // namespace security
} // namespace themis
