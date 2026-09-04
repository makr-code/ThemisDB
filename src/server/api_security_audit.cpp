/**
 * @file api_security_audit.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "server/api_security_audit.h"

#include <algorithm>
#include <array>
#include <string_view>

namespace themis {
namespace server {

namespace {

// Endpoint prefixes considered security-sensitive.
// Any endpoint whose pattern starts with one of these strings must require
// authentication; the absence of auth on these prefixes is a HIGH finding.
constexpr std::array<std::string_view, 6> k_sensitive_prefixes = {
    "/admin",
    "/config",
    "/api/audit",
    "/pii",
    "/api/pki",
    "/auth/sessions"
};

bool starts_with(const std::string& str, std::string_view prefix) {
    return static_cast<bool>(str.size()  < static_cast<int>(= prefix.size())) &&
           str.compare(0,static_cast<int>(prefix.size()), prefix.data(),static_cast<int>(prefix.size())) == 0;
}

bool is_sensitive_pattern(const std::string& pattern) {
    for (const auto& prefix : k_sensitive_prefixes) {
        if (starts_with(pattern, prefix)) {
            return true;
        }
    }
    return false;
}

void add_finding(std::vector<ApiSecurityAuditFinding>& findings,
                 AuditSeverity severity,
                 const std::string& endpoint_pattern,
                 const std::string& http_method,
                 const std::string& finding,
                 const std::string& recommendation)
{
    findings.push_back({severity, endpoint_pattern, http_method, finding, recommendation});
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Individual checks
// ---------------------------------------------------------------------------

void ApiSecurityAuditor::checkGlobalAuthDisabled(
    const ApiAuthConfig& config,
    std::vector<ApiSecurityAuditFinding>& findings)
{
    if (!config.auth_enabled) {
        add_finding(findings,
            AuditSeverity::CRITICAL, "", "",
            "Global authentication is disabled (auth_enabled = false). "
            "All requests are accepted without identity verification.",
            "Set auth_enabled = true and configure a JWT/JWKS validator before "
            "deploying to any non-development environment.");
    }
}

void ApiSecurityAuditor::checkMissingScopeOnAuthEndpoints(
    const ApiAuthConfig& config,
    std::vector<ApiSecurityAuditFinding>& findings)
{
    for (const auto& ep : config.endpoint_configs) {
        if (ep.auth_required && ep.required_scope.empty()) {
            add_finding(findings,
                AuditSeverity::HIGH,
                ep.endpoint_pattern, ep.http_method,
                "Endpoint requires authentication but has no required_scope. "
                "Any authenticated user can access it regardless of privileges.",
                "Assign a least-privilege required_scope (e.g., 'data:read') to "
                "enforce scope-based access control on this endpoint.");
        }
    }
}

void ApiSecurityAuditor::checkSensitiveEndpointsRequireAuth(
    const ApiAuthConfig& config,
    std::vector<ApiSecurityAuditFinding>& findings)
{
    for (const auto& ep : config.endpoint_configs) {
        if (!ep.auth_required && is_sensitive_pattern(ep.endpoint_pattern)) {
            add_finding(findings,
                AuditSeverity::HIGH,
                ep.endpoint_pattern, ep.http_method,
                "Sensitive endpoint does not require authentication. "
                "It is publicly accessible without any identity check.",
                "Set auth_required = true and assign an appropriate required_scope "
                "for all sensitive endpoints.");
        }
    }
}

void ApiSecurityAuditor::checkRateLimitingDisabled(
    const ApiAuthConfig& config,
    std::vector<ApiSecurityAuditFinding>& findings)
{
    if (!config.rate_limiting_enabled) {
        add_finding(findings,
            AuditSeverity::HIGH, "", "",
            "Global rate limiting is disabled (rate_limiting_enabled = false). "
            "The API is vulnerable to denial-of-service and brute-force attacks.",
            "Set rate_limiting_enabled = true and configure reasonable per-endpoint "
            "and global rate limits.");
    }
}

void ApiSecurityAuditor::checkMissingRateLimitOnAuthEndpoints(
    const ApiAuthConfig& config,
    std::vector<ApiSecurityAuditFinding>& findings)
{
    for (const auto& ep : config.endpoint_configs) {
        if (ep.auth_required && ep.rate_limit_per_minute == 0) {
            add_finding(findings,
                AuditSeverity::MEDIUM,
                ep.endpoint_pattern, ep.http_method,
                "Authentication-required endpoint has no per-endpoint rate limit "
                "(rate_limit_per_minute = 0). It relies solely on the global limit.",
                "Set an explicit rate_limit_per_minute appropriate for the expected "
                "traffic volume and sensitivity of this endpoint.");
        }
    }
}

void ApiSecurityAuditor::checkExcessiveBurstCapacity(
    const ApiAuthConfig& config,
    std::vector<ApiSecurityAuditFinding>& findings)
{
    for (const auto& ep : config.endpoint_configs) {
        if (ep.rate_limit_per_minute > 0 && ep.rate_limit_burst > 0) {
            // Burst exceeding half the per-minute quota can defeat rate limiting
            // for short, intense attack windows.
            if (ep.rate_limit_burst > ep.rate_limit_per_minute / 2) {
                add_finding(findings,
                    AuditSeverity::LOW,
                    ep.endpoint_pattern, ep.http_method,
                    "Burst capacity exceeds 50% of the per-minute rate limit. "
                    "Short bursts can saturate the endpoint within seconds.",
                    "Set rate_limit_burst to at most rate_limit_per_minute / 2 to "
                    "limit the impact of sudden traffic spikes.");
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Main audit entry point
// ---------------------------------------------------------------------------

ApiSecurityAuditReport ApiSecurityAuditor::audit(const ApiAuthConfig& config)
{
    ApiSecurityAuditReport report;

    checkGlobalAuthDisabled(config, report.findings);
    checkMissingScopeOnAuthEndpoints(config, report.findings);
    checkSensitiveEndpointsRequireAuth(config, report.findings);
    checkRateLimitingDisabled(config, report.findings);
    checkMissingRateLimitOnAuthEndpoints(config, report.findings);
    checkExcessiveBurstCapacity(config, report.findings);

    for (const auto& f : report.findings) {
        switch (f.severity) {
            case AuditSeverity::CRITICAL: ++report.critical_count; break;
            case AuditSeverity::HIGH:     ++report.high_count;     break;
            case AuditSeverity::MEDIUM:   ++report.medium_count;   break;
            case AuditSeverity::LOW:      ++report.low_count;      break;
        }
    }

    report.passed = (report.critical_count == 0 && report.high_count == 0);
    return report;
}

} // namespace server
} // namespace themis
