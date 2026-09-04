/**
 * @file policy_validation.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=1, M=47, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "governance/policy_validation.h"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <queue>
#include <sstream>
#include <unordered_set>

#include "governance/ccpa_rules.h"
#include "governance/pci_dss_rules.h"
#include "utils/logger.h"

namespace themis {
namespace governance {

// ========== PolicyValidator::ConflictResult Implementation ==========

nlohmann::json PolicyValidator::ConflictResult::toJson() const {
    nlohmann::json j;
    j["conflict_id"]          = conflict_id;
    j["conflict_type"]        = conflict_type;
    j["conflicting_rule_ids"] = conflicting_rule_ids;
    j["description"]          = description;
    j["severity"]             = severity;
    j["recommendation"]       = recommendation;
    return j;
}

// ========== PolicyValidator::EffectivenessMetrics Implementation ==========

nlohmann::json PolicyValidator::EffectivenessMetrics::toJson() const {
    nlohmann::json j;
    j["rule_id"]             = rule_id;
    j["hit_count"]           = hit_count;
    j["last_used"]           = last_used;
    j["created_at"]          = created_at;
    j["days_since_last_use"] = days_since_last_use;
    j["is_unused"]           = is_unused;
    j["effectiveness_score"] = effectiveness_score;
    return j;
}

// ========== PolicyValidator::SecurityCheckResult Implementation ==========

nlohmann::json PolicyValidator::SecurityCheckResult::toJson() const {
    nlohmann::json j;
    j["check_id"]       = check_id;
    j["check_type"]     = check_type;
    j["rule_id"]        = rule_id;
    j["severity"]       = severity;
    j["description"]    = description;
    j["recommendation"] = recommendation;
    j["passed"]         = passed;
    return j;
}

// ========== PolicyValidator::ValidationReport Implementation ==========

nlohmann::json PolicyValidator::ValidationReport::toJson() const {
    nlohmann::json j;
    j["total_rules_checked"]        = total_rules_checked;
    j["conflicts_found"]            = conflicts_found;
    j["security_issues_found"]      = security_issues_found;
    j["effectiveness_issues_found"] = effectiveness_issues_found;
    j["generated_at"]               = generated_at;

    nlohmann::json conflicts_arr = nlohmann::json::array();
    for (const auto &conflict : conflicts) {
        conflicts_arr.push_back(conflict.toJson());
    }
    j["conflicts"] = conflicts_arr;

    nlohmann::json security_arr = nlohmann::json::array();
    for (const auto &check : security_checks) {
        security_arr.push_back(check.toJson());
    }
    j["security_checks"] = security_arr;

    j["recommendations"] = recommendations;
    return j;
}

// ========== PolicyValidator Implementation ==========

std::vector<PolicyValidator::ConflictResult> PolicyValidator::detectConflicts(const PolicyManager &policy_mgr) const {
    THEMIS_DEBUG("Starting comprehensive conflict detection");

    std::vector<ConflictResult> all_conflicts;

    // Detect contradictory rules
    auto contradictory = detectContradictoryRules(policy_mgr);
    all_conflicts.insert(all_conflicts.end(), contradictory.begin(), contradictory.end());

    // Detect overlapping permissions
    auto overlapping = detectOverlappingPermissions(policy_mgr);
    all_conflicts.insert(all_conflicts.end(), overlapping.begin(), overlapping.end());

    // Detect circular dependencies
    auto circular = detectCircularDependencies(policy_mgr);
    all_conflicts.insert(all_conflicts.end(), circular.begin(), circular.end());

    // Detect CCPA/HIPAA cross-framework conflicts
    auto ccpa_hipaa = detectCcpaHipaaConflicts(policy_mgr);
    all_conflicts.insert(all_conflicts.end(), ccpa_hipaa.begin(), ccpa_hipaa.end());

    // Detect PCI-DSS/GDPR cross-framework conflicts
    auto pci_dss_gdpr = detectPciDssGdprConflicts(policy_mgr);
    all_conflicts.insert(all_conflicts.end(), pci_dss_gdpr.begin(), pci_dss_gdpr.end());

    THEMIS_INFO("Detected {} total conflicts",static_cast<int>(all_conflicts.size()));

    return all_conflicts;
}

std::vector<PolicyValidator::ConflictResult>
PolicyValidator::detectContradictoryRules(const PolicyManager &policy_mgr) const {
    std::vector<ConflictResult> conflicts;
    auto all_rules = policy_mgr.listRules();

    THEMIS_DEBUG("Checking {} rules for contradictions",static_cast<int>(all_rules.size()));

    // Compare each pair of rules
    for (size_t i = 0; i < all_rules.size(); ++i) {
        if (!all_rules[i].enabled) {
            continue;
        }

        for (size_t j = i + 1; j < all_rules.size(); ++j) {
            if (!all_rules[j].enabled) {
                continue;
            }

            const auto &rule1 = all_rules[i];
            const auto &rule2 = all_rules[j];

            // Check if rules apply to same resources
            bool has_common_resource = false;
            for (const auto &r1 : rule1.resources) {
                for (const auto &r2 : rule2.resources) {
                    if (r1 == r2 || r1 == "*" || r2 == "*") {
                        has_common_resource = true;
                        break;
                    }
                }
                if (has_common_resource) {
                    break;
                }
            }

            if (!has_common_resource) {
                continue;
            }

            // Check if rules apply to same actions
            bool has_common_action = false;
            for (const auto &a1 : rule1.actions) {
                for (const auto &a2 : rule2.actions) {
                    if (a1 == a2 || a1 == "*" || a2 == "*") {
                        has_common_action = true;
                        break;
                    }
                }
                if (has_common_action) {
                    break;
                }
            }

            if (!has_common_action) {
                continue;
            }

            // Check for contradictory effects
            if (rule1.allow_export != rule2.allow_export) {
                ConflictResult conflict;
                conflict.conflict_id          = "contradict_" + rule1.id + "_" + rule2.id;
                conflict.conflict_type        = "contradictory";
                conflict.conflicting_rule_ids = {rule1.id, rule2.id};
                conflict.description    = "Rules '" + rule1.name + "' and '" + rule2.name
                                          + "' have contradictory export permissions for overlapping resources/actions";
                conflict.severity       = "high";
                conflict.recommendation = "Review and align export permissions or narrow rule scope";
                conflicts.push_back(conflict);
            }

            if (rule1.require_encryption != rule2.require_encryption) {
                ConflictResult conflict;
                conflict.conflict_id          = "contradict_enc_" + rule1.id + "_" + rule2.id;
                conflict.conflict_type        = "contradictory";
                conflict.conflicting_rule_ids = {rule1.id, rule2.id};
                conflict.description
                    = "Rules '" + rule1.name + "' and '" + rule2.name + "' have contradictory encryption requirements";
                conflict.severity       = "critical";
                conflict.recommendation = "Enforce encryption requirement consistently";
                conflicts.push_back(conflict);
            }

            if (rule1.allow_cache != rule2.allow_cache) {
                ConflictResult conflict;
                conflict.conflict_id          = "contradict_cache_" + rule1.id + "_" + rule2.id;
                conflict.conflict_type        = "contradictory";
                conflict.conflicting_rule_ids = {rule1.id, rule2.id};
                conflict.description
                    = "Rules '" + rule1.name + "' and '" + rule2.name + "' have contradictory cache permissions";
                conflict.severity       = "medium";
                conflict.recommendation = "Align caching policies or use priority to resolve";
                conflicts.push_back(conflict);
            }
        }
    }

    THEMIS_INFO("Found {} contradictory rule conflicts",static_cast<int>(conflicts.size()));

    return conflicts;
}

std::vector<PolicyValidator::ConflictResult>
PolicyValidator::detectOverlappingPermissions(const PolicyManager &policy_mgr) const {
    std::vector<ConflictResult> conflicts;
    auto all_rules = policy_mgr.listRules();

    THEMIS_DEBUG("Checking for overlapping permissions among {} rules",static_cast<int>(all_rules.size()));

    // Group rules by resource/action patterns
    std::unordered_map<std::string, std::vector<std::string>> pattern_rules;

    for (const auto &rule : all_rules) {
        if (!rule.enabled) {
            continue;
        }

        for (const auto &resource : rule.resources) {
            for (const auto &action : rule.actions) {
                std::string key = resource + ":" + action;
                pattern_rules[key].push_back(rule.id);
            }
        }
    }

    // Identify patterns with multiple rules (potential overlaps)
    for (const auto &[pattern, rule_ids] : pattern_rules) {
        if (static_cast<int>(rule_ids.size()) > 1) { // Two or more rules covering the same resource/action
            ConflictResult conflict;
            conflict.conflict_id          = "overlap_" + pattern;
            conflict.conflict_type        = "overlapping";
            conflict.conflicting_rule_ids = rule_ids;

            size_t colon_pos     = pattern.find(':');
            std::string resource = pattern.substr(0, colon_pos);
            std::string action   = pattern.substr(colon_pos + 1);

            conflict.description    = "Multiple rules (" + std::to_string(rule_ids.size()) + ") apply to resource '"
                                      + resource + "' and action '" + action + "'";
            conflict.severity       = "medium";
            conflict.recommendation = "Consider merging overlapping rules or adjusting priorities";
            conflicts.push_back(conflict);
        }
    }

    THEMIS_INFO("Found {} overlapping permission conflicts",static_cast<int>(conflicts.size()));

    return conflicts;
}

std::vector<PolicyValidator::ConflictResult>
PolicyValidator::detectCircularDependencies(const PolicyManager &policy_mgr) const {
    std::vector<ConflictResult> conflicts;
    auto all_rules = policy_mgr.listRules();

    THEMIS_DEBUG("Checking for circular dependencies in {} rules",static_cast<int>(all_rules.size()));

    // Build an undirected conflict graph: an edge between rule A and rule B
    // exists when all three conditions hold:
    //   1. Both rules are enabled.
    //   2. They share the same priority (equal priority makes the conflict
    //      irresolvable by ordering alone, forming a "cycle" in evaluation).
    //   3. They overlap on at least one resource AND one action pattern.
    //   4. Their access-control effects contradict each other.
    //
    // A connected component of size >= 3 in this graph is reported as a
    // circular dependency because no deterministic evaluation order can
    // resolve all pairwise contradictions simultaneously.
    std::unordered_map<std::string, std::vector<std::string>> conflict_graph;

    auto overlaps_resource = [](const PolicyRule &r1, const PolicyRule &r2) {
        for (const auto &a : r1.resources) {
            for (const auto &b : r2.resources) {
                if (a == b || a == "*" || b == "*") {
                    return true;
                }
            }
        }
        return false;
    };

    auto overlaps_action = [](const PolicyRule &r1, const PolicyRule &r2) {
        for (const auto &a : r1.actions) {
            for (const auto &b : r2.actions) {
                if (a == b || a == "*" || b == "*") {
                    return true;
                }
            }
        }
        return false;
    };

    auto is_contradictory = [](const PolicyRule &r1, const PolicyRule &r2) {
        return (r1.require_encryption != r2.require_encryption) || (r1.allow_export != r2.allow_export)
               || (r1.allow_cache != r2.allow_cache);
    };

    for (std::size_t i = 0; i < all_rules.size(); ++i) {
        const auto &r1 = all_rules[i];
        if (!r1.enabled) {
            continue;
        }

        for (std::size_t j = i + 1; j < all_rules.size(); ++j) {
            const auto &r2 = all_rules[j];
            if (!r2.enabled) {
                continue;
            }

            if (r1.priority != r2.priority) {
                continue;
            }
            if (!overlaps_resource(r1, r2)) {
                continue;
            }
            if (!overlaps_action(r1, r2)) {
                continue;
            }
            if (!is_contradictory(r1, r2)) {
                continue;
            }

            conflict_graph[r1.id].push_back(r2.id);
            conflict_graph[r2.id].push_back(r1.id);
        }
    }

    // BFS to find connected components of size >= 3
    std::unordered_set<std::string> visited;

    for (const auto &kv : conflict_graph) {
        const std::string &start = kv.first;
        if (visited.count(start)) {
            continue;
        }

        std::vector<std::string> component;
        std::queue<std::string> q;
        q.push(start);
        visited.insert(start);

        while (!q.empty()) {
            std::string cur = q.front();
            q.pop();
            component.push_back(cur);

            for (const auto &nb : conflict_graph.at(cur)) {
                if (!visited.count(nb)) {
                    visited.insert(nb);
                    q.push(nb);
                }
            }
        }

        if (static_cast<int>(component.size()) >= 3) {
            ConflictResult conflict;
            conflict.conflict_id          = "circular_" + component[0];
            conflict.conflict_type        = "circular";
            conflict.conflicting_rule_ids = component;
            conflict.severity             = "high";
            conflict.description    = "Rules form an irresolvable priority cycle: " + std::to_string(component.size())
                                      + " rules share the same priority, overlapping resources/actions, "
                                        "and contradictory effects";
            conflict.recommendation = "Assign distinct priorities to break the evaluation cycle, "
                                      "narrow overlapping resource patterns, or merge contradictory rules";
            conflicts.push_back(std::move(conflict));
        }
    }

    THEMIS_INFO("Found {} circular dependency conflicts",static_cast<int>(conflicts.size()));

    return conflicts;
}

std::unordered_map<std::string, PolicyValidator::EffectivenessMetrics>
PolicyValidator::calculateEffectiveness(const PolicyManager &policy_mgr,
                                        const std::unordered_map<std::string, int> &hit_counts) const {
    std::unordered_map<std::string, EffectivenessMetrics> metrics_map;
    auto all_rules = policy_mgr.listRules();

    THEMIS_DEBUG("Calculating effectiveness for {} rules",static_cast<int>(all_rules.size()));

    int64_t now
        = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    for (const auto &rule : all_rules) {
        EffectivenessMetrics metrics;
        metrics.rule_id    = rule.id;
        metrics.created_at = rule.created_at;

        // Get hit count
        auto hit_it = hit_counts.find(rule.id);
        if (hit_it != hit_counts.end()) {
            metrics.hit_count = hit_it->second;
            metrics.last_used = now; // Assume recent if has hits
        } else {
            metrics.hit_count = 0;
            metrics.last_used = rule.created_at;
        }

        // Calculate days since last use
        if (metrics.last_used > 0) {
            int64_t seconds_since_use   = now - metrics.last_used;
            metrics.days_since_last_use = static_cast<int>(seconds_since_use / (24 * 3600));
        }

        // Determine if unused
        metrics.is_unused = (metrics.hit_count == 0);

        // Calculate effectiveness score (0-100)
        // Factors: hit count, recency, rule age
        double hit_score = std::min(100.0, static_cast<double>(metrics.hit_count) * 10.0);

        double recency_score = 100.0;
        if (metrics.days_since_last_use > 0) {
            recency_score = std::max(0.0, 100.0 - (metrics.days_since_last_use * 0.5));
        }

        double age_penalty = 0.0;
        int64_t age_days   = (now - rule.created_at) / (24 * 3600);
        if (age_days > 365) {
            age_penalty = std::min(20.0, (age_days - 365) * 0.05);
        }

        metrics.effectiveness_score = std::max(0.0, (hit_score * 0.5 + recency_score * 0.5) - age_penalty);

        metrics_map[rule.id] = metrics;
    }

    THEMIS_INFO("Calculated effectiveness metrics for {} rules",static_cast<int>(metrics_map.size()));

    return metrics_map;
}

std::vector<std::string> PolicyValidator::identifyUnusedRules(const PolicyManager &policy_mgr,
                                                              const std::unordered_map<std::string, int> &hit_counts,
                                                              int min_days_unused) const {
    std::vector<std::string> unused_rules;
    auto metrics = calculateEffectiveness(policy_mgr, hit_counts);

    THEMIS_DEBUG("Identifying unused rules (threshold: {} days)", min_days_unused);

    for (const auto &[rule_id, metric] : metrics) {
        if (metric.is_unused || metric.days_since_last_use >= min_days_unused) {
            unused_rules.push_back(rule_id);
        }
    }

    THEMIS_INFO("Identified {} unused rules",static_cast<int>(unused_rules.size()));

    return unused_rules;
}

std::vector<PolicyValidator::SecurityCheckResult>
PolicyValidator::performSecurityChecks(const PolicyManager &policy_mgr) const {
    THEMIS_DEBUG("Performing comprehensive security checks");

    std::vector<SecurityCheckResult> all_checks;

    // Check for overly permissive rules
    auto permissive = checkOverlyPermissive(policy_mgr);
    all_checks.insert(all_checks.end(), permissive.begin(), permissive.end());

    // Check encryption requirements
    auto encryption = checkEncryptionRequirements(policy_mgr);
    all_checks.insert(all_checks.end(), encryption.begin(), encryption.end());

    // Check audit logging
    auto audit = checkAuditLogging(policy_mgr);
    all_checks.insert(all_checks.end(), audit.begin(), audit.end());

    // Check retention compliance
    auto retention = checkRetentionCompliance(policy_mgr);
    all_checks.insert(all_checks.end(), retention.begin(), retention.end());

    THEMIS_INFO("Completed {} security checks",static_cast<int>(all_checks.size()));

    return all_checks;
}

std::vector<PolicyValidator::SecurityCheckResult>
PolicyValidator::checkOverlyPermissive(const PolicyManager &policy_mgr) const {
    std::vector<SecurityCheckResult> checks;
    auto all_rules = policy_mgr.listRules();

    THEMIS_DEBUG("Checking for overly permissive rules");

    for (const auto &rule : all_rules) {
        if (!rule.enabled) {
            continue;
        }

        SecurityCheckResult check;
        check.rule_id    = rule.id;
        check.check_type = "overly_permissive";
        check.passed     = true;

        // Check for wildcard resources with broad actions
        bool has_wildcard_resource = false;
        for (const auto &resource : rule.resources) {
            if (resource == "*") {
                has_wildcard_resource = true;
                break;
            }
        }

        bool has_wildcard_action = false;
        for (const auto &action : rule.actions) {
            if (action == "*") {
                has_wildcard_action = true;
                break;
            }
        }

        if (has_wildcard_resource && has_wildcard_action) {
            check.check_id       = "overly_permissive_" + rule.id;
            check.passed         = false;
            check.severity       = "high";
            check.description    = "Rule '" + rule.name + "' allows all actions on all resources";
            check.recommendation = "Narrow the scope to specific resources and actions";
            checks.push_back(check);
        } else if (has_wildcard_resource || has_wildcard_action) {
            // Check if combined with weak security
            if (!rule.require_encryption && !rule.audit_access) {
                check.check_id = "permissive_no_security_" + rule.id;
                check.passed   = false;
                check.severity = "medium";
                check.description
                    = "Rule '" + rule.name + "' is permissive (wildcards) but lacks encryption and audit controls";
                check.recommendation = "Add encryption requirements or audit logging";
                checks.push_back(check);
            }
        }

        // Check for export without restrictions
        if (rule.allow_export && !rule.require_encryption && !rule.audit_access) {
            check.check_id       = "export_no_controls_" + rule.id;
            check.passed         = false;
            check.severity       = "high";
            check.description    = "Rule '" + rule.name + "' allows export without encryption or audit controls";
            check.recommendation = "Require encryption and audit for data export";
            checks.push_back(check);
        }
    }

    THEMIS_INFO("Found {} overly permissive issues",static_cast<int>(checks.size()));

    return checks;
}

std::vector<PolicyValidator::SecurityCheckResult>
PolicyValidator::checkEncryptionRequirements(const PolicyManager &policy_mgr) const {
    std::vector<SecurityCheckResult> checks;
    auto all_rules = policy_mgr.listRules();

    THEMIS_DEBUG("Checking encryption requirements");

    for (const auto &rule : all_rules) {
        if (!rule.enabled) {
            continue;
        }

        // Check if sensitive classification lacks encryption
        bool is_sensitive = (rule.classification_level == "geheim" || rule.classification_level == "streng-geheim"
                             || rule.classification_level == "vs-nfd");

        if (is_sensitive && !rule.require_encryption) {
            SecurityCheckResult check;
            check.check_id       = "missing_encryption_" + rule.id;
            check.rule_id        = rule.id;
            check.check_type     = "missing_encryption";
            check.passed         = false;
            check.severity       = "critical";
            check.description    = "Rule '" + rule.name + "' handles '" + rule.classification_level
                                   + "' data but doesn't require encryption";
            check.recommendation = "Enable encryption requirement for sensitive data";
            checks.push_back(check);
        }

        // Check if export is allowed without encryption
        if (rule.allow_export && !rule.require_encryption) {
            SecurityCheckResult check;
            check.check_id       = "export_no_encryption_" + rule.id;
            check.rule_id        = rule.id;
            check.check_type     = "missing_encryption";
            check.passed         = false;
            check.severity       = "high";
            check.description    = "Rule '" + rule.name + "' allows export without encryption";
            check.recommendation = "Require encryption for data export";
            checks.push_back(check);
        }
    }

    THEMIS_INFO("Found {} encryption requirement issues",static_cast<int>(checks.size()));

    return checks;
}

std::vector<PolicyValidator::SecurityCheckResult>
PolicyValidator::checkAuditLogging(const PolicyManager &policy_mgr) const {
    std::vector<SecurityCheckResult> checks;
    auto all_rules = policy_mgr.listRules();

    THEMIS_DEBUG("Checking audit logging configuration");

    for (const auto &rule : all_rules) {
        if (!rule.enabled) {
            continue;
        }

        // Sensitive data should have audit logging
        bool is_sensitive = (rule.classification_level == "geheim" || rule.classification_level == "streng-geheim"
                             || rule.classification_level == "vs-nfd");

        if (is_sensitive && !rule.audit_access && !rule.audit_changes) {
            SecurityCheckResult check;
            check.check_id       = "missing_audit_" + rule.id;
            check.rule_id        = rule.id;
            check.check_type     = "missing_audit";
            check.passed         = false;
            check.severity       = "high";
            check.description    = "Rule '" + rule.name + "' handles '" + rule.classification_level
                                   + "' data but doesn't enable audit logging";
            check.recommendation = "Enable audit_access and audit_changes for sensitive data";
            checks.push_back(check);
        }

        // Export should be audited
        if (rule.allow_export && !rule.audit_access) {
            SecurityCheckResult check;
            check.check_id       = "export_no_audit_" + rule.id;
            check.rule_id        = rule.id;
            check.check_type     = "missing_audit";
            check.passed         = false;
            check.severity       = "medium";
            check.description    = "Rule '" + rule.name + "' allows export without audit logging";
            check.recommendation = "Enable audit_access to track data exports";
            checks.push_back(check);
        }
    }

    THEMIS_INFO("Found {} audit logging issues",static_cast<int>(checks.size()));

    return checks;
}

std::vector<PolicyValidator::SecurityCheckResult>
PolicyValidator::checkRetentionCompliance(const PolicyManager &policy_mgr, int min_retention_days) const {
    std::vector<SecurityCheckResult> checks;
    auto all_rules = policy_mgr.listRules();

    THEMIS_DEBUG("Checking retention compliance (min: {} days)", min_retention_days);

    for (const auto &rule : all_rules) {
        if (!rule.enabled) {
            continue;
        }

        if (rule.retention_days < min_retention_days) {
            SecurityCheckResult check;
            check.check_id    = "retention_too_short_" + rule.id;
            check.rule_id     = rule.id;
            check.check_type  = "retention_compliance";
            check.passed      = false;
            check.severity    = "medium";
            check.description = "Rule '" + rule.name + "' has retention period (" + std::to_string(rule.retention_days)
                                + " days) below minimum (" + std::to_string(min_retention_days) + " days)";
            check.recommendation = "Increase retention period to meet compliance requirements";
            checks.push_back(check);
        }

        // Check for excessively long retention
        if (rule.retention_days > 3650) { // > 10 years
            SecurityCheckResult check;
            check.check_id       = "retention_too_long_" + rule.id;
            check.rule_id        = rule.id;
            check.check_type     = "retention_compliance";
            check.passed         = false;
            check.severity       = "low";
            check.description    = "Rule '" + rule.name + "' has very long retention period ("
                                   + std::to_string(rule.retention_days) + " days)";
            check.recommendation = "Review if such long retention is necessary and compliant with GDPR";
            checks.push_back(check);
        }
    }

    THEMIS_INFO("Found {} retention compliance issues",static_cast<int>(checks.size()));

    return checks;
}

PolicyValidator::ValidationReport
PolicyValidator::generateValidationReport(const PolicyManager &policy_mgr,
                                          const std::unordered_map<std::string, int> &hit_counts) const {
    THEMIS_INFO("Generating comprehensive validation report");

    ValidationReport report;
    auto all_rules             = policy_mgr.listRules();
    report.total_rules_checked = static_cast<int>(all_rules.size());

    // Detect conflicts
    report.conflicts       = detectConflicts(policy_mgr);
    report.conflicts_found = static_cast<int>(report.conflicts.size());

    // Perform security checks
    report.security_checks = performSecurityChecks(policy_mgr);
    for (const auto &check : report.security_checks) {
        if (!check.passed) {
            report.security_issues_found++;
        }
    }

    // Calculate effectiveness
    auto unused                       = identifyUnusedRules(policy_mgr, hit_counts);
    report.effectiveness_issues_found = static_cast<int>(unused.size());

    // Generate recommendations
    if (!report.conflicts.empty()) {
        report.recommendations.push_back("Address " + std::to_string(report.conflicts_found) + " rule conflicts");
    }
    if (report.security_issues_found > 0) {
        report.recommendations.push_back("Fix " + std::to_string(report.security_issues_found) + " security issues");
    }
    if (report.effectiveness_issues_found > 0) {
        report.recommendations.push_back("Review " + std::to_string(report.effectiveness_issues_found)
                                         + " unused rules");
    }

    report.generated_at
        = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    THEMIS_INFO("Validation report complete: {} conflicts, {} security issues, {} effectiveness issues",
                report.conflicts_found, report.security_issues_found, report.effectiveness_issues_found);

    return report;
}

std::vector<PolicyValidator::SecurityCheckResult> PolicyValidator::validateSingleRule(const PolicyRule &rule) const {
    std::vector<SecurityCheckResult> checks;

    THEMIS_DEBUG("Validating single rule: {}", rule.id);

    // Check for overly permissive patterns
    bool has_wildcard_resource = false;
    for (const auto &resource : rule.resources) {
        if (resource == "*") {
            has_wildcard_resource = true;
            break;
        }
    }

    bool has_wildcard_action = false;
    for (const auto &action : rule.actions) {
        if (action == "*") {
            has_wildcard_action = true;
            break;
        }
    }

    if (has_wildcard_resource && has_wildcard_action) {
        SecurityCheckResult check;
        check.check_id       = "overly_permissive";
        check.rule_id        = rule.id;
        check.check_type     = "overly_permissive";
        check.passed         = false;
        check.severity       = "high";
        check.description    = "Rule allows all actions on all resources";
        check.recommendation = "Narrow scope to specific resources and actions";
        checks.push_back(check);
    }

    // Check encryption for sensitive data
    bool is_sensitive = (rule.classification_level == "geheim" || rule.classification_level == "streng-geheim"
                         || rule.classification_level == "vs-nfd");

    if (is_sensitive && !rule.require_encryption) {
        SecurityCheckResult check;
        check.check_id       = "missing_encryption";
        check.rule_id        = rule.id;
        check.check_type     = "missing_encryption";
        check.passed         = false;
        check.severity       = "critical";
        check.description    = "Sensitive data without encryption requirement";
        check.recommendation = "Enable require_encryption";
        checks.push_back(check);
    }

    // Check audit for sensitive operations
    if (is_sensitive && !rule.audit_access) {
        SecurityCheckResult check;
        check.check_id       = "missing_audit";
        check.rule_id        = rule.id;
        check.check_type     = "missing_audit";
        check.passed         = false;
        check.severity       = "high";
        check.description    = "Sensitive data access without audit logging";
        check.recommendation = "Enable audit_access";
        checks.push_back(check);
    }

    // Check export controls
    if (rule.allow_export && !rule.require_encryption) {
        SecurityCheckResult check;
        check.check_id       = "export_no_encryption";
        check.rule_id        = rule.id;
        check.check_type     = "missing_encryption";
        check.passed         = false;
        check.severity       = "high";
        check.description    = "Export allowed without encryption";
        check.recommendation = "Enable require_encryption for exports";
        checks.push_back(check);
    }

    return checks;
}

// ========== PolicyMetricsCollector::RuleMetrics Implementation ==========

nlohmann::json PolicyMetricsCollector::RuleMetrics::toJson() const {
    nlohmann::json j;
    j["rule_id"]                  = rule_id;
    j["evaluation_count"]         = evaluation_count;
    j["match_count"]              = match_count;
    j["total_evaluation_time_us"] = total_evaluation_time_us;
    j["avg_evaluation_time_us"]   = avg_evaluation_time_us;
    j["last_evaluation_time"]     = last_evaluation_time;
    j["match_rate"]               = match_rate;
    return j;
}

// ========== PolicyMetricsCollector::PerformanceImpact Implementation ==========

nlohmann::json PolicyMetricsCollector::PerformanceImpact::toJson() const {
    nlohmann::json j;
    j["rule_id"]                 = rule_id;
    j["avg_evaluation_time_us"]  = avg_evaluation_time_us;
    j["performance_category"]    = performance_category;
    j["impact_description"]      = impact_description;
    j["optimization_suggestion"] = optimization_suggestion;
    return j;
}

// ========== PolicyMetricsCollector Implementation ==========

void PolicyMetricsCollector::recordEvaluation(const std::string &rule_id, bool matched, int64_t evaluation_time_us) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto &metrics   = metrics_[rule_id];
    metrics.rule_id = rule_id;
    metrics.evaluation_count++;

    if (matched) {
        metrics.match_count++;
    }

    metrics.total_evaluation_time_us += evaluation_time_us;
    metrics.avg_evaluation_time_us = metrics.total_evaluation_time_us / metrics.evaluation_count;

    metrics.last_evaluation_time
        = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    if (metrics.evaluation_count > 0) {
        metrics.match_rate = (static_cast<double>(metrics.match_count) / metrics.evaluation_count) * 100.0;
    }
}

std::optional<PolicyMetricsCollector::RuleMetrics>
PolicyMetricsCollector::getRuleMetrics(const std::string &rule_id) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = metrics_.find(rule_id);
    if (it != metrics_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::unordered_map<std::string, PolicyMetricsCollector::RuleMetrics> PolicyMetricsCollector::getAllMetrics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return metrics_;
}

std::vector<PolicyMetricsCollector::PerformanceImpact> PolicyMetricsCollector::analyzePerformanceImpact() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<PerformanceImpact> impacts;

    for (const auto &[rule_id, metrics] : metrics_) {
        PerformanceImpact impact;
        impact.rule_id                = rule_id;
        impact.avg_evaluation_time_us = metrics.avg_evaluation_time_us;

        // Categorize performance
        if (metrics.avg_evaluation_time_us < 100) {
            impact.performance_category    = "fast";
            impact.impact_description      = "Excellent performance";
            impact.optimization_suggestion = "No optimization needed";
        } else if (metrics.avg_evaluation_time_us < 500) {
            impact.performance_category    = "normal";
            impact.impact_description      = "Acceptable performance";
            impact.optimization_suggestion = "Consider caching if evaluated frequently";
        } else if (metrics.avg_evaluation_time_us < 2000) {
            impact.performance_category    = "slow";
            impact.impact_description      = "Slow evaluation detected";
            impact.optimization_suggestion = "Review rule complexity and simplify conditions";
        } else {
            impact.performance_category    = "critical";
            impact.impact_description      = "Critical performance issue";
            impact.optimization_suggestion = "Immediate optimization required - simplify or split rule";
        }

        impacts.push_back(impact);
    }

    // Sort by avg time (slowest first)
    std::sort(impacts.begin(), impacts.end(), [](const PerformanceImpact &a, const PerformanceImpact &b) {
        return a.avg_evaluation_time_us > b.avg_evaluation_time_us;
    });

    return impacts;
}

std::vector<std::string> PolicyMetricsCollector::getSlowRules(int64_t threshold_us) const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<std::string> slow_rules;

    for (const auto &[rule_id, metrics] : metrics_) {
        if (metrics.avg_evaluation_time_us >= threshold_us) {
            slow_rules.push_back(rule_id);
        }
    }

    return slow_rules;
}

nlohmann::json PolicyMetricsCollector::exportMetrics() const {
    std::lock_guard<std::mutex> lock(mutex_);

    nlohmann::json j = nlohmann::json::array();

    for (const auto &[rule_id, metrics] : metrics_) {
        j.push_back(metrics.toJson());
    }

    return j;
}

bool PolicyMetricsCollector::importMetrics(const nlohmann::json &j) {
    if (!j.is_array()) {
        THEMIS_ERROR("Import metrics: expected array");
        return false;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    try {
        for (const auto &item : j) {
            RuleMetrics metrics = {};

            if (item.contains("rule_id")) {
                metrics.rule_id = item["rule_id"];
            }
            if (item.contains("evaluation_count")) {
                metrics.evaluation_count = item["evaluation_count"];
            }
            if (item.contains("match_count")) {
                metrics.match_count = item["match_count"];
            }
            if (item.contains("total_evaluation_time_us")) {
                metrics.total_evaluation_time_us = item["total_evaluation_time_us"];
            }
            if (item.contains("avg_evaluation_time_us")) {
                metrics.avg_evaluation_time_us = item["avg_evaluation_time_us"];
            }
            if (item.contains("last_evaluation_time")) {
                metrics.last_evaluation_time = item["last_evaluation_time"];
            }
            if (item.contains("match_rate")) {
                metrics.match_rate = item["match_rate"];
            }

            if (!metrics.rule_id.empty()) {
                metrics_[metrics.rule_id] = metrics;
            }
        }

        THEMIS_INFO("Imported metrics for {} rules",static_cast<int>(metrics_.size()));
        return true;
    } catch (const std::exception &e) {
        THEMIS_ERROR("Failed to import metrics: {}", e.what());
        return false;
    }
}

void PolicyMetricsCollector::resetMetrics() {
    std::lock_guard<std::mutex> lock(mutex_);
    metrics_.clear();
    THEMIS_INFO("All metrics reset");
}

void PolicyMetricsCollector::resetRuleMetrics(const std::string &rule_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    metrics_.erase(rule_id);
    THEMIS_DEBUG("Reset metrics for rule: {}", rule_id);
}

// ========== PolicyOptimizer::OptimizationRecommendation Implementation ==========

nlohmann::json PolicyOptimizer::OptimizationRecommendation::toJson() const {
    nlohmann::json j;
    j["recommendation_id"] = recommendation_id;
    j["rule_id"]           = rule_id;
    j["optimization_type"] = optimization_type;
    j["description"]       = description;
    j["rationale"]         = rationale;
    j["expected_benefit"]  = expected_benefit;
    j["priority"]          = priority;
    return j;
}

// ========== PolicyOptimizer::OptimizationReport Implementation ==========

nlohmann::json PolicyOptimizer::OptimizationReport::toJson() const {
    nlohmann::json j;
    j["total_recommendations"]         = total_recommendations;
    j["high_priority_recommendations"] = high_priority_recommendations;
    j["summary"]                       = summary;
    j["generated_at"]                  = generated_at;

    nlohmann::json recs_arr = nlohmann::json::array();
    for (const auto &rec : recommendations) {
        recs_arr.push_back(rec.toJson());
    }
    j["recommendations"] = recs_arr;

    return j;
}

// ========== PolicyOptimizer Implementation ==========

std::vector<PolicyOptimizer::OptimizationRecommendation> PolicyOptimizer::generateRecommendations(
    const PolicyManager &policy_mgr, const PolicyValidator::ValidationReport & /*validation_report*/,
    const std::unordered_map<std::string, PolicyMetricsCollector::RuleMetrics> &metrics) const {
    THEMIS_INFO("Generating optimization recommendations");

    std::vector<OptimizationRecommendation> all_recommendations;

    // Get merge recommendations
    auto merges = recommendMerges(policy_mgr);
    all_recommendations.insert(all_recommendations.end(), merges.begin(), merges.end());

    // Get simplification recommendations
    auto simplifications = recommendSimplifications(policy_mgr);
    all_recommendations.insert(all_recommendations.end(), simplifications.begin(), simplifications.end());

    // Get reordering recommendations
    auto reorderings = recommendReordering(policy_mgr, metrics);
    all_recommendations.insert(all_recommendations.end(), reorderings.begin(), reorderings.end());

    // Get removal recommendations (build hit counts from metrics)
    std::unordered_map<std::string, int> hit_counts = {};

    for (const auto &[rule_id, metric] : metrics) {
        hit_counts[rule_id] = metric.match_count;
    }
    auto removals = recommendRemovals(policy_mgr, hit_counts);
    all_recommendations.insert(all_recommendations.end(), removals.begin(), removals.end());

    THEMIS_INFO("Generated {} optimization recommendations",static_cast<int>(all_recommendations.size()));

    return all_recommendations;
}

