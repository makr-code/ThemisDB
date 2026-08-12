/**
 * @file compliance_reporting.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "governance/policy_manager.h"
#include "governance/ccpa_rules.h"
#include <atomic>
#include <mutex>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <memory>
#include <nlohmann/json.hpp>

namespace themis {
namespace governance {

/**
 * @brief Error codes for compliance reporter operations (7350-7399 range).
 * 
 * Used for structured error handling in compliance report generation,
 * state management, and reporting pipeline.
 */
enum class ComplianceError {
    kSuccess               = 0,      // No error
    kConflictDetected      = 7350,   // Policy conflict detected during report
    kReportingFailed       = 7351,   // Report generation failed
    kStateInvalid          = 7352,   // Reporter state transition invalid
    kResourceExhausted     = 7353,   // Memory/resource exhaustion
    kHtmlGenerationFailed  = 7354,   // HTML generation failed
};

/**
 * @brief Result struct for compliance report operations with error semantics.
 * 
 * Provides structured error handling with error codes, messages, and
 * optional success data. Thread-safe and auditable.
 */
struct ComplianceReporterResult {
    /// Error classification
    ComplianceError error = ComplianceError::kSuccess;
    
    /// Human-readable error message (empty if kSuccess)
    std::string error_message;
    
    /// Report content (filled if kSuccess)
    std::string report_content;
    
    /// Report format (JSON, CSV, HTML, PDF)
    std::string report_format;
    
    /// Unix timestamp (milliseconds) when result was generated
    int64_t generated_at_ms = 0;
    
    /// Diagnostic code for integration with DiagnosticAggregator
    int32_t diagnostic_code = 0;
    
    /**
     * @brief Check if operation succeeded.
     * @return true if error == kSuccess
     */
    [[nodiscard]] bool isSuccess() const {
        return error == ComplianceError::kSuccess;
    }
    
    /**
     * @brief Get string representation of error code.
     * @return Human-readable error name
     */
    [[nodiscard]] std::string getErrorName() const;
};

/// PolicyCoverageAnalyzer identifies resources without policies and calculates coverage
class PolicyCoverageAnalyzer {
public:
    struct CoverageResult {
        int total_resources_checked = 0;
        int covered_resources = 0;
        int uncovered_resources = 0;
        double coverage_percentage = 0.0;
        std::vector<std::string> uncovered_resource_list;
        std::unordered_map<std::string, int> coverage_by_action;
        
        nlohmann::json toJson() const;
    };
    
    struct OverlapResult {
        std::string resource_pattern;
        std::string action_pattern;
        std::vector<std::string> overlapping_rule_ids;
        int overlap_count = 0;
        
        nlohmann::json toJson() const;
    };
    
    /// Analyze coverage for a list of resources
    CoverageResult analyzeCoverage(
        const PolicyManager& policy_mgr,
        const std::vector<std::string>& resources,
        const std::vector<std::string>& actions = {"*"}
    ) const;
    
    /// Detect overlapping rules (multiple rules apply to same resource/action)
    std::vector<OverlapResult> detectOverlaps(const PolicyManager& policy_mgr) const;
    
    /// Find policy gaps (resources that should have policies but don't)
    std::vector<std::string> findGaps(
        const PolicyManager& policy_mgr,
        const std::vector<std::string>& expected_resources
    ) const;
};

/// ComplianceGapDetector compares policies vs compliance requirements
class ComplianceGapDetector {
public:
    struct ComplianceRequirement {
        std::string id;
        std::string name;
        std::string framework;                     // GDPR, SOC2, HIPAA, etc.
        std::string description;
        std::vector<std::string> required_resources;
        bool requires_encryption = false;
        bool requires_signature = false;
        bool requires_audit = false;
        int min_retention_days = 0;
        
        nlohmann::json toJson() const;
        static ComplianceRequirement fromJson(const nlohmann::json& j);
    };
    
    struct ComplianceGap {
        std::string requirement_id;
        std::string requirement_name;
        std::string gap_type;                      // missing_policy, missing_control, insufficient_coverage
        std::string description;
        std::vector<std::string> affected_resources;
        std::string recommendation;
        
        nlohmann::json toJson() const;
    };
    
    struct ComplianceStatus {
        std::string framework;
        int total_requirements = 0;
        int met_requirements = 0;
        int unmet_requirements = 0;
        double compliance_percentage = 0.0;
        std::vector<ComplianceGap> gaps;
        
        nlohmann::json toJson() const;
    };
    
    /// Add a compliance requirement
    void addRequirement(const ComplianceRequirement& req);
    
    /// Detect gaps between requirements and current policies
    std::vector<ComplianceGap> detectGaps(const PolicyManager& policy_mgr) const;
    
