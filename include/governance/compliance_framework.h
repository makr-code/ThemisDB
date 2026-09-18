/**
 * @file compliance_framework.h
 * @brief Compliance framework integration for ThemisDB governance module
 * @version 1.0.0
 * @date 2026-08-18
 * 
 * @details
 * Provides unified compliance requirement mapping and validation for multiple
 * regulatory frameworks:
 * - EU AI Act
 * - SOC 2 Type I/II
 * - ISO 27001:2022
 * - GDPR
 * - CCPA/CPRA
 * - HIPAA
 * - PCI-DSS
 * 
 * Supports:
 * - Framework → Requirement → Control mapping with versioning
 * - Regulatory section to technical control linkage
 * - Multi-framework simultaneous validation
 * - Compliance violation reporting with severity
 * - Evidence collection and remediation tracking
 * 
 * @note Production-ready with performance optimization
 * @note Performance targets:
 *       - Validation check latency ≤1s
 *       - Report generation ≤5s
 *       - Framework query ≤100ms
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <memory>
#include <nlohmann/json.hpp>
#include <chrono>
#include <optional>
#include <mutex>

namespace themis {
namespace governance {

// ============================================================================
// Type Definitions & Enums
// ============================================================================

enum class ComplianceFramework {
    kEuAiAct,      ///< EU AI Act (2024)
    kSoc2TypeI,    ///< SOC 2 Type I
    kSoc2TypeII,   ///< SOC 2 Type II
    kIso27001,     ///< ISO 27001:2022
    kGdpr,         ///< General Data Protection Regulation
    kCcpa,         ///< California Consumer Privacy Act
    kHipaa,        ///< Health Insurance Portability and Accountability Act
    kPciDss,       ///< Payment Card Industry Data Security Standard
};

enum class ComplianceStatus {
    kCompliant,           ///< Requirement fully met
    kNonCompliant,        ///< Requirement not met
    kPartiallyCompliant,  ///< Partially implemented
    kNotApplicable,       ///< Not applicable to system
    kPendingReview,       ///< Awaiting assessment
};

enum class ComplianceSeverity {
    kCritical,  ///< Must fix immediately
    kHigh,      ///< Should fix soon
    kMedium,    ///< Address in regular maintenance
    kLow,       ///< Consider for improvement
};

// ============================================================================
// Compliance Requirement Definition
// ============================================================================

struct ComplianceRequirement {
    std::string requirement_id;           ///< Unique ID (e.g., "GDPR-A.32.1")
    ComplianceFramework framework;        ///< Source framework
    std::string requirement_text;         ///< Full requirement description
    std::string regulatory_section;       ///< Reference section (e.g., "Article 32")
    ComplianceSeverity severity;          ///< Importance level
    std::vector<std::string> control_ids; ///< Linked control IDs
    std::string category;                 ///< Domain (e.g., "encryption", "access-control")
    bool is_mandatory = true;             ///< Whether requirement is mandatory
    int version = 1;                      ///< Requirement version
    int64_t created_at_ms = 0;            ///< Creation timestamp
    int64_t updated_at_ms = 0;            ///< Last update timestamp
    nlohmann::json metadata;              ///< Additional metadata
    
    /**
     * @brief To Json.
     * @return Return value.
     */
    nlohmann::json toJson() const;
    
    /**
     * @brief From Json.
     * @param[in] j Input parameter.
     * @return Return value.
     */
    static ComplianceRequirement fromJson(const nlohmann::json& j);
};

// ============================================================================
// Compliance Control Definition
// ============================================================================

struct ComplianceControl {
    std::string control_id;               ///< Unique control ID (e.g., "CTL-ENCRYPTION-001")
    ComplianceFramework framework;        ///< Source framework
    std::string control_name;             ///< Display name
    std::string description;              ///< Technical description
    std::string implementation_detail;    ///< How it's implemented
    bool is_automated = true;             ///< Whether control is automated
    std::vector<std::string> policy_rules;///< Associated policy rules
    std::vector<std::string> evidence_types; ///< Types of evidence collected
    int version = 1;                      ///< Control version
    int64_t created_at_ms = 0;            ///< Creation timestamp
    
    /**
     * @brief To Json.
     * @return Return value.
     */
    nlohmann::json toJson() const;
    
    /**
     * @brief From Json.
     * @param[in] j Input parameter.
     * @return Return value.
     */
    static ComplianceControl fromJson(const nlohmann::json& j);
};

// ============================================================================
// Compliance Evidence
// ============================================================================

struct ComplianceEvidence {
    std::string evidence_id;         ///< Unique evidence ID
    std::string control_id;          ///< Associated control
    std::string requirement_id;      ///< Associated requirement
    std::string evidence_type;       ///< Type (policy_rule, access_log, encryption_status, etc.)
    int64_t timestamp_ms = 0;        ///< When evidence was collected
    std::string detail;              ///< Evidence description
    bool satisfies_requirement = true; ///< Whether evidence satisfies requirement
    nlohmann::json metadata;         ///< Additional metadata
    
    /**
     * @brief To Json.
     * @return Return value.
     */
    nlohmann::json toJson() const;
};

// ============================================================================
// Compliance Violation
// ============================================================================