std::vector<PolicyOptimizer::OptimizationRecommendation>
PolicyOptimizer::recommendMerges(const PolicyManager &policy_mgr) const {
    std::vector<OptimizationRecommendation> recommendations;
    auto all_rules = policy_mgr.listRules();

    THEMIS_DEBUG("Analyzing rules for merge opportunities");

    // Find rules with identical resources and actions
    std::unordered_map<std::string, std::vector<std::string>> similar_groups;

    for (const auto &rule : all_rules) {
        if (!rule.enabled) {
            continue;
        }

        // Create a signature based on resources and actions
        std::string signature = {};
        for (const auto &resource : rule.resources) {
            signature += resource + ";";
        }
        signature += "|";
        for (const auto &action : rule.actions) {
            signature += action + ";";
        }

        similar_groups[signature].push_back(rule.id);
    }

    // Recommend merging groups with multiple rules
    for (const auto &[signature, rule_ids] : similar_groups) {
        if (static_cast<int>(rule_ids.size()) > 1) {
            for (const auto &rule_id : rule_ids) {
                OptimizationRecommendation rec;
                rec.recommendation_id = "merge_" + rule_id;
                rec.rule_id           = rule_id;
                rec.optimization_type = "merge";
                rec.description = "Rule can be merged with " + std::to_string(static_cast<int>(rule_ids.size()) - 1) + " similar rule(s)";
                rec.rationale   = "Multiple rules with identical resource/action patterns";
                rec.expected_benefit = "Reduced complexity and improved maintainability";
                rec.priority         = 6;
                recommendations.push_back(rec);
            }
        }
    }

    THEMIS_INFO("Found {} merge recommendations",static_cast<int>(recommendations.size()));

    return recommendations;
}

