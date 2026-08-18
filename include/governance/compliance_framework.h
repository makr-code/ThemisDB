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

/// Supported compliance frameworks
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

/// Compliance status for a requirement or control
enum class ComplianceStatus {
    kCompliant,           ///< Requirement fully met
    kNonCompliant,        ///< Requirement not met
    kPartiallyCompliant,  ///< Partially implemented
    kNotApplicable,       ///< Not applicable to system
    kPendingReview,       ///< Awaiting assessment
};

/// Severity level for compliance violation
enum class ComplianceSeverity {
    kCritical,  ///< Must fix immediately
    kHigh,      ///< Should fix soon
    kMedium,    ///< Address in regular maintenance
    kLow,       ///< Consider for improvement
};

// ============================================================================
// Compliance Requirement Definition
// ============================================================================

/**
 * @brief Represents a single compliance requirement from a framework
 * 
 * Maps regulatory requirement to one or more technical controls
 * with evidence collection and remediation tracking.
 */
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
    
    /// Convert to JSON representation
    nlohmann::json toJson() const;
    
    /// Create from JSON representation
    static ComplianceRequirement fromJson(const nlohmann::json& j);
};

// ============================================================================
// Compliance Control Definition
// ============================================================================

/**
 * @brief Represents a technical control implementing a requirement
 * 
 * Controls are specific technical or procedural measures that satisfy
 * one or more compliance requirements.
 */
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
    
    /// Convert to JSON representation
    nlohmann::json toJson() const;
    
    /// Create from JSON representation
    static ComplianceControl fromJson(const nlohmann::json& j);
};

// ============================================================================
// Compliance Evidence
// ============================================================================

/**
 * @brief Evidence item supporting compliance claim
 * 
 * Records specific evidence that a control is implemented or a requirement
 * is satisfied, with timestamp and optional metadata.
 */
struct ComplianceEvidence {
    std::string evidence_id;         ///< Unique evidence ID
    std::string control_id;          ///< Associated control
    std::string requirement_id;      ///< Associated requirement
    std::string evidence_type;       ///< Type (policy_rule, access_log, encryption_status, etc.)
    int64_t timestamp_ms = 0;        ///< When evidence was collected
    std::string detail;              ///< Evidence description
    bool satisfies_requirement = true; ///< Whether evidence satisfies requirement
    nlohmann::json metadata;         ///< Additional metadata
    
    /// Convert to JSON representation
    nlohmann::json toJson() const;
};

// ============================================================================
// Compliance Violation
// ============================================================================

/**
 * @brief Represents a compliance violation
 * 
 * When a requirement is not met, a violation is recorded with
 * remediation guidance and deadline.
 */
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
    
    /// Convert to JSON representation
    nlohmann::json toJson() const;
};

// ============================================================================
// Compliance Framework Registry
// ============================================================================

/**
 * @brief Registry of all compliance requirements and controls for a framework
 * 
 * Maintains complete mapping of requirements to controls with
 * versioning support.
 */
class ComplianceFrameworkRegistry {
public:
    ComplianceFrameworkRegistry() = default;
    ~ComplianceFrameworkRegistry() = default;
    
    /// Add a requirement to the registry
    bool addRequirement(const ComplianceRequirement& req);
    
    /// Add a control to the registry
    bool addControl(const ComplianceControl& ctl);
    
    /// Get requirement by ID
    std::optional<ComplianceRequirement> getRequirement(const std::string& req_id) const;
    
    /// Get control by ID
    std::optional<ComplianceControl> getControl(const std::string& ctl_id) const;
    
    /// List all requirements for a framework
    std::vector<ComplianceRequirement> getRequirements(ComplianceFramework fw) const;
    
    /// List all controls for a framework
    std::vector<ComplianceControl> getControls(ComplianceFramework fw) const;
    
    /// Get requirements by category
    std::vector<ComplianceRequirement> getRequirementsByCategory(
        ComplianceFramework fw,
        const std::string& category) const;
    
    /// Get total requirement count for framework
    int getRequirementCount(ComplianceFramework fw) const;
    
    /// Get total control count for framework
    int getControlCount(ComplianceFramework fw) const;
    
    /// Export framework as JSON
    nlohmann::json exportToJson(ComplianceFramework fw) const;
    
    /// Import framework from JSON
    bool importFromJson(const nlohmann::json& j);
    
    /// Clear all data
    void clear();

private:
    mutable std::mutex mu_;
    
    /// Requirements indexed by ID
    std::unordered_map<std::string, ComplianceRequirement> requirements_;
    
    /// Controls indexed by ID
    std::unordered_map<std::string, ComplianceControl> controls_;
    
    /// Requirement IDs by framework
    std::map<ComplianceFramework, std::vector<std::string>> requirements_by_framework_;
    
    /// Control IDs by framework
    std::map<ComplianceFramework, std::vector<std::string>> controls_by_framework_;
};

// ============================================================================
// Compliance Context
// ============================================================================

/**
 * @brief Context for compliance validation
 * 
 * Captures system state, policies, and configuration needed for
 * compliance validation.
 */
struct ComplianceContext {
    std::string system_id;                    ///< System being validated
    std::vector<std::string> enabled_frameworks; ///< Frameworks to validate
    std::unordered_map<std::string, std::string> policy_state; ///< Current policies
    std::unordered_map<std::string, bool> control_status; ///< Control implementation status
    std::vector<ComplianceEvidence> evidence; ///< Collected evidence
    int64_t validation_time_ms = 0;           ///< When validation occurred
    nlohmann::json metadata;                  ///< Additional context
    
    /// Convert to JSON representation
    nlohmann::json toJson() const;
};

// ============================================================================
// Compliance Status Report
// ============================================================================

/**
 * @brief High-level compliance status for a framework
 * 
 * Summarizes overall compliance with one framework including
 * metrics and violation summary.
 */
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
    
    /// Convert to JSON representation
    nlohmann::json toJson() const;
};

// ============================================================================
// Compliance Validation Result
// ============================================================================

/**
 * @brief Result of compliance validation
 * 
 * Contains validation status, findings, and evidence for audit trail.
 */
struct ComplianceValidationResult {
    bool success = true;
    std::string error_message;
    int64_t validation_time_ms = 0;
    int64_t elapsed_ms = 0;  ///< Validation duration
    
    std::vector<ComplianceStatusReport> framework_reports;
    std::vector<ComplianceViolation> all_violations;
    
    /// Convert to JSON representation
    nlohmann::json toJson() const;
};

} // namespace governance
} // namespace themis
