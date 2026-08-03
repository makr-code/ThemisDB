/**
 * @file authorization_policy.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief Attributes describing the requesting subject (user, service, device).
 */
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

/**
 * @brief Attributes describing the resource being accessed.
 */
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

/**
 * @brief Environmental context at the moment of the access request.
 *
 * Zero-value-initialised; populate only the fields relevant to the policy.
 */
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

/**
 * @brief XACML-aligned tri-valued policy decision.
 *
 * ALLOW and DENY are definitive.  NOT_APPLICABLE indicates that this
 * policy has no opinion; the decision engine combines multiple policies
 * using a combining algorithm (e.g., deny-overrides, permit-overrides).
 */
enum class PolicyDecision { ALLOW, DENY, NOT_APPLICABLE };

// ---------------------------------------------------------------------------
// PolicyEvaluationResult — structured result from a single policy evaluation
// ---------------------------------------------------------------------------

/**
 * @brief Result of evaluating an IAuthorizationPolicy.
 */
struct PolicyEvaluationResult {
    PolicyDecision       decision = PolicyDecision::NOT_APPLICABLE;
    std::string          policy_id;
    std::string          reason;
    std::vector<std::string> applicable_policies; ///< IDs of all matched sub-policies.
};

// ---------------------------------------------------------------------------
// IAuthorizationPolicy — ABAC policy evaluation interface
// ---------------------------------------------------------------------------

/**
 * @brief Pure-virtual ABAC policy evaluation interface.
 *
 * Each IAuthorizationPolicy implementation encapsulates a single named policy
 * document (OPA Rego, Cedar, XACML, or custom rule engine).  The policy
 * engine composes multiple instances using a combining algorithm.
 *
 * ### Contract
 * - `evaluate()` is read-only and must be safe to call concurrently.
 * - `reload()` performs a hot-reload from the backing store; it may briefly
 *   block concurrent `evaluate()` calls while swapping the rule set.
 * - Returning NOT_APPLICABLE leaves the decision to other registered policies.
 */
class IAuthorizationPolicy {
public:
    virtual ~IAuthorizationPolicy() = default;

    /**
     * @brief Evaluate the policy for the given subject/resource/action triple.
     *
     * @param subject      Attributes of the requesting principal.
     * @param resource     Attributes of the target resource.
     * @param action       Requested action (e.g., "read", "write", "delete").
     * @param environment  Optional contextual attributes.
     * @return PolicyEvaluationResult containing the decision and audit metadata.
     */
    [[nodiscard]] virtual PolicyEvaluationResult evaluate(
        const SubjectAttributes&     subject,
        const ResourceAttributes&    resource,
        const std::string&           action,
        const EnvironmentAttributes& environment = {}
    ) const = 0;

    /// Unique identifier for this policy (used for logging and combining).
    [[nodiscard]] virtual std::string policyId() const = 0;

    /// Policy document version string (e.g., semver or git SHA).
    [[nodiscard]] virtual std::string policyVersion() const = 0;

    /**
     * @brief Hot-reload the policy document from its backing store.
     *
     * @return `true` if the reload succeeded; `false` on parse/validation error.
     */
    [[nodiscard]] virtual bool reload() = 0;
};

} // namespace auth
} // namespace themis