std::vector<PolicyOptimizer::OptimizationRecommendation>
PolicyOptimizer::recommendSimplifications(const PolicyManager &policy_mgr) const {
    std::vector<OptimizationRecommendation> recommendations;
    auto all_rules = policy_mgr.listRules();

    THEMIS_DEBUG("Analyzing rules for simplification opportunities");

    for (const auto &rule : all_rules) {
        if (!rule.enabled) {
            continue;
        }

        // Check for overly complex rules
        bool is_complex = false;
        std::string complexity_reason = {};

        if (static_cast<int>(rule.resources.size()) > 10) {
            is_complex        = true;
            complexity_reason = "Too many resource patterns (" + std::to_string(rule.resources.size()) + ")";
        } else if (static_cast<int>(rule.actions.size()) > 10) {
            is_complex        = true;
            complexity_reason = "Too many action patterns (" + std::to_string(rule.actions.size()) + ")";
        } else if (static_cast<int>(rule.required_roles.size()) > 5) {
            is_complex        = true;
            complexity_reason = "Too many required roles (" + std::to_string(rule.required_roles.size()) + ")";
        }

        if (is_complex) {
            OptimizationRecommendation rec;
            rec.recommendation_id = "simplify_" + rule.id;
            rec.rule_id           = rule.id;
            rec.optimization_type = "simplify";
            rec.description       = "Rule complexity can be reduced";
            rec.rationale         = complexity_reason;
            rec.expected_benefit  = "Faster evaluation and easier maintenance";
            rec.priority          = 5;
            recommendations.push_back(rec);
        }
    }

    THEMIS_INFO("Found {} simplification recommendations",static_cast<int>(recommendations.size()));

    return recommendations;
}

