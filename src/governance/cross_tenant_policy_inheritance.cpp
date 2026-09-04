/**
 * @file cross_tenant_policy_inheritance.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "governance/cross_tenant_policy_inheritance.h"

#include <algorithm>
#include <chrono>

#include "observability/metrics_collector.h"
#include "utils/audit_logger.h"
#include "utils/logger.h"

namespace themis {
namespace governance {

// ---------------------------------------------------------------------------
// Audit trail
// ---------------------------------------------------------------------------

void CrossTenantPolicyInheritance::setAuditLogger(std::shared_ptr<themis::utils::AuditLogger> logger) {
    std::lock_guard<std::mutex> lock(mutex_);
    audit_logger_ = std::move(logger);
}

// ---------------------------------------------------------------------------
// Hierarchy registration
// ---------------------------------------------------------------------------

bool CrossTenantPolicyInheritance::registerTenant(const std::string &tenant_id, const std::string &parent_tenant_id) {
    if (tenant_id.empty()) {
        THEMIS_ERROR("CrossTenantPolicyInheritance::registerTenant: tenant_id must not be empty");
        return false;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    // Guard against self-parenting.
    if (!parent_tenant_id.empty() && parent_tenant_id == tenant_id) {
        THEMIS_ERROR("CrossTenantPolicyInheritance::registerTenant: tenant '{}' cannot be its own parent", tenant_id);
        return false;
    }

    // Check whether setting this parent would create a cycle.  We temporarily
    // insert the new entry so that wouldCreateCycle can walk the chain.
    if (!parent_tenant_id.empty()) {
        // Temporarily record to allow cycle detection to see the new edge.
        auto &entry                  = tenants_[tenant_id];
        const std::string old_parent = entry.parent_id;
        entry.parent_id              = parent_tenant_id;

        if (wouldCreateCycle(tenant_id, parent_tenant_id)) {
            entry.parent_id = old_parent; // revert
            THEMIS_ERROR("CrossTenantPolicyInheritance::registerTenant: registering '{}' under '{}' "
                         "would create a cycle",
                         tenant_id, parent_tenant_id);
            return false;
        }
    } else {
        tenants_[tenant_id].parent_id = "";
    }

    THEMIS_INFO("CrossTenantPolicyInheritance: registered tenant '{}' with parent '{}'", tenant_id,
                parent_tenant_id.empty() ? "(root)" : parent_tenant_id);
    return true;
}

void CrossTenantPolicyInheritance::unregisterTenant(const std::string &tenant_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = tenants_.find(tenant_id);
    if (it == tenants_.end()) {
        return;
    }
    tenants_.erase(it);

    // Promote children of the removed tenant to root tenants.
    for (auto &[tid, entry] : tenants_) {
        if (entry.parent_id == tenant_id) {
            entry.parent_id.clear();
            THEMIS_INFO("CrossTenantPolicyInheritance: tenant '{}' promoted to root after parent '{}' "
                        "was removed",
                        tid, tenant_id);
        }
    }
}

void CrossTenantPolicyInheritance::setTenantPolicyManager(const std::string &tenant_id,
                                                          std::shared_ptr<PolicyManager> policy_manager) {
    std::lock_guard<std::mutex> lock(mutex_);
    // Auto-register the tenant as a root if it has not been registered yet.
    tenants_[tenant_id].policy_manager = std::move(policy_manager);
}

std::shared_ptr<PolicyManager>
CrossTenantPolicyInheritance::getTenantPolicyManager(const std::string &tenant_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = tenants_.find(tenant_id);
    if (it == tenants_.end()) {
        return nullptr;
    }
    return it->second.policy_manager;
}

// ---------------------------------------------------------------------------
// Hierarchy queries
// ---------------------------------------------------------------------------

std::string CrossTenantPolicyInheritance::getParentTenantId(const std::string &tenant_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = tenants_.find(tenant_id);
    if (it == tenants_.end()) {
        return "";
    }
    return it->second.parent_id;
}

std::vector<std::string> CrossTenantPolicyInheritance::getAncestors(const std::string &tenant_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return getAncestorsLocked(tenant_id);
}

std::vector<std::string> CrossTenantPolicyInheritance::listTenants() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> result = {};

    result.reserve(tenants_.size());
    for (const auto &[tid, _] : tenants_) {
        result.push_back(tid);
    }
    return result;
}

// ---------------------------------------------------------------------------
// Effective policy resolution
// ---------------------------------------------------------------------------

PolicyManager::PolicyDecision
CrossTenantPolicyInheritance::evaluateEffectivePolicy(const std::string &tenant_id, const std::string &resource,
                                                      const std::string &action,
                                                      const std::vector<std::string> &user_roles) const {
    // Build ancestor chain under the lock, then release before evaluating each
    // PolicyManager (which carries its own mutex).
    std::vector<std::string> chain;
    std::vector<std::shared_ptr<PolicyManager>> managers;
    std::shared_ptr<themis::utils::AuditLogger> audit_logger;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        chain = getAncestorsLocked(tenant_id);
        chain.push_back(tenant_id); // include the tenant itself at the end

        managers.reserve(chain.size());
        for (const auto &tid : chain) {
            auto it = tenants_.find(tid);
            managers.push_back((it != tenants_.end()) ? it->second.policy_manager : nullptr);
        }
        audit_logger = audit_logger_;
    }

    // Evaluate each level and merge, keeping the most-restrictive result.
    PolicyManager::PolicyDecision effective;
    bool first = true;

    for (std::size_t i = 0; i < managers.size(); ++i) {
        if (!managers[i]) {
            continue; // Tenant has no policy manager – skip
        }
        PolicyManager::PolicyDecision level_decision = managers[i]->evaluatePolicy(resource, action, user_roles);

        if (first) {
            effective = level_decision;
            first     = false;
        } else {
            effective = mergeDecisions(effective, level_decision);
        }
    }

    // Emit Prometheus counter for observability.
    observability::MetricsCollector::getInstance().addCounter(
        "governance_cross_tenant_policy_eval_total", 1,
        {{"tenant_id", tenant_id}, {"allowed", effective.allowed ? "true" : "false"}});

    // Write audit trail entry if a logger is attached.
    if (audit_logger) {
        nlohmann::json audit_event = {{"event_type", "cross_tenant_policy_evaluation"},
                                      {"tenant_id", tenant_id},
                                      {"resource", resource},
                                      {"action", action},
                                      {"ancestor_chain", chain},
                                      {"allowed", effective.allowed},
                                      {"require_encryption", effective.require_encryption},
                                      {"allow_export", effective.allow_export},
                                      {"allow_cache", effective.allow_cache},
                                      {"retention_days", effective.retention_days},
                                      {"redaction_level", effective.redaction_level},
                                      {"classification", effective.classification_level},
                                      {"timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(
                                                        std::chrono::system_clock::now().time_since_epoch())
                                                        .count()}};
        audit_logger->logEvent(audit_event);
    }

    return effective;
}

std::vector<PolicyRule> CrossTenantPolicyInheritance::resolveEffectiveRules(const std::string &tenant_id) const {
    std::vector<std::string> chain;
    std::vector<std::shared_ptr<PolicyManager>> managers;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        chain = getAncestorsLocked(tenant_id);
        chain.push_back(tenant_id);

        managers.reserve(chain.size());
        for (const auto &tid : chain) {
            auto it = tenants_.find(tid);
            managers.push_back((it != tenants_.end()) ? it->second.policy_manager : nullptr);
        }
    }

    std::vector<PolicyRule> all_rules = {};

    for (std::size_t i = 0; i < managers.size(); ++i) {
        if (!managers[i]) {
            continue;
        }
        auto rules = managers[i]->listRules();
        // Annotate each rule with its source tenant when created_by is unset.
        for (auto &rule : rules) {
            if (rule.created_by.empty()) {
                rule.created_by = chain[i];
            }
        }
        all_rules.insert(all_rules.end(), std::make_move_iterator(rules.begin()), std::make_move_iterator(rules.end()));
    }
    return all_rules;
}

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

bool CrossTenantPolicyInheritance::wouldCreateCycle(const std::string &tenant_id, const std::string &parent_id) const {
    // Walk up from parent_id; if we reach tenant_id we have a cycle.
    // The current state of tenants_ already reflects the would-be new edge,
    // so we just walk.
    std::string current         = parent_id;
    std::size_t steps           = 0;
    const std::size_t max_steps = static_cast<int>(tenants_.size()) + 1; // upper bound on depth

    while (!current.empty() && steps <= max_steps) {
        if (current == tenant_id) {
            return true;
        }
        auto it = tenants_.find(current);
        if (it == tenants_.end()) {
            break;
        }
        current = it->second.parent_id;
        ++steps;
    }
    return false;
}

std::vector<std::string> CrossTenantPolicyInheritance::getAncestorsLocked(const std::string &tenant_id) const {
    std::vector<std::string> ancestors;

    auto it = tenants_.find(tenant_id);
    if (it == tenants_.end()) {
        return ancestors;
    }

    std::string current         = it->second.parent_id;
    std::size_t steps           = 0;
    const std::size_t max_steps = static_cast<int>(tenants_.size()) + 1;

    while (!current.empty() && steps <= max_steps) {
        ancestors.push_back(current);
        auto pit = tenants_.find(current);
        if (pit == tenants_.end()) {
            break;
        }
        current = pit->second.parent_id;
        ++steps;
    }

    // Reverse to get root-first order.
    std::reverse(ancestors.begin(), ancestors.end());
    return ancestors;
}

// static
PolicyManager::PolicyDecision
CrossTenantPolicyInheritance::mergeDecisions(const PolicyManager::PolicyDecision &base,
                                             const PolicyManager::PolicyDecision &override_decision) {
    PolicyManager::PolicyDecision merged = base;

    // Security flags: OR (any rule requiring → required).
    merged.require_encryption = base.require_encryption || override_decision.require_encryption;
    merged.require_signature  = base.require_signature || override_decision.require_signature;
    merged.audit_access       = base.audit_access || override_decision.audit_access;
    merged.audit_changes      = base.audit_changes || override_decision.audit_changes;

    // Permission flags: AND (any rule denying → denied).
    merged.allow_export = base.allow_export && override_decision.allow_export;
    merged.allow_cache  = base.allow_cache && override_decision.allow_cache;

    // Retention: shortest wins.
    merged.retention_days = std::min(bas[[maybe_unused]] e.retention_day[[maybe_unused]] s, override_decisio[[maybe_unused]] n.retention_day[[maybe_unused]] s);

    // Redaction: strictest wins ("none" < "standard" < "strict").
    auto stricterRedaction = [](const std::string &a, const std::string &b) {
        auto rank = [](const std::string &r) {
            if (r == "strict") {
                return 2;
            }
            if (r == "standard") {
                return 1;
            }
            return 0; // "none" or unknown
        };
        return rank(a) >= rank(b) ? a : b;
    };
    merged.redaction_level = stricterRedaction(bas[[maybe_unused]] e.redaction_leve[[maybe_unused]] l, override_decisio[[maybe_unused]] n.redaction_leve[[maybe_unused]] l);

    // Classification level: use the higher (stricter) level.
    // Ordering: offen < vs-nfd < geheim < streng-geheim
    auto classificationRank = [](const std::string &c) {
        if (c == "streng-geheim") {
            return 3;
        }
        if (c == "geheim") {
            return 2;
        }
        if (c == "vs-nfd") {
            return 1;
        }
        return 0; // "offen" or unknown
    };
    if (classificationRan[[maybe_unused]] k(override_decisio[[maybe_unused]] n.classification_leve[[maybe_unused]] l) > classificationRank(base.classification_level)) {
        merged.classification_level = override_decision.classification_level;
    }

    // Merge applied rule IDs.
    merged.applied_rules.insert(merge[[maybe_unused]] d.applied_rule[[maybe_unused]] s.en[[maybe_unused]] d(), override_decision.applied_rules.begin(),
                                override_decision.applied_rules.end());

    // allowed: AND (any rule denying → denied).
    merged.allowed = base.allowed && override_decision.allowed;

    return merged;
}

} // namespace governance
} // namespace themis
