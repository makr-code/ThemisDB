/**
 * @file cross_tenant_policy_inheritance.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "governance/policy_manager.h"

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis {
namespace utils { class AuditLogger; }

namespace governance {

/**
 * @brief Manages a hierarchy of tenants and enables cross-tenant policy inheritance.
 *
 * A tenant may have at most one parent tenant.  Policy rules are inherited from
 * the full ancestor chain (root → … → parent → child).  When the same resource
 * and action pattern is covered by both an ancestor rule and a tenant-local
 * rule, the most-restrictive constraints win:
 *   - `require_encryption` : OR  (true if *any* rule requires it)
 *   - `require_signature`  : OR
 *   - `allow_export`       : AND (false if *any* rule denies it)
 *   - `allow_cache`        : AND
 *   - `retention_days`     : min (shortest retention wins)
 *   - `redaction_level`    : strictest of "none" < "standard" < "strict"
 *   - `audit_access`       : OR
 *   - `audit_changes`      : OR
 *
 * Thread-safety: all public methods are thread-safe.
 */
class CrossTenantPolicyInheritance {
public:
    CrossTenantPolicyInheritance() = default;

    // Non-copyable, movable.
    CrossTenantPolicyInheritance(const CrossTenantPolicyInheritance&) = delete;
    CrossTenantPolicyInheritance& operator=(const CrossTenantPolicyInheritance&) = delete;
    CrossTenantPolicyInheritance(CrossTenantPolicyInheritance&&) noexcept = default;
    CrossTenantPolicyInheritance& operator=(CrossTenantPolicyInheritance&&) noexcept = default;

    // -------------------------------------------------------------------------
    // Audit trail
    // -------------------------------------------------------------------------

    /**
     * @brief Attach an audit logger.
     *
     * When set, `evaluateEffectivePolicy()` writes a governance event for every
     * policy decision, following the same pattern as `PolicyEngine::evaluate()`.
     * Thread-safe; atomically replaces the previous logger.
     */
    void setAuditLogger(std::shared_ptr<themis::utils::AuditLogger> logger);

    // -------------------------------------------------------------------------
    // Hierarchy registration
    // -------------------------------------------------------------------------

    /**
     * @brief Register a tenant and optionally attach it to a parent tenant.
     *
     * Registering the same tenant_id again overwrites the parent association.
     * An empty or absent parent_tenant_id registers a root tenant.
     *
     * @param tenant_id        Unique tenant identifier.
     * @param parent_tenant_id Parent tenant identifier, or empty for root.
     * @return true on success; false when a cycle would be created.
     */
    bool registerTenant(const std::string& tenant_id,
                        const std::string& parent_tenant_id = "");

    /**
     * @brief Remove a tenant from the hierarchy.
     *
     * Any child tenants that referenced this tenant as their parent are
     * promoted to root tenants (their parent_id is cleared).
     */
    void unregisterTenant(const std::string& tenant_id);

    /**
     * @brief Attach a PolicyManager to a tenant.
     *
     * Replaces any previously attached PolicyManager for the same tenant.
     * Passing a null pointer detaches the policy manager.
     */
    void setTenantPolicyManager(const std::string& tenant_id,
                                 std::shared_ptr<PolicyManager> policy_manager);

    /**
     * @brief Retrieve the PolicyManager directly associated with a tenant.
     *
     * Returns nullptr if no PolicyManager has been attached.
     */
    std::shared_ptr<PolicyManager> getTenantPolicyManager(
        const std::string& tenant_id) const;

    // -------------------------------------------------------------------------
    // Hierarchy queries
    // -------------------------------------------------------------------------

    /// @return The parent tenant ID, or empty string if the tenant is a root.
    std::string getParentTenantId(const std::string& tenant_id) const;

    /**
     * @brief Return the ancestor chain from the root down to (but not including)
     *        the given tenant.
     *
     * Example: root → A → B → child  →  returns {"root", "A", "B"}
     */
    std::vector<std::string> getAncestors(const std::string& tenant_id) const;

    /// @return All registered tenant IDs.
    std::vector<std::string> listTenants() const;

    // -------------------------------------------------------------------------
    // Effective policy resolution
    // -------------------------------------------------------------------------

    /**
     * @brief Evaluate the effective policy for a tenant by merging its own rules
     *        with all inherited ancestor rules.
     *
     * Resolution order: rules from the root tenant are applied first, then each
     * intermediate ancestor in top-down order, and finally the tenant's own
     * rules.  When rules overlap, the most-restrictive constraints prevail (see
     * class-level documentation for the merge semantics per field).
     *
     * If an audit logger has been attached via setAuditLogger(), a
     * `cross_tenant_policy_evaluation` event is written for every call.
     *
     * @param tenant_id   Tenant whose effective policy is requested.
     * @param resource    Resource being accessed (e.g. "data/users").
     * @param action      Action being performed (e.g. "read").
     * @param user_roles  Roles of the requesting user.
     * @return Effective PolicyManager::PolicyDecision.
     */
    PolicyManager::PolicyDecision evaluateEffectivePolicy(
        const std::string& tenant_id,
        const std::string& resource,
        const std::string& action,
        const std::vector<std::string>& user_roles) const;

    /**
     * @brief Return a flat list of all effective rules for a tenant, combining
     *        ancestor-inherited rules with tenant-local rules.
     *
     * Rules are ordered from most-distant ancestor to tenant-local (top-down).
     * Each rule's `created_by` field is set to the source tenant_id when it
     * is empty in the original rule; otherwise the original value is preserved.
     */
    std::vector<PolicyRule> resolveEffectiveRules(
        const std::string& tenant_id) const;

private:
    mutable std::mutex mutex_;

    struct TenantEntry {
        std::string parent_id;                        ///< Empty = root tenant.
        std::shared_ptr<PolicyManager> policy_manager;
    };

    std::unordered_map<std::string, TenantEntry> tenants_;
    std::shared_ptr<themis::utils::AuditLogger> audit_logger_;

    // -------------------------------------------------------------------------
    // Internal helpers (called with mutex_ held)
    // -------------------------------------------------------------------------

    /// Detect cycles: would setting parent_id on tenant_id create a cycle?
    bool wouldCreateCycle(const std::string& tenant_id,
                          const std::string& parent_id) const;

    /// Resolve ancestors top-down (without lock – caller must hold mutex_).
    std::vector<std::string> getAncestorsLocked(
        const std::string& tenant_id) const;

    /// Merge two PolicyDecision objects, keeping the most-restrictive values.
    static PolicyManager::PolicyDecision mergeDecisions(
        const PolicyManager::PolicyDecision& base,
        const PolicyManager::PolicyDecision& override_decision);
};

} // namespace governance
} // namespace themis