std::vector<PolicyOptimizer::OptimizationRecommendation> PolicyOptimizer::recommendReordering(
    const PolicyManager &policy_mgr,
    const std::unordered_map<std::string, PolicyMetricsCollector::RuleMetrics> &metrics) const {
    std::vector<OptimizationRecommendation> recommendations;
    auto all_rules = policy_mgr.listRules();

    THEMIS_DEBUG("Analyzing rule ordering for optimization");

    // Build list of rules with their evaluation stats
    struct RuleStats {
        std::string rule_id = {};
        int priority = {};
        double match_rate = {};
        int64_t avg_time_us;
    };

    std::vector<RuleStats> rule_stats = {};

    for (const auto &rule : all_rules) {
        if (!rule.enabled) {
            continue;
        }

        RuleStats stats;
        stats.rule_id  = rule.id;
        stats.priority = rule.priority;

        auto metric_it = metrics.find(rule.id);
        if (metric_it != metrics.end()) {
            stats.match_rate  = metric_it->second.match_rate;
            stats.avg_time_us = metric_it->second.avg_evaluation_time_us;
        } else {
            stats.match_rate  = 0.0;
            stats.avg_time_us = 0;
        }

        rule_stats.push_back(stats);
    }

    // Find rules that should be reordered (high match rate but low priority)
    for (const auto &stats : rule_stats) {
        if (stats.match_rate > 50.0 && stats.priority < 5) {
            OptimizationRecommendation rec;
            rec.recommendation_id = "reorder_" + stats.rule_id;
            rec.rule_id           = stats.rule_id;
            rec.optimization_type = "reorder";
            rec.description       = "Rule has high match rate (" + std::to_string(static_cast<int>(stats.match_rate))
                                    + "%) but low priority";
            rec.rationale         = "Frequently matched rules should be evaluated earlier";
            rec.expected_benefit  = "Reduced average evaluation time";
            rec.priority          = 7;
            recommendations.push_back(rec);
        }
    }

    THEMIS_INFO("Found {} reordering recommendations",static_cast<int>(recommendations.size()));

    return recommendations;
}

