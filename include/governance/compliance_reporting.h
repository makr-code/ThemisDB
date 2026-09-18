/**
 * @file compliance_reporting.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

enum class ComplianceError {
    kSuccess               = 0,      // No error
    kConflictDetected      = 7350,   // Policy conflict detected during report
    kReportingFailed       = 7351,   // Report generation failed
    kStateInvalid          = 7352,   // Reporter state transition invalid
    kResourceExhausted     = 7353,   // Memory/resource exhaustion
    kHtmlGenerationFailed  = 7354,   // HTML generation failed
};

struct ComplianceReporterResult {
    ComplianceError error = ComplianceError::kSuccess;
    
    std::string error_message;
    
    std::string report_content;
    
    std::string report_format;
    
    int64_t generated_at_ms = 0;
    
    int32_t diagnostic_code = 0;
    
    [[nodiscard]] bool isSuccess() const {
        return error == ComplianceError::kSuccess;
    }
    
    [[nodiscard]] std::string getErrorName() const;
};

class PolicyCoverageAnalyzer {
public:
    struct CoverageResult {
        int total_resources_checked = 0;
        int covered_resources = 0;
        int uncovered_resources = 0;
        double coverage_percentage = 0.0;
        std::vector<std::string> uncovered_resource_list;
        std::unordered_map<std::string, int> coverage_by_action;
        
        /**
         * @brief To Json.
         * @return Return value.
         */
        nlohmann::json toJson() const;
    };
    
    struct OverlapResult {
        std::string resource_pattern;
        std::string action_pattern;
        std::vector<std::string> overlapping_rule_ids;
        int overlap_count = 0;
        
        /**
         * @brief To Json.
         * @return Return value.
         */
        nlohmann::json toJson() const;
    };
    
    CoverageResult analyzeCoverage(
        const PolicyManager& policy_mgr,
        const std::vector<std::string>& resources,
        const std::vector<std::string>& actions = {"*"}
    ) const;
    
    /**
     * @brief Detect Overlaps.
     * @param[in] policy_mgr Input parameter.
     * @return Return value.
     */
    std::vector<OverlapResult> detectOverlaps(const PolicyManager& policy_mgr) const;
    
    /**
     * @brief Find Gaps.
     * @param[in] policy_mgr Input parameter.
     * @param[in] expected_resources Input parameter.
     * @return Return value.
     */
    std::vector<std::string> findGaps(
        const PolicyManager& policy_mgr,
        const std::vector<std::string>& expected_resources
    ) const;
};

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
    
    struct ComplianceGap {
        std::string requirement_id;
        std::string requirement_name;
        std::string gap_type;                      // missing_policy, missing_control, insufficient_coverage
        std::string description;
        std::vector<std::string> affected_resources;
        std::string recommendation;
        
        /**
         * @brief To Json.
         * @return Return value.
         */
        nlohmann::json toJson() const;
    };
    
    struct ComplianceStatus {
        std::string framework;
        int total_requirements = 0;
        int met_requirements = 0;
        int unmet_requirements = 0;
        double compliance_percentage = 0.0;
        std::vector<ComplianceGap> gaps;
        
        /**
         * @brief To Json.
         * @return Return value.
         */
        nlohmann::json toJson() const;
    };
    
    /**
     * @brief Add Requirement.
     * @param[in] req Input parameter.
     */
    void addRequirement(const ComplianceRequirement& req);
    
    /**
     * @brief Detect Gaps.
     * @param[in] policy_mgr Input parameter.
     * @return Return value.
     */
    std::vector<ComplianceGap> detectGaps(const PolicyManager& policy_mgr) const;
    
    ComplianceStatus getComplianceStatus(
        const PolicyManager& policy_mgr,
        const std::string& framework = ""
    ) const;
    
    /**
     * @brief Load Requirements.
     * @param[in] path Input parameter.
     * @return True when the operation succeeds.
     */
    bool loadRequirements(const std::string& path);
    
    /**
     * @brief Export Requirements.
     * @return Return value.
     */
    nlohmann::json exportRequirements() const;
    
private:
    std::vector<ComplianceRequirement> requirements_;
    mutable std::mutex mutex_;
    
    /**
     * @brief Check Requirement.
     * @param[in] req Input parameter.
     * @param[in] policy_mgr Input parameter.
     * @return True when the operation succeeds.
     */
    bool checkRequirement(const ComplianceRequirement& req, const PolicyManager& policy_mgr) const;
};