    /// Get overall compliance status
    ComplianceStatus getComplianceStatus(
        const PolicyManager& policy_mgr,
        const std::string& framework = ""
    ) const;
    
    /// Load requirements from JSON file
    bool loadRequirements(const std::string& path);
    
    /// Export requirements as JSON
    nlohmann::json exportRequirements() const;
    
private:
    std::vector<ComplianceRequirement> requirements_;
    mutable std::mutex mutex_;
    
    bool checkRequirement(const ComplianceRequirement& req, const PolicyManager& policy_mgr) const;
};

/// Base interface for all compliance report types.
/// Concrete report structs derive from this interface to guarantee CSV export support.
struct IComplianceReport {
    virtual ~IComplianceReport() = default;
    /// Serialise the report as a CSV-formatted string.
    [[nodiscard]] virtual std::string toCSV() const = 0;
};

/// ComplianceReporter generates various audit reports
class ComplianceReporter {
public:
    enum class ReportFormat {
        JSON,
        CSV,
        HTML,
        PDF
    };
    
    struct PolicySummaryReport : public IComplianceReport {
        int total_rules = 0;
        int enabled_rules = 0;
        int disabled_rules = 0;
        std::unordered_map<std::string, int> rules_by_classification;
        std::unordered_map<std::string, int> rules_requiring_encryption;
        std::unordered_map<std::string, int> rules_with_audit;
        int64_t generated_at = 0;
        
        nlohmann::json toJson() const;
        std::string toCSV() const override;
        std::string toHTML() const;
    };
    
    struct ComplianceStatusReport : public IComplianceReport {
        std::string framework;
        double overall_compliance = 0.0;
        std::vector<std::string> compliant_controls;
        std::vector<std::string> non_compliant_controls;
        std::vector<ComplianceGapDetector::ComplianceGap> gaps;
        int64_t generated_at = 0;
        
        nlohmann::json toJson() const;
        std::string toCSV() const override;
        std::string toHTML() const;
    };
    
    struct AccessControlMatrix : public IComplianceReport {
        struct Entry {
            std::string role;
            std::string resource;
            std::vector<std::string> allowed_actions;
            bool requires_encryption;
            bool is_audited;
        };
        
        std::vector<Entry> entries;
        int64_t generated_at = 0;
        
        nlohmann::json toJson() const;
        std::string toCSV() const override;
        std::string toHTML() const;
    };
    
    struct RiskAssessmentReport : public IComplianceReport {
        struct RiskItem {
            std::string risk_id;
            std::string severity;                  // low, medium, high, critical
            std::string description;
            std::vector<std::string> affected_resources;
            std::string mitigation;
        };
        
        std::vector<RiskItem> risks;
        int high_risks = 0;
        int medium_risks = 0;
        int low_risks = 0;
        int64_t generated_at = 0;
        
        nlohmann::json toJson() const;
        std::string toCSV() const override;
        std::string toHTML() const;
    };
    
    struct ChangeHistoryReport : public IComplianceReport {
        std::vector<PolicyRuleVersion> changes;
        int total_changes = 0;
        std::unordered_map<std::string, int> changes_by_user;
        int64_t start_time = 0;
        int64_t end_time = 0;
        
        nlohmann::json toJson() const;
        std::string toCSV() const override;
        std::string toHTML() const;
    };

    /// CCPA/CPRA compliance report produced by generateCcpaReport().
    struct CcpaReport : public IComplianceReport {
        /// Data categories covered by the active PolicyRules (derived from
        /// classification levels present in the policy set).
        std::vector<std::string> data_categories;

        /// Rules that allow export (potential third-party disclosures).
        std::vector<std::string> third_party_disclosure_rule_ids;

        /// Total number of subjects registered as opted-out.
        int opt_out_count = 0;

        /// Number of rules that pass all CCPA compliance checks.
        int ccpa_compliant_rules = 0;

        /// Number of rules that fail at least one CCPA compliance check.
        int ccpa_non_compliant_rules = 0;

        /// Rules that do not satisfy the RightToKnow evaluator.
        std::vector<std::string> missing_right_to_know;

        /// Rules that do not satisfy the RightToDelete evaluator.
        std::vector<std::string> missing_right_to_delete;

        /// Rules that do not satisfy the OptOutOfSale evaluator.
        std::vector<std::string> missing_opt_out_of_sale;

        /// Rules that do not satisfy the DataPortability evaluator.
        std::vector<std::string> missing_data_portability;

        /// Time window this report covers (Unix timestamps).
        int64_t start_time = 0;
        int64_t end_time   = 0;

        /// Report generation timestamp.
        int64_t generated_at = 0;