std::vector<PolicyOptimizer::OptimizationRecommendation>
PolicyOptimizer::recommendRemovals(const PolicyManager &policy_mgr,
                                   const std::unordered_map<std::string, int> &hit_counts) const {
    std::vector<OptimizationRecommendation> recommendations;
    auto all_rules = policy_mgr.listRules();

    THEMIS_DEBUG("Analyzing rules for removal opportunities");

    int64_t now
        = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    for (const auto &rule : all_rules) {
        if (!rule.enabled) {
            continue;
        }

        auto hit_it = hit_counts.find(rule.id);
        int hits    = (hit_it != hit_counts.end()) ? hit_it->second : 0;

        // Calculate rule age
        int64_t age_days = (now - rule.created_at) / (24 * 3600);

        // Recommend removal if unused for long time
        if (hits == 0 && age_days > 90) {
            OptimizationRecommendation rec;
            rec.recommendation_id = "remove_" + rule.id;
            rec.rule_id           = rule.id;
            rec.optimization_type = "remove";
            rec.description       = "Rule has never been matched in " + std::to_string(age_days) + " days";
            rec.rationale         = "Unused rules add unnecessary overhead";
            rec.expected_benefit  = "Reduced policy evaluation time";
            rec.priority          = 8;
            recommendations.push_back(rec);
        } else if (hits < 5 && age_days > 180) {
            OptimizationRecommendation rec;
            rec.recommendation_id = "remove_" + rule.id;
            rec.rule_id           = rule.id;
            rec.optimization_type = "remove";
            rec.description
                = "Rule rarely matched (" + std::to_string(hits) + " times in " + std::to_string(age_days) + " days)";
            rec.rationale        = "Low-usage rules may be obsolete";
            rec.expected_benefit = "Simplified policy set";
            rec.priority         = 4;
            recommendations.push_back(rec);
        }
    }

    THEMIS_INFO("Found {} removal recommendations",static_cast<int>(recommendations.size()));

    return recommendations;
}

