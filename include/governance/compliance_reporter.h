/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            compliance_reporter.h                              ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-02-21 13:48:15                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     138                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "governance/policy_manager.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <nlohmann/json.hpp>

namespace themis {
namespace governance {

/// Coverage analysis result
struct CoverageAnalysis {
    int total_resources_analyzed = 0;
    int resources_with_policies = 0;
    int resources_without_policies = 0;
    double coverage_percentage = 0.0;
    std::vector<std::string> uncovered_resources;
    std::vector<std::string> overlapping_rules;  // Rules that overlap
    
    nlohmann::json toJson() const;
};

/// Compliance gap
struct ComplianceGap {
    std::string gap_type;                          // "missing_encryption", "no_audit", etc.
    std::string severity;                          // "critical", "high", "medium", "low"
    std::string description;
    std::vector<std::string> affected_resources;
    std::vector<std::string> recommendations;
    
    nlohmann::json toJson() const;
};

/// Compliance report
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
    
    nlohmann::json toJson() const;
};

/// Compliance reporter and analyzer
class ComplianceReporter {
public:
    ComplianceReporter(std::shared_ptr<PolicyManager> policy_manager);
    
    /// Analyze policy coverage
    /// @param resources List of resources to analyze
    /// @return Coverage analysis result
    CoverageAnalysis analyzeCoverage(const std::vector<std::string>& resources) const;
    
    /// Detect overlapping rules
    /// @return List of rule IDs that have overlapping conditions
    std::vector<std::pair<std::string, std::string>> detectOverlappingRules() const;
    
    /// Detect compliance gaps
    /// @return List of compliance gaps
    std::vector<ComplianceGap> detectGaps() const;
    
    /// Generate summary report
    /// @return Compliance report with summary statistics
    ComplianceReport generateSummaryReport() const;
    
    /// Generate compliance status report
    /// @param framework Optional compliance framework (e.g., "GDPR", "SOX")
    /// @return Compliance report with gap analysis
    ComplianceReport generateComplianceReport(const std::string& framework = "") const;
    
    /// Generate access control matrix
    /// @return JSON representation of access control matrix
    nlohmann::json generateAccessControlMatrix() const;
    
    /// Generate risk assessment report
    /// @return Compliance report focused on risk
    ComplianceReport generateRiskAssessmentReport() const;
    
    /// Export report in specified format
    /// @param report Report to export
    /// @param format Export format ("json", "csv")
    /// @return Exported data as string
    std::string exportReport(const ComplianceReport& report, const std::string& format) const;
    
private:
    std::shared_ptr<PolicyManager> policy_manager_;
    
    /// Helper: Calculate compliance score
    double calculateComplianceScore(const std::vector<ComplianceGap>& gaps) const;
    
    /// Helper: Convert report to CSV
    std::string reportToCSV(const ComplianceReport& report) const;
    
    /// Helper: Check if rule has required security controls
    bool hasRequiredControls(const PolicyRule& rule) const;
};

} // namespace governance
} // namespace themis
