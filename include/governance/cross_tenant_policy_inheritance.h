/**
 * @file cross_tenant_policy_inheritance.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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
     * @brief Set Audit Logger.
     * @param[in] logger Input parameter.
     */
    void setAuditLogger(std::shared_ptr<themis::utils::AuditLogger> logger);

    // -------------------------------------------------------------------------
    // Hierarchy registration
    // -------------------------------------------------------------------------

    bool registerTenant(const std::string& tenant_id,
                        const std::string& parent_tenant_id = "");

    /**
     * @brief Unregister Tenant.
     * @param[in] tenant_id Identifier of the tenant.
     */
    void unregisterTenant(const std::string& tenant_id);

    /**
     * @brief Set Tenant Policy Manager.
     * @param[in] tenant_id Identifier of the tenant.
     * @param[in] policy_manager Input parameter.
     */
    void setTenantPolicyManager(const std::string& tenant_id,
                                 std::shared_ptr<PolicyManager> policy_manager);

    /**
     * @brief Get Tenant Policy Manager.
     * @param[in] tenant_id Identifier of the tenant.
     * @return Return value.
     */
    std::shared_ptr<PolicyManager> getTenantPolicyManager(
        const std::string& tenant_id) const;

    // -------------------------------------------------------------------------
    // Hierarchy queries
    // -------------------------------------------------------------------------

    /**
     * @brief Get Parent Tenant Id.
     * @param[in] tenant_id Identifier of the tenant.
     * @return Return value.
     */
    std::string getParentTenantId(const std::string& tenant_id) const;

    /**
     * @brief Get Ancestors.
     * @param[in] tenant_id Identifier of the tenant.
     * @return Return value.
     */
    std::vector<std::string> getAncestors(const std::string& tenant_id) const;

    /**
     * @brief List Tenants.
     * @return Return value.
     */
    std::vector<std::string> listTenants() const;

    // -------------------------------------------------------------------------
    // Effective policy resolution
    // -------------------------------------------------------------------------

    /**
     * @brief Evaluate Effective Policy.
     * @param[in] tenant_id Identifier of the tenant.
     * @param[in] resource Input parameter.
     * @param[in] action Input parameter.
     * @param[in] user_roles Input parameter.
     * @return Return value.
     */
    PolicyManager::PolicyDecision evaluateEffectivePolicy(
        const std::string& tenant_id,
        const std::string& resource,
        const std::string& action,
        const std::vector<std::string>& user_roles) const;

    /**
     * @brief Resolve Effective Rules.
     * @param[in] tenant_id Identifier of the tenant.
     * @return Return value.
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


    /**
     * @brief Would Create Cycle.
     * @param[in] tenant_id Identifier of the tenant.
     * @param[in] parent_id Identifier of the parent.
     * @return True when the operation succeeds.
     */
    bool wouldCreateCycle(const std::string& tenant_id,
                          const std::string& parent_id) const;

    /**
     * @brief Get Ancestors Locked.
     * @param[in] tenant_id Identifier of the tenant.
     * @return Return value.
     */
    std::vector<std::string> getAncestorsLocked(
        const std::string& tenant_id) const;

    /**
     * @brief Merge Decisions.
     * @param[in] base Input parameter.
     * @param[in] override_decision Input parameter.
     * @return Return value.
     */
    static PolicyManager::PolicyDecision mergeDecisions(
        const PolicyManager::PolicyDecision& base,
        const PolicyManager::PolicyDecision& override_decision);
};

} // namespace governance
} // namespace themis
