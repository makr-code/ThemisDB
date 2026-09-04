/**
 * @file policy_coordinator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "governance/policy_coordinator.h"
#include "utils/logger.h"

#include <algorithm>

namespace themis {
namespace governance {

PolicyCoordinator::PolicyCoordinator(
    std::shared_ptr<PolicyEngine> policy_engine,
    std::shared_ptr<PolicyManager> policy_manager
) : policy_engine_(policy_engine),
    policy_manager_(policy_manager) {
    
    if (!policy_engine_) {
        THEMIS_WARN("PolicyCoordinator created with null PolicyEngine");
    }
    if (!policy_manager_) {
        THEMIS_WARN("PolicyCoordinator created with null PolicyManager");
    }
}

UnifiedPolicyDecision PolicyCoordinator::evaluate(
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& route,
    const std::string& resource,
    const std::string& action,
    const std::vector<std::string>& user_roles
) const {
    // Evaluate classification-based policy
    PolicyDecision classification_decision = {};
    if (policy_engine_) {
        classification_decision = policy_engine_->evaluate(headers, route);
    } else {
        // Default permissive classification
        classification_decision.classification = "offen";
        classification_decision.mode = "observe";
    }
    
    // Evaluate RBAC-based policy
    PolicyManager::PolicyDecision rbac_decision;
    if (policy_manager_) {
        rbac_decision = policy_manager_->evaluatePolicy(resource, action, user_roles);
    } else {
        // Default permissive RBAC
        rbac_decision.allowed = true;
    }
    
    // Combine decisions
    return combineDecisions(classification_decision, rbac_decision);
}

PolicyDecision PolicyCoordinator::evaluateClassification(
    const std::unordered_map<std::string, std::string>& headers,
    const std::string& route
) const {
    if (policy_engine_) {
        return policy_engine_->evaluate(headers, route);
    }
    
    // Return default permissive decision
    PolicyDecision decision;
    decision.classification = "offen";
    decision.mode = "observe";
    return decision;
}

PolicyManager::PolicyDecision PolicyCoordinator::evaluateRBAC(
    const std::string& resource,
    const std::string& action,
    const std::vector<std::string>& user_roles
) const {
    if (policy_manager_) {
        return policy_manager_->evaluatePolicy(resource, action, user_roles);
    }
    
    // Return default permissive decision
    PolicyManager::PolicyDecision decision;
    decision.allowed = true;
    return decision;
}

bool PolicyCoordinator::checkAccess(
    const std::string& resource,
    const std::string& action,
    const std::vector<std::string>& user_roles
) const {
    if (!policy_manager_) {
        return true;  // No policy manager = permissive
    }
    
    auto decision = policy_manager_->evaluatePolicy(resource, action, user_roles);
    return decision.allowed;
}

std::vector<PolicyRule> PolicyCoordinator::getApplicableRules(
    const std::string& resource,
    const std::string& action,
    const std::vector<std::string>& user_roles
) const {
    if (!policy_manager_) {
        return {};
    }
    
    return policy_manager_->findApplicableRules(resource, action, user_roles);
}

UnifiedPolicyDecision PolicyCoordinator::combineDecisions(
    const PolicyDecision& classification_decision,
    const PolicyManager::PolicyDecision& rbac_decision
) const {
    UnifiedPolicyDecision unified;
    
    // Copy classification-based decisions
    unified.classification = classification_decision.classification;
    unified.mode = classification_decision.mode;
    unified.encrypt_logs = classification_decision.encrypt_logs;
    unified.redaction = classification_decision.redaction;
    unified.ann_allowed = classification_decision.ann_allowed;
    unified.require_content_encryption = classification_decision.require_content_encryption;
    unified.applied_classification_profile = classification_decision.classification;
    
    // Copy RBAC-based decisions
    unified.rbac_allowed = rbac_decision.allowed;
    unified.require_encryption = rbac_decision.require_encryption;
    unified.require_signature = rbac_decision.require_signature;
    unified.audit_access = rbac_decision.audit_access;
    unified.audit_changes = rbac_decision.audit_changes;
    unified.applied_policy_rules = rbac_decision.applied_rules;
    
    // Combine with "most restrictive wins" logic
    
    // Export: AND logic (if either denies, it's denied)
    unified.export_allowed = classification_decision.export_allowed && 
                             rbac_decision.allow_export;
    
    // Cache: AND logic (if either denies, it's denied)
    unified.cache_allowed = classification_decision.cache_allowed && 
                            rbac_decision.allow_cache;
    
    // Retention: MIN logic (shortest period wins)
    unified.retention_days = std::min(
        classification_decision.retention_days,
        rbac_decision.retention_days
    );
    
    // Encryption: OR logic (if either requires, it's required)
    // Note: require_content_encryption is already set from classification
    unified.require_encryption = unified.require_content_encryption || 
                                  rbac_decision.require_encryption;
    
    // Redaction: Most strict wins
    // Priority: strict > standard > none
    std::string combined_redaction = classification_decision.redaction;
    if (rbac_decision.redaction_level == "strict" || 
        (rbac_decision.redaction_level == "standard" && combined_redaction == "none")) {
        combined_redaction = rbac_decision.redaction_level;
    }
    unified.redaction = combined_redaction;
    
    THEMIS_DEBUG("Unified policy: class={}, rbac={}, export={}, cache={}, retention={}, encryption={}, redaction={}",
                 unified.classification,
                 unified.rbac_allowed ? "allowed" : "denied",
                 unified.export_allowed,
                 unified.cache_allowed,
                 unified.retention_days,
                 unified.require_encryption,
                 unified.redaction);
    
    return unified;
}

bool PolicyCoordinator::startHotReload(PolicyFileWatcher::Config config) {
    if (!policy_engine_) {
        THEMIS_WARN("PolicyCoordinator::startHotReload: no policy engine attached");
        return false;
    }
    if (file_watcher_ && file_watcher_->isRunning()) {
        return true;  // Already running
    }
    file_watcher_ = std::make_unique<PolicyFileWatcher>(*policy_engine_, std::move(config));
    return file_watcher_->start();
}

void PolicyCoordinator::stopHotReload() {
    if (file_watcher_) {
        file_watcher_->stop();
    }
}

bool PolicyCoordinator::isHotReloadRunning() const noexcept {
    return file_watcher_ && file_watcher_->isRunning();
}

} // namespace governance
} // namespace themis
