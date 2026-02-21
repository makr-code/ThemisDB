/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            policy_coordinator.h                               ║
  Version:         0.0.21                                             ║
  Last Modified:   2026-02-21 19:19:59                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     180                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "governance/policy_engine.h"
#include "governance/policy_manager.h"
#include "security/rbac.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis {
namespace governance {

/// Unified policy decision combining classification and RBAC
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

/**
 * @brief PolicyCoordinator - Unified governance layer
 * 
 * Combines PolicyEngine (classification-based) and PolicyManager (RBAC-based)
 * for comprehensive governance and access control.
 * 
 * Evaluation flow:
 * 1. PolicyEngine evaluates classification from headers
 * 2. PolicyManager evaluates RBAC rules for resource/action/roles
 * 3. Combine decisions with "most restrictive wins" logic
 */
class PolicyCoordinator {
public:
    /**
     * @brief Construct coordinator with both policy systems
     * @param policy_engine Classification-based policy engine
     * @param policy_manager RBAC-based policy manager
     */
    PolicyCoordinator(
        std::shared_ptr<PolicyEngine> policy_engine,
        std::shared_ptr<PolicyManager> policy_manager
    );
    
    /**
     * @brief Evaluate unified policy decision
     * @param headers HTTP headers (for classification)
     * @param route Route identifier (for classification)
     * @param resource Resource being accessed (for RBAC)
     * @param action Action being performed (for RBAC)
     * @param user_roles User's roles (for RBAC)
     * @return Unified policy decision combining both systems
     */
    UnifiedPolicyDecision evaluate(
        const std::unordered_map<std::string, std::string>& headers,
        const std::string& route,
        const std::string& resource,
        const std::string& action,
        const std::vector<std::string>& user_roles
    ) const;
    
    /**
     * @brief Evaluate only classification policy (backward compatible)
     * @param headers HTTP headers
     * @param route Route identifier
     * @return Classification-based policy decision
     */
    PolicyDecision evaluateClassification(
        const std::unordered_map<std::string, std::string>& headers,
        const std::string& route
    ) const;
    
    /**
     * @brief Evaluate only RBAC policy
     * @param resource Resource being accessed
     * @param action Action being performed
     * @param user_roles User's roles
     * @return RBAC-based policy decision
     */
    PolicyManager::PolicyDecision evaluateRBAC(
        const std::string& resource,
        const std::string& action,
        const std::vector<std::string>& user_roles
    ) const;
    
    /**
     * @brief Check if user has required roles for resource/action
     * @param resource Resource identifier
     * @param action Action identifier
     * @param user_roles User's roles
     * @return true if user has required roles
     */
    bool checkAccess(
        const std::string& resource,
        const std::string& action,
        const std::vector<std::string>& user_roles
    ) const;
    
    /**
     * @brief Get applicable policy rules for resource/action
     * @param resource Resource identifier
     * @param action Action identifier
     * @param user_roles User's roles
     * @return Vector of applicable rules
     */
    std::vector<PolicyRule> getApplicableRules(
        const std::string& resource,
        const std::string& action,
        const std::vector<std::string>& user_roles
    ) const;
    
    /**
     * @brief Get policy engine (for direct access if needed)
     */
    std::shared_ptr<PolicyEngine> getPolicyEngine() const { return policy_engine_; }
    
    /**
     * @brief Get policy manager (for direct access if needed)
     */
    std::shared_ptr<PolicyManager> getPolicyManager() const { return policy_manager_; }
    
private:
    std::shared_ptr<PolicyEngine> policy_engine_;
    std::shared_ptr<PolicyManager> policy_manager_;
    
    /// Combine decisions from both systems (most restrictive wins)
    UnifiedPolicyDecision combineDecisions(
        const PolicyDecision& classification_decision,
        const PolicyManager::PolicyDecision& rbac_decision
    ) const;
};

} // namespace governance
} // namespace themis
