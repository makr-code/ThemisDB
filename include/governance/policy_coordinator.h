/**
 * @file policy_coordinator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "governance/policy_engine.h"
#include "governance/policy_file_watcher.h"
#include "governance/policy_manager.h"
#include "security/rbac.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis {
namespace governance {

struct UnifiedPolicyDecision {
    // From PolicyEngine (classification-based)
    std::string classification;         // Normalized classification level
    std::string mode;                   // "enforce" | "observe"
    bool encrypt_logs = true;
    std::string redaction = "standard";
    bool ann_allowed = true;
    bool require_content_encryption = false;
    
    // From PolicyManager (RBAC-based)
    bool rbac_allowed = true;          // RBAC access decision
    bool require_encryption = false;    // Data encryption required
    bool require_signature = false;     // Digital signature required
    
    // Combined decisions (most restrictive wins)
    bool export_allowed = true;
    bool cache_allowed = true;
    int retention_days = 365;
    bool audit_access = false;
    bool audit_changes = false;
    
    // Applied rules
    std::vector<std::string> applied_policy_rules;  // PolicyManager rule IDs
    std::string applied_classification_profile;     // PolicyEngine profile
};

class PolicyCoordinator {
public:
    PolicyCoordinator(
        std::shared_ptr<PolicyEngine> policy_engine,
        std::shared_ptr<PolicyManager> policy_manager
    );
    
    UnifiedPolicyDecision evaluate(
        const std::unordered_map<std::string, std::string>& headers,
        const std::string& route,
        const std::string& resource,
        const std::string& action,
        const std::vector<std::string>& user_roles
    ) const;
    
    PolicyDecision evaluateClassification(
        const std::unordered_map<std::string, std::string>& headers,
        const std::string& route
    ) const;
    
    /**
     * @brief Evaluate RBAC.
     * @param[in] resource Input parameter.
     * @param[in] action Input parameter.
     * @param[in] user_roles Input parameter.
     * @return Return value.
     */
    PolicyManager::PolicyDecision evaluateRBAC(
        const std::string& resource,
        const std::string& action,
        const std::vector<std::string>& user_roles
    ) const;
    
    /**
     * @brief Check Access.
     * @param[in] resource Input parameter.
     * @param[in] action Input parameter.
     * @param[in] user_roles Input parameter.
     * @return True when the operation succeeds.
     */
    bool checkAccess(
        const std::string& resource,
        const std::string& action,
        const std::vector<std::string>& user_roles
    ) const;
    
    /**
     * @brief Get Applicable Rules.
     * @param[in] resource Input parameter.
     * @param[in] action Input parameter.
     * @param[in] user_roles Input parameter.
     * @return Return value.
     */
    std::vector<PolicyRule> getApplicableRules(
        const std::string& resource,
        const std::string& action,
        const std::vector<std::string>& user_roles
    ) const;
    
    std::shared_ptr<PolicyEngine> getPolicyEngine() const { return policy_engine_; }
    
    std::shared_ptr<PolicyManager> getPolicyManager() const { return policy_manager_; }

    bool startHotReload(PolicyFileWatcher::Config config = {});

    /**
     * @brief Stop Hot Reload.
     */
    void stopHotReload();

    /**
     * @brief Is Hot Reload Running.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
    bool isHotReloadRunning() const noexcept;

private:
    std::shared_ptr<PolicyEngine> policy_engine_;
    std::shared_ptr<PolicyManager> policy_manager_;
    std::unique_ptr<PolicyFileWatcher> file_watcher_;
    
    /**
     * @brief Combine Decisions.
     * @param[in] classification_decision Input parameter.
     * @param[in] rbac_decision Input parameter.
     * @return Return value.
     */
    UnifiedPolicyDecision combineDecisions(
        const PolicyDecision& classification_decision,
        const PolicyManager::PolicyDecision& rbac_decision
    ) const;
};

} // namespace governance
} // namespace themis
