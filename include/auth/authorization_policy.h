/**
 * @file authorization_policy.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 *
 * @note **Interface-Only Header**: Policy evaluation is delegated to external adapters
 *       (OPA, Apache Ranger). No direct .cpp implementation. See opa_adapter.cpp and
 *       ranger_adapter.cpp in src/server/.
 */

#pragma once

#include <chrono>
#include <map>
#include <string>
#include <vector>

namespace themis {
namespace auth {

// ---------------------------------------------------------------------------
// SubjectAttributes — identity and role attributes of the requesting principal
// ---------------------------------------------------------------------------

struct SubjectAttributes {
    std::string subject_id;
    std::string role;
    std::vector<std::string> groups;
    std::map<std::string, std::string> attributes;  ///< Custom key=value attributes.
    std::string tenant_id;
    std::string clearance_level;  ///< e.g. "confidential", "secret", "top_secret"
};

// ---------------------------------------------------------------------------
// ResourceAttributes — attributes of the target resource
// ---------------------------------------------------------------------------

struct ResourceAttributes {
    std::string resource_id;
    std::string resource_type;   ///< e.g. "document", "collection", "query"
    std::string owner_id;
    std::string classification;  ///< Data sensitivity label (e.g. "confidential").
    std::map<std::string, std::string> labels;
};

// ---------------------------------------------------------------------------
// EnvironmentAttributes — contextual attributes at the time of the request
// ---------------------------------------------------------------------------

struct EnvironmentAttributes {
    std::string client_ip;
    std::string geo_region;
    std::chrono::system_clock::time_point request_time;
    bool        is_mfa_verified  = false;
    std::string device_trust_level; ///< e.g. "managed", "personal", "unknown"
};

// ---------------------------------------------------------------------------
// PolicyDecision — tri-valued decision per XACML semantics
// ---------------------------------------------------------------------------

enum class PolicyDecision { ALLOW, DENY, NOT_APPLICABLE };

// ---------------------------------------------------------------------------
// PolicyEvaluationResult — structured result from a single policy evaluation
// ---------------------------------------------------------------------------

struct PolicyEvaluationResult {
    PolicyDecision       decision = PolicyDecision::NOT_APPLICABLE;
    std::string          policy_id;
    std::string          reason;
    std::vector<std::string> applicable_policies; ///< IDs of all matched sub-policies.
};

// ---------------------------------------------------------------------------
// IAuthorizationPolicy — ABAC policy evaluation interface
// ---------------------------------------------------------------------------

class IAuthorizationPolicy {
public:
    /**
     * @brief IAuthorization Policy.
     * @return Return value.
     */
    virtual ~IAuthorizationPolicy() = default;

    [[nodiscard]] virtual PolicyEvaluationResult evaluate(
        const SubjectAttributes&     subject,
        const ResourceAttributes&    resource,
        const std::string&           action,
        const EnvironmentAttributes& environment = {}
    ) const = 0;

    [[nodiscard]] virtual std::string policyId() const = 0;

    [[nodiscard]] virtual std::string policyVersion() const = 0;

    [[nodiscard]] virtual bool reload() = 0;
};

} // namespace auth
} // namespace themis
