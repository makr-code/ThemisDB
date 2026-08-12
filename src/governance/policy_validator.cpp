/**
 * @file policy_validator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=6, M=13, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "governance/policy_validator.h"

#include <algorithm>
#include <chrono>
#include <queue>
#include <unordered_map>
#include <unordered_set>

#include "utils/logger.h"

namespace themis {
namespace governance {

// ========== PolicyConflict Implementation ==========

nlohmann::json PolicyConflict::toJson() const {
    nlohmann::json j;
    j["conflict_type"]          = conflict_type;
    j["severity"]               = severity;
    j["affected_rules"]         = affected_rules;
    j["description"]            = description;
    j["resolution_suggestions"] = resolution_suggestions;
    return j;
}

// ========== RuleEffectiveness Implementation ==========

nlohmann::json RuleEffectiveness::toJson() const {
    nlohmann::json j;
    j["rule_id"]               = rule_id;
    j["hit_count"]             = hit_count;
    j["is_unused"]             = is_unused;
    j["performance_impact_ms"] = performance_impact_ms;
    j["effectiveness_rating"]  = effectiveness_rating;
    return j;
}

// ========== SecurityViolation Implementation ==========

nlohmann::json SecurityViolation::toJson() const {
    nlohmann::json j;
    j["violation_type"]  = violation_type;
    j["severity"]        = severity;
    j["affected_rules"]  = affected_rules;
    j["description"]     = description;
    j["recommendations"] = recommendations;
    return j;
}

// ========== ValidationReport Implementation ==========

nlohmann::json ValidationReport::toJson() const {
    nlohmann::json j;
    j["report_id"]    = report_id;
    j["generated_at"] = generated_at;

    nlohmann::json conflicts_array = nlohmann::json::array();
    for (const auto &c : conflicts) {
        conflicts_array.push_back(c.toJson());
    }
    j["conflicts"] = conflicts_array;

    nlohmann::json violations_array = nlohmann::json::array();
    for (const auto &v : violations) {
        violations_array.push_back(v.toJson());
    }
    j["violations"] = violations_array;

    nlohmann::json metrics_array = nlohmann::json::array();
    for (const auto &m : effectiveness_metrics) {
        metrics_array.push_back(m.toJson());
    }
    j["effectiveness_metrics"] = metrics_array;

    j["has_critical_issues"] = has_critical_issues;
    j["total_issues"]        = total_issues;
    j["validation_score"]    = validation_score;

    return j;
}

// ========== PolicyValidator Implementation ==========

PolicyValidator::PolicyValidator(std::shared_ptr<PolicyManager> policy_manager)
    : policy_manager_(std::move(policy_manager)) {
    if (!policy_manager_) {
        THEMIS_WARN("PolicyValidator created with null PolicyManager");
    }
}

std::vector<PolicyConflict> PolicyValidator::detectConflicts() const {
    std::vector<PolicyConflict> conflicts;

    if (!policy_manager_) {
        return conflicts;
    }
    auto rules = policy_manager_->listRules();

    // Check each pair for contradictions, skipping disabled rules
    for (size_t i = 0; i < rules.size(); i++) {
        if (!rules[i].enabled) {
            continue;
        }
        for (size_t j = i + 1; j < rules.size(); j++) {
            if (!rules[j].enabled) {
                continue;
            }
            if (areContradictory(rules[i], rules[j])) {
                PolicyConflict conflict;
                conflict.conflict_type = "contradictory";
                // Determine severity based on which attribute conflicts
                if (rules[i].require_encryption != rules[j].require_encryption) {
                    conflict.severity = "critical";
                } else if (rules[i].allow_export != rules[j].allow_export) {
                    conflict.severity = "high";
                } else {
                    conflict.severity = "medium";
                }
                conflict.affected_rules = {rules[i].id, rules[j].id};
                conflict.description    = "Rules '" + rules[i].name + "' and '" + rules[j].name
                                          + "' have contradictory requirements for overlapping resources/actions";
                conflict.resolution_suggestions = {"Review rule priorities", "Merge or consolidate rules",
                                                   "Clarify rule scope to eliminate overlap"};
                conflicts.push_back(conflict);
            }
        }
    }

    return conflicts;
}

std::vector<PolicyConflict> PolicyValidator::detectOverlappingPermissions() const {
    std::vector<PolicyConflict> overlaps;

    if (!policy_manager_) {
        return overlaps;
    }
    auto rules = policy_manager_->listRules();

    for (size_t i = 0; i < rules.size(); i++) {
        if (!rules[i].enabled) {
            continue;
        }
        for (size_t j = i + 1; j < rules.size(); j++) {
            if (!rules[j].enabled) {
                continue;
            }
            const auto &r1 = rules[i];
            const auto &r2 = rules[j];

            // Check resource overlap
            bool res_overlap = false;
            for (const auto &res1 : r1.resources) {
                for (const auto &res2 : r2.resources) {
                    if (res1 == res2 || res1 == "*" || res2 == "*") {
                        res_overlap = true;
                        break;
                    }
                }
                if (res_overlap) {
                    break;
                }
            }
            if (!res_overlap) {
                continue;
            }

            // Check action overlap
            bool act_overlap = false;
            for (const auto &act1 : r1.actions) {
                for (const auto &act2 : r2.actions) {
                    if (act1 == act2 || act1 == "*" || act2 == "*") {
                        act_overlap = true;
                        break;
                    }
                }
                if (act_overlap) {
                    break;
                }
            }

            if (act_overlap && r1.priority == r2.priority) {
                PolicyConflict conflict;
                conflict.conflict_type          = "overlapping";
                conflict.severity               = "medium";
                conflict.affected_rules         = {r1.id, r2.id};
                conflict.description            = "Rules '" + r1.name + "' and '" + r2.name
                                                  + "' overlap on the same resource and action with equal priority,"
                                                    " creating evaluation ambiguity";
                conflict.resolution_suggestions = {"Assign distinct priorities to establish a clear evaluation order",
                                                   "Narrow the resource or action patterns to eliminate the overlap",
                                                   "Consider merging rules if they have identical effects"};
                overlaps.push_back(conflict);
            }
        }
    }

    return overlaps;
}

std::vector<PolicyConflict> PolicyValidator::detectCircularDependencies() const {
    std::vector<PolicyConflict> circular;

    auto rules = policy_manager_->listRules();

    // Build an undirected conflict graph:
    // An edge between rule A and rule B exists when both rules overlap on at
    // least one resource/action AND have the same priority AND their effects
    // contradict each other.  A connected component of size >= 3 in this graph
    // represents a "circular" evaluation problem: no deterministic priority
    // ordering can resolve the group because every rule in the component is
    // simultaneously in conflict with at least one other member.
    std::unordered_map<std::string, std::vector<std::string>> conflict_graph;

    for (std::size_t i = 0; i < rules.size(); ++i) {
        const auto &r1 = rules[i];
        if (!r1.enabled) {
            continue;
        }

        for (std::size_t j = i + 1; j < rules.size(); ++j) {
            const auto &r2 = rules[j];
            if (!r2.enabled) {
                continue;
            }

            // Only rules with the same priority can form an irresolvable cycle
            if (r1.priority != r2.priority) {
                continue;
            }

            // Check for resource overlap
            bool res_overlap = false;
            for (const auto &ra : r1.resources) {
                for (const auto &rb : r2.resources) {
                    if (ra == rb || ra == "*" || rb == "*") {
                        res_overlap = true;
                        break;
                    }
                }
                if (res_overlap) {
                    break;
                }
            }
            if (!res_overlap) {
                continue;
            }

            // Check for action overlap
            bool act_overlap = false;
            for (const auto &aa : r1.actions) {
                for (const auto &ab : r2.actions) {
                    if (aa == ab || aa == "*" || ab == "*") {
                        act_overlap = true;
                        break;
                    }
                }
                if (act_overlap) {
                    break;
                }
            }
            if (!act_overlap) {
                continue;
            }

            // Contradictory effects at the same priority ⟹ edge in conflict graph
            if (areContradictory(r1, r2)) {
                conflict_graph[r1.id].push_back(r2.id);
                conflict_graph[r2.id].push_back(r1.id);
            }
        }
    }

    // BFS over the conflict graph to find connected components of size >= 3
    std::unordered_set<std::string> visited;

    for (const auto &kv : conflict_graph) {
        const std::string &start_id = kv.first;
        if (visited.count(start_id) > 0) {
            continue;
        }

        // BFS
        std::vector<std::string> component;
        std::queue<std::string> q;
        q.push(start_id);
        visited.insert(start_id);

        while (!q.empty()) {
            std::string cur = q.front();
            q.pop();
            component.push_back(cur);

            for (const auto &neighbor : conflict_graph.at(cur)) {
                if (visited.count(neighbor) == 0) {
                    visited.insert(neighbor);
                    q.push(neighbor);
                }
            }
        }

        if (component.size() >= 3) {
            PolicyConflict conflict;
            conflict.conflict_type  = "circular";
            conflict.severity       = "high";
            conflict.affected_rules = component;
            conflict.description    = "Rules form an irresolvable priority cycle: " + std::to_string(component.size())
                                      + " rules share the same priority, overlapping resources/actions, "
                                        "and contradictory effects";
            conflict.resolution_suggestions = {"Assign distinct priorities to break the evaluation cycle",
                                               "Refactor rules to eliminate overlapping resource/action patterns",
                                               "Merge conflicting rules into a single authoritative rule"};
            circular.push_back(conflict);
        }
    }

    return circular;
}

std::vector<RuleEffectiveness> PolicyValidator::calculateEffectiveness() const {
    std::vector<RuleEffectiveness> metrics;

    auto rules = policy_manager_->listRules();

    for (const auto &rule : rules) {
        RuleEffectiveness metric;
        metric.rule_id = rule.id;

        // Get hit count if tracked
        auto hit_it = rule_hits_.find(rule.id);
        if (hit_it != rule_hits_.end()) {
            metric.hit_count = hit_it->second;
        }

        // Get evaluation time if tracked
        auto eval_it = rule_eval_times_.find(rule.id);
        if (eval_it != rule_eval_times_.end()) {
            metric.performance_impact_ms = eval_it->second / std::max(1, metric.hit_count);
        }

        // Determine effectiveness rating
        metric.is_unused = (metric.hit_count == 0);
        if (metric.is_unused) {
            metric.effectiveness_rating = "unused";
        } else if (metric.hit_count > 100) {
            metric.effectiveness_rating = "high";
        } else if (metric.hit_count > 10) {
            metric.effectiveness_rating = "medium";
        } else {
            metric.effectiveness_rating = "low";
        }

        metrics.push_back(metric);
    }

    return metrics;
}

std::vector<std::string> PolicyValidator::detectUnusedRules() const {
    std::vector<std::string> unused;

    auto rules = policy_manager_->listRules();

    for (const auto &rule : rules) {
        auto hit_it = rule_hits_.find(rule.id);
        if (hit_it == rule_hits_.end() || hit_it->second == 0) {
            unused.push_back(rule.id);
        }
    }

    return unused;
}

std::vector<SecurityViolation> PolicyValidator::checkSecurityBestPractices() const {
    std::vector<SecurityViolation> violations;

    auto rules = policy_manager_->listRules();

    std::vector<std::string> overly_permissive;
    std::vector<std::string> weak_encryption;
    std::vector<std::string> missing_audit;
    std::vector<std::string> long_retention;

    for (const auto &rule : rules) {
        if (!rule.enabled) {
            continue;
        }

        // Check for overly permissive rules
        if (rule.actions.size() == 1 && rule.actions[0] == "*" && rule.resources.size() == 1
            && rule.resources[0] == "*") {
            overly_permissive.push_back(rule.id);
        }

        // Check encryption for sensitive data
        if (!rule.require_encryption
            && (rule.classification_level == "geheim" || rule.classification_level == "streng-geheim")) {
            weak_encryption.push_back(rule.id);
        }

        // Check audit requirements
        if (!rule.audit_access && !rule.audit_changes && rule.priority > 50) {
            missing_audit.push_back(rule.id);
        }

        // Check retention period
        if (rule.retention_days > 3650) { // > 10 years
            long_retention.push_back(rule.id);
        }
    }

    if (!overly_permissive.empty()) {
        SecurityViolation v;
        v.violation_type  = "overly_permissive";
        v.severity        = "critical";
        v.affected_rules  = overly_permissive;
        v.description     = "Rules grant access to all resources and actions";
        v.recommendations = {"Restrict resource patterns to specific paths",
                             "Limit actions to necessary operations only", "Apply principle of least privilege"};
        violations.push_back(v);
    }

    if (!weak_encryption.empty()) {
        SecurityViolation v;
        v.violation_type  = "weak_encryption";
        v.severity        = "high";
        v.affected_rules  = weak_encryption;
        v.description     = "Sensitive data without encryption requirements";
        v.recommendations = {"Enable require_encryption for classified data"};
        violations.push_back(v);
    }

    if (!missing_audit.empty()) {
        SecurityViolation v;
        v.violation_type  = "missing_audit";
        v.severity        = "medium";
        v.affected_rules  = missing_audit;
        v.description     = "High-priority rules without audit logging";
        v.recommendations = {"Enable audit_access or audit_changes"};
        violations.push_back(v);
    }

    return violations;
}

std::vector<SecurityViolation> PolicyValidator::detectCcpaConflicts() const {
    std::vector<SecurityViolation> violations;

    auto rules = policy_manager_->listRules();

    // HIPAA mandates a minimum 6-year (2190 day) retention for medical records.
    // CCPA grants California residents the right to request deletion at any time.
    // A policy rule that enforces long-retention without acknowledging CCPA
    // deletion rights represents a regulatory conflict that must be resolved
    // (typically by carving out a HIPAA exemption in the CCPA workflow).
    constexpr int HIPAA_MIN_RETENTION_DAYS = 2190; // 6 years

    std::vector<std::string> hipaa_ccpa_conflict_rules;
    std::vector<std::string> ccpa_export_block_rules;

    for (const auto &rule : rules) {
        if (!rule.enabled) {
            continue;
        }

        // Detect HIPAA-style long-retention rules that conflict with CCPA deletion
        if (rule.retention_days >= HIPAA_MIN_RETENTION_DAYS) {
            hipaa_ccpa_conflict_rules.push_back(rule.id);
        }

        // Detect rules that block export on all resources (would prevent CCPA
        // data portability from being fulfilled)
        bool blocks_all_export = !rule.allow_export && (rule.resources.size() == 1 && rule.resources[0] == "*");
        if (blocks_all_export) {
            ccpa_export_block_rules.push_back(rule.id);
        }
    }

    if (!hipaa_ccpa_conflict_rules.empty()) {
        SecurityViolation v;
        v.violation_type  = "ccpa_hipaa_retention_conflict";
        v.severity        = "medium";
        v.affected_rules  = hipaa_ccpa_conflict_rules;
        v.description     = "Rules enforce long retention (≥ 6 years) that may conflict with "
                            "CCPA right-to-delete (Cal. Civ. Code § 1798.105). "
                            "HIPAA-covered entities may invoke the HIPAA exemption, but the "
                            "conflict must be explicitly documented in policy.";
        v.recommendations = {"Document the HIPAA exemption to CCPA deletion in the rule description",
                             "Ensure CCPA deletion requests trigger a HIPAA-exemption review workflow",
                             "Consider splitting PHI resources from general PI resources for cleaner control"};
        violations.push_back(v);
    }

    if (!ccpa_export_block_rules.empty()) {
        SecurityViolation v;
        v.violation_type  = "ccpa_portability_blocked";
        v.severity        = "high";
        v.affected_rules  = ccpa_export_block_rules;
        v.description     = "Rules block export on all resources, which would prevent fulfilling "
                            "CCPA data portability requests (Cal. Civ. Code § 1798.100).";
        v.recommendations = {"Allow export for CCPA data portability requests by scoping the export restriction "
                             "to specific non-PI resources rather than '*'",
                             "Add a dedicated portability exception rule with higher priority"};
        violations.push_back(v);
    }

    return violations;
}

ValidationReport PolicyValidator::validateRuleset() const {
    ValidationReport report;
    report.report_id    = "validation_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    report.generated_at = std::chrono::system_clock::now().time_since_epoch().count() / 1000000000;

    // Detect all issues
    report.conflicts = detectConflicts();
    auto overlaps    = detectOverlappingPermissions();
    report.conflicts.insert(report.conflicts.end(), overlaps.begin(), overlaps.end());
    auto circular = detectCircularDependencies();
    report.conflicts.insert(report.conflicts.end(), circular.begin(), circular.end());

    report.violations = checkSecurityBestPractices();

    // Include CCPA/HIPAA conflict detection (per FUTURE_ENHANCEMENTS.md)
    auto ccpa_violations = detectCcpaConflicts();
    report.violations.insert(report.violations.end(), ccpa_violations.begin(), ccpa_violations.end());

    report.effectiveness_metrics = calculateEffectiveness();

    // Calculate summary
    report.total_issues = static_cast<int>(report.conflicts.size() + report.violations.size());

    for (const auto &c : report.conflicts) {
        if (c.severity == "critical" || c.severity == "high") {
            report.has_critical_issues = true;
            break;
        }
    }

    if (!report.has_critical_issues) {
        for (const auto &v : report.violations) {
            if (v.severity == "critical" || v.severity == "high") {
                report.has_critical_issues = true;
                break;
            }
        }
    }

    report.validation_score = calculateValidationScore(report);

    THEMIS_INFO("Validation complete: {} issues found, score: {:.1f}", report.total_issues, report.validation_score);

    return report;
}

std::vector<std::string> PolicyValidator::validateSingleRule(const PolicyRule &rule) const {
    std::vector<std::string> issues;

    // Check for empty required fields
    if (rule.id.empty()) {
        issues.push_back("Rule ID is required");
    }

    if (rule.resources.empty()) {
        issues.push_back("At least one resource pattern is required");
    }

    if (rule.actions.empty()) {
        issues.push_back("At least one action is required");
    }

    // Check security best practices
    if (!followsSecurityBestPractices(rule)) {
        issues.push_back("Rule does not follow security best practices");
    }

    return issues;
}

void PolicyValidator::recordRuleHit(const std::string &rule_id, double evaluation_time_ms) {
    rule_hits_[rule_id]++;
    rule_eval_times_[rule_id] += evaluation_time_ms;
}

bool PolicyValidator::areContradictory(const PolicyRule &rule1, const PolicyRule &rule2) const {
    // Rules are contradictory if they apply to overlapping resources/actions but have opposite effects

    // Check resource overlap (including wildcard "*")
    bool res_overlap = false;
    for (const auto &r1_res : rule1.resources) {
        for (const auto &r2_res : rule2.resources) {
            if (r1_res == r2_res || r1_res == "*" || r2_res == "*") {
                res_overlap = true;
                break;
            }
        }
        if (res_overlap) {
            break;
        }
    }
    if (!res_overlap) {
        return false;
    }

    // Check action overlap (including wildcard "*")
    bool act_overlap = false;
    for (const auto &r1_act : rule1.actions) {
        for (const auto &r2_act : rule2.actions) {
            if (r1_act == r2_act || r1_act == "*" || r2_act == "*") {
                act_overlap = true;
                break;
            }
        }
        if (act_overlap) {
            break;
        }
    }
    if (!act_overlap) {
        return false;
    }

    // Check for contradictory access-control settings
    return (rule1.require_encryption != rule2.require_encryption || rule1.allow_export != rule2.allow_export
            || rule1.allow_cache != rule2.allow_cache);
}

bool PolicyValidator::followsSecurityBestPractices(const PolicyRule &rule) const {
    // Basic checks
    if (rule.actions.size() == 1 && rule.actions[0] == "*" && rule.resources.size() == 1 && rule.resources[0] == "*") {
        return false; // Too permissive
    }

    // Sensitive data should have encryption
    if ((rule.classification_level == "geheim" || rule.classification_level == "streng-geheim")
        && !rule.require_encryption) {
        return false;
    }

    return true;
}

double PolicyValidator::calculateValidationScore(const ValidationReport &report) const {
    double score = 100.0;

    // Deduct for conflicts
    for (const auto &c : report.conflicts) {
        if (c.severity == "critical") {
            score -= 15.0;
        } else if (c.severity == "high") {
            score -= 10.0;
        } else if (c.severity == "medium") {
            score -= 5.0;
        } else {
            score -= 2.0;
        }
    }

    // Deduct for violations
    for (const auto &v : report.violations) {
        if (v.severity == "critical") {
            score -= 20.0;
        } else if (v.severity == "high") {
            score -= 15.0;
        } else if (v.severity == "medium") {
            score -= 10.0;
        } else {
            score -= 5.0;
        }
    }

    return std::max(0.0, score);
}

} // namespace governance
} // namespace themis
