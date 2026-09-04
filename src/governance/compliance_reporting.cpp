/**
 * @file compliance_reporting.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=62, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "governance/compliance_reporting.h"
#include "governance/ccpa_rules.h"
#include "governance/governance_diagnostics.h"
#include "utils/logger.h"

#include <algorithm>
#include <fstream>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <unordered_set>
#include <ctime>
#include <cstdio>
#include <atomic>
#include <memory>

namespace themis {
namespace governance {

namespace {

std::unordered_set<std::string> collect_action_candidates(const PolicyManager& policy_mgr) {
    std::unordered_set<std::string> action_candidates = {"*"};
    for (const auto& rule : policy_mgr.listRules()) {
        if (!rule.enabled) {
            continue;
        }
        if (rule.actions.empty()) {
            action_candidates.insert("*");
            continue;
        }
        for (const auto& action : rule.actions) {
            action_candidates.insert(action);
        }
    }
    return action_candidates;
}

std::vector<PolicyRule> find_applicable_rules_for_any_action(
    const PolicyManager& policy_mgr,
    const std::string& resource,
    const std::unordered_set<std::string>& action_candidates
) {
    std::vector<PolicyRule> applicable_rules;
    std::unordered_set<std::string> seen_rule_ids;

    for (const auto& action : action_candidates) {
        auto rules_for_action = policy_mgr.findApplicableRules(resource, action, {});
        for (const auto& rule : rules_for_action) {
            if (!seen_rule_ids.insert(rule.id).second) {
                continue;
            }
            applicable_rules.push_back(rule);
        }
    }

    return applicable_rules;
}

} // namespace

// ========== ComplianceReporterResult Implementation ==========

std::string ComplianceReporterResult::getErrorName() const {
    switch (error) {
        case ComplianceError::kSuccess:
            return "SUCCESS";
        case ComplianceError::kConflictDetected:
            return "CONFLICT_DETECTED";
        case ComplianceError::kReportingFailed:
            return "REPORTING_FAILED";
        case ComplianceError::kStateInvalid:
            return "STATE_INVALID";
        case ComplianceError::kResourceExhausted:
            return "RESOURCE_EXHAUSTED";
        case ComplianceError::kHtmlGenerationFailed:
            return "HTML_GENERATION_FAILED";
        default:
            return "UNKNOWN_ERROR";
    }
}

// ========== ComplianceReporter Implementation ==========

ComplianceReporter::ComplianceReporter() : state_(ReporterState::DRAFT) {
}

ComplianceReporter::ReporterState ComplianceReporter::getState() const {
    return state_.load(std::memory_order_acquire);
}

bool ComplianceReporter::isReadyForReporting() const {
    return getState() == ReporterState::DRAFT;
}

bool ComplianceReporter::transitionState(ReporterState expected, ReporterState target) const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    ReporterState current = state_.load(std::memory_order_relaxed);
    if (current != expected) {
        return false;
    }
    
    state_.store(target, std::memory_order_release);
    return true;
}

std::string ComplianceReporter::generateHTMLOptimized(
    const std::string& title,
    const std::vector<std::string>& headers,
    const std::vector<std::vector<std::string>>& rows
) const {
    // Estimate capacity: reasonable buffer for headers and rows
    size_t estimated_size = 2048;  // Base HTML structure
    for (const auto& header : headers) {
        estimated_size += static_cast<int>(header.size()) + 20;  // Add markup overhead
    }
    for (const auto& row : rows) {
        for (const auto& cell : row) {
            estimated_size += static_cast<int>(cell.size()) + 20;
        }
    }
    
    std::ostringstream html = {};
    
    // HTML header
    html << "<html><head><title>" << title << "</title>"
         << "<style>"
         << "body{font-family:Arial,sans-serif;margin:20px;}"
         << "table{border-collapse:collapse;width:100%;margin-top:20px;}"
         << "th,td{border:1px solid #ddd;padding:8px;text-align:left;}"
         << "th{background-color:#4CAF50;color:white;}"
         << "</style></head><body>"
         << "<h1>" << title << "</h1>";
    
    // Table with headers
    html << "<table><tr>";
    for (const auto& header : headers) {
        html << "<th>" << header << "</th>";
    }
    html << "</tr>";
    
    // Table rows
    for (const auto& row : rows) {
        html << "<tr>";
        for (const auto& cell : row) {
            html << "<td>" << cell << "</td>";
        }
        html << "</tr>";
    }
    
    html << "</table></body></html>";
    return html.str();
}

void ComplianceReporter::recordComplianceDiagnostic(
    int32_t code,
    const std::string& message,
    const std::string& component
) const {
    auto& aggregator = getGlobalDiagnosticAggregator();
    
    GovernanceDiagnostic diag;
    diag.code = static_cast<GovDiagnosticCode>(code);
    diag.component = component;
    diag.description = message;
    diag.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    // Add remediation steps based on error type
    if (code == 7351) {  // kReportingFailed
        diag.remediation_steps = {
            "Check policy manager availability",
            "Verify compliance detector configuration",
            "Review system logs for resource constraints"
        };
    }
    
    aggregator.recordDiagnostic(diag);
}

ComplianceReporterResult ComplianceReporter::generatePolicySummaryWithResult(
    const PolicyManager& policy_mgr
) {
    ComplianceReporterResult result;
    result.generated_at_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    result.report_format = "JSON";
    
    // Validate state
    if (!isReadyForReporting()) {
        result.error = ComplianceError::kStateInvalid;
        result.error_message = "Reporter not in DRAFT state; report generation in progress or failed";
        result.diagnostic_code = 7352;  // kStateInvalid
        recordComplianceDiagnostic(7352, result.error_message);
        return result;
    }
    
    // Transition to REPORTING
    if (!transitionState(ReporterState::DRAFT, ReporterState::REPORTING)) {
        result.error = ComplianceError::kStateInvalid;
        result.error_message = "Failed to transition to REPORTING state";
        result.diagnostic_code = 7352;
        recordComplianceDiagnostic(7352, result.error_message);
        return result;
    }
    
    try {
        // Generate the report
        auto report = generatePolicySummary(policy_mgr);
        result.report_content = report.toJson().dump(2);  // Pretty-printed JSON
        result.error = ComplianceError::kSuccess;
        
        // Transition to FINALIZED
        transitionState(ReporterState::REPORTING, ReporterState::FINALIZED);
        
    } catch (const std::exception& ex) {
        result.error = ComplianceError::kReportingFailed;
        result.error_message = std::string("Report generation failed: ") + ex.what();
        result.diagnostic_code = 7351;  // kReportingFailed
        recordComplianceDiagnostic(7351, result.error_message);
        
        // Transition to FAILED
        transitionState(ReporterState::REPORTING, ReporterState::FAILED);
    }
    
    return result;
}

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
    result.total_resources_checked = static_cast<int>(resources.size());
    
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
        if (!rule.enabled) {
          continue;
        }
        
        for (const auto& resource : rule.resources) {
            for (const auto& action : rule.actions) {
                std::string key = resource + ":" + action;
                pattern_map[key].push_back(rule.id);
            }
        }
    }
    
    // Identify patterns with multiple rules
    for (const auto& [pattern, rule_ids] : pattern_map) {
        if (static_cast<int>(rule_ids.size()) > 1) {
            OverlapResult overlap;
            
            size_t colon_pos = pattern.find(':');
            overlap.resource_pattern = pattern.substr(0, colon_pos);
            overlap.action_pattern = pattern.substr(colon_pos + 1);
            overlap.overlapping_rule_ids = rule_ids;
            overlap.overlap_count = static_cast<int>(rule_ids.size());
            
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

    // Consider explicit-action rules when evaluating resource-level coverage.
    const auto action_candidates = collect_action_candidates(policy_mgr);

    for (const auto& resource : expected_resources) {
        const auto applicable_rules =
            find_applicable_rules_for_any_action(policy_mgr, resource, action_candidates);
        const bool covered = !applicable_rules.empty();

        if (!covered) {
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
    ComplianceRequirement req = {};
    if (j.contains("id")) {
      req.id = j["id"].get<std::string>();
    }
    if (j.contains("name")) {
      req.name = j["name"].get<std::string>();
    }
    if (j.contains("framework")) {
      req.framework = j["framework"].get<std::string>();
    }
    if (j.contains("description")) {
      req.description = j["description"].get<std::string>();
    }
    if (j.contains("required_resources")) {
      req.required_resources = j["required_resources"].get<std::vector<std::string>>();
    }
    if (j.contains("requires_encryption")) {
      req.requires_encryption = j["requires_encryption"].get<bool>();
    }
    if (j.contains("requires_signature")) {
      req.requires_signature = j["requires_signature"].get<bool>();
    }
    if (j.contains("requires_audit")) {
      req.requires_audit = j["requires_audit"].get<bool>();
    }
    if (j.contains("min_retention_days")) {
      req.min_retention_days = j["min_retention_days"].get<int>();
    }
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
    std::vector<ComplianceRequirement> requirements_snapshot;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        requirements_snapshot = requirements_;
    }

    std::vector<ComplianceGap> gaps;
    const auto action_candidates = collect_action_candidates(policy_mgr);
    
    THEMIS_DEBUG("Detecting compliance gaps for {} requirements", requirements_snapshot.size());
    
    for (const auto& req : requirements_snapshot) {
        if (!checkRequirement(req, policy_mgr)) {
            // Analyze specific gaps
            for (const auto& resource : req.required_resources) {
                auto applicable_rules =
                    find_applicable_rules_for_any_action(policy_mgr, resource, action_candidates);
                
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
                                            std::string s = {};
                                            for (size_t i = 0; i < missing_controls.size(); i++) {
                                                if (i > 0) {
                                                  s += ", ";
                                                }
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
    ComplianceStatus status;
    status.framework = framework.empty() ? "all" : framework;
    
    // Filter requirements by framework
    std::vector<ComplianceRequirement> filtered_reqs;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& req : requirements_) {
            if (framework.empty() || req.framework == framework) {
                filtered_reqs.push_back(req);
            }
        }
    }
    
    status.total_requirements = static_cast<int>(filtered_reqs.size());
    
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
    const auto action_candidates = collect_action_candidates(policy_mgr);

    // Check if all required resources have appropriate policies
    for (const auto& resource : req.required_resources) {
        auto applicable_rules =
            find_applicable_rules_for_any_action(policy_mgr, resource, action_candidates);
        
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
    std::ostringstream csv = {};
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
    std::ostringstream html = {};
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
    std::ostringstream csv = {};
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
    std::ostringstream html = {};
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
    std::ostringstream csv = {};
    csv << "Role,Resource,Allowed Actions,Requires Encryption,Is Audited\n";
    
    for (const auto& entry : entries) {
        csv << entry.role << ",";
        csv << entry.resource << ",";
        csv << "\"";
        for (size_t i = 0; i < entry.allowed_actions.size(); i++) {
            if (i > 0) {
              csv << ";";
            }
            csv << entry.allowed_actions[i];
        }
        csv << "\",";
        csv << (entry.requires_encryption ? "Yes" : "No") << ",";
        csv << (entry.is_audited ? "Yes" : "No") << "\n";
    }
    
    return csv.str();
}

std::string ComplianceReporter::AccessControlMatrix::toHTML() const {
    std::ostringstream html = {};
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
            if (i > 0) {
              html << ", ";
            }
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
    std::ostringstream csv = {};
    csv << "Risk ID,Severity,Description,Affected Resources,Mitigation\n";
    
    for (const auto& risk : risks) {
        csv << risk.risk_id << ",";
        csv << risk.severity << ",";
        csv << "\"" << risk.description << "\",";
        csv << "\"";
        for (size_t i = 0; i < risk.affected_resources.size(); i++) {
            if (i > 0) {
              csv << ";";
            }
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
    std::ostringstream html = {};
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
            if (i > 0) {
              html << ", ";
            }
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
    std::ostringstream csv = {};
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
    std::ostringstream html = {};
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
    
    report.total_rules = static_cast<int>(all_rules.size());
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
        if (!rule.enabled) {
          continue;
        }
        
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
                if (resource == "*") {
                  is_permissive = true;
                }
            }
            for (const auto& action : rule.actions) {
                if (action == "*") {
                  is_permissive = true;
                }
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
    
    report.total_changes = static_cast<int>(report.changes.size());
    
    THEMIS_INFO("Generated change history report: {} changes", report.total_changes);
    
    return report;
}

// ========== Export Helper Functions ==========

namespace {

/// Escape special PDF string characters
std::string escapePDFString(const std::string& s) {
    std::string result = {};
    result.reserve(s.size());
    for (unsigned char c : s) {
        if (c == '(')       result += "\\(";
        else if (c == ')')  result += "\\)";
        else if (c == '\\') result += "\\\\";
        else if (c >= 32 && c <= 126) result += static_cast<char>(c);
        else result += ' ';
    }
    return result;
}

/// Escape a value for CSV: wrap in double-quotes if it contains commas, quotes, or newlines.
std::string csvEscape(const std::string& val) {
    bool needs_quoting = val.find_first_of(",\"\n\r") != std::string::npos;
    if (!needs_quoting) {
      return val;
    }
    std::string out = {};
    out.reserve(static_cast<int>(val.size()) + 2);
    out += '"';
    for (char c : val) {
        if (c == '"') out += '"'; // RFC 4180: escape double-quote by doubling
        out += c;
    }
    out += '"';
    return out;
}

/// Convert a JSON scalar value to a plain string for CSV output.
std::string jsonScalarToString(const nlohmann::json& v) {
    if (v.is_string()) {
      return v.get<std::string>();
    }
    if (v.is_null()) {
      return "";
    }
    return v.dump();
}

/// Generate a CSV document from a compliance report JSON.
/// Top-level object keys are emitted as rows: Field,Value.
/// Nested objects/arrays are JSON-serialised into the value column.
std::string generateCSVFromJson(const nlohmann::json& report) {
    std::ostringstream csv = {};
    csv << "Field,Value\n";
    if (report.is_object()) {
        for (const auto& [key, val] : report.items()) {
            csv << csvEscape(key) << ",";
            if (val.is_object() || val.is_array()) {
                csv << csvEscape(val.dump());
            } else {
                csv << csvEscape(jsonScalarToString(val));
            }
            csv << "\n";
        }
    } else {
        csv << "data," << csvEscape(report.dump()) << "\n";
    }
    return csv.str();
}

/// Recursively render a JSON value to HTML
void jsonToHtml(std::ostringstream& html, const nlohmann::json& j, int depth = 0) {
    if (j.is_object()) {
        html << "<table style='width:100%;border-collapse:collapse;margin:4px 0;'>";
        for (const auto& [key, val] : j.items()) {
            html << "<tr>";
            html << "<td style='border:1px solid #ccc;padding:5px;font-weight:bold;background:#f5f5f5;width:30%;vertical-align:top'>"
                 << key << "</td>";
            html << "<td style='border:1px solid #ccc;padding:5px;'>";
            jsonToHtml(html, val, depth + 1);
            html << "</td></tr>";
        }
        html << "</table>";
    } else if (j.is_array()) {
        if (j.empty()) {
            html << "<em>(empty)</em>";
        } else {
            html << "<ol style='margin:0;padding-left:20px;'>";
            for (const auto& item : j) {
                html << "<li>";
                jsonToHtml(html, item, depth + 1);
                html << "</li>";
            }
            html << "</ol>";
        }
    } else if (j.is_string()) {
        html << j.get<std::string>();
    } else if (j.is_null()) {
        html << "<em>null</em>";
    } else {
        html << j.dump();
    }
}

/// Generate a full HTML document from a compliance report JSON
std::string generateHTMLFromJson(const nlohmann::json& report) {
    std::ostringstream html;
    html << "<!DOCTYPE html><html><head>"
         << "<meta charset='UTF-8'>"
         << "<title>ThemisDB Compliance Report</title>"
         << "<style>"
         << "body{font-family:Arial,sans-serif;margin:30px;color:#333;}"
         << "h1{color:#2c3e50;border-bottom:2px solid #4CAF50;padding-bottom:10px;}"
         << "h2{color:#34495e;margin-top:20px;}"
         << "table{border-collapse:collapse;width:100%;margin:10px 0;}"
         << "th,td{border:1px solid #ddd;padding:8px;text-align:left;}"
         << "th{background-color:#4CAF50;color:white;}"
         << "tr:nth-child(even){background-color:#f9f9f9;}"
         << ".meta{color:#666;font-size:0.9em;margin-bottom:20px;}"
         << "@media print{body{margin:15px;}}"
         << "</style></head><body>";

    html << "<h1>ThemisDB Compliance Report</h1>";

    // Print generated_at if present
    if (report.contains("generated_at")) {
        std::time_t ts = 0;
        if (report["generated_at"].is_number()) {
            ts = static_cast<std::time_t>(report["generated_at"].get<int64_t>());
        }
        char buf[32] = {};
        std::tm* tm_info = std::localtime(&ts);
        if (tm_info) {
            std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm_info);
        }
        html << "<p class='meta'>Generated: " << buf << "</p>";
    }

    html << "<h2>Report Summary</h2>";
    jsonToHtml(html, report);
    html << "</body></html>";
    return html.str();
}

/// Flatten a JSON value to human-readable text lines, with optional key prefix
void flattenJsonToLines(const nlohmann::json& j,
                        std::vector<std::string>& lines,
                        const std::string& prefix = "") {
    if (j.is_object()) {
        for (const auto& [key, val] : j.items()) {
            std::string full_key = prefix.empty() ? key : (prefix + "." + key);
            if (val.is_object() || val.is_array()) {
                lines.push_back(full_key + ":");
                flattenJsonToLines(val, lines, "  " + full_key);
            } else {
                std::string val_str = val.is_string() ? val.get<std::string>() : val.dump();
                lines.push_back(full_key + ": " + val_str);
            }
        }
    } else if (j.is_array()) {
        for (size_t i = 0; i < j.size(); ++i) {
            std::string idx_prefix = prefix + "[" + std::to_string(i) + "]";
            if (j[i].is_object() || j[i].is_array()) {
                lines.push_back(idx_prefix + ":");
                flattenJsonToLines(j[i], lines, "  " + idx_prefix);
            } else {
                std::string val_str = j[i].is_string() ? j[i].get<std::string>() : j[i].dump();
                lines.push_back(idx_prefix + ": " + val_str);
            }
        }
    } else {
        std::string val_str = j.is_string() ? j.get<std::string>() : j.dump();
        lines.push_back(prefix.empty() ? val_str : (prefix + ": " + val_str));
    }
}

/// Build a minimal valid PDF-1.4 document containing the given title and text lines.
/// Uses only the Helvetica and Helvetica-Bold Type1 base fonts (no font embedding required).
std::string buildPDF(const std::string& title, const std::vector<std::string>& lines) {
    // PDF page geometry (US Letter)
    constexpr double PAGE_W  = 612.0;
    constexpr double PAGE_H  = 792.0;
    constexpr double MARGIN  = 50.0;
    constexpr double TITLE_SIZE = 14.0;
    constexpr double BODY_SIZE  = 9.0;
    constexpr double LINE_H  = 12.0;

    // Build one content stream per page
    std::vector<std::string> page_streams;
    page_streams.push_back("");

    // Build page streams with explicit Td positioning
    double y = PAGE_H - MARGIN;
    page_streams.back() += "BT\n";
    page_streams.back() += "/F1 " + std::to_string(static_cast<int>(TITLE_SIZE)) + " Tf\n";
    page_streams.back() += std::to_string(static_cast<int>(MARGIN)) + " " +
                           std::to_string(static_cast<int>(y)) + " Td\n";
    page_streams.back() += "(" + escapePDFString(title) + ") Tj\n";
    y -= TITLE_SIZE * 1.8;

    // Separator line (drawn as thin filled rectangle)
    page_streams.back() += "ET\n";
    page_streams.back() += std::to_string(static_cast<int>(MARGIN)) + " " +
                           std::to_string(static_cast<int>(y + 4)) +
                           " " + std::to_string(static_cast<int>(PAGE_W - MARGIN * 2)) +
                           " 1 re f\n";
    y -= LINE_H;

    page_streams.back() += "BT\n";
    page_streams.back() += "/F2 " + std::to_string(static_cast<int>(BODY_SIZE)) + " Tf\n";

    for (const auto& line : lines) {
        if (y < MARGIN) {
            // Close current page BT block, start new page
            page_streams.back() += "ET\n";
            page_streams.push_back("");
            y = PAGE_H - MARGIN;
            page_streams.back() += "BT\n";
            page_streams.back() += "/F2 " +
                std::to_string(static_cast<int>(BODY_SIZE)) + " Tf\n";
        }
        page_streams.back() += std::to_string(static_cast<int>(MARGIN)) + " " +
                               std::to_string(static_cast<int>(y)) + " Td\n";
        // Truncate very long lines
        std::string display = line.size() > 100 ? line.substr(0, 97) + "..." : line;
        page_streams.back() += "(" + escapePDFString(display) + ") Tj\n";
        y -= LINE_H;
    }
    page_streams.back() += "ET\n";

    // Assemble PDF binary
    std::string pdf = {};
    pdf.reserve(4096);
    pdf += "%PDF-1.4\n";
    // Binary comment to mark file as containing binary data
    pdf += "%\xE2\xE3\xCF\xD3\n";

    // Object layout:
    //   1: Catalog
    //   2: Pages
    //   3..3+P-1: Page objects  (P = page count)
    //   3+P..3+2P-1: Content streams
    //   3+2P: Font F1 (Helvetica-Bold)
    //   3+2P+1: Font F2 (Helvetica)
    int P = static_cast<int>(page_streams.size());
    int base_page   = 3;
    int base_stream = base_page + P;
    int font_f1_id  = base_stream + P;
    int font_f2_id  = font_f1_id + 1;
    int total_objs  = font_f2_id + 1;  // exclusive upper bound

    std::vector<size_t> offsets(static_cast<size_t>(total_objs) + 1, 0);

    auto appendObj = [&](int id, const std::string& dict, const std::string& stream_data = "") {
        offsets[static_cast<size_t>(id)] = pdf.size();
        pdf += std::to_string(id) + " 0 obj\n";
        if (stream_data.empty()) {
            pdf += dict + "\n";
        } else {
            // Insert /Length into the dict
            std::string d = dict;
            auto pos = d.rfind(">>");
            if (pos != std::string::npos) {
                d.insert(pos, " /Length " + std::to_string(stream_data.size()));
            }
            pdf += d + "\nstream\n";
            pdf += stream_data;
            pdf += "\nendstream\n";
        }
        pdf += "endobj\n";
    };

    // Object 1: Catalog
    appendObj(1, "<< /Type /Catalog /Pages 2 0 R >>");

    // Object 2: Pages
    {
        std::string kids = "[";
        for (int i = 0; i < P; ++i) {
            kids += std::to_string(base_page + i) + " 0 R ";
        }
        kids += "]";
        appendObj(2, "<< /Type /Pages /Kids " + kids + " /Count " + std::to_string(P) + " >>");
    }

    // Font resource dict (shared by all pages)
    std::string font_res = "<< /F1 " + std::to_string(font_f1_id) + " 0 R"
                         + " /F2 " + std::to_string(font_f2_id) + " 0 R >>";

    // Page objects
    for (int i = 0; i < P; ++i) {
        int page_id   = base_page + i;
        int stream_id = base_stream + i;
        std::string page_dict =
            "<< /Type /Page /Parent 2 0 R"
            " /MediaBox [0 0 " + std::to_string(static_cast<int>(PAGE_W)) +
            " " + std::to_string(static_cast<int>(PAGE_H)) + "]"
            " /Contents " + std::to_string(stream_id) + " 0 R"
            " /Resources << /Font " + font_res + " >> >>";
        appendObj(page_id, page_dict);
    }

    // Content stream objects
    for (int i = 0; i < P; ++i) {
        appendObj(base_stream + i, "<<>>", page_streams[static_cast<size_t>(i)]);
    }

    // Font objects (Type1 base fonts — no embedding required by PDF spec)
    appendObj(font_f1_id,
        "<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica-Bold >>");
    appendObj(font_f2_id,
        "<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica >>");

    // Cross-reference table
    size_t xref_offset = pdf.size();
    pdf += "xref\n";
    pdf += "0 " + std::to_string(total_objs) + "\n";
    // Free entry for object 0
    pdf += "0000000000 65535 f \n";
    for (int i = 1; i < total_objs; ++i) {
        char buf[22];
        std::snprintf(buf, sizeof(buf), "%010zu 00000 n \n",
                      offsets[static_cast<size_t>(i)]);
        pdf += buf;
    }

    // Trailer
    pdf += "trailer\n";
    pdf += "<< /Size " + std::to_string(total_objs) + " /Root 1 0 R >>\n";
    pdf += "startxref\n";
    pdf += std::to_string(xref_offset) + "\n";
    pdf += "%%EOF\n";

    return pdf;
}

} // anonymous namespace

// ========== ComplianceReporter::exportReport Implementation ==========

std::string ComplianceReporter::exportReport(
    const nlohmann::json& report,
    ReportFormat format
) const {
    switch (format) {
        case ReportFormat::JSON:
            return report.dump(2);

        case ReportFormat::CSV:
            THEMIS_INFO("Generating CSV compliance report");
            return generateCSVFromJson(report);

        case ReportFormat::HTML: {
            THEMIS_INFO("Generating HTML compliance report");
            return generateHTMLFromJson(report);
        }

        case ReportFormat::PDF: {
            THEMIS_INFO("Generating PDF compliance report");
            std::string title = "ThemisDB Compliance Report";
            if (report.contains("report_type") && report["report_type"].is_string()) {
                title += " - " + report["report_type"].get<std::string>();
            }
            std::vector<std::string> lines = {};

            if (report.contains("generated_at") && report["generated_at"].is_number()) {
                std::time_t ts = static_cast<std::time_t>(
                    report["generated_at"].get<int64_t>());
                char buf[32] = {};
                std::tm* tm_info = std::localtime(&ts);
                if (tm_info) {
                    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm_info);
                }
                lines.push_back("Generated: " + std::string(buf));
            }
            lines.push_back("");
            flattenJsonToLines(report, lines);
            return buildPDF(title, lines);
        }

        default:
            THEMIS_ERROR("Unknown report format");
            return "";
    }
}

std::string ComplianceReporter::generateHTMLHeader(const std::string& title) const {
    std::ostringstream html = {};
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

// ========== ComplianceReporter::CcpaReport Implementation ==========

nlohmann::json ComplianceReporter::CcpaReport::toJson() const {
    nlohmann::json j;
    j["data_categories"]                  = data_categories;
    j["third_party_disclosure_rule_ids"]  = third_party_disclosure_rule_ids;
    j["opt_out_count"]                    = opt_out_count;
    j["ccpa_compliant_rules"]             = ccpa_compliant_rules;
    j["ccpa_non_compliant_rules"]         = ccpa_non_compliant_rules;
    j["missing_right_to_know"]            = missing_right_to_know;
    j["missing_right_to_delete"]          = missing_right_to_delete;
    j["missing_opt_out_of_sale"]          = missing_opt_out_of_sale;
    j["missing_data_portability"]         = missing_data_portability;
    j["start_time"]                       = start_time;
    j["end_time"]                         = end_time;
    j["generated_at"]                     = generated_at;
    return j;
}

std::string ComplianceReporter::CcpaReport::toCSV() const {
    std::ostringstream csv = {};
    csv << "Field,Value\n";
    csv << "opt_out_count," << opt_out_count << "\n";
    csv << "ccpa_compliant_rules," << ccpa_compliant_rules << "\n";
    csv << "ccpa_non_compliant_rules," << ccpa_non_compliant_rules << "\n";
    csv << "missing_right_to_know_count," << missing_right_to_know.size() << "\n";
    csv << "missing_right_to_delete_count," << missing_right_to_delete.size() << "\n";
    csv << "missing_opt_out_of_sale_count," << missing_opt_out_of_sale.size() << "\n";
    csv << "missing_data_portability_count," << missing_data_portability.size() << "\n";
    csv << "third_party_disclosure_count," << third_party_disclosure_rule_ids.size() << "\n";
    csv << "generated_at," << generated_at << "\n";
    return csv.str();
}

// ========== ComplianceReporter::generateCcpaReport Implementation ==========

ComplianceReporter::CcpaReport ComplianceReporter::generateCcpaReport(
    const PolicyManager& policy_mgr,
    int opt_out_count_param,
    int64_t start_time,
    int64_t end_time
) const {
    THEMIS_INFO("Generating CCPA/CPRA compliance report");

    CcpaReport report;
    report.start_time    = start_time;
    report.end_time      = end_time;
    report.generated_at  = std::chrono::duration_cast<std::chrono::seconds>(
                               std::chrono::system_clock::now().time_since_epoch()
                           ).count();
    report.opt_out_count = opt_out_count_param;

    // Rule evaluators (stateless, stack-allocated)
    RightToKnow   rtk;
    RightToDelete  rtd;
    OptOutOfSale   oos;
    DataPortability dp;

    // Collect unique data categories from classification levels
    std::unordered_set<std::string> categories_seen;
    auto all_rules = policy_mgr.listRules();

    for (const auto& rule : all_rules) {
        if (!rule.enabled) {
          continue;
        }

        // Collect data categories from classification levels
        if (!rule.classification_level.empty()) {
            categories_seen.insert(rule.classification_level);
        }

        // Identify rules that allow export (third-party disclosure candidates)
        if (rule.allow_export) {
            report.third_party_disclosure_rule_ids.push_back(rule.id);
        }

        // Evaluate all CCPA rules
        bool rule_fully_compliant = true;

        // Right to Know: requires audit_access
        if (!rtk.evaluate(rule)) {
            report.missing_right_to_know.push_back(rule.id);
            rule_fully_compliant = false;
        }

        // Right to Delete: requires audit_changes + reasonable retention
        if (!rtd.evaluate(rule)) {
            report.missing_right_to_delete.push_back(rule.id);
            rule_fully_compliant = false;
        }

        // Opt-Out of Sale: export must be gated (disabled or signature-required)
        if (!oos.evaluate(rule)) {
            report.missing_opt_out_of_sale.push_back(rule.id);
            rule_fully_compliant = false;
        }

        // Data Portability: export or audit access must be available
        if (!dp.evaluate(rule)) {
            report.missing_data_portability.push_back(rule.id);
            rule_fully_compliant = false;
        }

        if (rule_fully_compliant) {
            report.ccpa_compliant_rules++;
        } else {
            report.ccpa_non_compliant_rules++;
        }
    }

    // Build sorted data categories list
    report.data_categories.assign(categories_seen.begin(), categories_seen.end());
    std::sort(report.data_categories.begin(), report.data_categories.end());

    THEMIS_INFO(
        "CCPA report: {} compliant, {} non-compliant rules; {} opt-out subjects; "
        "{} third-party disclosure candidates",
        report.ccpa_compliant_rules, report.ccpa_non_compliant_rules,
        report.opt_out_count, report.third_party_disclosure_rule_ids.size());

    return report;
}

} // namespace governance
} // namespace themis
