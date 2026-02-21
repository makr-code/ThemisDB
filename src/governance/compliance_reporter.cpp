/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            compliance_reporter.cpp                            ║
  Version:         0.0.8                                              ║
  Last Modified:   2026-02-21 12:09:01                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     459                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "governance/compliance_reporter.h"
#include "utils/logger.h"

#include <chrono>
#include <sstream>
#include <algorithm>

namespace themis {
namespace governance {

// ========== CoverageAnalysis Implementation ==========

nlohmann::json CoverageAnalysis::toJson() const {
    nlohmann::json j;
    j["total_resources_analyzed"] = total_resources_analyzed;
    j["resources_with_policies"] = resources_with_policies;
    j["resources_without_policies"] = resources_without_policies;
    j["coverage_percentage"] = coverage_percentage;
    j["uncovered_resources"] = uncovered_resources;
    j["overlapping_rules"] = overlapping_rules;
    return j;
}

// ========== ComplianceGap Implementation ==========

nlohmann::json ComplianceGap::toJson() const {
    nlohmann::json j;
    j["gap_type"] = gap_type;
    j["severity"] = severity;
    j["description"] = description;
    j["affected_resources"] = affected_resources;
    j["recommendations"] = recommendations;
    return j;
}

// ========== ComplianceReport Implementation ==========

nlohmann::json ComplianceReport::toJson() const {
    nlohmann::json j;
    j["report_id"] = report_id;
    j["generated_at"] = generated_at;
    j["report_type"] = report_type;
    j["total_rules"] = total_rules;
    j["active_rules"] = active_rules;
    j["inactive_rules"] = inactive_rules;
    
    nlohmann::json gaps_array = nlohmann::json::array();
    for (const auto& gap : gaps) {
        gaps_array.push_back(gap.toJson());
    }
    j["gaps"] = gaps_array;
    j["compliance_score"] = compliance_score;
    j["details"] = details;
    
    return j;
}

// ========== ComplianceReporter Implementation ==========

ComplianceReporter::ComplianceReporter(std::shared_ptr<PolicyManager> policy_manager)
    : policy_manager_(std::move(policy_manager))
{
    if (!policy_manager_) {
        THEMIS_WARN("ComplianceReporter created with null PolicyManager");
    }
}

CoverageAnalysis ComplianceReporter::analyzeCoverage(
    const std::vector<std::string>& resources
) const {
    CoverageAnalysis analysis;
    analysis.total_resources_analyzed = resources.size();
    
    auto rules = policy_manager_->listRules();
    
    for (const auto& resource : resources) {
        bool has_policy = false;
        
        for (const auto& rule : rules) {
            if (rule.appliesTo(resource, "*")) {
                has_policy = true;
                break;
            }
        }
        
        if (has_policy) {
            analysis.resources_with_policies++;
        } else {
            analysis.resources_without_policies++;
            analysis.uncovered_resources.push_back(resource);
        }
    }
    
    if (analysis.total_resources_analyzed > 0) {
        analysis.coverage_percentage = 
            (static_cast<double>(analysis.resources_with_policies) / 
             analysis.total_resources_analyzed) * 100.0;
    }
    
    // Detect overlapping rules
    auto overlaps = detectOverlappingRules();
    for (const auto& [rule1, rule2] : overlaps) {
        analysis.overlapping_rules.push_back(rule1 + " <-> " + rule2);
    }
    
    THEMIS_INFO("Coverage analysis: {:.1f}% ({}/{})", 
        analysis.coverage_percentage,
        analysis.resources_with_policies,
        analysis.total_resources_analyzed);
    
    return analysis;
}

std::vector<std::pair<std::string, std::string>> ComplianceReporter::detectOverlappingRules() const {
    std::vector<std::pair<std::string, std::string>> overlaps;
    
    auto rules = policy_manager_->listRules();
    
    // Check each pair of rules for overlap
    for (size_t i = 0; i < rules.size(); i++) {
        for (size_t j = i + 1; j < rules.size(); j++) {
            const auto& rule1 = rules[i];
            const auto& rule2 = rules[j];
            
            // Simple overlap check: same resources and actions
            bool resource_overlap = false;
            for (const auto& r1_res : rule1.resources) {
                for (const auto& r2_res : rule2.resources) {
                    if (r1_res == r2_res || r1_res == "*" || r2_res == "*") {
                        resource_overlap = true;
                        break;
                    }
                }
                if (resource_overlap) break;
            }
            
            if (resource_overlap) {
                bool action_overlap = false;
                for (const auto& r1_act : rule1.actions) {
                    for (const auto& r2_act : rule2.actions) {
                        if (r1_act == r2_act || r1_act == "*" || r2_act == "*") {
                            action_overlap = true;
                            break;
                        }
                    }
                    if (action_overlap) break;
                }
                
                if (action_overlap) {
                    overlaps.push_back({rule1.id, rule2.id});
                }
            }
        }
    }
    
    return overlaps;
}

std::vector<ComplianceGap> ComplianceReporter::detectGaps() const {
    std::vector<ComplianceGap> gaps;
    
    auto rules = policy_manager_->listRules();
    
    // Check for rules without encryption
    std::vector<std::string> no_encryption;
    std::vector<std::string> no_audit;
    std::vector<std::string> overly_permissive;
    
    for (const auto& rule : rules) {
        if (rule.enabled) {
            // Check encryption requirement
            if (!rule.require_encryption && rule.classification_level != "offen") {
                no_encryption.push_back(rule.id);
            }
            
            // Check audit requirements
            if (!rule.audit_access && !rule.audit_changes) {
                no_audit.push_back(rule.id);
            }
            
            // Check for overly permissive rules
            if (rule.allow_export && rule.allow_cache && 
                (rule.classification_level == "geheim" || rule.classification_level == "streng-geheim")) {
                overly_permissive.push_back(rule.id);
            }
        }
    }
    
    // Create gap entries
    if (!no_encryption.empty()) {
        ComplianceGap gap;
        gap.gap_type = "missing_encryption";
        gap.severity = "high";
        gap.description = "Rules without encryption requirements for sensitive data";
        gap.affected_resources = no_encryption;
        gap.recommendations = {
            "Enable 'require_encryption' for sensitive data rules",
            "Review classification levels and ensure appropriate encryption"
        };
        gaps.push_back(gap);
    }
    
    if (!no_audit.empty()) {
        ComplianceGap gap;
        gap.gap_type = "missing_audit";
        gap.severity = "medium";
        gap.description = "Rules without audit logging enabled";
        gap.affected_resources = no_audit;
        gap.recommendations = {
            "Enable 'audit_access' or 'audit_changes' for compliance",
            "Ensure audit trail for sensitive operations"
        };
        gaps.push_back(gap);
    }
    
    if (!overly_permissive.empty()) {
        ComplianceGap gap;
        gap.gap_type = "overly_permissive";
        gap.severity = "critical";
        gap.description = "Highly classified data with permissive export/cache settings";
        gap.affected_resources = overly_permissive;
        gap.recommendations = {
            "Disable 'allow_export' for classified data",
            "Disable 'allow_cache' for highly sensitive information",
            "Review and tighten access controls"
        };
        gaps.push_back(gap);
    }
    
    return gaps;
}

ComplianceReport ComplianceReporter::generateSummaryReport() const {
    ComplianceReport report;
    report.report_id = "summary_" + std::to_string(
        std::chrono::system_clock::now().time_since_epoch().count()
    );
    report.generated_at = std::chrono::system_clock::now().time_since_epoch().count() / 1000000000;
    report.report_type = "summary";
    
    auto stats = policy_manager_->getStats();
    report.total_rules = stats.total_rules;
    report.active_rules = stats.enabled_rules;
    report.inactive_rules = stats.disabled_rules;
    
    report.gaps = detectGaps();
    report.compliance_score = calculateComplianceScore(report.gaps);
    
    // Add classification breakdown
    report.details["rules_by_classification"] = nlohmann::json::object();
    for (const auto& [level, count] : stats.rules_by_classification) {
        report.details["rules_by_classification"][level] = count;
    }
    
    return report;
}

ComplianceReport ComplianceReporter::generateComplianceReport(const std::string& framework) const {
    ComplianceReport report;
    report.report_id = "compliance_" + framework + "_" + std::to_string(
        std::chrono::system_clock::now().time_since_epoch().count()
    );
    report.generated_at = std::chrono::system_clock::now().time_since_epoch().count() / 1000000000;
    report.report_type = "compliance";
    
    auto stats = policy_manager_->getStats();
    report.total_rules = stats.total_rules;
    report.active_rules = stats.enabled_rules;
    report.inactive_rules = stats.disabled_rules;
    
    report.gaps = detectGaps();
    report.compliance_score = calculateComplianceScore(report.gaps);
    
    // Framework-specific analysis
    if (!framework.empty()) {
        report.details["framework"] = framework;
        report.details["framework_requirements"] = nlohmann::json::array();
        
        // Add framework-specific requirements
        if (framework == "GDPR") {
            report.details["framework_requirements"].push_back("Data encryption at rest and in transit");
            report.details["framework_requirements"].push_back("Audit trail for data access");
            report.details["framework_requirements"].push_back("Data retention policies");
        } else if (framework == "SOX") {
            report.details["framework_requirements"].push_back("Access control and segregation of duties");
            report.details["framework_requirements"].push_back("Audit logging of all changes");
            report.details["framework_requirements"].push_back("Regular policy reviews");
        }
    }
    
    return report;
}

nlohmann::json ComplianceReporter::generateAccessControlMatrix() const {
    nlohmann::json matrix = nlohmann::json::object();
    
    auto rules = policy_manager_->listRules();
    
    // Build matrix: resource -> action -> required_roles
    for (const auto& rule : rules) {
        if (!rule.enabled) continue;
        
        for (const auto& resource : rule.resources) {
            if (!matrix.contains(resource)) {
                matrix[resource] = nlohmann::json::object();
            }
            
            for (const auto& action : rule.actions) {
                if (!matrix[resource].contains(action)) {
                    matrix[resource][action] = nlohmann::json::array();
                }
                
                for (const auto& role : rule.required_roles) {
                    // Add role if not already in list
                    bool found = false;
                    for (const auto& existing : matrix[resource][action]) {
                        if (existing.get<std::string>() == role) {
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        matrix[resource][action].push_back(role);
                    }
                }
            }
        }
    }
    
    return matrix;
}

ComplianceReport ComplianceReporter::generateRiskAssessmentReport() const {
    ComplianceReport report;
    report.report_id = "risk_" + std::to_string(
        std::chrono::system_clock::now().time_since_epoch().count()
    );
    report.generated_at = std::chrono::system_clock::now().time_since_epoch().count() / 1000000000;
    report.report_type = "risk_assessment";
    
    auto stats = policy_manager_->getStats();
    report.total_rules = stats.total_rules;
    report.active_rules = stats.enabled_rules;
    report.inactive_rules = stats.disabled_rules;
    
    report.gaps = detectGaps();
    report.compliance_score = calculateComplianceScore(report.gaps);
    
    // Calculate risk metrics
    int critical_gaps = 0, high_gaps = 0, medium_gaps = 0, low_gaps = 0;
    for (const auto& gap : report.gaps) {
        if (gap.severity == "critical") critical_gaps++;
        else if (gap.severity == "high") high_gaps++;
        else if (gap.severity == "medium") medium_gaps++;
        else if (gap.severity == "low") low_gaps++;
    }
    
    report.details["risk_summary"] = {
        {"critical_gaps", critical_gaps},
        {"high_gaps", high_gaps},
        {"medium_gaps", medium_gaps},
        {"low_gaps", low_gaps},
        {"overall_risk_level", critical_gaps > 0 ? "critical" : (high_gaps > 0 ? "high" : "medium")}
    };
    
    return report;
}

std::string ComplianceReporter::exportReport(
    const ComplianceReport& report,
    const std::string& format
) const {
    if (format == "json") {
        return report.toJson().dump(2);
    } else if (format == "csv") {
        return reportToCSV(report);
    }
    
    return report.toJson().dump(2); // Default to JSON
}

double ComplianceReporter::calculateComplianceScore(const std::vector<ComplianceGap>& gaps) const {
    if (gaps.empty()) {
        return 100.0;
    }
    
    // Deduct points based on severity
    double score = 100.0;
    for (const auto& gap : gaps) {
        if (gap.severity == "critical") score -= 20.0;
        else if (gap.severity == "high") score -= 15.0;
        else if (gap.severity == "medium") score -= 10.0;
        else if (gap.severity == "low") score -= 5.0;
    }
    
    return std::max(0.0, score);
}

std::string ComplianceReporter::reportToCSV(const ComplianceReport& report) const {
    std::ostringstream csv;
    
    // Header
    csv << "Report ID,Generated At,Type,Total Rules,Active Rules,Compliance Score\n";
    
    // Data
    csv << report.report_id << ","
        << report.generated_at << ","
        << report.report_type << ","
        << report.total_rules << ","
        << report.active_rules << ","
        << report.compliance_score << "\n";
    
    // Gaps section
    csv << "\nGap Type,Severity,Description,Affected Count\n";
    for (const auto& gap : report.gaps) {
        csv << gap.gap_type << ","
            << gap.severity << ","
            << "\"" << gap.description << "\","
            << gap.affected_resources.size() << "\n";
    }
    
    return csv.str();
}

bool ComplianceReporter::hasRequiredControls(const PolicyRule& rule) const {
    // Check if rule has minimum required controls
    return rule.require_encryption && 
           (rule.audit_access || rule.audit_changes) &&
           !rule.required_roles.empty();
}

} // namespace governance
} // namespace themis
