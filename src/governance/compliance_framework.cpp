/**
 * @file compliance_framework.cpp
 * @brief Compliance framework implementation
 * @version 1.0.0
 * @date 2026-08-18
 * 
 * Implements core compliance framework structures and registry.
 */

#include "governance/compliance_framework.h"
#include "utils/uuid.h"
#include <chrono>
#include <sstream>

namespace themis {
namespace governance {

// ============================================================================
// ComplianceRequirement Implementation
// ============================================================================

nlohmann::json ComplianceRequirement::toJson() const {
    return nlohmann::json{
        {"requirement_id", requirement_id},
        {"framework", static_cast<int>(framework)},
        {"requirement_text", requirement_text},
        {"regulatory_section", regulatory_section},
        {"severity", static_cast<int>(severity)},
        {"control_ids", control_ids},
        {"category", category},
        {"is_mandatory", is_mandatory},
        {"version", version},
        {"created_at_ms", created_at_ms},
        {"updated_at_ms", updated_at_ms},
        {"metadata", metadata}
    };
}

ComplianceRequirement ComplianceRequirement::fromJson(const nlohmann::json& j) {
    ComplianceRequirement req;
    if (j.contains("requirement_id")) req.requirement_id = j["requirement_id"];
    if (j.contains("framework")) req.framework = static_cast<ComplianceFramework>(j["framework"].get<int>());
    if (j.contains("requirement_text")) req.requirement_text = j["requirement_text"];
    if (j.contains("regulatory_section")) req.regulatory_section = j["regulatory_section"];
    if (j.contains("severity")) req.severity = static_cast<ComplianceSeverity>(j["severity"].get<int>());
    if (j.contains("control_ids")) req.control_ids = j["control_ids"].get<std::vector<std::string>>();
    if (j.contains("category")) req.category = j["category"];
    if (j.contains("is_mandatory")) req.is_mandatory = j["is_mandatory"];
    if (j.contains("version")) req.version = j["version"];
    if (j.contains("created_at_ms")) req.created_at_ms = j["created_at_ms"];
    if (j.contains("updated_at_ms")) req.updated_at_ms = j["updated_at_ms"];
    if (j.contains("metadata")) req.metadata = j["metadata"];
    return req;
}

// ============================================================================
// ComplianceControl Implementation
// ============================================================================

nlohmann::json ComplianceControl::toJson() const {
    return nlohmann::json{
        {"control_id", control_id},
        {"framework", static_cast<int>(framework)},
        {"control_name", control_name},
        {"description", description},
        {"implementation_detail", implementation_detail},
        {"is_automated", is_automated},
        {"policy_rules", policy_rules},
        {"evidence_types", evidence_types},
        {"version", version},
        {"created_at_ms", created_at_ms}
    };
}

ComplianceControl ComplianceControl::fromJson(const nlohmann::json& j) {
    ComplianceControl ctl;
    if (j.contains("control_id")) ctl.control_id = j["control_id"];
    if (j.contains("framework")) ctl.framework = static_cast<ComplianceFramework>(j["framework"].get<int>());
    if (j.contains("control_name")) ctl.control_name = j["control_name"];
    if (j.contains("description")) ctl.description = j["description"];
    if (j.contains("implementation_detail")) ctl.implementation_detail = j["implementation_detail"];
    if (j.contains("is_automated")) ctl.is_automated = j["is_automated"];
    if (j.contains("policy_rules")) ctl.policy_rules = j["policy_rules"].get<std::vector<std::string>>();
    if (j.contains("evidence_types")) ctl.evidence_types = j["evidence_types"].get<std::vector<std::string>>();
    if (j.contains("version")) ctl.version = j["version"];
    if (j.contains("created_at_ms")) ctl.created_at_ms = j["created_at_ms"];
    return ctl;
}

// ============================================================================
// ComplianceEvidence Implementation
// ============================================================================

nlohmann::json ComplianceEvidence::toJson() const {
    return nlohmann::json{
        {"evidence_id", evidence_id},
        {"control_id", control_id},
        {"requirement_id", requirement_id},
        {"evidence_type", evidence_type},
        {"timestamp_ms", timestamp_ms},
        {"detail", detail},
        {"satisfies_requirement", satisfies_requirement},
        {"metadata", metadata}
    };
}

// ============================================================================
// ComplianceViolation Implementation
// ============================================================================

nlohmann::json ComplianceViolation::toJson() const {
    return nlohmann::json{
        {"violation_id", violation_id},
        {"requirement_id", requirement_id},
        {"control_id", control_id},
        {"framework", static_cast<int>(framework)},
        {"severity", static_cast<int>(severity)},
        {"description", description},
        {"remediation_guidance", remediation_guidance},
        {"detected_at_ms", detected_at_ms},
        {"remediation_deadline_ms", remediation_deadline_ms},
        {"is_remediated", is_remediated},
        {"remediated_at_ms", remediated_at_ms},
        {"remediation_evidence", remediation_evidence}
    };
}

// ============================================================================
// ComplianceContext Implementation
// ============================================================================

nlohmann::json ComplianceContext::toJson() const {
    return nlohmann::json{
        {"system_id", system_id},
        {"enabled_frameworks", enabled_frameworks},
        {"policy_state", policy_state},
        {"control_status", control_status},
        {"evidence_count", static_cast<int>(evidence.size())},
        {"validation_time_ms", validation_time_ms},
        {"metadata", metadata}
    };
}

// ============================================================================
// ComplianceStatusReport Implementation
// ============================================================================

nlohmann::json ComplianceStatusReport::toJson() const {
    nlohmann::json violations_json = nlohmann::json::array();
    for (const auto& v : violations) {
        violations_json.push_back(v.toJson());
    }
    
    nlohmann::json evidence_json = nlohmann::json::array();
    for (const auto& e : evidence) {
        evidence_json.push_back(e.toJson());
    }
    
    return nlohmann::json{
        {"report_id", report_id},
        {"framework", static_cast<int>(framework)},
        {"generated_at_ms", generated_at_ms},
        {"total_requirements", total_requirements},
        {"compliant_requirements", compliant_requirements},
        {"non_compliant_requirements", non_compliant_requirements},
        {"partial_requirements", partial_requirements},
        {"na_requirements", na_requirements},
        {"compliance_score", compliance_score},
        {"overall_status", static_cast<int>(overall_status)},
        {"violations_count", static_cast<int>(violations.size())},
        {"violations", violations_json},
        {"evidence_count", static_cast<int>(evidence.size())},
        {"evidence", evidence_json}
    };
}

// ============================================================================
// ComplianceValidationResult Implementation
// ============================================================================

nlohmann::json ComplianceValidationResult::toJson() const {
    nlohmann::json reports = nlohmann::json::array();
    for (const auto& r : framework_reports) {
        reports.push_back(r.toJson());
    }
    
    nlohmann::json violations = nlohmann::json::array();
    for (const auto& v : all_violations) {
        violations.push_back(v.toJson());
    }
    
    return nlohmann::json{
        {"success", success},
        {"error_message", error_message},
        {"validation_time_ms", validation_time_ms},
        {"elapsed_ms", elapsed_ms},
        {"framework_reports", reports},
        {"violations_count", static_cast<int>(all_violations.size())},
        {"violations", violations}
    };
}

// ============================================================================
// ComplianceFrameworkRegistry Implementation
// ============================================================================

bool ComplianceFrameworkRegistry::addRequirement(const ComplianceRequirement& req) {
    std::lock_guard<std::mutex> lock(mu_);
    
    if (requirements_.count(req.requirement_id) > 0) {
        return false;  // Already exists
    }
    
    requirements_[req.requirement_id] = req;
    requirements_by_framework_[req.framework].push_back(req.requirement_id);
    
    return true;
}

bool ComplianceFrameworkRegistry::addControl(const ComplianceControl& ctl) {
    std::lock_guard<std::mutex> lock(mu_);
    
    if (controls_.count(ctl.control_id) > 0) {
        return false;  // Already exists
    }
    
    controls_[ctl.control_id] = ctl;
    controls_by_framework_[ctl.framework].push_back(ctl.control_id);
    
    return true;
}

std::optional<ComplianceRequirement> ComplianceFrameworkRegistry::getRequirement(
    const std::string& req_id) const {
    std::lock_guard<std::mutex> lock(mu_);
    
    auto it = requirements_.find(req_id);
    if (it != requirements_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<ComplianceControl> ComplianceFrameworkRegistry::getControl(
    const std::string& ctl_id) const {
    std::lock_guard<std::mutex> lock(mu_);
    
    auto it = controls_.find(ctl_id);
    if (it != controls_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<ComplianceRequirement> ComplianceFrameworkRegistry::getRequirements(
    ComplianceFramework fw) const {
    std::lock_guard<std::mutex> lock(mu_);
    
    std::vector<ComplianceRequirement> results;
    
    auto it = requirements_by_framework_.find(fw);
    if (it != requirements_by_framework_.end()) {
        for (const auto& req_id : it->second) {
            auto req_it = requirements_.find(req_id);
            if (req_it != requirements_.end()) {
                results.push_back(req_it->second);
            }
        }
    }
    
    return results;
}

std::vector<ComplianceControl> ComplianceFrameworkRegistry::getControls(
    ComplianceFramework fw) const {
    std::lock_guard<std::mutex> lock(mu_);
    
    std::vector<ComplianceControl> results;
    
    auto it = controls_by_framework_.find(fw);
    if (it != controls_by_framework_.end()) {
        for (const auto& ctl_id : it->second) {
            auto ctl_it = controls_.find(ctl_id);
            if (ctl_it != controls_.end()) {
                results.push_back(ctl_it->second);
            }
        }
    }
    
    return results;
}

std::vector<ComplianceRequirement> ComplianceFrameworkRegistry::getRequirementsByCategory(
    ComplianceFramework fw,
    const std::string& category) const {
    std::lock_guard<std::mutex> lock(mu_);
    
    std::vector<ComplianceRequirement> results;
    
    auto it = requirements_by_framework_.find(fw);
    if (it != requirements_by_framework_.end()) {
        for (const auto& req_id : it->second) {
            auto req_it = requirements_.find(req_id);
            if (req_it != requirements_.end() && req_it->second.category == category) {
                results.push_back(req_it->second);
            }
        }
    }
    
    return results;
}

int ComplianceFrameworkRegistry::getRequirementCount(ComplianceFramework fw) const {
    std::lock_guard<std::mutex> lock(mu_);
    
    auto it = requirements_by_framework_.find(fw);
    if (it != requirements_by_framework_.end()) {
        return it->second.size();
    }
    return 0;
}

int ComplianceFrameworkRegistry::getControlCount(ComplianceFramework fw) const {
    std::lock_guard<std::mutex> lock(mu_);
    
    auto it = controls_by_framework_.find(fw);
    if (it != controls_by_framework_.end()) {
        return it->second.size();
    }
    return 0;
}

nlohmann::json ComplianceFrameworkRegistry::exportToJson(ComplianceFramework fw) const {
    std::lock_guard<std::mutex> lock(mu_);
    
    nlohmann::json requirements_json = nlohmann::json::array();
    nlohmann::json controls_json = nlohmann::json::array();
    
    auto req_it = requirements_by_framework_.find(fw);
    if (req_it != requirements_by_framework_.end()) {
        for (const auto& req_id : req_it->second) {
            auto it = requirements_.find(req_id);
            if (it != requirements_.end()) {
                requirements_json.push_back(it->second.toJson());
            }
        }
    }
    
    auto ctl_it = controls_by_framework_.find(fw);
    if (ctl_it != controls_by_framework_.end()) {
        for (const auto& ctl_id : ctl_it->second) {
            auto it = controls_.find(ctl_id);
            if (it != controls_.end()) {
                controls_json.push_back(it->second.toJson());
            }
        }
    }
    
    return nlohmann::json{
        {"framework", static_cast<int>(fw)},
        {"requirements", requirements_json},
        {"controls", controls_json}
    };
}

bool ComplianceFrameworkRegistry::importFromJson(const nlohmann::json& j) {
    std::lock_guard<std::mutex> lock(mu_);
    
    try {
        if (!j.contains("requirements") || !j.contains("controls")) {
            return false;
        }
        
        for (const auto& req_json : j["requirements"]) {
            auto req = ComplianceRequirement::fromJson(req_json);
            requirements_[req.requirement_id] = req;
            requirements_by_framework_[req.framework].push_back(req.requirement_id);
        }
        
        for (const auto& ctl_json : j["controls"]) {
            auto ctl = ComplianceControl::fromJson(ctl_json);
            controls_[ctl.control_id] = ctl;
            controls_by_framework_[ctl.framework].push_back(ctl.control_id);
        }
        
        return true;
    } catch (...) {
        return false;
    }
}

void ComplianceFrameworkRegistry::clear() {
    std::lock_guard<std::mutex> lock(mu_);
    requirements_.clear();
    controls_.clear();
    requirements_by_framework_.clear();
    controls_by_framework_.clear();
}

} // namespace governance
} // namespace themis
