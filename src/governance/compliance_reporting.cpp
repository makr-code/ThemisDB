/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            compliance_reporting.cpp                           ║
  Version:         0.0.18                                             ║
  Last Modified:   2026-02-21 18:44:08                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     1093                                           ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "governance/compliance_reporting.h"
#include "utils/logger.h"

#include <algorithm>
#include <fstream>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <cmath>

namespace themis {
namespace governance {

// ========== PolicyCoverageAnalyzer::CoverageResult Implementation ==========

nlohmann::json PolicyCoverageAnalyzer::CoverageResult::toJson() const {
    nlohmann::json j;
    j["total_resources_checked"] = total_resources_checked;
    j["covered_resources"] = covered_resources;
    j["uncovered_resources"] = uncovered_resources;
    j["coverage_percentage"] = coverage_percentage;
    j["uncovered_resource_list"] = uncovered_resource_list;
    j["coverage_by_action"] = coverage_by_action;
    return j;
}

// ========== PolicyCoverageAnalyzer::OverlapResult Implementation ==========

nlohmann::json PolicyCoverageAnalyzer::OverlapResult::toJson() const {
    nlohmann::json j;
    j["resource_pattern"] = resource_pattern;
    j["action_pattern"] = action_pattern;
    j["overlapping_rule_ids"] = overlapping_rule_ids;
    j["overlap_count"] = overlap_count;
    return j;
}

// ========== PolicyCoverageAnalyzer Implementation ==========

PolicyCoverageAnalyzer::CoverageResult PolicyCoverageAnalyzer::analyzeCoverage(
    const PolicyManager& policy_mgr,
    const std::vector<std::string>& resources,
    const std::vector<std::string>& actions
) const {
    CoverageResult result;
    result.total_resources_checked = resources.size();
    
    THEMIS_DEBUG("Analyzing coverage for {} resources across {} actions", 
                 resources.size(), actions.size());
    
    for (const auto& resource : resources) {
        bool is_covered = false;
        
        for (const auto& action : actions) {
            // Check if any rule applies to this resource/action combination
            auto applicable_rules = policy_mgr.findApplicableRules(resource, action, {});
            
            if (!applicable_rules.empty()) {
                is_covered = true;
                result.coverage_by_action[action]++;
            }
        }
        
        if (is_covered) {
            result.covered_resources++;
        } else {
            result.uncovered_resources++;
            result.uncovered_resource_list.push_back(resource);
        }
    }
    
    if (result.total_resources_checked > 0) {
        result.coverage_percentage = (static_cast<double>(result.covered_resources) / 
                                     result.total_resources_checked) * 100.0;
    }
    
    THEMIS_INFO("Coverage analysis complete: {:.2f}% ({}/{})", 
                result.coverage_percentage, result.covered_resources, result.total_resources_checked);
    
    return result;
}

std::vector<PolicyCoverageAnalyzer::OverlapResult> PolicyCoverageAnalyzer::detectOverlaps(
    const PolicyManager& policy_mgr
) const {
    std::vector<OverlapResult> overlaps;
    auto all_rules = policy_mgr.listRules();
    
    THEMIS_DEBUG("Detecting overlaps among {} policy rules", all_rules.size());
    
    // Group rules by resource and action patterns
    std::unordered_map<std::string, std::vector<std::string>> pattern_map;
    
    for (const auto& rule : all_rules) {
        if (!rule.enabled) continue;
        
        for (const auto& resource : rule.resources) {
            for (const auto& action : rule.actions) {
                std::string key = resource + ":" + action;
                pattern_map[key].push_back(rule.id);
            }
        }
    }
    
    // Identify patterns with multiple rules
    for (const auto& [pattern, rule_ids] : pattern_map) {
        if (rule_ids.size() > 1) {
            OverlapResult overlap;
            
            size_t colon_pos = pattern.find(':');
            overlap.resource_pattern = pattern.substr(0, colon_pos);
            overlap.action_pattern = pattern.substr(colon_pos + 1);
            overlap.overlapping_rule_ids = rule_ids;
            overlap.overlap_count = rule_ids.size();
            
            overlaps.push_back(overlap);
        }
    }
    
    THEMIS_INFO("Detected {} overlapping patterns", overlaps.size());
    
    return overlaps;
}

std::vector<std::string> PolicyCoverageAnalyzer::findGaps(
    const PolicyManager& policy_mgr,
    const std::vector<std::string>& expected_resources
) const {
    std::vector<std::string> gaps;
    
    THEMIS_DEBUG("Finding gaps for {} expected resources", expected_resources.size());
    
    for (const auto& resource : expected_resources) {
        // Check if any enabled rule covers this resource
        auto applicable_rules = policy_mgr.findApplicableRules(resource, "*", {});
        
        if (applicable_rules.empty()) {
            gaps.push_back(resource);
        }
    }
    
    THEMIS_INFO("Found {} resource gaps", gaps.size());
    
    return gaps;
}

// ========== ComplianceGapDetector::ComplianceRequirement Implementation ==========

nlohmann::json ComplianceGapDetector::ComplianceRequirement::toJson() const {
    nlohmann::json j;
    j["id"] = id;
    j["name"] = name;
    j["framework"] = framework;
    j["description"] = description;
    j["required_resources"] = required_resources;
    j["requires_encryption"] = requires_encryption;
    j["requires_signature"] = requires_signature;
    j["requires_audit"] = requires_audit;
    j["min_retention_days"] = min_retention_days;
    return j;
}

ComplianceGapDetector::ComplianceRequirement 
ComplianceGapDetector::ComplianceRequirement::fromJson(const nlohmann::json& j) {
    ComplianceRequirement req;
    if (j.contains("id")) req.id = j["id"].get<std::string>();
    if (j.contains("name")) req.name = j["name"].get<std::string>();
    if (j.contains("framework")) req.framework = j["framework"].get<std::string>();
    if (j.contains("description")) req.description = j["description"].get<std::string>();
    if (j.contains("required_resources")) req.required_resources = j["required_resources"].get<std::vector<std::string>>();
    if (j.contains("requires_encryption")) req.requires_encryption = j["requires_encryption"].get<bool>();
    if (j.contains("requires_signature")) req.requires_signature = j["requires_signature"].get<bool>();
    if (j.contains("requires_audit")) req.requires_audit = j["requires_audit"].get<bool>();
    if (j.contains("min_retention_days")) req.min_retention_days = j["min_retention_days"].get<int>();
    return req;
}

// ========== ComplianceGapDetector::ComplianceGap Implementation ==========

nlohmann::json ComplianceGapDetector::ComplianceGap::toJson() const {
    nlohmann::json j;
    j["requirement_id"] = requirement_id;
    j["requirement_name"] = requirement_name;
    j["gap_type"] = gap_type;
    j["description"] = description;
    j["affected_resources"] = affected_resources;
    j["recommendation"] = recommendation;
    return j;
}

// ========== ComplianceGapDetector::ComplianceStatus Implementation ==========

nlohmann::json ComplianceGapDetector::ComplianceStatus::toJson() const {
    nlohmann::json j;
    j["framework"] = framework;
    j["total_requirements"] = total_requirements;
    j["met_requirements"] = met_requirements;
    j["unmet_requirements"] = unmet_requirements;
    j["compliance_percentage"] = compliance_percentage;
    
    nlohmann::json gaps_json = nlohmann::json::array();
    for (const auto& gap : gaps) {
        gaps_json.push_back(gap.toJson());
    }
    j["gaps"] = gaps_json;
    
    return j;
}

// ========== ComplianceGapDetector Implementation ==========

void ComplianceGapDetector::addRequirement(const ComplianceRequirement& req) {
    std::lock_guard<std::mutex> lock(mutex_);
    requirements_.push_back(req);
    THEMIS_DEBUG("Added compliance requirement: {} ({})", req.id, req.name);
}

std::vector<ComplianceGapDetector::ComplianceGap> 
ComplianceGapDetector::detectGaps(const PolicyManager& policy_mgr) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<ComplianceGap> gaps;
    
