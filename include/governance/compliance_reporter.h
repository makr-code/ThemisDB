/**
 * @file compliance_reporter.h
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
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <cstdint>
#include <climits>
#include <nlohmann/json.hpp>

namespace themis {
namespace governance {

struct BiasFieldStats {
    std::string field_name;                           ///< Name of the field (e.g., "gender", "age_group")
    std::unordered_map<std::string, size_t> group_counts; ///< Group label → document count
    size_t total_count = 0;                           ///< Total documents with this field populated

    double representation_ratio = 0.0;

    double demographic_parity_score = 0.0;

    /**
     * @brief To Json.
     * @return Return value.
     */
    nlohmann::json toJson() const;
};

struct BiasAuditReport {
    std::string report_id;
    std::string adapter_id;          ///< LoRA adapter / model identifier
    std::string dataset_id;          ///< Export job ID used as the dataset identifier
    int64_t generated_at = 0;        ///< Unix epoch milliseconds

    std::vector<BiasFieldStats> field_stats; ///< Per-field statistics

    double overall_bias_score = 0.0;

    std::string status;

    std::vector<std::string> recommendations; ///< Actionable remediation advice

    /**
     * @brief To Json.
     * @return Return value.
     */
    nlohmann::json toJson() const;
};

struct CoverageAnalysis {
    int total_resources_analyzed = 0;
    int resources_with_policies = 0;
    int resources_without_policies = 0;
    double coverage_percentage = 0.0;
    std::vector<std::string> uncovered_resources;
    std::vector<std::string> overlapping_rules;  // Rules that overlap
    
    /**
     * @brief To Json.
     * @return Return value.
     */
    nlohmann::json toJson() const;
};

struct ComplianceGap {
    std::string gap_type;                          // "missing_encryption", "no_audit", etc.
    std::string severity;                          // "critical", "high", "medium", "low"
    std::string description;
    std::vector<std::string> affected_resources;
    std::vector<std::string> recommendations;
    
    /**
     * @brief To Json.
     * @return Return value.
     */
    nlohmann::json toJson() const;
};

struct ComplianceReport {
    std::string report_id;
    std::int64_t generated_at;
    std::string report_type;                       // "coverage", "compliance", "summary"
    
    // Summary statistics
    int total_rules = 0;
    int active_rules = 0;
    int inactive_rules = 0;
    
    // Compliance status
    std::vector<ComplianceGap> gaps;
    double compliance_score = 0.0;  // 0-100
    
    // Additional data
    nlohmann::json details;
    
    /**
     * @brief To Json.
     * @return Return value.
     */
    nlohmann::json toJson() const;
};

struct RuleEvaluationEntry {
    int64_t     timestamp_ms              = 0;     ///< Unix epoch milliseconds
    std::string route;                             ///< API route / resource evaluated
    std::string classification;                    ///< Classification level applied
    std::string mode;                              ///< "enforce" or "observe"
    bool        require_content_encryption = false;
    bool        ccpa_opted_out             = false;
    bool        export_allowed             = true;
    std::string user_id;                           ///< Optional requesting user

    /**
     * @brief From Json.
     * @param[in] j Input parameter.
     * @return Return value.
     */
    static RuleEvaluationEntry fromJson(const nlohmann::json& j);
};

struct TimeWindowReport {
    int64_t window_start_ms = 0;          ///< Inclusive window start (Unix ms; 0 = epoch)
    int64_t window_end_ms   = INT64_MAX;  ///< Inclusive window end (Unix ms)
    int64_t generated_at    = 0;          ///< Report generation time (Unix seconds)

    std::string framework;

    // ── Evaluation counters ──────────────────────────────────────────────────
    int total_evaluations            = 0;
    int enforce_mode_evaluations     = 0;
    int observe_mode_evaluations     = 0;
    int ccpa_opted_out_count         = 0;
    int encryption_required_count    = 0;
    int export_blocked_count         = 0;

    // ── Breakdowns ───────────────────────────────────────────────────────────
    std::unordered_map<std::string, int> evaluations_by_classification;
    std::unordered_map<std::string, int> evaluations_by_route;

    // ── Policy-level compliance ───────────────────────────────────────────────
    double compliance_score = 0.0;         ///< 0–100
    std::vector<ComplianceGap> gaps;

    /**
     * @brief To Json.
     * @return Return value.
     */
    nlohmann::json toJson() const;
    /**
     * @brief To CSV.
     * @return Return value.
     */
    std::string toCSV() const;
};

class ComplianceReporter {
public:
    ComplianceReporter(std::shared_ptr<PolicyManager> policy_manager);
    
    /**
     * @brief Analyze Coverage.
     * @param[in] resources Input parameter.
     * @return Return value.
     */
    CoverageAnalysis analyzeCoverage(const std::vector<std::string>& resources) const;
    
    std::vector<std::pair<std::string, std::string>> detectOverlappingRules() const;
    
    /**
     * @brief Detect Gaps.
     * @return Return value.
     */
    std::vector<ComplianceGap> detectGaps() const;
    
    /**
     * @brief Generate Summary Report.
     * @return Return value.
     */
    ComplianceReport generateSummaryReport() const;
    
    ComplianceReport generateComplianceReport(const std::string& framework = "") const;
    
    nlohmann::json generateCcpaReport(
        const CcpaRuleSet& rule_set,
        int64_t window_start_ms = 0,
        int64_t window_end_ms   = INT64_MAX
    ) const;

    BiasAuditReport generateBiasAuditReport(
        const std::string& adapter_id,
        const std::string& dataset_id,
        const std::unordered_map<std::string,
              std::unordered_map<std::string, size_t>>& field_stats
    ) const;
    
    /**
     * @brief Generate Access Control Matrix.
     * @return Return value.
     */
    nlohmann::json generateAccessControlMatrix() const;
    
    /**
     * @brief Generate Risk Assessment Report.
     * @return Return value.
     */
    ComplianceReport generateRiskAssessmentReport() const;

    TimeWindowReport generateTimeWindowReport(
        const std::vector<RuleEvaluationEntry>& entries,
        int64_t window_start_ms = 0,
        int64_t window_end_ms   = INT64_MAX,
        const std::string& framework = "") const;

    /**
     * @brief Export Report.
     * @param[in] report Input parameter.
     * @param[in] format Input parameter.
     * @return Return value.
     */
    std::string exportReport(const ComplianceReport& report, const std::string& format) const;
    
private:
    std::shared_ptr<PolicyManager> policy_manager_;
    
    /**
     * @brief Calculate Compliance Score.
     * @param[in] gaps Input parameter.
     * @return Return value.
     */
    double calculateComplianceScore(const std::vector<ComplianceGap>& gaps) const;
    
    /**
     * @brief Report To CSV.
     * @param[in] report Input parameter.
     * @return Return value.
     */
    std::string reportToCSV(const ComplianceReport& report) const;
    
    /**
     * @brief Has Required Controls.
     * @param[in] rule Input parameter.
     * @return True when the operation succeeds.
     */
    bool hasRequiredControls(const PolicyRule& rule) const;
};

} // namespace governance
} // namespace themis
