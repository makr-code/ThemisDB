/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            policy_validator.cpp                               ║
  Version:         0.0.7                                              ║
  Last Modified:   2026-02-21 11:48:42                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     455                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "governance/policy_validator.h"
#include "utils/logger.h"

#include <chrono>
#include <algorithm>

namespace themis {
namespace governance {

// ========== PolicyConflict Implementation ==========

nlohmann::json PolicyConflict::toJson() const {
    nlohmann::json j;
    j["conflict_type"] = conflict_type;
    j["severity"] = severity;
    j["affected_rules"] = affected_rules;
    j["description"] = description;
    j["resolution_suggestions"] = resolution_suggestions;
    return j;
}

// ========== RuleEffectiveness Implementation ==========

nlohmann::json RuleEffectiveness::toJson() const {
    nlohmann::json j;
    j["rule_id"] = rule_id;
    j["hit_count"] = hit_count;
    j["is_unused"] = is_unused;
    j["performance_impact_ms"] = performance_impact_ms;
    j["effectiveness_rating"] = effectiveness_rating;
    return j;
}

// ========== SecurityViolation Implementation ==========

nlohmann::json SecurityViolation::toJson() const {
    nlohmann::json j;
    j["violation_type"] = violation_type;
    j["severity"] = severity;
    j["affected_rules"] = affected_rules;
    j["description"] = description;
    j["recommendations"] = recommendations;
    return j;
}

// ========== ValidationReport Implementation ==========

nlohmann::json ValidationReport::toJson() const {
    nlohmann::json j;
    j["report_id"] = report_id;
    j["generated_at"] = generated_at;
    
    nlohmann::json conflicts_array = nlohmann::json::array();
    for (const auto& c : conflicts) {
        conflicts_array.push_back(c.toJson());
    }
    j["conflicts"] = conflicts_array;
    
    nlohmann::json violations_array = nlohmann::json::array();
    for (const auto& v : violations) {
        violations_array.push_back(v.toJson());
    }
    j["violations"] = violations_array;
    
    nlohmann::json metrics_array = nlohmann::json::array();
    for (const auto& m : effectiveness_metrics) {
        metrics_array.push_back(m.toJson());
    }
    j["effectiveness_metrics"] = metrics_array;
    
    j["has_critical_issues"] = has_critical_issues;
    j["total_issues"] = total_issues;
    j["validation_score"] = validation_score;
    
    return j;
}

// ========== PolicyValidator Implementation ==========

PolicyValidator::PolicyValidator(std::shared_ptr<PolicyManager> policy_manager)
    : policy_manager_(std::move(policy_manager))
{
    if (!policy_manager_) {
        THEMIS_WARN("PolicyValidator created with null PolicyManager");
    }
}

std::vector<PolicyConflict> PolicyValidator::detectConflicts() const {
    std::vector<PolicyConflict> conflicts;
    
    auto rules = policy_manager_->listRules();
    
    // Check each pair for contradictions
    for (size_t i = 0; i < rules.size(); i++) {
        for (size_t j = i + 1; j < rules.size(); j++) {
            if (areContradictory(rules[i], rules[j])) {
                PolicyConflict conflict;
                conflict.conflict_type = "contradictory";
                conflict.severity = "high";
                conflict.affected_rules = {rules[i].id, rules[j].id};
                conflict.description = "Rules have contradictory requirements";
                conflict.resolution_suggestions = {
                    "Review rule priorities",
                    "Merge or consolidate rules",
                    "Clarify rule scope"
                };
                conflicts.push_back(conflict);
            }
        }
    }
    
    return conflicts;
}

std::vector<PolicyConflict> PolicyValidator::detectOverlappingPermissions() const {
    std::vector<PolicyConflict> overlaps;
    
    auto rules = policy_manager_->listRules();
    
    for (size_t i = 0; i < rules.size(); i++) {
        for (size_t j = i + 1; j < rules.size(); j++) {
            const auto& r1 = rules[i];
            const auto& r2 = rules[j];
            
            // Check resource overlap
            bool has_overlap = false;
            for (const auto& res1 : r1.resources) {
                for (const auto& res2 : r2.resources) {
                    if (res1 == res2 || res1 == "*" || res2 == "*") {
                        has_overlap = true;
                        break;
                    }
                }
                if (has_overlap) break;
            }
            
            if (has_overlap && r1.priority == r2.priority) {
                PolicyConflict conflict;
                conflict.conflict_type = "overlapping";
                conflict.severity = "medium";
                conflict.affected_rules = {r1.id, r2.id};
                conflict.description = "Rules overlap with same priority";
                conflict.resolution_suggestions = {
                    "Adjust rule priorities",
                    "Make resource patterns more specific"
                };
                overlaps.push_back(conflict);
            }
        }
    }
    
    return overlaps;
}

std::vector<PolicyConflict> PolicyValidator::detectCircularDependencies() const {
    // Simplified: Check for rules that might create circular permission chains
    std::vector<PolicyConflict> circular;
    
    // In a real implementation, this would do dependency graph analysis
    // For now, we just return empty as circular deps are complex to detect
    
    return circular;
}

std::vector<RuleEffectiveness> PolicyValidator::calculateEffectiveness() const {
    std::vector<RuleEffectiveness> metrics;
    
    auto rules = policy_manager_->listRules();
    
    for (const auto& rule : rules) {
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
    
    for (const auto& rule : rules) {
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
    
    for (const auto& rule : rules) {
        if (!rule.enabled) continue;
        
        // Check for overly permissive rules
        if (rule.actions.size() == 1 && rule.actions[0] == "*" && 
            rule.resources.size() == 1 && rule.resources[0] == "*") {
            overly_permissive.push_back(rule.id);
        }
        
        // Check encryption for sensitive data
        if (!rule.require_encryption && 
            (rule.classification_level == "geheim" || rule.classification_level == "streng-geheim")) {
            weak_encryption.push_back(rule.id);
        }
        
        // Check audit requirements
        if (!rule.audit_access && !rule.audit_changes && rule.priority > 50) {
            missing_audit.push_back(rule.id);
        }
        
        // Check retention period
        if (rule.retention_days > 3650) {  // > 10 years
            long_retention.push_back(rule.id);
        }
    }
    
    if (!overly_permissive.empty()) {
        SecurityViolation v;
        v.violation_type = "overly_permissive";
        v.severity = "critical";
        v.affected_rules = overly_permissive;
        v.description = "Rules grant access to all resources and actions";
        v.recommendations = {
            "Restrict resource patterns to specific paths",
            "Limit actions to necessary operations only",
            "Apply principle of least privilege"
        };
        violations.push_back(v);
    }
    
    if (!weak_encryption.empty()) {
        SecurityViolation v;
        v.violation_type = "weak_encryption";
        v.severity = "high";
        v.affected_rules = weak_encryption;
        v.description = "Sensitive data without encryption requirements";
        v.recommendations = {"Enable require_encryption for classified data"};
        violations.push_back(v);
    }
    
    if (!missing_audit.empty()) {
        SecurityViolation v;
        v.violation_type = "missing_audit";
        v.severity = "medium";
        v.affected_rules = missing_audit;
        v.description = "High-priority rules without audit logging";
        v.recommendations = {"Enable audit_access or audit_changes"};
        violations.push_back(v);
    }
    
    return violations;
}

ValidationReport PolicyValidator::validateRuleset() const {
    ValidationReport report;
    report.report_id = "validation_" + std::to_string(
        std::chrono::system_clock::now().time_since_epoch().count()
    );
    report.generated_at = std::chrono::system_clock::now().time_since_epoch().count() / 1000000000;
    
    // Detect all issues
    report.conflicts = detectConflicts();
    auto overlaps = detectOverlappingPermissions();
    report.conflicts.insert(report.conflicts.end(), overlaps.begin(), overlaps.end());
    
    report.violations = checkSecurityBestPractices();
    report.effectiveness_metrics = calculateEffectiveness();
    
    // Calculate summary
    report.total_issues = report.conflicts.size() + report.violations.size();
    
    for (const auto& c : report.conflicts) {
        if (c.severity == "critical" || c.severity == "high") {
            report.has_critical_issues = true;
            break;
        }
    }
    
    if (!report.has_critical_issues) {
        for (const auto& v : report.violations) {
            if (v.severity == "critical" || v.severity == "high") {
                report.has_critical_issues = true;
                break;
            }
        }
    }
    
    report.validation_score = calculateValidationScore(report);
    
    THEMIS_INFO("Validation complete: {} issues found, score: {:.1f}", 
        report.total_issues, report.validation_score);
    
    return report;
}

std::vector<std::string> PolicyValidator::validateSingleRule(const PolicyRule& rule) const {
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

void PolicyValidator::recordRuleHit(const std::string& rule_id, double evaluation_time_ms) {
    rule_hits_[rule_id]++;
    rule_eval_times_[rule_id] += evaluation_time_ms;
}

bool PolicyValidator::areContradictory(const PolicyRule& rule1, const PolicyRule& rule2) const {
    // Rules are contradictory if they apply to same resource/action but have opposite effects
    
    // Check resource/action overlap
    bool has_overlap = false;
    for (const auto& r1_res : rule1.resources) {
        for (const auto& r2_res : rule2.resources) {
            if (r1_res == r2_res) {
                has_overlap = true;
                break;
            }
        }
        if (has_overlap) break;
    }
    
    if (!has_overlap) return false;
    
    // Check for contradictory settings
    if (rule1.require_encryption != rule2.require_encryption ||
        rule1.allow_export != rule2.allow_export ||
        rule1.allow_cache != rule2.allow_cache) {
        return true;
    }
    
    return false;
}

bool PolicyValidator::followsSecurityBestPractices(const PolicyRule& rule) const {
    // Basic checks
    if (rule.actions.size() == 1 && rule.actions[0] == "*" &&
        rule.resources.size() == 1 && rule.resources[0] == "*") {
        return false;  // Too permissive
    }
    
    // Sensitive data should have encryption
    if ((rule.classification_level == "geheim" || rule.classification_level == "streng-geheim") &&
        !rule.require_encryption) {
        return false;
    }
    
    return true;
}

double PolicyValidator::calculateValidationScore(const ValidationReport& report) const {
    double score = 100.0;
    
    // Deduct for conflicts
    for (const auto& c : report.conflicts) {
        if (c.severity == "critical") score -= 15.0;
        else if (c.severity == "high") score -= 10.0;
        else if (c.severity == "medium") score -= 5.0;
        else score -= 2.0;
    }
    
    // Deduct for violations
    for (const auto& v : report.violations) {
        if (v.severity == "critical") score -= 20.0;
        else if (v.severity == "high") score -= 15.0;
        else if (v.severity == "medium") score -= 10.0;
        else score -= 5.0;
    }
    
    return std::max(0.0, score);
}

} // namespace governance
} // namespace themis