PolicyOptimizer::OptimizationReport PolicyOptimizer::generateOptimizationReport(
    const PolicyManager &policy_mgr, const PolicyValidator::ValidationReport &validation_report,
    const std::unordered_map<std::string, PolicyMetricsCollector::RuleMetrics> &metrics) const {
    THEMIS_INFO("Generating comprehensive optimization report");

    OptimizationReport report;

    report.recommendations       = generateRecommendations(policy_mgr, validation_report, metrics);
    report.total_recommendations = static_cast<int>(report.recommendations.size());

    // Count high priority recommendations (priority >= 7)
    for (const auto &rec : report.recommendations) {
        if (rec.priority >= 7) {
            report.high_priority_recommendations++;
        }
    }

    // Generate summary
    std::ostringstream summary = {};
    summary << "Found " << report.total_recommendations << " optimization opportunities. ";
    summary << report.high_priority_recommendations << " are high priority. ";

    // Count by type
    std::unordered_map<std::string, int> type_counts = {};

    for (const auto &rec : report.recommendations) {
        type_counts[rec.optimization_type]++;
    }

    for (const auto &[type, count] : type_counts) {
        summary << count << " " << type << ", ";
    }

    report.summary = summary.str();
    if (!report.summary.empty() && report.summary.back() == ' ') {
        report.summary.pop_back();
        report.summary.pop_back(); // Remove trailing ", "
    }

    report.generated_at
        = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    THEMIS_INFO("Optimization report complete: {} recommendations", report.total_recommendations);

    return report;
}