    THEMIS_DEBUG("Detecting compliance gaps for {} requirements", requirements_.size());
    
    for (const auto& req : requirements_) {
        if (!checkRequirement(req, policy_mgr)) {
            // Analyze specific gaps
            for (const auto& resource : req.required_resources) {
                auto applicable_rules = policy_mgr.findApplicableRules(resource, "*", {});
                
                if (applicable_rules.empty()) {
                    ComplianceGap gap;
                    gap.requirement_id = req.id;
                    gap.requirement_name = req.name;
                    gap.gap_type = "missing_policy";
                    gap.description = "No policy covers required resource: " + resource;
                    gap.affected_resources.push_back(resource);
                    gap.recommendation = "Create a policy rule for resource: " + resource;
                    gaps.push_back(gap);
                } else {
                    // Check for missing controls
                    bool has_all_controls = true;
                    std::vector<std::string> missing_controls;
                    
                    for (const auto& rule : applicable_rules) {
                        if (req.requires_encryption && !rule.require_encryption) {
                            has_all_controls = false;
                            missing_controls.push_back("encryption");
                        }
                        if (req.requires_signature && !rule.require_signature) {
                            has_all_controls = false;
                            missing_controls.push_back("signature");
                        }
                        if (req.requires_audit && !rule.audit_access && !rule.audit_changes) {
                            has_all_controls = false;
                            missing_controls.push_back("audit");
                        }
                        if (req.min_retention_days > 0 && rule.retention_days < req.min_retention_days) {
                            has_all_controls = false;
                            missing_controls.push_back("retention");
                        }
                    }
                    
                    if (!has_all_controls && !missing_controls.empty()) {
                        ComplianceGap gap;
                        gap.requirement_id = req.id;
                        gap.requirement_name = req.name;
                        gap.gap_type = "missing_control";
                        gap.description = "Policy exists but missing controls: " + 
                                        [&]() {
                                            std::string s;
                                            for (size_t i = 0; i < missing_controls.size(); i++) {
                                                if (i > 0) s += ", ";
                                                s += missing_controls[i];
                                            }
                                            return s;
                                        }();
                        gap.affected_resources.push_back(resource);
                        gap.recommendation = "Update policy to include required controls";
                        gaps.push_back(gap);
                    }
                }
            }
        }
    }
    
