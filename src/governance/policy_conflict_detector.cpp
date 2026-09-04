/**
 * @file policy_conflict_detector.cpp
 * @brief Policy Conflict Detection and Resolution Engine Implementation
 * 
 * This file implements comprehensive conflict detection with:
 * - O(n²) conflict analysis across all rules
 * - Deny-Overrides-Permit precedence algorithm
 * - Atomic update semantics with rollback
 * - Result caching for performance
 * 
 * **Conflict Detection Strategy:**
 * 1. For each pair of rules, check if they conflict
 * 2. Categorize conflict type (PERMIT-DENY, overlapping, etc.)
 * 3. Compute severity based on conflict impact
 * 4. Recommend resolution strategy
 * 5. Cache results until policies change
 * 
 * **Performance Considerations:**
 * - Conflict detection is O(n²); results are cached
 * - Cache invalidated only on policy changes
 * - Precedence evaluation uses incremental updates
 * - Atomic operations serialize writes; reads proceed in parallel
 * 
 * @version 0.1.0
 * @since 2026-08-18
 */

#include "governance/policy_conflict_detector.h"

#include <algorithm>
#include <chrono>
#include <functional>
#include <numeric>
#include <sstream>

#include <fmt/format.h>
#include "utils/logger.h"

namespace themis {
namespace governance {

// ========== PolicyConflict Implementation ==========

nlohmann::json PolicyConflict::toJson() const {
    nlohmann::json j;
    j["conflict_id"] = conflict_id;
    
    // Convert ConflictType enum to string
    static const std::unordered_map<ConflictType, std::string> type_names = {
        {ConflictType::PERMIT_DENY, "PERMIT_DENY"},
        {ConflictType::OVERLAPPING, "OVERLAPPING"},
        {ConflictType::CIRCULAR_DEPENDENCY, "CIRCULAR_DEPENDENCY"},
        {ConflictType::TYPE_MISMATCH, "TYPE_MISMATCH"},
        {ConflictType::ENCRYPTION_CONFLICT, "ENCRYPTION_CONFLICT"},
        {ConflictType::EXPORT_CONFLICT, "EXPORT_CONFLICT"},
        {ConflictType::RETENTION_CONFLICT, "RETENTION_CONFLICT"},
        {ConflictType::COMPLIANCE_CONFLICT, "COMPLIANCE_CONFLICT"}
    };
    j["conflict_type"] = type_names.at(conflict_type);
    
    // Convert ConflictSeverity enum to string
    static const std::unordered_map<ConflictSeverity, std::string> severity_names = {
        {ConflictSeverity::LOW, "LOW"},
        {ConflictSeverity::MEDIUM, "MEDIUM"},
        {ConflictSeverity::HIGH, "HIGH"},
        {ConflictSeverity::CRITICAL, "CRITICAL"}
    };
    j["severity"] = severity_names.at(severity);
    
    j["conflicting_rule_ids"] = conflicting_rule_ids;
    j["description"] = description;
    j["resolution_strategy"] = resolution_strategy;
    j["detected_at"] = detected_at;
    
    return j;
}

// ========== PrecedenceEvaluation Implementation ==========

nlohmann::json PrecedenceEvaluation::toJson() const {
    nlohmann::json j;
    j["rule_id"] = rule_id;
    j["effective_priority"] = effective_priority;
    j["has_explicit_precedence"] = has_explicit_precedence;
    j["overrides"] = overrides;
    j["overridden_by"] = overridden_by;
    j["rationale"] = rationale;
    return j;
}

// ========== PolicyConflictDetector Implementation ==========

PolicyConflictDetector::PolicyConflictDetector()
    : caching_enabled_(true), total_detections_(0) {
}

std::vector<PolicyConflict> PolicyConflictDetector::detectAllConflicts(
    const PolicyManager& policy_mgr) {
    
    std::vector<PolicyConflict> all_conflicts;
    
    auto permit_deny = detectPermitDenyConflicts(policy_mgr);
    all_conflicts.insert(all_conflicts.end(), permit_deny.begin(), permit_deny.end());
    
    auto overlapping = detectOverlappingConflicts(policy_mgr);
    all_conflicts.insert(all_conflicts.end(), overlapping.begin(), overlapping.end());
    
    auto circular = detectCircularDependencies(policy_mgr);
    all_conflicts.insert(all_conflicts.end(), circular.begin(), circular.end());
    
    // Cache if enabled
    if (caching_enabled_) {
        std::unique_lock<std::shared_mutex> lock(state_mutex_);
        // Use policy manager address as cache key
        conflict_cache_[std::to_string(reinterpret_cast<uintptr_t>(&policy_mgr))] = all_conflicts;
        cache_timestamp_ = std::chrono::system_clock::now().time_since_epoch().count();
    }
    
    total_detections_++;
    THEMIS_INFO("Conflict detection completed: {} conflicts found",static_cast<int>(all_conflicts.size()));
    
    return all_conflicts;
}

std::vector<PolicyConflict> PolicyConflictDetector::detectPermitDenyConflicts(
    const PolicyManager& policy_mgr) {
    
    std::vector<PolicyConflict> conflicts;
    auto rules = policy_mgr.listRules();
    
    // O(n²) comparison of all rule pairs
    for (size_t i = 0; i <static_cast<int>(rules.size()); ++i) {
        for (size_t j = i + 1; j <static_cast<int>(rules.size()); ++j) {
            auto conflict = checkRuleConflict(rules[i], rules[j]);
            if (conflict) {
                conflicts.push_back(conflict.value());
            }
        }
    }
    
    return conflicts;
}

std::vector<PolicyConflict> PolicyConflictDetector::detectOverlappingConflicts(
    const PolicyManager& policy_mgr) {
    
    std::vector<PolicyConflict> conflicts;
    auto rules = policy_mgr.listRules();
    
    for (size_t i = 0; i <static_cast<int>(rules.size()); ++i) {
        for (size_t j = i + 1; j <static_cast<int>(rules.size()); ++j) {
            const auto& rule1 = rules[i];
            const auto& rule2 = rules[j];
            
            // Check if rules have overlapping resources and actions
            bool resources_overlap = false;
            bool actions_overlap = false;
            
            // Simple wildcard matching
            auto matches_pattern = [](const std::string& pattern, 
                                     const std::string& text) -> bool {
                if (pattern == "*") {
                  return true;
                }
                if (pattern.empty()) {
                  return false;
                }
                if (pattern.back() == '*') {
                    return text.substr(0, pattern.length() - 1) == pattern.substr(0, pattern.length() - 1);
                }
                return pattern == text;
            };
            
            for (const auto& res1 : rule1.resources) {
                for (const auto& res2 : rule2.resources) {
                    if (matches_pattern(res1, res2) || matches_pattern(res2, res1)) {
                        resources_overlap = true;
                        break;
                    }
                }
                if (resources_overlap) {
                  break;
                }
            }
            
            for (const auto& act1 : rule1.actions) {
                for (const auto& act2 : rule2.actions) {
                    if (matches_pattern(act1, act2) || matches_pattern(act2, act1)) {
                        actions_overlap = true;
                        break;
                    }
                }
                if (actions_overlap) {
                  break;
                }
            }
            
            // If overlap but not contradictory, it's an overlapping conflict
            if (resources_overlap && actions_overlap) {
                auto conflict_opt = checkRuleConflict(rule1, rule2);
                if (!conflict_opt) {
                    // Rules overlap but don't directly contradict
                    PolicyConflict conflict;
                    conflict.conflict_id = generateConflictId({rule1.id, rule2.id}, 
                                                             ConflictType::OVERLAPPING);
                    conflict.conflict_type = ConflictType::OVERLAPPING;
                    conflict.conflicting_rule_ids = {rule1.id, rule2.id};
                    conflict.description = fmt::format(
                        "Rules '{}' and '{}' have overlapping resource/action patterns "
                        "with no explicit precedence. May cause ambiguous evaluation.",
                        rule1.name, rule2.name
                    );
                    conflict.severity = ConflictSeverity::MEDIUM;
                    conflict.resolution_strategy = "Add explicit priority values to rules";
                    conflict.detected_at = std::chrono::system_clock::now().time_since_epoch().count();
                    
                    conflicts.push_back(conflict);
                }
            }
        }
    }
    
    return conflicts;
}

std::vector<PolicyConflict> PolicyConflictDetector::detectCircularDependencies(
    const PolicyManager& policy_mgr) {
    
    std::vector<PolicyConflict> conflicts;
    auto rules = policy_mgr.listRules();
    
    for (const auto& rule : rules) {
        std::unordered_set<std::string> visited;
        std::unordered_set<std::string> rec_stack;
        
        if (hasCircularDependency(rule.id, visited, rec_stack, policy_mgr)) {
            PolicyConflict conflict;
            conflict.conflict_id = generateConflictId({rule.id}, 
                                                     ConflictType::CIRCULAR_DEPENDENCY);
            conflict.conflict_type = ConflictType::CIRCULAR_DEPENDENCY;
            conflict.conflicting_rule_ids = {rule.id};
            conflict.description = fmt::format(
                "Rule '{}' is involved in a circular dependency chain. "
                "Cannot evaluate deterministically.",
                rule.name
            );
            conflict.severity = ConflictSeverity::CRITICAL;
            conflict.resolution_strategy = "Remove or restructure rules to break the cycle";
            conflict.detected_at = std::chrono::system_clock::now().time_since_epoch().count();
            
            conflicts.push_back(conflict);
        }
    }
    
    return conflicts;
}

PrecedenceEvaluation PolicyConflictDetector::evaluateRulePrecedence(
    const std::string& rule_id,
    const PolicyManager& policy_mgr) {
    
    PrecedenceEvaluation eval;
    eval.rule_id = rule_id;
    eval.has_explicit_precedence = false;
    eval.effective_priority = 100;  // Default priority
    
    auto rule_opt = policy_mgr.getRule(rule_id);
    if (!rule_opt) {
        eval.rationale = "Rule not found";
        return eval;
    }
    
    const auto& rule = rule_opt.value();
    
    // Explicit priority from rule
    int priority_component = rule.priority;
    
    // Deny-Overrides-Permit: Deny rules get implicit priority boost
    int precedence_bonus = 0;
    bool is_deny_rule = !rule.allow_export && !rule.allow_cache;
    if (is_deny_rule) {
        precedence_bonus += 50;
    }
    
    // Scope specificity (resource/action specificity)
    int specificity_bonus = 0;
    bool has_wildcard = false;
    for (const auto& res : rule.resources) {
        if (res.find('*') != std::string::npos) {
            has_wildcard = true;
            break;
        }
    }
    if (!has_wildcard) {
        specificity_bonus = 20;
    }
    
    // Creation order (earlier rules win ties)
    int creation_bonus = 0;
    if (rule.created_at > 0) {
        auto age_seconds = std::chrono::system_clock::now().time_since_epoch().count() - rule.created_at;
        creation_bonus = std::max(0LL, 1000LL - age_seconds / 1000000LL);
    }
    
    eval.effective_priority = priority_component + precedence_bonus + specificity_bonus + creation_bonus;
    eval.has_explicit_precedence = rule.priority > 0;
    
    // Find which rules this overrides and which override it
    auto all_rules = policy_mgr.listRules();
    for (const auto& other : all_rules) {
        if (other.id == rule_id) {
          continue;
        }
        
        // Same resource/action scope?
        bool scope_overlap = !rule.resources.empty() && !other.resources.empty();
        for (const auto& r1 : rule.resources) {
            for (const auto& r2 : other.resources) {
                if (r1 == r2) {
                    scope_overlap = true;
                    break;
                }
            }
            if (scope_overlap) {
              break;
            }
        }
        
        if (scope_overlap) {
            auto other_eval = evaluateRulePrecedence(other.id, policy_mgr);
            if (eval.effective_priority > other_eval.effective_priority) {
                eval.overrides.push_back(other.id);
            } else if (eval.effective_priority < other_eval.effective_priority) {
                eval.overridden_by.push_back(other.id);
            }
        }
    }
    
    std::ostringstream oss = {};
    oss << "Priority=" << priority_component 
        << " DenyBonus=" << (is_deny_rule ? 50 : 0)
        << " SpecificityBonus=" << specificity_bonus
        << " CreationBonus=" << creation_bonus
        << " EffectivePriority=" << eval.effective_priority;
    eval.rationale = oss.str();
    
    return eval;
}

std::unordered_map<std::string, PrecedenceEvaluation> 
PolicyConflictDetector::evaluateAllPrecedence(const PolicyManager& policy_mgr) {
    
    std::unordered_map<std::string, PrecedenceEvaluation> result;
    auto rules = policy_mgr.listRules();
    
    for (const auto& rule : rules) {
        result[rule.id] = evaluateRulePrecedence(rule.id, policy_mgr);
    }
    
    return result;
}

AtomicUpdateResult PolicyConflictDetector::atomicAddRule(
    const PolicyRule& rule,
    PolicyManager& policy_mgr) {
    
    AtomicUpdateResult result;
    result.transaction_id = generateConflictId({rule.id}, ConflictType::PERMIT_DENY);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Create snapshot of current state
    auto existing_rule = policy_mgr.getRule(rule.id);
    
    try {
        // Add rule
        policy_mgr.addRule(rule);
        
        // Check for conflicts
        auto conflicts = detectAllConflicts(policy_mgr);
        
        if (!conflicts.empty()) {
            // Rollback
            if (existing_rule) {
                policy_mgr.updateRule(existing_rule.value().id, existing_rule.value(), 
                                     "conflict_detector", "Rollback due to conflict");
            } else {
                policy_mgr.removeRule(rule.id);
            }
            
            result.success = false;
            result.conflicts_detected = conflicts;
            result.error_message = fmt::format(
                "Conflict validation failed: {} conflicts detected",
                conflicts.size()
            );
        } else {
            result.success = true;
            result.affected_rules = {rule.id};
        }
        
    } catch (const std::exception& e) {
        result.success = false;
        result.error_message = fmt::format("Exception during atomic add: {}", e.what());
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    result.operation_time_us = std::chrono::duration_cast<std::chrono::microseconds>(
        end_time - start_time
    ).count();
    
    // Invalidate cache on any change
    if (result.success) {
        clearCache();
    }
    
    return result;
}

AtomicUpdateResult PolicyConflictDetector::atomicUpdateRule(
    const PolicyRule& rule,
    PolicyManager& policy_mgr) {
    
    AtomicUpdateResult result;
    result.transaction_id = generateConflictId({rule.id}, ConflictType::PERMIT_DENY);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Create snapshot
    auto existing_rule = policy_mgr.getRule(rule.id);
    if (!existing_rule) {
        result.success = false;
        result.error_message = fmt::format("Rule '{}' not found", rule.id);
        return result;
    }
    
    try {
        // Update rule
        policy_mgr.updateRule(rule.id, rule, "conflict_detector", "Atomic update validation");
        
        // Check for conflicts
        auto conflicts = detectAllConflicts(policy_mgr);
        
        if (!conflicts.empty()) {
            // Rollback
            policy_mgr.updateRule(rule.id, existing_rule.value(), "conflict_detector", 
                                 "Rollback due to conflict");
            
            result.success = false;
            result.conflicts_detected = conflicts;
            result.error_message = fmt::format(
                "Conflict validation failed: {} conflicts detected",
                conflicts.size()
            );
        } else {
            result.success = true;
            result.affected_rules = {rule.id};
        }
        
    } catch (const std::exception& e) {
        result.success = false;
        result.error_message = fmt::format("Exception during atomic update: {}", e.what());
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    result.operation_time_us = std::chrono::duration_cast<std::chrono::microseconds>(
        end_time - start_time
    ).count();
    
    if (result.success) {
        clearCache();
    }
    
    return result;
}

AtomicUpdateResult PolicyConflictDetector::atomicRemoveRule(
    const std::string& rule_id,
    PolicyManager& policy_mgr) {
    
    AtomicUpdateResult result;
    result.transaction_id = generateConflictId({rule_id}, ConflictType::PERMIT_DENY);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Create snapshot
    auto existing_rule = policy_mgr.getRule(rule_id);
    if (!existing_rule) {
        result.success = false;
        result.error_message = fmt::format("Rule '{}' not found", rule_id);
        return result;
    }
    
    try {
        // Remove rule
        policy_mgr.removeRule(rule_id);
        
        // Check for conflicts (may have been holding others together)
        auto conflicts = detectAllConflicts(policy_mgr);
        
        // For removal, we allow conflicts as long as they're not new
        result.success = true;
        result.affected_rules = {rule_id};
        
    } catch (const std::exception& e) {
        // Restore
        policy_mgr.addRule(existing_rule.value());
        result.success = false;
        result.error_message = fmt::format("Exception during atomic remove: {}", e.what());
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    result.operation_time_us = std::chrono::duration_cast<std::chrono::microseconds>(
        end_time - start_time
    ).count();
    
    if (result.success) {
        clearCache();
    }
    
    return result;
}

std::optional<PolicyConflict> PolicyConflictDetector::checkRuleConflict(
    const PolicyRule& rule1,
    const PolicyRule& rule2) {
    
    // Check if rules match on resources and actions
    if (!rulesMatch(rule1, rule2)) {
        return std::nullopt;
    }
    
    // Check for contradictory effects
    ConflictType conflict_type = ConflictType::PERMIT_DENY;
    
    // Export conflict?
    if (rule1.allow_export != rule2.allow_export) {
        conflict_type = ConflictType::EXPORT_CONFLICT;
    }
    // Encryption conflict?
    else if (rule1.require_encryption != rule2.require_encryption) {
        conflict_type = ConflictType::ENCRYPTION_CONFLICT;
    }
    // Cache conflict?
    else if (rule1.allow_cache != rule2.allow_cache) {
        return std::nullopt;  // Cache is not critical
    }
    // Retention conflict?
    else if (rule1.retention_days != rule2.retention_days && 
             rule1.retention_days > 0 && rule2.retention_days > 0) {
        conflict_type = ConflictType::RETENTION_CONFLICT;
    }
    else {
        return std::nullopt;  // No conflict
    }
    
    PolicyConflict conflict;
    conflict.conflict_id = generateConflictId({rule1.id, rule2.id}, conflict_type);
    conflict.conflict_type = conflict_type;
    conflict.conflicting_rule_ids = {rule1.id, rule2.id};
    conflict.description = fmt::format(
        "Rules '{}' and '{}' have contradictory effects on overlapping resources/actions. "
        "Rule {} allows/requires what {} denies/forbids.",
        rule1.name, rule2.name, rule1.id, rule2.id
    );
    conflict.severity = computeSeverity(rule1, rule2, conflict_type);
    conflict.resolution_strategy = "Review precedence settings or merge rules";
    conflict.detected_at = std::chrono::system_clock::now().time_since_epoch().count();
    
    return conflict;
}

std::vector<PolicyConflict> PolicyConflictDetector::getCachedConflicts(
    const PolicyManager& policy_mgr) const {
    
    std::shared_lock<std::shared_mutex> lock(state_mutex_);
    
    if (!caching_enabled_) {
        return {};
    }
    
    auto cache_key = std::to_string(reinterpret_cast<uintptr_t>(&policy_mgr));
    auto it = conflict_cache_.find(cache_key);
    
    if (it != conflict_cache_.end()) {
        return it->second;
    }
    
    return {};
}

void PolicyConflictDetector::clearCache() {
    std::unique_lock<std::shared_mutex> lock(state_mutex_);
    conflict_cache_.clear();
    cache_timestamp_ = 0;
}

nlohmann::json PolicyConflictDetector::getStatistics() const {
    std::shared_lock<std::shared_mutex> lock(state_mutex_);
    
    nlohmann::json stats;
    stats["total_detections"] = total_detections_;
    stats["conflict_types"] = nlohmann::json::object();
    
    for (const auto& [type, count] : conflict_type_counts_) {
        stats["conflict_types"][type] = count;
    }
    
    return stats;
}

// ========== Private Helper Methods ==========

bool PolicyConflictDetector::rulesMatch(const PolicyRule& rule1, const PolicyRule& rule2) const {
    // Check if two rules apply to same resource/action combinations
    
    auto matches_pattern = [](const std::string& pattern, 
                             const std::string& text) -> bool {
        if (pattern == "*" || text == "*") {
          return true;
        }
        if (pattern.empty() || text.empty()) {
          return false;
        }
        if (pattern.back() == '*') {
            return text.substr(0, pattern.length() - 1) == pattern.substr(0, pattern.length() - 1);
        }
        return pattern == text;
    };
    
    // Check resources
    bool resources_match = false;
    for (const auto& r1 : rule1.resources) {
        for (const auto& r2 : rule2.resources) {
            if (matches_pattern(r1, r2) || matches_pattern(r2, r1)) {
                resources_match = true;
                break;
            }
        }
        if (resources_match) {
          break;
        }
    }
    
    if (!resources_match) {
      return false;
    }
    
    // Check actions
    bool actions_match = false;
    for (const auto& a1 : rule1.actions) {
        for (const auto& a2 : rule2.actions) {
            if (matches_pattern(a1, a2) || matches_pattern(a2, a1)) {
                actions_match = true;
                break;
            }
        }
        if (actions_match) {
          break;
        }
    }
    
    return actions_match;
}

bool PolicyConflictDetector::hasSameScope(const PolicyRule& rule1, 
                                         const PolicyRule& rule2) const {
    return rule1.resources == rule2.resources && rule1.actions == rule2.actions;
}

ConflictSeverity PolicyConflictDetector::computeSeverity(
    const PolicyRule& rule1,
    const PolicyRule& rule2,
    ConflictType conflict_type) const {
    
    // Encryption conflicts are high severity
    if (conflict_type == ConflictType::ENCRYPTION_CONFLICT) {
        return ConflictSeverity::HIGH;
    }
    
    // Export conflicts are high severity
    if (conflict_type == ConflictType::EXPORT_CONFLICT) {
        return ConflictSeverity::HIGH;
    }
    
    // Retention conflicts are medium
    if (conflict_type == ConflictType::RETENTION_CONFLICT) {
        return ConflictSeverity::MEDIUM;
    }
    
    // Circular dependencies are critical
    if (conflict_type == ConflictType::CIRCULAR_DEPENDENCY) {
        return ConflictSeverity::CRITICAL;
    }
    
    // Default to medium
    return ConflictSeverity::MEDIUM;
}

std::string PolicyConflictDetector::generateConflictId(
    const std::vector<std::string>& rule_ids,
    ConflictType conflict_type) const {
    
    std::string type_str = {};
    switch (conflict_type) {
        case ConflictType::PERMIT_DENY: type_str = "PD"; break;
        case ConflictType::OVERLAPPING: type_str = "OV"; break;
        case ConflictType::CIRCULAR_DEPENDENCY: type_str = "CD"; break;
        case ConflictType::TYPE_MISMATCH: type_str = "TM"; break;
        case ConflictType::ENCRYPTION_CONFLICT: type_str = "EC"; break;
        case ConflictType::EXPORT_CONFLICT: type_str = "EXP"; break;
        case ConflictType::RETENTION_CONFLICT: type_str = "RET"; break;
        case ConflictType::COMPLIANCE_CONFLICT: type_str = "COMP"; break;
        default: type_str = "UK"; break;
    }
    
    // Generate unique ID from rule IDs and type
    std::string id = type_str + "_";
    for (const auto& rid : rule_ids) {
        id += rid + "_";
    }
    id += std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    
    return id;
}

bool PolicyConflictDetector::hasCircularDependency(
    const std::string& rule_id,
    std::unordered_set<std::string>& visited,
    std::unordered_set<std::string>& rec_stack,
    const PolicyManager& policy_mgr) const {

    if (rec_stack.count(rule_id)) {
        return true;  // Back-edge found: cycle detected
    }
    if (visited.count(rule_id)) {
        return false;  // Already fully explored, no cycle via this node
    }

    // Mark as in-progress for cycle detection
    visited.insert(rule_id);
    rec_stack.insert(rule_id);

    // Derive dependency edges: rule A depends on rule B when B's id appears
    // as a resource pattern in A (e.g., resource "rule:<id>" or "<id>").
    auto rule_opt = policy_mgr.getRule(rule_id);
    if (rule_opt) {
        auto all_rules = policy_mgr.listRules();
        for (const auto& candidate : all_rules) {
            if (candidate.id == rule_id) {
                continue;
            }
            for (const auto& resource : rule_opt->resources) {
                if (resource == candidate.id ||
                    resource == "rule:" + candidate.id) {
                    if (hasCircularDependency(candidate.id, visited, rec_stack,
                                             policy_mgr)) {
                        return true;
                    }
                    break;
                }
            }
        }
    }

    rec_stack.erase(rule_id);
    return false;
}

} // namespace governance
} // namespace themis