// ========== PolicyValidator CCPA/HIPAA Conflict Detection ==========

std::vector<PolicyValidator::ConflictResult>
PolicyValidator::detectCcpaHipaaConflicts(const PolicyManager &policy_mgr) const {
    std::vector<ConflictResult> conflicts;
    CcpaRuleSet ccpa_rules;
    auto all_rules = policy_mgr.listRules();

    THEMIS_DEBUG("Checking {} rules for CCPA/HIPAA cross-framework conflicts",static_cast<int>(all_rules.size()));

    for (const auto &rule : all_rules) {
        if (!rule.enabled) {
            continue;
        }

        auto rule_conflicts = ccpa_rules.detectHipaaConflicts(rule);
        for (const auto &desc : rule_conflicts) {
            ConflictResult result;
            result.conflict_id          = "ccpa_hipaa_" + rule.id;
            result.conflict_type        = "ccpa_hipaa";
            result.conflicting_rule_ids = {rule.id};
            result.description          = desc;
            result.severity             = "high";
            result.recommendation       = "Review rule '" + rule.id
                                          + "' to ensure both CCPA and HIPAA "
                                            "requirements are satisfied simultaneously. Enable audit_changes "
                                            "alongside audit_access, and verify that the retention period "
                                            "meets HIPAA's 6-year minimum while still permitting CCPA deletion.";
            conflicts.push_back(std::move(result));
        }
    }

    THEMIS_INFO("Found {} CCPA/HIPAA cross-framework conflicts",static_cast<int>(conflicts.size()));

    return conflicts;
}

