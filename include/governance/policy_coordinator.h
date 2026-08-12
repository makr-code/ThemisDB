/**
 * @file policy_coordinator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

    /**
     * @brief Start automatic hot-reload of the governance policy YAML file.
     *
     * Creates and starts a @c PolicyFileWatcher that polls the file loaded into
     * the @c PolicyEngine and calls @c PolicyEngine::reloadIfChanged() whenever
     * the file modification time changes.  The coordinator owns the watcher;
     * call @c stopHotReload() or destroy the coordinator to stop it.
     *
     * Calling @c startHotReload() while the watcher is already running is a
     * no-op and returns @c true.
     *
     * @param config  Watcher configuration (poll interval, debounce, callback).
     * @return @c true on success, @c false if no policy engine is attached.
     */
    bool startHotReload(PolicyFileWatcher::Config config = {});

    /**
     * @brief Stop the hot-reload background thread.
     *
     * Blocks until the watcher thread has joined.  Safe to call even if
     * @c startHotReload() was never called.
     */
    void stopHotReload();

    /// @return @c true if the hot-reload watcher is currently running.
    bool isHotReloadRunning() const noexcept;

private:
    std::shared_ptr<PolicyEngine> policy_engine_;
    std::shared_ptr<PolicyManager> policy_manager_;
    std::unique_ptr<PolicyFileWatcher> file_watcher_;
    
    /// Combine decisions from both systems (most restrictive wins)
    UnifiedPolicyDecision combineDecisions(
        const PolicyDecision& classification_decision,
        const PolicyManager::PolicyDecision& rbac_decision
    ) const;
};

} // namespace governance
} // namespace themis
