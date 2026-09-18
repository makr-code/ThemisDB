/**
 * @file api_security_audit.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

enum class AuditSeverity : uint8_t {
    LOW      = 0,
    MEDIUM   = 1,
    HIGH     = 2,
    CRITICAL = 3
};

struct ApiSecurityAuditFinding {
    AuditSeverity   severity;
    std::string     endpoint_pattern;   ///< Empty string for global findings
    std::string     http_method;        ///< Empty string for global findings
    std::string     finding;            ///< Human-readable description of the issue
    std::string     recommendation;     ///< Actionable remediation guidance
};

struct ApiSecurityAuditReport {
    std::vector<ApiSecurityAuditFinding> findings;

    uint32_t critical_count = 0;
    uint32_t high_count     = 0;
    uint32_t medium_count   = 0;
    uint32_t low_count      = 0;

    bool passed = true;
};

class ApiSecurityAuditor {
public:
    /**
     * @brief Audit.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    static ApiSecurityAuditReport audit(const ApiAuthConfig& config);

private:
    /**
     * @brief Check Global Auth Disabled.
     * @param[in] config Input parameter.
     * @param[in,out] findings Input/output parameter.
     */
    static void checkGlobalAuthDisabled(
        const ApiAuthConfig& config,
        std::vector<ApiSecurityAuditFinding>& findings);

    /**
     * @brief Check Missing Scope On Auth Endpoints.
     * @param[in] config Input parameter.
     * @param[in,out] findings Input/output parameter.
     */
    static void checkMissingScopeOnAuthEndpoints(
        const ApiAuthConfig& config,
        std::vector<ApiSecurityAuditFinding>& findings);

    /**
     * @brief Check Sensitive Endpoints Require Auth.
     * @param[in] config Input parameter.
     * @param[in,out] findings Input/output parameter.
     */
    static void checkSensitiveEndpointsRequireAuth(
        const ApiAuthConfig& config,
        std::vector<ApiSecurityAuditFinding>& findings);

    /**
     * @brief Check Rate Limiting Disabled.
     * @param[in] config Input parameter.
     * @param[in,out] findings Input/output parameter.
     */
    static void checkRateLimitingDisabled(
        const ApiAuthConfig& config,
        std::vector<ApiSecurityAuditFinding>& findings);

    /**
     * @brief Check Missing Rate Limit On Auth Endpoints.
     * @param[in] config Input parameter.
     * @param[in,out] findings Input/output parameter.
     */
    static void checkMissingRateLimitOnAuthEndpoints(
        const ApiAuthConfig& config,
        std::vector<ApiSecurityAuditFinding>& findings);

    /**
     * @brief Check Excessive Burst Capacity.
     * @param[in] config Input parameter.
     * @param[in,out] findings Input/output parameter.
     */
    static void checkExcessiveBurstCapacity(
        const ApiAuthConfig& config,
        std::vector<ApiSecurityAuditFinding>& findings);
};

} // namespace server
} // namespace themis