// ========== PolicyValidator PCI-DSS/GDPR Conflict Detection ==========

std::vector<PolicyValidator::ConflictResult>
PolicyValidator::detectPciDssGdprConflicts(const PolicyManager &policy_mgr) const {
    std::vector<ConflictResult> conflicts;
    PciDssRuleSet pci_rules;
    auto all_rules = policy_mgr.listRules();

    THEMIS_DEBUG("Checking {} rules for PCI-DSS/GDPR cross-framework conflicts",static_cast<int>(all_rules.size()));

    for (const auto &rule : all_rules) {
        if (!rule.enabled) {
            continue;
        }

        auto rule_conflicts = pci_rules.detectGdprConflicts(rule);
        for (const auto &desc : rule_conflicts) {
            ConflictResult result;
            result.conflict_id          = "pci_dss_gdpr_" + rule.id;
            result.conflict_type        = "pci_dss_gdpr";
            result.conflicting_rule_ids = {rule.id};
            result.description          = desc;
            result.severity             = "high";
            result.recommendation       = "Review rule '" + rule.id
                                          + "' to ensure both PCI-DSS and GDPR "
                                            "requirements are satisfied simultaneously. Enable require_encryption "
                                            "for any rule that allows export, and ensure audit-log retention_days "
                                            "meets the PCI-DSS Req 10.7 minimum of 365 days.";
            conflicts.push_back(std::move(result));
        }
    }

    THEMIS_INFO("Found {} PCI-DSS/GDPR cross-framework conflicts",static_cast<int>(conflicts.size()));

    return conflicts;
}

} // namespace governance
} // namespace themis
