/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            compliance_reporting.h                             ║
  Version:         0.0.21                                             ║
  Last Modified:   2026-02-21 19:19:59                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     264                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "governance/policy_manager.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <memory>
#include <nlohmann/json.hpp>

namespace themis {
namespace governance {

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

/// ComplianceReporter generates various audit reports
class ComplianceReporter {
public:
    enum class ReportFormat {
        JSON,
        CSV,
        HTML,
        PDF
    };
    
    struct PolicySummaryReport {
        int total_rules = 0;
        int enabled_rules = 0;
        int disabled_rules = 0;
        std::unordered_map<std::string, int> rules_by_classification;
        std::unordered_map<std::string, int> rules_requiring_encryption;
        std::unordered_map<std::string, int> rules_with_audit;
        int64_t generated_at = 0;
        
        nlohmann::json toJson() const;
        std::string toCSV() const;
        std::string toHTML() const;
    };
    
    struct ComplianceStatusReport {
        std::string framework;
        double overall_compliance = 0.0;
        std::vector<std::string> compliant_controls;
        std::vector<std::string> non_compliant_controls;
        std::vector<ComplianceGapDetector::ComplianceGap> gaps;
        int64_t generated_at = 0;
        
        nlohmann::json toJson() const;
        std::string toCSV() const;
        std::string toHTML() const;
    };
    
    struct AccessControlMatrix {
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
        std::string toCSV() const;
        std::string toHTML() const;
    };
    
    struct RiskAssessmentReport {
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
        std::string toCSV() const;
        std::string toHTML() const;
    };
    
    struct ChangeHistoryReport {
        std::vector<PolicyRuleVersion> changes;
        int total_changes = 0;
        std::unordered_map<std::string, int> changes_by_user;
        int64_t start_time = 0;
        int64_t end_time = 0;
        
        nlohmann::json toJson() const;
        std::string toCSV() const;
        std::string toHTML() const;
    };
    
    /// Generate policy summary report
    PolicySummaryReport generatePolicySummary(const PolicyManager& policy_mgr) const;
    
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
    
    /// Export report in specified format
    std::string exportReport(const nlohmann::json& report, ReportFormat format) const;
    
private:
    std::string generateHTMLHeader(const std::string& title) const;
    std::string generateHTMLFooter() const;
};

} // namespace governance
} // namespace themis