    THEMIS_INFO("Detected {} compliance gaps", gaps.size());
    
    return gaps;
}

ComplianceGapDetector::ComplianceStatus ComplianceGapDetector::getComplianceStatus(
    const PolicyManager& policy_mgr,
    const std::string& framework
) const {
    std::lock_guard<std::mutex> lock(mutex_);
    ComplianceStatus status;
    status.framework = framework.empty() ? "all" : framework;
    
    // Filter requirements by framework
    std::vector<ComplianceRequirement> filtered_reqs;
    for (const auto& req : requirements_) {
        if (framework.empty() || req.framework == framework) {
            filtered_reqs.push_back(req);
        }
    }
    
    status.total_requirements = filtered_reqs.size();
    
    for (const auto& req : filtered_reqs) {
        if (checkRequirement(req, policy_mgr)) {
            status.met_requirements++;
        } else {
            status.unmet_requirements++;
        }
    }
    
    if (status.total_requirements > 0) {
        status.compliance_percentage = (static_cast<double>(status.met_requirements) / 
                                       status.total_requirements) * 100.0;
    }
    
    // Get detailed gaps
    status.gaps = detectGaps(policy_mgr);
    
    THEMIS_INFO("Compliance status for {}: {:.2f}% ({}/{})", 
                status.framework, status.compliance_percentage, 
                status.met_requirements, status.total_requirements);
    
    return status;
}

bool ComplianceGapDetector::loadRequirements(const std::string& path) {
    try {
        std::ifstream file(path);
        if (!file.is_open()) {
            THEMIS_ERROR("Failed to open requirements file: {}", path);
            return false;
        }
        
        nlohmann::json j;
        file >> j;
        
        std::lock_guard<std::mutex> lock(mutex_);
        requirements_.clear();
        
        if (j.contains("requirements") && j["requirements"].is_array()) {
            for (const auto& req_json : j["requirements"]) {
                requirements_.push_back(ComplianceRequirement::fromJson(req_json));
            }
        }
        
        THEMIS_INFO("Loaded {} compliance requirements from {}", requirements_.size(), path);
        return true;
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Failed to load requirements from {}: {}", path, e.what());
        return false;
    }
}

nlohmann::json ComplianceGapDetector::exportRequirements() const {
    std::lock_guard<std::mutex> lock(mutex_);
    nlohmann::json j;
    
    nlohmann::json reqs_json = nlohmann::json::array();
    for (const auto& req : requirements_) {
        reqs_json.push_back(req.toJson());
    }
    j["requirements"] = reqs_json;
    
    return j;
}

bool ComplianceGapDetector::checkRequirement(
    const ComplianceRequirement& req,
    const PolicyManager& policy_mgr
) const {
    // Check if all required resources have appropriate policies
    for (const auto& resource : req.required_resources) {
        auto applicable_rules = policy_mgr.findApplicableRules(resource, "*", {});
        
        if (applicable_rules.empty()) {
            return false; // No policy for this resource
        }
        
        // Check if policies meet the requirements
        bool meets_encryption = !req.requires_encryption;
        bool meets_signature = !req.requires_signature;
        bool meets_audit = !req.requires_audit;
        bool meets_retention = true;
        
        for (const auto& rule : applicable_rules) {
            if (req.requires_encryption && rule.require_encryption) {
                meets_encryption = true;
            }
            if (req.requires_signature && rule.require_signature) {
                meets_signature = true;
            }
            if (req.requires_audit && (rule.audit_access || rule.audit_changes)) {
                meets_audit = true;
            }
            if (req.min_retention_days > 0 && rule.retention_days < req.min_retention_days) {
                meets_retention = false;
            }
        }
        
        if (!meets_encryption || !meets_signature || !meets_audit || !meets_retention) {
            return false;
        }
    }
    
    return true;
}

// ========== ComplianceReporter::PolicySummaryReport Implementation ==========

nlohmann::json ComplianceReporter::PolicySummaryReport::toJson() const {
    nlohmann::json j;
    j["total_rules"] = total_rules;
    j["enabled_rules"] = enabled_rules;
    j["disabled_rules"] = disabled_rules;
    j["rules_by_classification"] = rules_by_classification;
    j["rules_requiring_encryption"] = rules_requiring_encryption;
    j["rules_with_audit"] = rules_with_audit;
    j["generated_at"] = generated_at;
    return j;
}

std::string ComplianceReporter::PolicySummaryReport::toCSV() const {
    std::ostringstream csv;
    csv << "Metric,Value\n";
    csv << "Total Rules," << total_rules << "\n";
    csv << "Enabled Rules," << enabled_rules << "\n";
    csv << "Disabled Rules," << disabled_rules << "\n";
    csv << "\nClassification,Count\n";
    for (const auto& [classification, count] : rules_by_classification) {
        csv << classification << "," << count << "\n";
    }
    csv << "\nEncryption Required,Count\n";
    for (const auto& [key, count] : rules_requiring_encryption) {
        csv << key << "," << count << "\n";
    }
    csv << "\nAudit Enabled,Count\n";
    for (const auto& [key, count] : rules_with_audit) {
        csv << key << "," << count << "\n";
    }
    return csv.str();
}

std::string ComplianceReporter::PolicySummaryReport::toHTML() const {
    std::ostringstream html;
    html << "<html><head><title>Policy Summary Report</title>";
    html << "<style>body{font-family:Arial,sans-serif;margin:20px;}";
    html << "table{border-collapse:collapse;width:100%;margin-top:20px;}";
    html << "th,td{border:1px solid #ddd;padding:8px;text-align:left;}";
    html << "th{background-color:#4CAF50;color:white;}</style></head><body>";
    html << "<h1>Policy Summary Report</h1>";
    html << "<p>Generated: " << std::put_time(std::localtime(&generated_at), "%Y-%m-%d %H:%M:%S") << "</p>";
    
    html << "<h2>Overview</h2><table><tr><th>Metric</th><th>Value</th></tr>";
    html << "<tr><td>Total Rules</td><td>" << total_rules << "</td></tr>";
    html << "<tr><td>Enabled Rules</td><td>" << enabled_rules << "</td></tr>";
    html << "<tr><td>Disabled Rules</td><td>" << disabled_rules << "</td></tr>";
    html << "</table>";
    
    html << "<h2>Rules by Classification</h2><table><tr><th>Classification</th><th>Count</th></tr>";
    for (const auto& [classification, count] : rules_by_classification) {
        html << "<tr><td>" << classification << "</td><td>" << count << "</td></tr>";
    }
    html << "</table>";
    
    html << "</body></html>";
    return html.str();
}

// ========== ComplianceReporter::ComplianceStatusReport Implementation ==========

nlohmann::json ComplianceReporter::ComplianceStatusReport::toJson() const {
    nlohmann::json j;
    j["framework"] = framework;
    j["overall_compliance"] = overall_compliance;
    j["compliant_controls"] = compliant_controls;
    j["non_compliant_controls"] = non_compliant_controls;
    
    nlohmann::json gaps_json = nlohmann::json::array();
    for (const auto& gap : gaps) {
        gaps_json.push_back(gap.toJson());
    }
    j["gaps"] = gaps_json;
    j["generated_at"] = generated_at;
    
    return j;
}

std::string ComplianceReporter::ComplianceStatusReport::toCSV() const {
    std::ostringstream csv;
    csv << "Framework," << framework << "\n";
    csv << "Overall Compliance," << overall_compliance << "%\n\n";
    csv << "Control Type,Control Name\n";
    for (const auto& control : compliant_controls) {
        csv << "Compliant," << control << "\n";
    }
    for (const auto& control : non_compliant_controls) {
        csv << "Non-Compliant," << control << "\n";
    }
    return csv.str();
}

std::string ComplianceReporter::ComplianceStatusReport::toHTML() const {
    std::ostringstream html;
    html << "<html><head><title>Compliance Status Report</title>";
    html << "<style>body{font-family:Arial,sans-serif;margin:20px;}";
    html << "table{border-collapse:collapse;width:100%;margin-top:20px;}";
    html << "th,td{border:1px solid #ddd;padding:8px;text-align:left;}";
    html << "th{background-color:#4CAF50;color:white;}";
    html << ".compliant{color:green;}.non-compliant{color:red;}</style></head><body>";
    html << "<h1>Compliance Status Report - " << framework << "</h1>";
    html << "<p>Overall Compliance: <strong>" << std::fixed << std::setprecision(2) 
         << overall_compliance << "%</strong></p>";
    
    html << "<h2>Compliant Controls (" << compliant_controls.size() << ")</h2><ul>";
    for (const auto& control : compliant_controls) {
        html << "<li class='compliant'>" << control << "</li>";
    }
    html << "</ul>";
    
    html << "<h2>Non-Compliant Controls (" << non_compliant_controls.size() << ")</h2><ul>";
    for (const auto& control : non_compliant_controls) {
        html << "<li class='non-compliant'>" << control << "</li>";
    }
    html << "</ul>";
    
    html << "</body></html>";
    return html.str();
}

// ========== ComplianceReporter::AccessControlMatrix Implementation ==========

nlohmann::json ComplianceReporter::AccessControlMatrix::toJson() const {
    nlohmann::json j;
    nlohmann::json entries_json = nlohmann::json::array();
    
    for (const auto& entry : entries) {
        nlohmann::json entry_json;
        entry_json["role"] = entry.role;
        entry_json["resource"] = entry.resource;
        entry_json["allowed_actions"] = entry.allowed_actions;
        entry_json["requires_encryption"] = entry.requires_encryption;
        entry_json["is_audited"] = entry.is_audited;
        entries_json.push_back(entry_json);
    }
    
    j["entries"] = entries_json;
    j["generated_at"] = generated_at;
    return j;
}

std::string ComplianceReporter::AccessControlMatrix::toCSV() const {
    std::ostringstream csv;
    csv << "Role,Resource,Allowed Actions,Requires Encryption,Is Audited\n";
    
    for (const auto& entry : entries) {
        csv << entry.role << ",";
        csv << entry.resource << ",";
        csv << "\"";
        for (size_t i = 0; i < entry.allowed_actions.size(); i++) {
            if (i > 0) csv << ";";
            csv << entry.allowed_actions[i];
        }
        csv << "\",";
        csv << (entry.requires_encryption ? "Yes" : "No") << ",";
        csv << (entry.is_audited ? "Yes" : "No") << "\n";
    }
    
    return csv.str();
}

std::string ComplianceReporter::AccessControlMatrix::toHTML() const {
    std::ostringstream html;
    html << "<html><head><title>Access Control Matrix</title>";
    html << "<style>body{font-family:Arial,sans-serif;margin:20px;}";
    html << "table{border-collapse:collapse;width:100%;margin-top:20px;}";
    html << "th,td{border:1px solid #ddd;padding:8px;text-align:left;}";
    html << "th{background-color:#4CAF50;color:white;}</style></head><body>";
    html << "<h1>Access Control Matrix</h1>";
    
    html << "<table><tr><th>Role</th><th>Resource</th><th>Allowed Actions</th>";
    html << "<th>Encryption</th><th>Audited</th></tr>";
    
    for (const auto& entry : entries) {
        html << "<tr><td>" << entry.role << "</td>";
        html << "<td>" << entry.resource << "</td>";
        html << "<td>";
        for (size_t i = 0; i < entry.allowed_actions.size(); i++) {
            if (i > 0) html << ", ";
            html << entry.allowed_actions[i];
        }
        html << "</td>";
        html << "<td>" << (entry.requires_encryption ? "Yes" : "No") << "</td>";
        html << "<td>" << (entry.is_audited ? "Yes" : "No") << "</td>";
        html << "</tr>";
    }
    
    html << "</table></body></html>";
    return html.str();
}

// ========== ComplianceReporter::RiskAssessmentReport Implementation ==========

nlohmann::json ComplianceReporter::RiskAssessmentReport::toJson() const {
    nlohmann::json j;
    nlohmann::json risks_json = nlohmann::json::array();
    
    for (const auto& risk : risks) {
        nlohmann::json risk_json;
        risk_json["risk_id"] = risk.risk_id;
        risk_json["severity"] = risk.severity;
        risk_json["description"] = risk.description;
        risk_json["affected_resources"] = risk.affected_resources;
        risk_json["mitigation"] = risk.mitigation;
        risks_json.push_back(risk_json);
    }
    
    j["risks"] = risks_json;
    j["high_risks"] = high_risks;
    j["medium_risks"] = medium_risks;
    j["low_risks"] = low_risks;
    j["generated_at"] = generated_at;
    return j;
}

std::string ComplianceReporter::RiskAssessmentReport::toCSV() const {
    std::ostringstream csv;
    csv << "Risk ID,Severity,Description,Affected Resources,Mitigation\n";
    
    for (const auto& risk : risks) {
        csv << risk.risk_id << ",";
        csv << risk.severity << ",";
        csv << "\"" << risk.description << "\",";
        csv << "\"";
        for (size_t i = 0; i < risk.affected_resources.size(); i++) {
            if (i > 0) csv << ";";
            csv << risk.affected_resources[i];
        }
        csv << "\",";
        csv << "\"" << risk.mitigation << "\"\n";
    }
    
    csv << "\nSummary\n";
    csv << "High Risks," << high_risks << "\n";
    csv << "Medium Risks," << medium_risks << "\n";
    csv << "Low Risks," << low_risks << "\n";
    
    return csv.str();
}

std::string ComplianceReporter::RiskAssessmentReport::toHTML() const {
    std::ostringstream html;
    html << "<html><head><title>Risk Assessment Report</title>";
    html << "<style>body{font-family:Arial,sans-serif;margin:20px;}";
    html << "table{border-collapse:collapse;width:100%;margin-top:20px;}";
    html << "th,td{border:1px solid #ddd;padding:8px;text-align:left;}";
    html << "th{background-color:#4CAF50;color:white;}";
    html << ".high{background-color:#ffcccc;}.medium{background-color:#fff4cc;}";
    html << ".low{background-color:#ccffcc;}</style></head><body>";
    html << "<h1>Risk Assessment Report</h1>";
    
    html << "<h2>Risk Summary</h2>";
    html << "<p>High Risks: <span class='high'>" << high_risks << "</span> | ";
    html << "Medium Risks: <span class='medium'>" << medium_risks << "</span> | ";
    html << "Low Risks: <span class='low'>" << low_risks << "</span></p>";
    
    html << "<h2>Detailed Risks</h2>";
    html << "<table><tr><th>Risk ID</th><th>Severity</th><th>Description</th>";
    html << "<th>Affected Resources</th><th>Mitigation</th></tr>";
    
    for (const auto& risk : risks) {
        std::string severity_class = risk.severity;
        html << "<tr class='" << severity_class << "'>";
        html << "<td>" << risk.risk_id << "</td>";
        html << "<td>" << risk.severity << "</td>";
        html << "<td>" << risk.description << "</td>";
        html << "<td>";
        for (size_t i = 0; i < risk.affected_resources.size(); i++) {
            if (i > 0) html << ", ";
            html << risk.affected_resources[i];
        }
        html << "</td>";
        html << "<td>" << risk.mitigation << "</td>";
        html << "</tr>";
    }
    
    html << "</table></body></html>";
    return html.str();
}

// ========== ComplianceReporter::ChangeHistoryReport Implementation ==========

nlohmann::json ComplianceReporter::ChangeHistoryReport::toJson() const {
    nlohmann::json j;
    nlohmann::json changes_json = nlohmann::json::array();
    
    for (const auto& change : changes) {
        nlohmann::json change_json;
        change_json["version"] = change.version;
        change_json["timestamp"] = change.timestamp;
        change_json["modified_by"] = change.author;  // Use author field instead
        change_json["change_description"] = change.change_description;
        change_json["rule_id"] = change.rule_id;  // Store rule_id instead of full rule
        changes_json.push_back(change_json);
    }
    
    j["changes"] = changes_json;
    j["total_changes"] = total_changes;
    j["changes_by_user"] = changes_by_user;
    j["start_time"] = start_time;
    j["end_time"] = end_time;
    return j;
}

std::string ComplianceReporter::ChangeHistoryReport::toCSV() const {
    std::ostringstream csv;
    csv << "Version,Timestamp,Modified By,Description\n";
    
    for (const auto& change : changes) {
        csv << change.version << ",";
        csv << change.timestamp << ",";
        csv << change.author << ",";
        csv << "\"" << change.change_description << "\"\n";
    }
    
    csv << "\nSummary\n";
    csv << "Total Changes," << total_changes << "\n";
    csv << "\nChanges by User\n";
    csv << "User,Count\n";
    for (const auto& [user, count] : changes_by_user) {
        csv << user << "," << count << "\n";
    }
    
    return csv.str();
}

std::string ComplianceReporter::ChangeHistoryReport::toHTML() const {
    std::ostringstream html;
    html << "<html><head><title>Change History Report</title>";
    html << "<style>body{font-family:Arial,sans-serif;margin:20px;}";
    html << "table{border-collapse:collapse;width:100%;margin-top:20px;}";
    html << "th,td{border:1px solid #ddd;padding:8px;text-align:left;}";
    html << "th{background-color:#4CAF50;color:white;}</style></head><body>";
    html << "<h1>Change History Report</h1>";
    html << "<p>Total Changes: " << total_changes << "</p>";
    
    html << "<h2>Changes by User</h2><table><tr><th>User</th><th>Count</th></tr>";
    for (const auto& [user, count] : changes_by_user) {
        html << "<tr><td>" << user << "</td><td>" << count << "</td></tr>";
    }
    html << "</table>";
    
    html << "<h2>Change Details</h2>";
    html << "<table><tr><th>Version</th><th>Timestamp</th><th>Modified By</th><th>Description</th></tr>";
    for (const auto& change : changes) {
        html << "<tr><td>" << change.version << "</td>";
        html << "<td>" << change.timestamp << "</td>";
        html << "<td>" << change.author << "</td>";
        html << "<td>" << change.change_description << "</td></tr>";
    }
    html << "</table>";
    
    html << "</body></html>";
    return html.str();
}

// ========== ComplianceReporter Implementation ==========

ComplianceReporter::PolicySummaryReport ComplianceReporter::generatePolicySummary(
    const PolicyManager& policy_mgr
) const {
    PolicySummaryReport report;
    auto all_rules = policy_mgr.listRules();
    
    report.total_rules = all_rules.size();
    report.generated_at = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    
    for (const auto& rule : all_rules) {
        if (rule.enabled) {
            report.enabled_rules++;
        } else {
            report.disabled_rules++;
        }
        
        // Count by classification
        if (!rule.classification_level.empty()) {
            report.rules_by_classification[rule.classification_level]++;
        }
        
        // Count encryption requirements
        if (rule.require_encryption) {
            report.rules_requiring_encryption["with_encryption"]++;
        } else {
            report.rules_requiring_encryption["without_encryption"]++;
        }
        
        // Count audit settings
        if (rule.audit_access || rule.audit_changes) {
            report.rules_with_audit["with_audit"]++;
        } else {
            report.rules_with_audit["without_audit"]++;
        }
    }
    
    THEMIS_INFO("Generated policy summary report: {} total rules", report.total_rules);
    
    return report;
}

ComplianceReporter::ComplianceStatusReport ComplianceReporter::generateComplianceStatus(
    const PolicyManager& policy_mgr,
    const ComplianceGapDetector& detector,
    const std::string& framework
) const {
    ComplianceStatusReport report;
    report.framework = framework.empty() ? "all" : framework;
    report.generated_at = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    
    auto status = detector.getComplianceStatus(policy_mgr, framework);
    report.overall_compliance = status.compliance_percentage;
    report.gaps = status.gaps;
    
    // Generate control lists (simplified - would be more detailed in production)
    auto requirements = detector.exportRequirements();
    if (requirements.contains("requirements")) {
        for (const auto& req_json : requirements["requirements"]) {
            auto req = ComplianceGapDetector::ComplianceRequirement::fromJson(req_json);
            if (framework.empty() || req.framework == framework) {
                // Check if requirement is met
                bool is_met = true;
                for (const auto& gap : report.gaps) {
                    if (gap.requirement_id == req.id) {
                        is_met = false;
                        break;
                    }
                }
                
                if (is_met) {
                    report.compliant_controls.push_back(req.name);
                } else {
                    report.non_compliant_controls.push_back(req.name);
                }
            }
        }
    }
    
    THEMIS_INFO("Generated compliance status report for {}: {:.2f}% compliant", 
                report.framework, report.overall_compliance);
    
    return report;
}

ComplianceReporter::AccessControlMatrix ComplianceReporter::generateAccessControlMatrix(
    const PolicyManager& policy_mgr
) const {
    AccessControlMatrix matrix;
    matrix.generated_at = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    
    auto all_rules = policy_mgr.listRules();
    
    // Build matrix entries from policy rules
    for (const auto& rule : all_rules) {
        if (!rule.enabled) continue;
        
        for (const auto& resource : rule.resources) {
            for (const auto& role : rule.required_roles) {
                AccessControlMatrix::Entry entry;
                entry.role = role.empty() ? "*" : role;
                entry.resource = resource;
                entry.allowed_actions = rule.actions;
                entry.requires_encryption = rule.require_encryption;
                entry.is_audited = rule.audit_access || rule.audit_changes;
                
                matrix.entries.push_back(entry);
            }
            
            // If no required roles, create an entry for all roles
            if (rule.required_roles.empty()) {
                AccessControlMatrix::Entry entry;
                entry.role = "*";
                entry.resource = resource;
                entry.allowed_actions = rule.actions;
                entry.requires_encryption = rule.require_encryption;
                entry.is_audited = rule.audit_access || rule.audit_changes;
                
                matrix.entries.push_back(entry);
            }
        }
    }
    
    THEMIS_INFO("Generated access control matrix with {} entries", matrix.entries.size());
    
    return matrix;
}

ComplianceReporter::RiskAssessmentReport ComplianceReporter::generateRiskAssessment(
    const PolicyManager& policy_mgr
) const {
    RiskAssessmentReport report;
    report.generated_at = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    
    auto all_rules = policy_mgr.listRules();
    
    // Analyze rules for potential risks
    for (const auto& rule : all_rules) {
        // Check for disabled rules
        if (!rule.enabled) {
            RiskAssessmentReport::RiskItem risk;
            risk.risk_id = "DISABLED_" + rule.id;
            risk.severity = "medium";
            risk.description = "Policy rule is disabled: " + rule.name;
            risk.affected_resources = rule.resources;
            risk.mitigation = "Review and enable the rule if still needed, or remove it";
            report.risks.push_back(risk);
            report.medium_risks++;
        }
        
        // Check for overly permissive rules
        if (rule.enabled) {
            bool is_permissive = false;
            for (const auto& resource : rule.resources) {
                if (resource == "*") is_permissive = true;
            }
            for (const auto& action : rule.actions) {
                if (action == "*") is_permissive = true;
            }
            
            if (is_permissive && !rule.require_encryption && !rule.audit_access) {
                RiskAssessmentReport::RiskItem risk;
                risk.risk_id = "PERMISSIVE_" + rule.id;
                risk.severity = "high";
                risk.description = "Overly permissive rule without encryption or audit: " + rule.name;
                risk.affected_resources = rule.resources;
                risk.mitigation = "Add encryption and audit requirements, or restrict resource/action scope";
                report.risks.push_back(risk);
                report.high_risks++;
            }
        }
        
        // Check for missing encryption on sensitive data
        if (rule.enabled && !rule.require_encryption) {
            bool is_sensitive = false;
            for (const auto& resource : rule.resources) {
                if (resource.find("secret") != std::string::npos ||
                    resource.find("password") != std::string::npos ||
                    resource.find("key") != std::string::npos ||
                    resource.find("private") != std::string::npos) {
                    is_sensitive = true;
                    break;
                }
            }
            
            if (is_sensitive) {
                RiskAssessmentReport::RiskItem risk;
                risk.risk_id = "NO_ENCRYPTION_" + rule.id;
                risk.severity = "high";
                risk.description = "Sensitive resource without encryption requirement: " + rule.name;
                risk.affected_resources = rule.resources;
                risk.mitigation = "Enable encryption requirement for this rule";
                report.risks.push_back(risk);
                report.high_risks++;
            }
        }
        
        // Check for short retention periods
        if (rule.enabled && rule.retention_days < 30) {
            RiskAssessmentReport::RiskItem risk;
            risk.risk_id = "SHORT_RETENTION_" + rule.id;
            risk.severity = "low";
            risk.description = "Short retention period (" + std::to_string(rule.retention_days) + " days): " + rule.name;
            risk.affected_resources = rule.resources;
            risk.mitigation = "Review retention requirements and adjust if necessary";
            report.risks.push_back(risk);
            report.low_risks++;
        }
    }
    
    THEMIS_INFO("Generated risk assessment report: {} high, {} medium, {} low risks", 
                report.high_risks, report.medium_risks, report.low_risks);
    
    return report;
}

ComplianceReporter::ChangeHistoryReport ComplianceReporter::generateChangeHistory(
    const PolicyManager& policy_mgr,
    int64_t start_time,
    int64_t end_time
) const {
    ChangeHistoryReport report;
    report.start_time = start_time;
    report.end_time = end_time;
    
    // Get all rules and their version histories
    auto all_rules = policy_mgr.listRules();
    
    for (const auto& rule : all_rules) {
        // Get audit trail for each rule
        auto versions = policy_mgr.getAuditTrail(rule.id, start_time, end_time);
        
        for (const auto& version : versions) {
            report.changes.push_back(version);
            report.changes_by_user[version.author]++;
        }
    }
    
    report.total_changes = report.changes.size();
    
    THEMIS_INFO("Generated change history report: {} changes", report.total_changes);
    
    return report;
}

std::string ComplianceReporter::exportReport(
    const nlohmann::json& report,
    ReportFormat format
) const {
    switch (format) {
        case ReportFormat::JSON:
            return report.dump(2);
            
        case ReportFormat::CSV:
            THEMIS_ERROR("CSV export not implemented for generic JSON reports");
            return "";
            
        case ReportFormat::HTML:
            THEMIS_ERROR("HTML export not implemented for generic JSON reports");
            return "";
            
        case ReportFormat::PDF:
            THEMIS_ERROR("PDF export not implemented");
            return "";
            
        default:
            THEMIS_ERROR("Unknown report format");
            return "";
    }
}

std::string ComplianceReporter::generateHTMLHeader(const std::string& title) const {
    std::ostringstream html;
    html << "<!DOCTYPE html><html><head>";
    html << "<meta charset='UTF-8'>";
    html << "<title>" << title << "</title>";
    html << "<style>";
    html << "body { font-family: Arial, sans-serif; margin: 20px; }";
    html << "h1 { color: #333; }";
    html << "table { border-collapse: collapse; width: 100%; margin-top: 20px; }";
    html << "th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }";
    html << "th { background-color: #4CAF50; color: white; }";
    html << "tr:nth-child(even) { background-color: #f2f2f2; }";
    html << "</style></head><body>";
    return html.str();
}

std::string ComplianceReporter::generateHTMLFooter() const {
    return "</body></html>";
}

} // namespace governance
} // namespace themis
