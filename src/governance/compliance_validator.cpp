/**
 * @file compliance_validator.cpp
 * @brief Compliance validation engine implementation
 * @version 1.0.0
 * @date 2026-08-18
 * 
 * Implements validators for all supported compliance frameworks.
 */

#include "governance/compliance_validator.h"
#include "utils/logger.h"
#include "utils/uuid.h"
#include <algorithm>
#include <chrono>
#include <sstream>

namespace themis {
namespace governance {

namespace {
    /// Generate unique ID for evidence/violations
    inline std::string generateUuid() {
        return themis::utils::generate_uuid_v4();
    }
}

// ============================================================================
// ISO 27001 Validator Implementation
// ============================================================================

ComplianceFramework Iso27001Validator::getFramework() const {
    return ComplianceFramework::kIso27001;
}

ComplianceStatus Iso27001Validator::validateRequirement(
    const ComplianceRequirement& req,
    const ComplianceContext& ctx) {
    
    if (req.framework != ComplianceFramework::kIso27001) {
        return ComplianceStatus::kNotApplicable;
    }
    
    // Validate controls linked to this requirement
    bool all_controls_met = true;
    bool any_control_met = false;
    
    for (const auto& control_id : req.control_ids) {
        // Check if control is implemented
        auto status_it = ctx.control_status.find(control_id);
        if (status_it != ctx.control_status.end() && status_it->second) {
            any_control_met = true;
        } else {
            all_controls_met = false;
        }
    }
    
    if (all_controls_met && !req.control_ids.empty()) {
        return ComplianceStatus::kCompliant;
    } else if (any_control_met) {
        return ComplianceStatus::kPartiallyCompliant;
    } else {
        return ComplianceStatus::kNonCompliant;
    }
}

ComplianceStatus Iso27001Validator::validateControl(
    const ComplianceControl& ctl,
    const ComplianceContext& ctx) {
    
    if (ctl.framework != ComplianceFramework::kIso27001) {
        return ComplianceStatus::kNotApplicable;
    }
    
    // Check if control is in policy state
    for (const auto& policy_rule : ctl.policy_rules) {
        auto it = ctx.policy_state.find(policy_rule);
        if (it != ctx.policy_state.end() && !it->second.empty()) {
            return ComplianceStatus::kCompliant;
        }
    }
    
    return ComplianceStatus::kNonCompliant;
}

std::vector<ComplianceEvidence> Iso27001Validator::collectEvidence(
    const ComplianceControl& ctl,
    const ComplianceContext& ctx) {
    
    std::vector<ComplianceEvidence> evidence;
    
    for (const auto& evidence_type : ctl.evidence_types) {
        ComplianceEvidence e;
        e.evidence_id = generateUuid();
        e.control_id = ctl.control_id;
        e.evidence_type = evidence_type;
        e.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        e.detail = "Collected " + evidence_type + " for control " + ctl.control_id;
        e.satisfies_requirement = true;
        
        evidence.push_back(e);
    }
    
    return evidence;
}

bool Iso27001Validator::checkPolicySatisfaction(
    const ComplianceRequirement& req,
    const ComplianceContext& ctx) {
    
    return validateRequirement(req, ctx) == ComplianceStatus::kCompliant;
}

// ============================================================================
// SOC 2 Validator Implementation
// ============================================================================

ComplianceFramework Soc2Validator::getFramework() const {
    return ComplianceFramework::kSoc2TypeI;
}

ComplianceStatus Soc2Validator::validateRequirement(
    const ComplianceRequirement& req,
    const ComplianceContext& ctx) {
    
    if (req.framework != ComplianceFramework::kSoc2TypeI &&
        req.framework != ComplianceFramework::kSoc2TypeII) {
        return ComplianceStatus::kNotApplicable;
    }
    
    // SOC 2 validation logic
    bool all_controls_met = true;
    bool any_control_met = false;
    
    for (const auto& control_id : req.control_ids) {
        auto status_it = ctx.control_status.find(control_id);
        if (status_it != ctx.control_status.end() && status_it->second) {
            any_control_met = true;
        } else {
            all_controls_met = false;
        }
    }
    
    if (all_controls_met && !req.control_ids.empty()) {
        return ComplianceStatus::kCompliant;
    } else if (any_control_met) {
        return ComplianceStatus::kPartiallyCompliant;
    } else {
        return ComplianceStatus::kNonCompliant;
    }
}

ComplianceStatus Soc2Validator::validateControl(
    const ComplianceControl& ctl,
    const ComplianceContext& ctx) {
    
    if (ctl.framework != ComplianceFramework::kSoc2TypeI &&
        ctl.framework != ComplianceFramework::kSoc2TypeII) {
        return ComplianceStatus::kNotApplicable;
    }
    
    for (const auto& policy_rule : ctl.policy_rules) {
        auto it = ctx.policy_state.find(policy_rule);
        if (it != ctx.policy_state.end() && !it->second.empty()) {
            return ComplianceStatus::kCompliant;
        }
    }
    
    return ComplianceStatus::kNonCompliant;
}

std::vector<ComplianceEvidence> Soc2Validator::collectEvidence(
    const ComplianceControl& ctl,
    const ComplianceContext& ctx) {
    
    std::vector<ComplianceEvidence> evidence;
    
    for (const auto& evidence_type : ctl.evidence_types) {
        ComplianceEvidence e;
        e.evidence_id = generateUuid();
        e.control_id = ctl.control_id;
        e.evidence_type = evidence_type;
        e.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        e.detail = "SOC 2 control evidence: " + evidence_type;
        e.satisfies_requirement = true;
        
        evidence.push_back(e);
    }
    
    return evidence;
}

bool Soc2Validator::checkPolicySatisfaction(
    const ComplianceRequirement& req,
    const ComplianceContext& ctx) {
    
    return validateRequirement(req, ctx) == ComplianceStatus::kCompliant;
}

// ============================================================================
// GDPR Validator Implementation
// ============================================================================

ComplianceFramework GdprValidator::getFramework() const {
    return ComplianceFramework::kGdpr;
}

ComplianceStatus GdprValidator::validateRequirement(
    const ComplianceRequirement& req,
    const ComplianceContext& ctx) {
    
    if (req.framework != ComplianceFramework::kGdpr) {
        return ComplianceStatus::kNotApplicable;
    }
    
    bool all_controls_met = true;
    bool any_control_met = false;
    
    for (const auto& control_id : req.control_ids) {
        auto status_it = ctx.control_status.find(control_id);
        if (status_it != ctx.control_status.end() && status_it->second) {
            any_control_met = true;
        } else {
            all_controls_met = false;
        }
    }
    
    if (all_controls_met && !req.control_ids.empty()) {
        return ComplianceStatus::kCompliant;
    } else if (any_control_met) {
        return ComplianceStatus::kPartiallyCompliant;
    } else {
        return ComplianceStatus::kNonCompliant;
    }
}

ComplianceStatus GdprValidator::validateControl(
    const ComplianceControl& ctl,
    const ComplianceContext& ctx) {
    
    if (ctl.framework != ComplianceFramework::kGdpr) {
        return ComplianceStatus::kNotApplicable;
    }
    
    for (const auto& policy_rule : ctl.policy_rules) {
        auto it = ctx.policy_state.find(policy_rule);
        if (it != ctx.policy_state.end() && !it->second.empty()) {
            return ComplianceStatus::kCompliant;
        }
    }
    
    return ComplianceStatus::kNonCompliant;
}

std::vector<ComplianceEvidence> GdprValidator::collectEvidence(
    const ComplianceControl& ctl,
    const ComplianceContext& ctx) {
    
    std::vector<ComplianceEvidence> evidence;
    
    for (const auto& evidence_type : ctl.evidence_types) {
        ComplianceEvidence e;
        e.evidence_id = generateUuid();
        e.control_id = ctl.control_id;
        e.evidence_type = evidence_type;
        e.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        e.detail = "GDPR compliance evidence: " + evidence_type;
        e.satisfies_requirement = true;
        
        evidence.push_back(e);
    }
    
    return evidence;
}

bool GdprValidator::checkPolicySatisfaction(
    const ComplianceRequirement& req,
    const ComplianceContext& ctx) {
    
    return validateRequirement(req, ctx) == ComplianceStatus::kCompliant;
}

// ============================================================================
// CCPA Validator Implementation
// ============================================================================

ComplianceFramework CcpaValidator::getFramework() const {
    return ComplianceFramework::kCcpa;
}

ComplianceStatus CcpaValidator::validateRequirement(
    const ComplianceRequirement& req,
    const ComplianceContext& ctx) {
    
    if (req.framework != ComplianceFramework::kCcpa) {
        return ComplianceStatus::kNotApplicable;
    }
    
    bool all_controls_met = true;
    bool any_control_met = false;
    
    for (const auto& control_id : req.control_ids) {
        auto status_it = ctx.control_status.find(control_id);
        if (status_it != ctx.control_status.end() && status_it->second) {
            any_control_met = true;
        } else {
            all_controls_met = false;
        }
    }
    
    if (all_controls_met && !req.control_ids.empty()) {
        return ComplianceStatus::kCompliant;
    } else if (any_control_met) {
        return ComplianceStatus::kPartiallyCompliant;
    } else {
        return ComplianceStatus::kNonCompliant;
    }
}

ComplianceStatus CcpaValidator::validateControl(
    const ComplianceControl& ctl,
    const ComplianceContext& ctx) {
    
    if (ctl.framework != ComplianceFramework::kCcpa) {
        return ComplianceStatus::kNotApplicable;
    }
    
    for (const auto& policy_rule : ctl.policy_rules) {
        auto it = ctx.policy_state.find(policy_rule);
        if (it != ctx.policy_state.end() && !it->second.empty()) {
            return ComplianceStatus::kCompliant;
        }
    }
    
    return ComplianceStatus::kNonCompliant;
}

std::vector<ComplianceEvidence> CcpaValidator::collectEvidence(
    const ComplianceControl& ctl,
    const ComplianceContext& ctx) {
    
    std::vector<ComplianceEvidence> evidence;
    
    for (const auto& evidence_type : ctl.evidence_types) {
        ComplianceEvidence e;
        e.evidence_id = generateUuid();
        e.control_id = ctl.control_id;
        e.evidence_type = evidence_type;
        e.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        e.detail = "CCPA compliance evidence: " + evidence_type;
        e.satisfies_requirement = true;
        
        evidence.push_back(e);
    }
    
    return evidence;
}

bool CcpaValidator::checkPolicySatisfaction(
    const ComplianceRequirement& req,
    const ComplianceContext& ctx) {
    
    return validateRequirement(req, ctx) == ComplianceStatus::kCompliant;
}

// ============================================================================
// HIPAA Validator Implementation
// ============================================================================

ComplianceFramework HipaaValidator::getFramework() const {
    return ComplianceFramework::kHipaa;
}

ComplianceStatus HipaaValidator::validateRequirement(
    const ComplianceRequirement& req,
    const ComplianceContext& ctx) {
    
    if (req.framework != ComplianceFramework::kHipaa) {
        return ComplianceStatus::kNotApplicable;
    }
    
    bool all_controls_met = true;
    bool any_control_met = false;
    
    for (const auto& control_id : req.control_ids) {
        auto status_it = ctx.control_status.find(control_id);
        if (status_it != ctx.control_status.end() && status_it->second) {
            any_control_met = true;
        } else {
            all_controls_met = false;
        }
    }
    
    if (all_controls_met && !req.control_ids.empty()) {
        return ComplianceStatus::kCompliant;
    } else if (any_control_met) {
        return ComplianceStatus::kPartiallyCompliant;
    } else {
        return ComplianceStatus::kNonCompliant;
    }
}

ComplianceStatus HipaaValidator::validateControl(
    const ComplianceControl& ctl,
    const ComplianceContext& ctx) {
    
    if (ctl.framework != ComplianceFramework::kHipaa) {
        return ComplianceStatus::kNotApplicable;
    }
    
    for (const auto& policy_rule : ctl.policy_rules) {
        auto it = ctx.policy_state.find(policy_rule);
        if (it != ctx.policy_state.end() && !it->second.empty()) {
            return ComplianceStatus::kCompliant;
        }
    }
    
    return ComplianceStatus::kNonCompliant;
}

std::vector<ComplianceEvidence> HipaaValidator::collectEvidence(
    const ComplianceControl& ctl,
    const ComplianceContext& ctx) {
    
    std::vector<ComplianceEvidence> evidence;
    
    for (const auto& evidence_type : ctl.evidence_types) {
        ComplianceEvidence e;
        e.evidence_id = generateUuid();
        e.control_id = ctl.control_id;
        e.evidence_type = evidence_type;
        e.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        e.detail = "HIPAA compliance evidence: " + evidence_type;
        e.satisfies_requirement = true;
        
        evidence.push_back(e);
    }
    
    return evidence;
}

bool HipaaValidator::checkPolicySatisfaction(
    const ComplianceRequirement& req,
    const ComplianceContext& ctx) {
    
    return validateRequirement(req, ctx) == ComplianceStatus::kCompliant;
}

// ============================================================================
// PCI-DSS Validator Implementation
// ============================================================================

ComplianceFramework PciDssValidator::getFramework() const {
    return ComplianceFramework::kPciDss;
}

ComplianceStatus PciDssValidator::validateRequirement(
    const ComplianceRequirement& req,
    const ComplianceContext& ctx) {
    
    if (req.framework != ComplianceFramework::kPciDss) {
        return ComplianceStatus::kNotApplicable;
    }
    
    bool all_controls_met = true;
    bool any_control_met = false;
    
    for (const auto& control_id : req.control_ids) {
        auto status_it = ctx.control_status.find(control_id);
        if (status_it != ctx.control_status.end() && status_it->second) {
            any_control_met = true;
        } else {
            all_controls_met = false;
        }
    }
    
    if (all_controls_met && !req.control_ids.empty()) {
        return ComplianceStatus::kCompliant;
    } else if (any_control_met) {
        return ComplianceStatus::kPartiallyCompliant;
    } else {
        return ComplianceStatus::kNonCompliant;
    }
}

ComplianceStatus PciDssValidator::validateControl(
    const ComplianceControl& ctl,
    const ComplianceContext& ctx) {
    
    if (ctl.framework != ComplianceFramework::kPciDss) {
        return ComplianceStatus::kNotApplicable;
    }
    
    for (const auto& policy_rule : ctl.policy_rules) {
        auto it = ctx.policy_state.find(policy_rule);
        if (it != ctx.policy_state.end() && !it->second.empty()) {
            return ComplianceStatus::kCompliant;
        }
    }
    
    return ComplianceStatus::kNonCompliant;
}

std::vector<ComplianceEvidence> PciDssValidator::collectEvidence(
    const ComplianceControl& ctl,
    const ComplianceContext& ctx) {
    
    std::vector<ComplianceEvidence> evidence;
    
    for (const auto& evidence_type : ctl.evidence_types) {
        ComplianceEvidence e;
        e.evidence_id = generateUuid();
        e.control_id = ctl.control_id;
        e.evidence_type = evidence_type;
        e.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        e.detail = "PCI-DSS compliance evidence: " + evidence_type;
        e.satisfies_requirement = true;
        
        evidence.push_back(e);
    }
    
    return evidence;
}

bool PciDssValidator::checkPolicySatisfaction(
    const ComplianceRequirement& req,
    const ComplianceContext& ctx) {
    
    return validateRequirement(req, ctx) == ComplianceStatus::kCompliant;
}

// ============================================================================
// EU AI Act Validator Implementation
// ============================================================================

ComplianceFramework EuAiActValidator::getFramework() const {
    return ComplianceFramework::kEuAiAct;
}

ComplianceStatus EuAiActValidator::validateRequirement(
    const ComplianceRequirement& req,
    const ComplianceContext& ctx) {
    
    if (req.framework != ComplianceFramework::kEuAiAct) {
        return ComplianceStatus::kNotApplicable;
    }
    
    bool all_controls_met = true;
    bool any_control_met = false;
    
    for (const auto& control_id : req.control_ids) {
        auto status_it = ctx.control_status.find(control_id);
        if (status_it != ctx.control_status.end() && status_it->second) {
            any_control_met = true;
        } else {
            all_controls_met = false;
        }
    }
    
    if (all_controls_met && !req.control_ids.empty()) {
        return ComplianceStatus::kCompliant;
    } else if (any_control_met) {
        return ComplianceStatus::kPartiallyCompliant;
    } else {
        return ComplianceStatus::kNonCompliant;
    }
}

ComplianceStatus EuAiActValidator::validateControl(
    const ComplianceControl& ctl,
    const ComplianceContext& ctx) {
    
    if (ctl.framework != ComplianceFramework::kEuAiAct) {
        return ComplianceStatus::kNotApplicable;
    }
    
    for (const auto& policy_rule : ctl.policy_rules) {
        auto it = ctx.policy_state.find(policy_rule);
        if (it != ctx.policy_state.end() && !it->second.empty()) {
            return ComplianceStatus::kCompliant;
        }
    }
    
    return ComplianceStatus::kNonCompliant;
}

std::vector<ComplianceEvidence> EuAiActValidator::collectEvidence(
    const ComplianceControl& ctl,
    const ComplianceContext& ctx) {
    
    std::vector<ComplianceEvidence> evidence;
    
    for (const auto& evidence_type : ctl.evidence_types) {
        ComplianceEvidence e;
        e.evidence_id = generateUuid();
        e.control_id = ctl.control_id;
        e.evidence_type = evidence_type;
        e.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        e.detail = "EU AI Act compliance evidence: " + evidence_type;
        e.satisfies_requirement = true;
        
        evidence.push_back(e);
    }
    
    return evidence;
}

bool EuAiActValidator::checkPolicySatisfaction(
    const ComplianceRequirement& req,
    const ComplianceContext& ctx) {
    
    return validateRequirement(req, ctx) == ComplianceStatus::kCompliant;
}

// ============================================================================
// Compliance Validation Engine Implementation
// ============================================================================

ComplianceValidationEngine::ComplianceValidationEngine() {
    // Register all validators
    registerValidator(std::make_unique<Iso27001Validator>());
    registerValidator(std::make_unique<Soc2Validator>());
    registerValidator(std::make_unique<GdprValidator>());
    registerValidator(std::make_unique<CcpaValidator>());
    registerValidator(std::make_unique<HipaaValidator>());
    registerValidator(std::make_unique<PciDssValidator>());
    registerValidator(std::make_unique<EuAiActValidator>());
}

void ComplianceValidationEngine::registerValidator(
    std::unique_ptr<IComplianceValidator> validator) {
    std::lock_guard<std::mutex> lock(mu_);
    validators_[static_cast<int>(validator->getFramework())] = std::move(validator);
}

ComplianceStatus ComplianceValidationEngine::validateRequirement(
    const ComplianceRequirement& req,
    const ComplianceContext& ctx) {
    
    auto* validator = getValidator(req.framework);
    if (!validator) {
        return ComplianceStatus::kNotApplicable;
    }
    
    return validator->validateRequirement(req, ctx);
}

ComplianceStatus ComplianceValidationEngine::validateControl(
    const ComplianceControl& ctl,
    const ComplianceContext& ctx) {
    
    auto* validator = getValidator(ctl.framework);
    if (!validator) {
        return ComplianceStatus::kNotApplicable;
    }
    
    return validator->validateControl(ctl, ctx);
}

ComplianceStatusReport ComplianceValidationEngine::validateFramework(
    ComplianceFramework fw,
    const ComplianceFrameworkRegistry& registry,
    const ComplianceContext& ctx) {
    
    ComplianceStatusReport report;
    report.report_id = generateUuid();
    report.framework = fw;
    report.generated_at_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    auto requirements = registry.getRequirements(fw);
    report.total_requirements = requirements.size();
    
    int compliant = 0, non_compliant = 0, partial = 0, na = 0;
    
    for (const auto& req : requirements) {
        auto status = validateRequirement(req, ctx);
        
        switch (status) {
            case ComplianceStatus::kCompliant:
                compliant++;
                break;
            case ComplianceStatus::kNonCompliant: {
                non_compliant++;
                ComplianceViolation violation;
                violation.violation_id = generateUuid();
                violation.requirement_id = req.requirement_id;
                violation.framework = fw;
                violation.severity = req.severity;
                violation.description = req.requirement_text;
                violation.detected_at_ms = report.generated_at_ms;
                report.violations.push_back(violation);
                break;
            }
            case ComplianceStatus::kPartiallyCompliant:
                partial++;
                break;
            case ComplianceStatus::kNotApplicable:
                na++;
                break;
            default:
                break;
        }
    }
    
    report.compliant_requirements = compliant;
    report.non_compliant_requirements = non_compliant;
    report.partial_requirements = partial;
    report.na_requirements = na;
    
    if (report.total_requirements > 0) {
        report.compliance_score = (100.0 * compliant) / report.total_requirements;
    }
    
    if (report.compliance_score >= 95.0) {
        report.overall_status = ComplianceStatus::kCompliant;
    } else if (report.compliance_score >= 50.0) {
        report.overall_status = ComplianceStatus::kPartiallyCompliant;
    } else {
        report.overall_status = ComplianceStatus::kNonCompliant;
    }
    
    return report;
}

ComplianceValidationResult ComplianceValidationEngine::validateAll(
    const std::vector<ComplianceFramework>& frameworks,
    const ComplianceFrameworkRegistry& registry,
    const ComplianceContext& ctx) {
    
    auto start = std::chrono::high_resolution_clock::now();
    
    ComplianceValidationResult result;
    result.validation_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    for (const auto& fw : frameworks) {
        auto report = validateFramework(fw, registry, ctx);
        result.framework_reports.push_back(report);
        
        for (const auto& violation : report.violations) {
            result.all_violations.push_back(violation);
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    result.elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        end - start).count();
    
    result.success = result.all_violations.empty();
    
    return result;
}

std::vector<ComplianceViolation> ComplianceValidationEngine::detectViolations(
    const ComplianceValidationResult& result) {
    
    std::vector<ComplianceViolation> violations;
    
    for (const auto& violation : result.all_violations) {
        if (violation.is_remediated == false) {
            violations.push_back(violation);
        }
    }
    
    return violations;
}

std::string ComplianceValidationEngine::generateRemediationGuidance(
    const ComplianceViolation& violation) {
    
    std::ostringstream oss = {};
    oss << "Remediation for " << violation.requirement_id << ":\n";
    oss << "Violation: " << violation.description << "\n";
    
    switch (violation.severity) {
        case ComplianceSeverity::kCritical:
            oss << "Severity: CRITICAL - Address immediately\n";
            break;
        case ComplianceSeverity::kHigh:
            oss << "Severity: HIGH - Address within 7 days\n";
            break;
        case ComplianceSeverity::kMedium:
            oss << "Severity: MEDIUM - Address within 30 days\n";
            break;
        case ComplianceSeverity::kLow:
            oss << "Severity: LOW - Address in next maintenance cycle\n";
            break;
    }
    
    oss << "Recommended Actions:\n";
    oss << "1. Review the requirement: " << violation.requirement_id << "\n";
    oss << "2. Implement or verify the control: " << violation.control_id << "\n";
    oss << "3. Collect evidence of implementation\n";
    oss << "4. Mark remediation as complete\n";
    
    return oss.str();
}

IComplianceValidator* ComplianceValidationEngine::getValidator(
    ComplianceFramework fw) {
    
    std::lock_guard<std::mutex> lock(mu_);
    auto it = validators_.find(static_cast<int>(fw));
    if (it != validators_.end()) {
        return it->second.get();
    }
    return nullptr;
}

} // namespace governance
} // namespace themis