struct ComplianceViolation {
    std::string violation_id;               ///< Unique violation ID
    std::string requirement_id;             ///< Non-compliant requirement
    std::string control_id;                 ///< Failed control
    ComplianceFramework framework;          ///< Source framework
    ComplianceSeverity severity;            ///< Violation severity
    std::string description;                ///< What's not compliant
    std::string remediation_guidance;       ///< How to fix
    int64_t detected_at_ms = 0;             ///< When detected
    int64_t remediation_deadline_ms = 0;    ///< When to fix by
    bool is_remediated = false;             ///< Whether fixed
    int64_t remediated_at_ms = 0;           ///< When fixed
    std::string remediation_evidence;       ///< Evidence of remediation
    
    /**
     * @brief To Json.
     * @return Return value.
     */
    nlohmann::json toJson() const;
};

// ============================================================================
// Compliance Framework Registry
// ============================================================================

class ComplianceFrameworkRegistry {
public:
    ComplianceFrameworkRegistry() = default;
    ~ComplianceFrameworkRegistry() = default;
    
    /**
     * @brief Add Requirement.
     * @param[in] req Input parameter.
     * @return True when the operation succeeds.
     */
    bool addRequirement(const ComplianceRequirement& req);
    
    /**
     * @brief Add Control.
     * @param[in] ctl Input parameter.
     * @return True when the operation succeeds.
     */
    bool addControl(const ComplianceControl& ctl);
    
    /**
     * @brief Get Requirement.
     * @param[in] req_id Identifier of the req.
     * @return Return value.
     */
    std::optional<ComplianceRequirement> getRequirement(const std::string& req_id) const;
    
    /**
     * @brief Get Control.
     * @param[in] ctl_id Identifier of the ctl.
     * @return Return value.
     */
    std::optional<ComplianceControl> getControl(const std::string& ctl_id) const;
    
    /**
     * @brief Get Requirements.
     * @param[in] fw Input parameter.
     * @return Return value.
     */
    std::vector<ComplianceRequirement> getRequirements(ComplianceFramework fw) const;
    
    /**
     * @brief Get Controls.
     * @param[in] fw Input parameter.
     * @return Return value.
     */
    std::vector<ComplianceControl> getControls(ComplianceFramework fw) const;
    
    /**
     * @brief Get Requirements By Category.
     * @param[in] fw Input parameter.
     * @param[in] category Input parameter.
     * @return Return value.
     */
    std::vector<ComplianceRequirement> getRequirementsByCategory(
        ComplianceFramework fw,
        const std::string& category) const;
    
    /**
     * @brief Get Requirement Count.
     * @param[in] fw Input parameter.
     * @return Return value.
     */
    int getRequirementCount(ComplianceFramework fw) const;
    
    /**
     * @brief Get Control Count.
     * @param[in] fw Input parameter.
     * @return Return value.
     */
    int getControlCount(ComplianceFramework fw) const;
    
    /**
     * @brief Export To Json.
     * @param[in] fw Input parameter.
     * @return Return value.
     */
    nlohmann::json exportToJson(ComplianceFramework fw) const;
    
    /**
     * @brief Import From Json.
     * @param[in] j Input parameter.
     * @return True when the operation succeeds.
     */
    bool importFromJson(const nlohmann::json& j);
    
    /**
     * @brief Clear.
     */
    void clear();

private:
    mutable std::mutex mu_;
    
    std::unordered_map<std::string, ComplianceRequirement> requirements_;
    
    std::unordered_map<std::string, ComplianceControl> controls_;
    
    std::map<ComplianceFramework, std::vector<std::string>> requirements_by_framework_;
    
    std::map<ComplianceFramework, std::vector<std::string>> controls_by_framework_;
};

// ============================================================================
// Compliance Context
// ============================================================================

struct ComplianceContext {
    std::string system_id;                    ///< System being validated
    std::vector<std::string> enabled_frameworks; ///< Frameworks to validate
    std::unordered_map<std::string, std::string> policy_state; ///< Current policies
    std::unordered_map<std::string, bool> control_status; ///< Control implementation status
    std::vector<ComplianceEvidence> evidence; ///< Collected evidence
    int64_t validation_time_ms = 0;           ///< When validation occurred
    nlohmann::json metadata;                  ///< Additional context
    
    /**
     * @brief To Json.
     * @return Return value.
     */
    nlohmann::json toJson() const;
};

// ============================================================================
// Compliance Status Report
// ============================================================================

struct ComplianceStatusReport {
    std::string report_id;
    ComplianceFramework framework;
    int64_t generated_at_ms = 0;
    
    int total_requirements = 0;
    int compliant_requirements = 0;
    int non_compliant_requirements = 0;
    int partial_requirements = 0;
    int na_requirements = 0;
    
    double compliance_score = 0.0;  ///< 0–100 score
    ComplianceStatus overall_status = ComplianceStatus::kPendingReview;
    
    std::vector<ComplianceViolation> violations;
    std::vector<ComplianceEvidence> evidence;
    
    /**
     * @brief To Json.
     * @return Return value.
     */
    nlohmann::json toJson() const;
};

// ============================================================================
// Compliance Validation Result
// ============================================================================

struct ComplianceValidationResult {
    bool success = true;
    std::string error_message;
    int64_t validation_time_ms = 0;
    int64_t elapsed_ms = 0;  ///< Validation duration
    
    std::vector<ComplianceStatusReport> framework_reports;
    std::vector<ComplianceViolation> all_violations;
    
    /**
     * @brief To Json.
     * @return Return value.
     */
    nlohmann::json toJson() const;
};

} // namespace governance
} // namespace themis
