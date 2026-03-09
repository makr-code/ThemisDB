/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            zero_trust_auth_verifier.cpp                       ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 03:57:18                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     128                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • e8e02c9ec  2026-02-24  feat(auth): implement zero-trust continuous verification ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "auth/zero_trust_auth_verifier.h"
#include "auth/auth_audit_logger.h"
#include "utils/logger.h"

namespace themis {
namespace auth {

// ============================================================================
// Construction
// ============================================================================

ZeroTrustAuthVerifier::ZeroTrustAuthVerifier(
    const Config& config,
    TokenVerifier token_verifier)
    : config_(config)
    , enforcer_(std::move(token_verifier))
{}

// ============================================================================
// Network policy management
// ============================================================================

void ZeroTrustAuthVerifier::addNetworkPolicy(const security::NetworkPolicy& policy) {
    enforcer_.addNetworkPolicy(policy);
}

bool ZeroTrustAuthVerifier::removeNetworkPolicy(const std::string& policy_id) {
    return enforcer_.removeNetworkPolicy(policy_id);
}

std::vector<security::NetworkPolicy> ZeroTrustAuthVerifier::getNetworkPolicies() const {
    return enforcer_.getNetworkPolicies();
}

// ============================================================================
// Core: continuous per-request verification
// ============================================================================

ZeroTrustAuthVerifier::Decision ZeroTrustAuthVerifier::verify(const Request& req) {
    // Build the per-request context for the underlying enforcer
    security::ZeroTrustContext ctx;
    ctx.request_id = req.request_id;
    ctx.user_id    = req.user_id;
    ctx.client_ip  = req.client_ip;
    ctx.token      = req.token;
    ctx.resource   = req.resource;
    ctx.action     = req.action;
    ctx.device_id  = req.device_id;
    ctx.timestamp  = std::chrono::system_clock::now();

    // Delegate to security layer enforcer (performs token + network checks)
    security::VerificationResult zt = enforcer_.verify(ctx);

    Decision decision;
    decision.request_id       = req.request_id;
    decision.trust_score      = zt.trust_score;
    decision.identity_verified = zt.identity_verified;
    decision.network_ok       = zt.network_policy_passed;

    if (!zt.verified) {
        decision.allowed = false;
        decision.reason  = zt.reason;
        THEMIS_WARN("ZeroTrustAuth: denied user='{}' resource='{}' action='{}' "
                    "reason='{}' request='{}'",
                    req.user_id, req.resource, req.action,
                    zt.reason, req.request_id);
        if (audit_logger_) {
            AuthAuditLogger al(audit_logger_);
            al.logZeroTrustDenied(req.user_id, req.resource, zt.reason, req.request_id);
        }
        return decision;
    }

    // Apply minimum trust score threshold
    if (zt.trust_score < config_.min_trust_score) {
        decision.allowed = false;
        decision.reason  = "Trust score " + std::to_string(zt.trust_score) +
                           " below minimum threshold " +
                           std::to_string(config_.min_trust_score);
        THEMIS_WARN("ZeroTrustAuth: denied (low trust score {:.2f} < {:.2f}) "
                    "user='{}' request='{}'",
                    zt.trust_score, config_.min_trust_score,
                    req.user_id, req.request_id);
        if (audit_logger_) {
            AuthAuditLogger al(audit_logger_);
            al.logZeroTrustDenied(req.user_id, req.resource, decision.reason, req.request_id);
        }
        return decision;
    }

    // All checks passed
    decision.allowed = true;
    decision.reason  = "Zero-trust continuous verification passed";
    THEMIS_DEBUG("ZeroTrustAuth: allowed user='{}' resource='{}' action='{}' "
                 "trust_score={:.2f} request='{}'",
                 req.user_id, req.resource, req.action,
                 zt.trust_score, req.request_id);
    if (audit_logger_) {
        AuthAuditLogger al(audit_logger_);
        al.logZeroTrustAllowed(req.user_id, req.resource, zt.trust_score, req.request_id);
    }
    return decision;
}

} // namespace auth
} // namespace themis