        nlohmann::json toJson() const;
        std::string toCSV() const override;
    };

    /// Atomic state for compliance reporter (Phase 2B enhancement)
    enum class ReporterState : int32_t {
        DRAFT       = 0,      // Reporter initialized, not generating reports
        REPORTING   = 1,      // Report generation in progress
        FINALIZED   = 2,      // Report generation complete
        FAILED      = 3,      // Report generation failed (terminal state)
    };

    /**
     * @brief Initialize compliance reporter with atomic state tracking.
     * @note Thread-safe constructor; sets initial state to DRAFT.
     */
    ComplianceReporter();
    
    /**
     * @brief Get current atomic state of reporter.
     * @return Current ReporterState
     */
    [[nodiscard]] ReporterState getState() const;
    
    /**
     * @brief Check if reporter is in valid state for report generation.
     * @return true if state == DRAFT (ready to start new report)
     */
    [[nodiscard]] bool isReadyForReporting() const;
    
    /// Generate policy summary report
    PolicySummaryReport generatePolicySummary(const PolicyManager& policy_mgr) const;
    
    /// Generate policy summary report with error semantics (Phase 2B enhancement)
    ComplianceReporterResult generatePolicySummaryWithResult(
        const PolicyManager& policy_mgr);

    
    /// Generate compliance status report
    ComplianceStatusReport generateComplianceStatus(
        const PolicyManager& policy_mgr,
        const ComplianceGapDetector& detector,
        const std::string& framework = ""
    ) const;
    
    /// Generate access control matrix
    AccessControlMatrix generateAccessControlMatrix(const PolicyManager& policy_mgr) const;
    
    /// Generate risk assessment report
    RiskAssessmentReport generateRiskAssessment(const PolicyManager& policy_mgr) const;
    
    /// Generate change history report
    ChangeHistoryReport generateChangeHistory(
        const PolicyManager& policy_mgr,
        int64_t start_time = 0,
        int64_t end_time = INT64_MAX
    ) const;
    
    /// Generate CCPA/CPRA compliance report.
    /// Evaluates all active PolicyRules against the CCPA rule set and
    /// summarises data categories, third-party disclosure candidates,
    /// opt-out counts, and compliance gaps.
    /// @param policy_mgr  Current policy manager instance.
    /// @param opt_out_count  Number of subjects currently opted out (provided
    ///        by the caller because the reporter does not own the opt-out registry).
    /// @param start_time  Start of the reporting window (Unix seconds, default: epoch).
    /// @param end_time    End of the reporting window (Unix seconds, default: MAX).
    /// @return Structured CCPA report as a CcpaReport.
    CcpaReport generateCcpaReport(
        const PolicyManager& policy_mgr,
        int opt_out_count = 0,
        int64_t start_time = 0,
        int64_t end_time = INT64_MAX
    ) const;

    /// Export report in specified format
    std::string exportReport(const nlohmann::json& report, ReportFormat format) const;
    
    /// Generate HTML header for reports
    [[nodiscard]] std::string generateHTMLHeader(const std::string& title) const;
    
    /// Generate HTML footer for reports
    [[nodiscard]] std::string generateHTMLFooter() const;
    
private:
    /// Atomic state tracking (Phase 2B enhancement)
    mutable std::atomic<ReporterState> state_{ReporterState::DRAFT};
    mutable std::mutex state_mutex_;
    
    /**
     * @brief Validate and update atomic state (Phase 2B enhancement).
     * 
     * Thread-safe state transition. Fails if state is invalid.
     * 
     * @param expected Expected current state
     * @param target Target state to transition to
     * @return true if transition succeeded, false if state mismatch
     */
    bool transitionState(ReporterState expected, ReporterState target) const;
    
    /**
     * @brief Generate HTML from string builder pattern (optimized, Phase 2B).
     * 
     * Avoids O(n²) concatenation by using pre-reserved string buffer
     * and ostringstream with appropriate capacity.
     * 
     * @param title Report title
     * @param headers Column headers
     * @param rows Data rows (each row is list of cell values)
     * @return Optimized HTML string
     */
    std::string generateHTMLOptimized(
        const std::string& title,
        const std::vector<std::string>& headers,
        const std::vector<std::vector<std::string>>& rows
    ) const;
    
    /**
     * @brief Record diagnostic for compliance failures (Phase 2B integration).
     * 
     * @param code Error code (e.g., ComplianceError::kReportingFailed)
     * @param message Error message
     * @param component Component name (default: "compliance_reporter")
     */
    void recordComplianceDiagnostic(
        int32_t code,
        const std::string& message,
        const std::string& component = "compliance_reporter"
    ) const;
    


};

} // namespace governance
} // namespace themis