struct IComplianceReport {
    /**
     * @brief ICompliance Report.
     * @return Return value.
     */
    virtual ~IComplianceReport() = default;
    [[nodiscard]] virtual std::string toCSV() const = 0;
};

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
        
        /**
         * @brief To Json.
         * @return Return value.
         */
        nlohmann::json toJson() const;
        std::string toCSV() const override;
        /**
         * @brief To HTML.
         * @return Return value.
         */
        std::string toHTML() const;
    };
    
    struct ComplianceStatusReport : public IComplianceReport {
        std::string framework;
        double overall_compliance = 0.0;
        std::vector<std::string> compliant_controls;
        std::vector<std::string> non_compliant_controls;
        std::vector<ComplianceGapDetector::ComplianceGap> gaps;
        int64_t generated_at = 0;
        
        /**
         * @brief To Json.
         * @return Return value.
         */
        nlohmann::json toJson() const;
        std::string toCSV() const override;
        /**
         * @brief To HTML.
         * @return Return value.
         */
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
        
        /**
         * @brief To Json.
         * @return Return value.
         */
        nlohmann::json toJson() const;
        std::string toCSV() const override;
        /**
         * @brief To HTML.
         * @return Return value.
         */
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
        
        /**
         * @brief To Json.
         * @return Return value.
         */
        nlohmann::json toJson() const;
        std::string toCSV() const override;
        /**
         * @brief To HTML.
         * @return Return value.
         */
        std::string toHTML() const;
    };
    
    struct ChangeHistoryReport : public IComplianceReport {
        std::vector<PolicyRuleVersion> changes;
        int total_changes = 0;
        std::unordered_map<std::string, int> changes_by_user;
        int64_t start_time = 0;
        int64_t end_time = 0;
        
        /**
         * @brief To Json.
         * @return Return value.
         */
        nlohmann::json toJson() const;
        std::string toCSV() const override;
        /**
         * @brief To HTML.
         * @return Return value.
         */
        std::string toHTML() const;
    };

    struct CcpaReport : public IComplianceReport {
        std::vector<std::string> data_categories;

        std::vector<std::string> third_party_disclosure_rule_ids;

        int opt_out_count = 0;

        int ccpa_compliant_rules = 0;

        int ccpa_non_compliant_rules = 0;

        std::vector<std::string> missing_right_to_know;

        std::vector<std::string> missing_right_to_delete;

        std::vector<std::string> missing_opt_out_of_sale;

        std::vector<std::string> missing_data_portability;

        int64_t start_time = 0;
        int64_t end_time   = 0;

        int64_t generated_at = 0;

        /**
         * @brief To Json.
         * @return Return value.
         */
        nlohmann::json toJson() const;
        std::string toCSV() const override;
    };

    enum class ReporterState : int32_t {
        DRAFT       = 0,      // Reporter initialized, not generating reports
        REPORTING   = 1,      // Report generation in progress
        FINALIZED   = 2,      // Report generation complete
        FAILED      = 3,      // Report generation failed (terminal state)
    };

    ComplianceReporter();
    
    [[nodiscard]] ReporterState getState() const;
    
    [[nodiscard]] bool isReadyForReporting() const;
    
    /**
     * @brief Generate Policy Summary.
     * @param[in] policy_mgr Input parameter.
     * @return Return value.
     */
    PolicySummaryReport generatePolicySummary(const PolicyManager& policy_mgr) const;
    
    /**
     * @brief Generate Policy Summary With Result.
     * @param[in] policy_mgr Input parameter.
     * @return Return value.
     */
    ComplianceReporterResult generatePolicySummaryWithResult(
        const PolicyManager& policy_mgr);

    
    ComplianceStatusReport generateComplianceStatus(
        const PolicyManager& policy_mgr,
        const ComplianceGapDetector& detector,
        const std::string& framework = ""
    ) const;
    
    /**
     * @brief Generate Access Control Matrix.
     * @param[in] policy_mgr Input parameter.
     * @return Return value.
     */
    AccessControlMatrix generateAccessControlMatrix(const PolicyManager& policy_mgr) const;
    
    /**
     * @brief Generate Risk Assessment.
     * @param[in] policy_mgr Input parameter.
     * @return Return value.
     */
    RiskAssessmentReport generateRiskAssessment(const PolicyManager& policy_mgr) const;
    
    ChangeHistoryReport generateChangeHistory(
        const PolicyManager& policy_mgr,
        int64_t start_time = 0,
        int64_t end_time = INT64_MAX
    ) const;
    
    CcpaReport generateCcpaReport(
        const PolicyManager& policy_mgr,
        int opt_out_count = 0,
        int64_t start_time = 0,
        int64_t end_time = INT64_MAX
    ) const;

    /**
     * @brief Export Report.
     * @param[in] report Input parameter.
     * @param[in] format Input parameter.
     * @return Return value.
     */
    std::string exportReport(const nlohmann::json& report, ReportFormat format) const;
    
    [[nodiscard]] std::string generateHTMLHeader(const std::string& title) const;
    
    [[nodiscard]] std::string generateHTMLFooter() const;
    
private:
    mutable std::atomic<ReporterState> state_{ReporterState::DRAFT};
    mutable std::mutex state_mutex_;
    
    /**
     * @brief Transition State.
     * @param[in] expected Input parameter.
     * @param[in] target Input parameter.
     * @return True when the operation succeeds.
     */
    bool transitionState(ReporterState expected, ReporterState target) const;
    
    /**
     * @brief Generate HTMLOptimized.
     * @param[in] title Input parameter.
     * @param[in] headers Input parameter.
     * @param[in] rows Input parameter.
     * @return Return value.
     */
    std::string generateHTMLOptimized(
        const std::string& title,
        const std::vector<std::string>& headers,
        const std::vector<std::vector<std::string>>& rows
    ) const;
    
    void recordComplianceDiagnostic(
        int32_t code,
        const std::string& message,
        const std::string& component = "compliance_reporter"
    ) const;
    


};

} // namespace governance
} // namespace themis
