/**
 * @file api_security_audit.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "server/api_auth_config.h"

#include <string>
#include <vector>
#include <cstdint>

namespace themis {
namespace server {

/**
 * @brief Severity level for a security audit finding
 */
enum class AuditSeverity : uint8_t {
    LOW      = 0,
    MEDIUM   = 1,
    HIGH     = 2,
    CRITICAL = 3
};

/**
 * @brief A single security finding produced by the API security audit
 */
struct ApiSecurityAuditFinding {
    AuditSeverity   severity;
    std::string     endpoint_pattern;   ///< Empty string for global findings
    std::string     http_method;        ///< Empty string for global findings
    std::string     finding;            ///< Human-readable description of the issue
    std::string     recommendation;     ///< Actionable remediation guidance
};

/**
 * @brief Aggregated report produced by ApiSecurityAuditor::audit()
 */
struct ApiSecurityAuditReport {
    std::vector<ApiSecurityAuditFinding> findings;

    uint32_t critical_count = 0;
    uint32_t high_count     = 0;
    uint32_t medium_count   = 0;
    uint32_t low_count      = 0;

    /// @brief True when no HIGH or CRITICAL findings are present
    bool passed = true;
};

/**
 * @brief Performs a static security audit of an ApiAuthConfig
 *
 * The auditor inspects the global settings and all endpoint-specific
 * configurations to identify security misconfigurations.  It produces a
 * structured report that can be evaluated at server start-up or in tests.
 *
 * Checks performed:
 *  - Global authentication disabled (CRITICAL)
 *  - Authentication-required endpoints with empty required_scope (HIGH)
 *  - Sensitive endpoint patterns without authentication (HIGH)
 *  - Rate limiting globally disabled (HIGH)
 *  - Authentication-required endpoints with no rate limit (MEDIUM)
 *  - Burst capacity exceeds half of per-minute rate limit (LOW)
 */
class ApiSecurityAuditor {
public:
    /**
     * @brief Run a full security audit against the given configuration
     * @param config The API authentication and rate-limiting configuration to audit
     * @return A report containing all findings and an overall pass/fail verdict
     */
    static ApiSecurityAuditReport audit(const ApiAuthConfig& config);

private:
    static void checkGlobalAuthDisabled(
        const ApiAuthConfig& config,
        std::vector<ApiSecurityAuditFinding>& findings);

    static void checkMissingScopeOnAuthEndpoints(
        const ApiAuthConfig& config,
        std::vector<ApiSecurityAuditFinding>& findings);

    static void checkSensitiveEndpointsRequireAuth(
        const ApiAuthConfig& config,
        std::vector<ApiSecurityAuditFinding>& findings);

    static void checkRateLimitingDisabled(
        const ApiAuthConfig& config,
        std::vector<ApiSecurityAuditFinding>& findings);

    static void checkMissingRateLimitOnAuthEndpoints(
        const ApiAuthConfig& config,
        std::vector<ApiSecurityAuditFinding>& findings);

    static void checkExcessiveBurstCapacity(
        const ApiAuthConfig& config,
        std::vector<ApiSecurityAuditFinding>& findings);
};

} // namespace server
} // namespace themis
