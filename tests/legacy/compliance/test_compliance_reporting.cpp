#include <gtest/gtest.h>
#include "governance/compliance_reporting.h"
#include "governance/policy_manager.h"
#include <chrono>
#include <fstream>
#include <filesystem>

using namespace themis::governance;

class ComplianceReportingTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a basic policy manager with test rules
        policy_mgr = std::make_unique<PolicyManager>();
        
        // Add test rules
        PolicyRule rule1;
        rule1.id = "rule1";
        rule1.name = "Test Rule 1";
        rule1.enabled = true;
        rule1.resources = {"data/users", "data/accounts"};
        rule1.actions = {"read", "write"};
        rule1.classification_level = "offen";
        rule1.require_encryption = true;
        rule1.audit_access = true;
        rule1.retention_days = 90;
        policy_mgr->addRule(rule1);
        
        PolicyRule rule2;
        rule2.id = "rule2";
        rule2.name = "Test Rule 2";
        rule2.enabled = true;
        rule2.resources = {"data/sensitive/*"};
        rule2.actions = {"*"};
        rule2.classification_level = "geheim";
        rule2.require_encryption = true;
        rule2.require_signature = true;
        rule2.audit_access = true;
        rule2.audit_changes = true;
        rule2.retention_days = 365;
        policy_mgr->addRule(rule2);
        
        PolicyRule rule3;
        rule3.id = "rule3";
        rule3.name = "Test Rule 3 (Disabled)";
        rule3.enabled = false;
        rule3.resources = {"data/temp"};
        rule3.actions = {"read"};
        rule3.retention_days = 7;
        policy_mgr->addRule(rule3);
        
        PolicyRule rule4;
        rule4.id = "rule4";
        rule4.name = "Test Rule 4 (Wildcard)";
        rule4.enabled = true;
        rule4.resources = {"*"};
        rule4.actions = {"*"};
        rule4.classification_level = "vs-nfd";
        rule4.required_roles = {"admin"};
        rule4.retention_days = 180;
        policy_mgr->addRule(rule4);
    }
    
    std::unique_ptr<PolicyManager> policy_mgr;
};

// ========== PolicyCoverageAnalyzer Tests ==========

TEST_F(ComplianceReportingTest, CoverageAnalyzer_BasicCoverage) {
    PolicyCoverageAnalyzer analyzer;
    
    std::vector<std::string> resources = {
        "data/users",
        "data/accounts",
        "data/sensitive/keys",
        "data/uncovered"
    };
    
    auto result = analyzer.analyzeCoverage(*policy_mgr, resources, {"read"});
    
    EXPECT_EQ(result.total_resources_checked, 4);
    EXPECT_GT(result.covered_resources, 0);
    EXPECT_GE(result.coverage_percentage, 0.0);
    EXPECT_LE(result.coverage_percentage, 100.0);
}

TEST_F(ComplianceReportingTest, CoverageAnalyzer_FullCoverage) {
    PolicyCoverageAnalyzer analyzer;
    
    std::vector<std::string> resources = {
        "data/users",
        "data/accounts"
    };
    
    auto result = analyzer.analyzeCoverage(*policy_mgr, resources, {"read"});
    
    EXPECT_EQ(result.covered_resources, 2);
    EXPECT_EQ(result.uncovered_resources, 0);
    EXPECT_DOUBLE_EQ(result.coverage_percentage, 100.0);
}

TEST_F(ComplianceReportingTest, CoverageAnalyzer_NoCoverage) {
    PolicyCoverageAnalyzer analyzer;
    
    std::vector<std::string> resources = {
        "data/uncovered1",
        "data/uncovered2"
    };
    
    auto result = analyzer.analyzeCoverage(*policy_mgr, resources, {"delete"});
    
    EXPECT_EQ(result.total_resources_checked, 2);
    EXPECT_EQ(result.uncovered_resource_list.size(), 2);
}

TEST_F(ComplianceReportingTest, CoverageAnalyzer_MultipleActions) {
    PolicyCoverageAnalyzer analyzer;
    
    std::vector<std::string> resources = {"data/users"};
    std::vector<std::string> actions = {"read", "write", "delete"};
    
    auto result = analyzer.analyzeCoverage(*policy_mgr, resources, actions);
    
    EXPECT_GT(result.coverage_by_action.size(), 0);
}

TEST_F(ComplianceReportingTest, CoverageAnalyzer_EmptyResources) {
    PolicyCoverageAnalyzer analyzer;
    
    auto result = analyzer.analyzeCoverage(*policy_mgr, {}, {"read"});
    
    EXPECT_EQ(result.total_resources_checked, 0);
    EXPECT_EQ(result.coverage_percentage, 0.0);
}

TEST_F(ComplianceReportingTest, CoverageAnalyzer_ToJson) {
    PolicyCoverageAnalyzer analyzer;
    
    std::vector<std::string> resources = {"data/users"};
    auto result = analyzer.analyzeCoverage(*policy_mgr, resources, {"read"});
    
    auto json = result.toJson();
    
    EXPECT_TRUE(json.contains("total_resources_checked"));
    EXPECT_TRUE(json.contains("covered_resources"));
    EXPECT_TRUE(json.contains("coverage_percentage"));
}

TEST_F(ComplianceReportingTest, CoverageAnalyzer_DetectOverlaps) {
    PolicyCoverageAnalyzer analyzer;
    
    // Add overlapping rule
    PolicyRule rule5;
    rule5.id = "rule5";
    rule5.name = "Overlapping Rule";
    rule5.enabled = true;
    rule5.resources = {"data/users"};
    rule5.actions = {"read"};
    policy_mgr->addRule(rule5);
    
    auto overlaps = analyzer.detectOverlaps(*policy_mgr);
    
    EXPECT_GT(overlaps.size(), 0);
    
    bool found_overlap = false;
    for (const auto& overlap : overlaps) {
        if (overlap.resource_pattern == "data/users" && overlap.action_pattern == "read") {
            found_overlap = true;
            EXPECT_GE(overlap.overlap_count, 2);
        }
    }
    EXPECT_TRUE(found_overlap);
}

TEST_F(ComplianceReportingTest, CoverageAnalyzer_NoOverlaps) {
    PolicyCoverageAnalyzer analyzer;
    
    // Remove all rules and add unique ones
    PolicyManager mgr;
    PolicyRule rule;
    rule.id = "unique1";
    rule.enabled = true;
    rule.resources = {"res1"};
    rule.actions = {"act1"};
    mgr.addRule(rule);
    
    auto overlaps = analyzer.detectOverlaps(mgr);
    
    EXPECT_EQ(overlaps.size(), 0);
}

TEST_F(ComplianceReportingTest, CoverageAnalyzer_FindGaps) {
    PolicyCoverageAnalyzer analyzer;
    
    std::vector<std::string> expected_resources = {
        "data/users",
        "data/missing1",
        "data/missing2"
    };
    
    auto gaps = analyzer.findGaps(*policy_mgr, expected_resources);
    
    EXPECT_GT(gaps.size(), 0);
    EXPECT_TRUE(std::find(gaps.begin(), gaps.end(), "data/missing1") != gaps.end() ||
                std::find(gaps.begin(), gaps.end(), "data/missing2") != gaps.end());
}

TEST_F(ComplianceReportingTest, CoverageAnalyzer_NoGaps) {
    PolicyCoverageAnalyzer analyzer;
    
    std::vector<std::string> expected_resources = {
        "data/users",
        "data/accounts"
    };
    
    auto gaps = analyzer.findGaps(*policy_mgr, expected_resources);
    
    EXPECT_EQ(gaps.size(), 0);
}

// ========== ComplianceGapDetector Tests ==========

TEST_F(ComplianceReportingTest, ComplianceGapDetector_AddRequirement) {
    ComplianceGapDetector detector;
    
    ComplianceGapDetector::ComplianceRequirement req;
    req.id = "GDPR-001";
    req.name = "Data Encryption";
    req.framework = "GDPR";
    req.description = "All personal data must be encrypted";
    req.required_resources = {"data/users"};
    req.requires_encryption = true;
    
    detector.addRequirement(req);
    
    auto exported = detector.exportRequirements();
    EXPECT_TRUE(exported.contains("requirements"));
    EXPECT_EQ(exported["requirements"].size(), 1);
}

TEST_F(ComplianceReportingTest, ComplianceGapDetector_RequirementToJson) {
    ComplianceGapDetector::ComplianceRequirement req;
    req.id = "SOC2-001";
    req.name = "Audit Logging";
    req.framework = "SOC2";
    req.requires_audit = true;
    req.min_retention_days = 365;
    
    auto json = req.toJson();
    
    EXPECT_EQ(json["id"], "SOC2-001");
    EXPECT_EQ(json["name"], "Audit Logging");
    EXPECT_EQ(json["framework"], "SOC2");
    EXPECT_TRUE(json["requires_audit"]);
}

TEST_F(ComplianceReportingTest, ComplianceGapDetector_RequirementFromJson) {
    nlohmann::json j;
    j["id"] = "HIPAA-001";
    j["name"] = "PHI Protection";
    j["framework"] = "HIPAA";
    j["description"] = "Protect patient health information";
    j["required_resources"] = {"data/health"};
    j["requires_encryption"] = true;
    j["requires_signature"] = true;
    j["requires_audit"] = true;
    j["min_retention_days"] = 2555;
    
    auto req = ComplianceGapDetector::ComplianceRequirement::fromJson(j);
    
    EXPECT_EQ(req.id, "HIPAA-001");
    EXPECT_EQ(req.framework, "HIPAA");
    EXPECT_TRUE(req.requires_encryption);
    EXPECT_EQ(req.min_retention_days, 2555);
}

TEST_F(ComplianceReportingTest, ComplianceGapDetector_DetectGaps_MissingPolicy) {
    ComplianceGapDetector detector;
    
    ComplianceGapDetector::ComplianceRequirement req;
    req.id = "REQ-001";
    req.name = "Uncovered Resource";
    req.required_resources = {"data/uncovered"};
    req.requires_encryption = true;
    
    detector.addRequirement(req);
    
    auto gaps = detector.detectGaps(*policy_mgr);
    
    EXPECT_GT(gaps.size(), 0);
    
    bool found_missing_policy = false;
    for (const auto& gap : gaps) {
        if (gap.gap_type == "missing_policy") {
            found_missing_policy = true;
        }
    }
    EXPECT_TRUE(found_missing_policy);
}

TEST_F(ComplianceReportingTest, ComplianceGapDetector_DetectGaps_MissingControl) {
    ComplianceGapDetector detector;
    
    // Add requirement that needs signature but our rule doesn't have it
    ComplianceGapDetector::ComplianceRequirement req;
    req.id = "REQ-002";
    req.name = "Signature Required";
    req.required_resources = {"data/users"};
    req.requires_signature = true;
    
    detector.addRequirement(req);
    
    auto gaps = detector.detectGaps(*policy_mgr);
    
    EXPECT_GT(gaps.size(), 0);
}

TEST_F(ComplianceReportingTest, ComplianceGapDetector_NoGaps) {
    ComplianceGapDetector detector;
    
    ComplianceGapDetector::ComplianceRequirement req;
    req.id = "REQ-003";
    req.name = "Basic Encryption";
    req.required_resources = {"data/users"};
    req.requires_encryption = true;
    
    detector.addRequirement(req);
    
    auto gaps = detector.detectGaps(*policy_mgr);
    
    // Should have no gaps since data/users has encryption enabled
    bool has_gap_for_users = false;
    for (const auto& gap : gaps) {
        if (gap.requirement_id == "REQ-003" && gap.gap_type == "missing_policy") {
            has_gap_for_users = true;
        }
    }
    EXPECT_FALSE(has_gap_for_users);
}

TEST_F(ComplianceReportingTest, ComplianceGapDetector_GetComplianceStatus) {
    ComplianceGapDetector detector;
    
    ComplianceGapDetector::ComplianceRequirement req1;
    req1.id = "GDPR-001";
    req1.framework = "GDPR";
    req1.required_resources = {"data/users"};
    req1.requires_encryption = true;
    detector.addRequirement(req1);
    
    ComplianceGapDetector::ComplianceRequirement req2;
    req2.id = "GDPR-002";
    req2.framework = "GDPR";
    req2.required_resources = {"data/missing"};
    req2.requires_encryption = true;
    detector.addRequirement(req2);
    
    auto status = detector.getComplianceStatus(*policy_mgr, "GDPR");
    
    EXPECT_EQ(status.framework, "GDPR");
    EXPECT_EQ(status.total_requirements, 2);
    EXPECT_GE(status.compliance_percentage, 0.0);
    EXPECT_LE(status.compliance_percentage, 100.0);
}

TEST_F(ComplianceReportingTest, ComplianceGapDetector_GetComplianceStatus_AllFrameworks) {
    ComplianceGapDetector detector;
    
    ComplianceGapDetector::ComplianceRequirement req1;
    req1.id = "GDPR-001";
    req1.framework = "GDPR";
    req1.required_resources = {"data/users"};
    req1.requires_encryption = true;
    detector.addRequirement(req1);
    
    ComplianceGapDetector::ComplianceRequirement req2;
    req2.id = "SOC2-001";
    req2.framework = "SOC2";
    req2.required_resources = {"data/accounts"};
    req2.requires_audit = true;
    detector.addRequirement(req2);
    
    auto status = detector.getComplianceStatus(*policy_mgr, "");
    
    EXPECT_EQ(status.framework, "all");
    EXPECT_EQ(status.total_requirements, 2);
}

TEST_F(ComplianceReportingTest, ComplianceGapDetector_ComplianceStatusToJson) {
    ComplianceGapDetector detector;
    
    ComplianceGapDetector::ComplianceRequirement req;
    req.id = "TEST-001";
    req.framework = "TEST";
    req.required_resources = {"data/test"};
    detector.addRequirement(req);
    
    auto status = detector.getComplianceStatus(*policy_mgr, "TEST");
    auto json = status.toJson();
    
    EXPECT_TRUE(json.contains("framework"));
    EXPECT_TRUE(json.contains("total_requirements"));
    EXPECT_TRUE(json.contains("compliance_percentage"));
    EXPECT_TRUE(json.contains("gaps"));
}

TEST_F(ComplianceReportingTest, ComplianceGapDetector_LoadRequirements) {
    ComplianceGapDetector detector;
    
    // Create a test file in portable temp directory with unique name
    auto temp_dir = std::filesystem::temp_directory_path();
    auto test_file = temp_dir / ("test_requirements_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".json");
    
    nlohmann::json j;
    j["requirements"] = nlohmann::json::array();
    nlohmann::json req_json;
    req_json["id"] = "FILE-001";
    req_json["name"] = "Test Requirement";
    req_json["framework"] = "TEST";
    req_json["required_resources"] = {"data/test"};
    j["requirements"].push_back(req_json);
    
    std::ofstream file(test_file);
    file << j.dump(2);
    file.close();
    
    bool loaded = detector.loadRequirements(test_file.string());
    
    EXPECT_TRUE(loaded);
    
    auto exported = detector.exportRequirements();
    EXPECT_EQ(exported["requirements"].size(), 1);
    
    // Clean up
    std::filesystem::remove(test_file);
}

// ========== ComplianceReporter Tests ==========

TEST_F(ComplianceReportingTest, ComplianceReporter_GeneratePolicySummary) {
    ComplianceReporter reporter;
    
    auto report = reporter.generatePolicySummary(*policy_mgr);
    
    EXPECT_EQ(report.total_rules, 4);
    EXPECT_EQ(report.enabled_rules, 3);
    EXPECT_EQ(report.disabled_rules, 1);
    EXPECT_GT(report.generated_at, 0);
}

TEST_F(ComplianceReportingTest, ComplianceReporter_PolicySummaryToJson) {
    ComplianceReporter reporter;
    
    auto report = reporter.generatePolicySummary(*policy_mgr);
    auto json = report.toJson();
    
    EXPECT_TRUE(json.contains("total_rules"));
    EXPECT_TRUE(json.contains("enabled_rules"));
    EXPECT_TRUE(json.contains("rules_by_classification"));
    EXPECT_EQ(json["total_rules"], 4);
}

TEST_F(ComplianceReportingTest, ComplianceReporter_PolicySummaryToCSV) {
    ComplianceReporter reporter;
    
    auto report = reporter.generatePolicySummary(*policy_mgr);
    auto csv = report.toCSV();
    
    EXPECT_FALSE(csv.empty());
    EXPECT_NE(csv.find("Total Rules"), std::string::npos);
    EXPECT_NE(csv.find("Enabled Rules"), std::string::npos);
}

TEST_F(ComplianceReportingTest, ComplianceReporter_PolicySummaryToHTML) {
    ComplianceReporter reporter;
    
    auto report = reporter.generatePolicySummary(*policy_mgr);
    auto html = report.toHTML();
    
    EXPECT_FALSE(html.empty());
    EXPECT_NE(html.find("<html>"), std::string::npos);
    EXPECT_NE(html.find("Policy Summary Report"), std::string::npos);
    EXPECT_NE(html.find("</html>"), std::string::npos);
}

TEST_F(ComplianceReportingTest, ComplianceReporter_GenerateComplianceStatus) {
    ComplianceReporter reporter;
    ComplianceGapDetector detector;
    
    ComplianceGapDetector::ComplianceRequirement req;
    req.id = "TEST-001";
    req.framework = "TEST";
    req.required_resources = {"data/users"};
    req.requires_encryption = true;
    detector.addRequirement(req);
    
    auto report = reporter.generateComplianceStatus(*policy_mgr, detector, "TEST");
    
    EXPECT_EQ(report.framework, "TEST");
    EXPECT_GT(report.generated_at, 0);
}

TEST_F(ComplianceReportingTest, ComplianceReporter_ComplianceStatusToJson) {
    ComplianceReporter reporter;
    ComplianceGapDetector detector;
    
    auto report = reporter.generateComplianceStatus(*policy_mgr, detector, "GDPR");
    auto json = report.toJson();
    
    EXPECT_TRUE(json.contains("framework"));
    EXPECT_TRUE(json.contains("overall_compliance"));
    EXPECT_TRUE(json.contains("gaps"));
}

TEST_F(ComplianceReportingTest, ComplianceReporter_ComplianceStatusToCSV) {
    ComplianceReporter reporter;
    ComplianceGapDetector detector;
    
    auto report = reporter.generateComplianceStatus(*policy_mgr, detector, "GDPR");
    auto csv = report.toCSV();
    
    EXPECT_FALSE(csv.empty());
    EXPECT_NE(csv.find("Framework"), std::string::npos);
}

TEST_F(ComplianceReportingTest, ComplianceReporter_ComplianceStatusToHTML) {
    ComplianceReporter reporter;
    ComplianceGapDetector detector;
    
    auto report = reporter.generateComplianceStatus(*policy_mgr, detector, "GDPR");
    auto html = report.toHTML();
    
    EXPECT_FALSE(html.empty());
    EXPECT_NE(html.find("Compliance Status Report"), std::string::npos);
}

TEST_F(ComplianceReportingTest, ComplianceReporter_GenerateAccessControlMatrix) {
    ComplianceReporter reporter;
    
    auto matrix = reporter.generateAccessControlMatrix(*policy_mgr);
    
    EXPECT_GT(matrix.entries.size(), 0);
    EXPECT_GT(matrix.generated_at, 0);
}

TEST_F(ComplianceReportingTest, ComplianceReporter_AccessControlMatrixToJson) {
    ComplianceReporter reporter;
    
    auto matrix = reporter.generateAccessControlMatrix(*policy_mgr);
    auto json = matrix.toJson();
    
    EXPECT_TRUE(json.contains("entries"));
    EXPECT_TRUE(json.contains("generated_at"));
    EXPECT_TRUE(json["entries"].is_array());
}

TEST_F(ComplianceReportingTest, ComplianceReporter_AccessControlMatrixToCSV) {
    ComplianceReporter reporter;
    
    auto matrix = reporter.generateAccessControlMatrix(*policy_mgr);
    auto csv = matrix.toCSV();
    
    EXPECT_FALSE(csv.empty());
    EXPECT_NE(csv.find("Role"), std::string::npos);
    EXPECT_NE(csv.find("Resource"), std::string::npos);
}

TEST_F(ComplianceReportingTest, ComplianceReporter_AccessControlMatrixToHTML) {
    ComplianceReporter reporter;
    
    auto matrix = reporter.generateAccessControlMatrix(*policy_mgr);
    auto html = matrix.toHTML();
    
    EXPECT_FALSE(html.empty());
    EXPECT_NE(html.find("Access Control Matrix"), std::string::npos);
}

TEST_F(ComplianceReportingTest, ComplianceReporter_GenerateRiskAssessment) {
    ComplianceReporter reporter;
    
    auto report = reporter.generateRiskAssessment(*policy_mgr);
    
    EXPECT_GT(report.generated_at, 0);
    EXPECT_GT(report.risks.size(), 0); // Should detect at least disabled rule
}

TEST_F(ComplianceReportingTest, ComplianceReporter_RiskAssessment_DetectsDisabledRules) {
    ComplianceReporter reporter;
    
    auto report = reporter.generateRiskAssessment(*policy_mgr);
    
    bool found_disabled_risk = false;
    for (const auto& risk : report.risks) {
        if (risk.risk_id.find("DISABLED") != std::string::npos) {
            found_disabled_risk = true;
            EXPECT_EQ(risk.severity, "medium");
        }
    }
    EXPECT_TRUE(found_disabled_risk);
}

TEST_F(ComplianceReportingTest, ComplianceReporter_RiskAssessment_DetectsPermissiveRules) {
    ComplianceReporter reporter;
    
    auto report = reporter.generateRiskAssessment(*policy_mgr);
    
    // Should detect rule4 which has wildcard resources and actions
    bool found_permissive = false;
    for (const auto& risk : report.risks) {
        if (risk.risk_id.find("PERMISSIVE") != std::string::npos) {
            found_permissive = true;
        }
    }
    EXPECT_TRUE(found_permissive);
}

TEST_F(ComplianceReportingTest, ComplianceReporter_RiskAssessmentToJson) {
    ComplianceReporter reporter;
    
    auto report = reporter.generateRiskAssessment(*policy_mgr);
    auto json = report.toJson();
    
    EXPECT_TRUE(json.contains("risks"));
    EXPECT_TRUE(json.contains("high_risks"));
    EXPECT_TRUE(json.contains("medium_risks"));
    EXPECT_TRUE(json.contains("low_risks"));
}

TEST_F(ComplianceReportingTest, ComplianceReporter_RiskAssessmentToCSV) {
    ComplianceReporter reporter;
    
    auto report = reporter.generateRiskAssessment(*policy_mgr);
    auto csv = report.toCSV();
    
    EXPECT_FALSE(csv.empty());
    EXPECT_NE(csv.find("Risk ID"), std::string::npos);
    EXPECT_NE(csv.find("Severity"), std::string::npos);
}

TEST_F(ComplianceReportingTest, ComplianceReporter_RiskAssessmentToHTML) {
    ComplianceReporter reporter;
    
    auto report = reporter.generateRiskAssessment(*policy_mgr);
    auto html = report.toHTML();
    
    EXPECT_FALSE(html.empty());
    EXPECT_NE(html.find("Risk Assessment Report"), std::string::npos);
}

TEST_F(ComplianceReportingTest, ComplianceReporter_GenerateChangeHistory) {
    ComplianceReporter reporter;
    
    // Update a rule to create history
    auto rule = policy_mgr->getRule("rule1");
    if (rule) {
        PolicyRule updated = *rule;
        updated.description = "Updated description";
        policy_mgr->updateRule("rule1", updated, "test_user", "Test update");
    }
    
    int64_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    auto report = reporter.generateChangeHistory(*policy_mgr, 0, now + 1000);
    
    EXPECT_GE(report.total_changes, 0);
    EXPECT_EQ(report.start_time, 0);
}

TEST_F(ComplianceReportingTest, ComplianceReporter_ChangeHistoryToJson) {
    ComplianceReporter reporter;
    
    int64_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    auto report = reporter.generateChangeHistory(*policy_mgr, 0, now + 1000);
    auto json = report.toJson();
    
    EXPECT_TRUE(json.contains("changes"));
    EXPECT_TRUE(json.contains("total_changes"));
    EXPECT_TRUE(json.contains("changes_by_user"));
}

TEST_F(ComplianceReportingTest, ComplianceReporter_ChangeHistoryToCSV) {
    ComplianceReporter reporter;
    
    int64_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    auto report = reporter.generateChangeHistory(*policy_mgr, 0, now + 1000);
    auto csv = report.toCSV();
    
    EXPECT_FALSE(csv.empty());
    EXPECT_NE(csv.find("Version"), std::string::npos);
}

TEST_F(ComplianceReportingTest, ComplianceReporter_ChangeHistoryToHTML) {
    ComplianceReporter reporter;
    
    int64_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    auto report = reporter.generateChangeHistory(*policy_mgr, 0, now + 1000);
    auto html = report.toHTML();
    
    EXPECT_FALSE(html.empty());
    EXPECT_NE(html.find("Change History Report"), std::string::npos);
}

TEST_F(ComplianceReportingTest, ComplianceReporter_ExportReportJSON) {
    ComplianceReporter reporter;
    
    nlohmann::json test_report;
    test_report["test"] = "data";
    test_report["value"] = 123;
    
    auto exported = reporter.exportReport(test_report, ComplianceReporter::ReportFormat::JSON);
    
    EXPECT_FALSE(exported.empty());
    EXPECT_NE(exported.find("test"), std::string::npos);
}

TEST_F(ComplianceReportingTest, ComplianceReporter_ExportReportHTML) {
    ComplianceReporter reporter;
    
    nlohmann::json test_report;
    test_report["report_type"] = "summary";
    test_report["generated_at"] = static_cast<int64_t>(
        std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()));
    test_report["total_rules"] = 5;
    test_report["compliance_score"] = 85.0;
    
    auto exported = reporter.exportReport(test_report, ComplianceReporter::ReportFormat::HTML);
    
    EXPECT_FALSE(exported.empty());
    EXPECT_NE(exported.find("<!DOCTYPE html>"), std::string::npos);
    EXPECT_NE(exported.find("Compliance Report"), std::string::npos);
    EXPECT_NE(exported.find("</html>"), std::string::npos);
    EXPECT_NE(exported.find("total_rules"), std::string::npos);
}

TEST_F(ComplianceReportingTest, ComplianceReporter_ExportReportPDF) {
    ComplianceReporter reporter;
    
    nlohmann::json test_report;
    test_report["report_type"] = "risk_assessment";
    test_report["generated_at"] = static_cast<int64_t>(
        std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()));
    test_report["total_rules"] = 10;
    test_report["compliance_score"] = 72.5;
    
    auto exported = reporter.exportReport(test_report, ComplianceReporter::ReportFormat::PDF);
    
    EXPECT_FALSE(exported.empty());
    // PDF must start with the PDF magic bytes
    EXPECT_EQ(exported.substr(0, 7), "%PDF-1.");
    // Must end with %%EOF marker
    EXPECT_NE(exported.rfind("%%EOF"), std::string::npos);
    // Must contain cross-reference table
    EXPECT_NE(exported.find("xref"), std::string::npos);
    EXPECT_NE(exported.find("trailer"), std::string::npos);
    EXPECT_NE(exported.find("startxref"), std::string::npos);
}

TEST_F(ComplianceReportingTest, ComplianceReporter_ExportReportPDF_MultiPage) {
    ComplianceReporter reporter;
    
    // Create a report with many fields to trigger multi-page PDF generation
    nlohmann::json test_report;
    test_report["report_type"] = "compliance";
    test_report["generated_at"] = static_cast<int64_t>(
        std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()));
    nlohmann::json items = nlohmann::json::array();
    for (int i = 0; i < 100; ++i) {
        items.push_back("item_" + std::to_string(i) + "_value");
    }
    test_report["items"] = items;
    
    auto exported = reporter.exportReport(test_report, ComplianceReporter::ReportFormat::PDF);
    
    EXPECT_FALSE(exported.empty());
    EXPECT_EQ(exported.substr(0, 7), "%PDF-1.");
    EXPECT_NE(exported.rfind("%%EOF"), std::string::npos);
}


TEST_F(ComplianceReportingTest, EdgeCase_EmptyPolicyManager) {
    PolicyManager empty_mgr;
    PolicyCoverageAnalyzer analyzer;
    
    std::vector<std::string> resources = {"data/test"};
    auto result = analyzer.analyzeCoverage(empty_mgr, resources);
    
    EXPECT_EQ(result.covered_resources, 0);
    EXPECT_EQ(result.uncovered_resources, 1);
}

TEST_F(ComplianceReportingTest, EdgeCase_WildcardCoverage) {
    PolicyCoverageAnalyzer analyzer;
    
    // rule4 wildcard requires admin role, and analyzer runs without user role context.
    std::vector<std::string> resources = {"any/resource/path"};
    auto result = analyzer.analyzeCoverage(*policy_mgr, resources);
    
    EXPECT_EQ(result.covered_resources, 0);
}

TEST_F(ComplianceReportingTest, EdgeCase_RequirementWithMultipleResources) {
    ComplianceGapDetector detector;
    
    ComplianceGapDetector::ComplianceRequirement req;
    req.id = "MULTI-001";
    req.required_resources = {"data/users", "data/accounts", "data/missing"};
    req.requires_encryption = true;
    
    detector.addRequirement(req);
    
    auto gaps = detector.detectGaps(*policy_mgr);
    
    EXPECT_GT(gaps.size(), 0);
}

TEST_F(ComplianceReportingTest, EdgeCase_ShortRetentionDetection) {
    // Add an enabled low-retention rule; fixture's short-retention rule is disabled.
    PolicyRule short_retention_rule;
    short_retention_rule.id = "rule-short-retention-enabled";
    short_retention_rule.name = "Short Retention Enabled";
    short_retention_rule.enabled = true;
    short_retention_rule.resources = {"data/short-retention"};
    short_retention_rule.actions = {"read"};
    short_retention_rule.retention_days = 7;
    policy_mgr->addRule(short_retention_rule);

    ComplianceReporter reporter;
    
    auto report = reporter.generateRiskAssessment(*policy_mgr);
    
    // rule3 has 7 days retention
    bool found_short_retention = false;
    for (const auto& risk : report.risks) {
        if (risk.risk_id.find("SHORT_RETENTION") != std::string::npos) {
            found_short_retention = true;
            EXPECT_EQ(risk.severity, "low");
        }
    }
    EXPECT_TRUE(found_short_retention);
}

TEST_F(ComplianceReportingTest, Integration_FullComplianceWorkflow) {
    // Complete workflow test
    PolicyCoverageAnalyzer analyzer;
    ComplianceGapDetector detector;
    ComplianceReporter reporter;
    
    // 1. Analyze coverage
    std::vector<std::string> resources = {"data/users", "data/accounts", "data/missing"};
    auto coverage = analyzer.analyzeCoverage(*policy_mgr, resources, {"read"});
    EXPECT_GT(coverage.coverage_percentage, 0.0);
    
    // 2. Add compliance requirements
    ComplianceGapDetector::ComplianceRequirement req;
    req.id = "INT-001";
    req.framework = "INTEGRATION_TEST";
    req.required_resources = {"data/users"};
    req.requires_encryption = true;
    detector.addRequirement(req);
    
    // 3. Check compliance
    auto status = detector.getComplianceStatus(*policy_mgr, "INTEGRATION_TEST");
    EXPECT_EQ(status.total_requirements, 1);
    
    // 4. Generate reports
    auto summary = reporter.generatePolicySummary(*policy_mgr);
    EXPECT_GT(summary.total_rules, 0);
    
    auto compliance_report = reporter.generateComplianceStatus(*policy_mgr, detector, "INTEGRATION_TEST");
    EXPECT_EQ(compliance_report.framework, "INTEGRATION_TEST");
    
    auto matrix = reporter.generateAccessControlMatrix(*policy_mgr);
    EXPECT_GT(matrix.entries.size(), 0);
    
    auto risk = reporter.generateRiskAssessment(*policy_mgr);
    EXPECT_GT(risk.risks.size(), 0);
}

TEST_F(ComplianceReportingTest, Integration_MultipleFormatsExport) {
    ComplianceReporter reporter;
    
    auto summary = reporter.generatePolicySummary(*policy_mgr);
    
    // Test all export formats
    auto json = summary.toJson();
    EXPECT_FALSE(json.dump().empty());
    
    auto csv = summary.toCSV();
    EXPECT_FALSE(csv.empty());
    
    auto html = summary.toHTML();
    EXPECT_FALSE(html.empty());
    EXPECT_NE(html.find("<html>"), std::string::npos);
}

TEST_F(ComplianceReportingTest, Integration_ComplexOverlapScenario) {
    PolicyCoverageAnalyzer analyzer;
    
    // Add multiple overlapping rules
    PolicyRule overlap1;
    overlap1.id = "overlap1";
    overlap1.enabled = true;
    overlap1.resources = {"shared/resource"};
    overlap1.actions = {"read"};
    policy_mgr->addRule(overlap1);
    
    PolicyRule overlap2;
    overlap2.id = "overlap2";
    overlap2.enabled = true;
    overlap2.resources = {"shared/resource"};
    overlap2.actions = {"read"};
    policy_mgr->addRule(overlap2);
    
    PolicyRule overlap3;
    overlap3.id = "overlap3";
    overlap3.enabled = true;
    overlap3.resources = {"shared/resource"};
    overlap3.actions = {"read"};
    policy_mgr->addRule(overlap3);
    
    auto overlaps = analyzer.detectOverlaps(*policy_mgr);
    
    bool found_triple_overlap = false;
    for (const auto& overlap : overlaps) {
        if (overlap.overlap_count >= 3) {
            found_triple_overlap = true;
        }
    }
    EXPECT_TRUE(found_triple_overlap);
}

// ============================================================================
// CSV Export Tests (Issue #4004)
// Verifies column headers, field values, and delimiter escaping for
// CcpaReport, AccessControlMatrix, ChangeHistoryReport, and the generic
// exportReport(json, CSV) path.
// ============================================================================

// ---------------------------------------------------------------------------
// CcpaReport::toCSV()
// ---------------------------------------------------------------------------

TEST_F(ComplianceReportingTest, CsvExport_CcpaReport_ColumnHeaders) {
    ComplianceReporter reporter;
    auto report = reporter.generateCcpaReport(*policy_mgr, 42);
    auto csv = report.toCSV();

    EXPECT_FALSE(csv.empty());
    // Verify the header row is present
    EXPECT_NE(csv.find("Field"), std::string::npos);
    EXPECT_NE(csv.find("Value"), std::string::npos);
}

TEST_F(ComplianceReportingTest, CsvExport_CcpaReport_FieldValues) {
    ComplianceReporter reporter;
    auto report = reporter.generateCcpaReport(*policy_mgr, 7);
    auto csv = report.toCSV();

    EXPECT_NE(csv.find("opt_out_count"), std::string::npos);
    EXPECT_NE(csv.find("7"), std::string::npos);
    EXPECT_NE(csv.find("ccpa_compliant_rules"), std::string::npos);
    EXPECT_NE(csv.find("ccpa_non_compliant_rules"), std::string::npos);
    EXPECT_NE(csv.find("generated_at"), std::string::npos);
    EXPECT_NE(csv.find("missing_right_to_know_count"), std::string::npos);
    EXPECT_NE(csv.find("missing_right_to_delete_count"), std::string::npos);
    EXPECT_NE(csv.find("missing_opt_out_of_sale_count"), std::string::npos);
    EXPECT_NE(csv.find("missing_data_portability_count"), std::string::npos);
    EXPECT_NE(csv.find("third_party_disclosure_count"), std::string::npos);
}

TEST_F(ComplianceReportingTest, CsvExport_CcpaReport_IsIComplianceReport) {
    // Verify that CcpaReport satisfies the IComplianceReport interface
    ComplianceReporter reporter;
    ComplianceReporter::CcpaReport report = reporter.generateCcpaReport(*policy_mgr, 0);
    const IComplianceReport& iface = report;
    auto csv = iface.toCSV();
    EXPECT_FALSE(csv.empty());
    EXPECT_NE(csv.find("opt_out_count"), std::string::npos);
}

// ---------------------------------------------------------------------------
// AccessControlMatrix::toCSV() — column headers and delimiter escaping
// ---------------------------------------------------------------------------

TEST_F(ComplianceReportingTest, CsvExport_AccessControlMatrix_ColumnHeaders) {
    ComplianceReporter reporter;
    auto matrix = reporter.generateAccessControlMatrix(*policy_mgr);
    auto csv = matrix.toCSV();

    EXPECT_FALSE(csv.empty());
    // Mandatory column headers
    EXPECT_NE(csv.find("Role"), std::string::npos);
    EXPECT_NE(csv.find("Resource"), std::string::npos);
    EXPECT_NE(csv.find("Allowed Actions"), std::string::npos);
    EXPECT_NE(csv.find("Requires Encryption"), std::string::npos);
    EXPECT_NE(csv.find("Is Audited"), std::string::npos);
}

TEST_F(ComplianceReportingTest, CsvExport_AccessControlMatrix_DelimiterEscaping) {
    ComplianceReporter::AccessControlMatrix matrix;
    // Entry whose allowed_actions list is joined with ';' and wrapped in quotes
    ComplianceReporter::AccessControlMatrix::Entry e;
    e.role = "admin";
    e.resource = "data/users";
    e.allowed_actions = {"read", "write", "delete"};
    e.requires_encryption = true;
    e.is_audited = true;
    matrix.entries.push_back(e);

    auto csv = matrix.toCSV();
    // Multiple actions are joined with ';' and wrapped in double-quotes
    EXPECT_NE(csv.find("\"read;write;delete\""), std::string::npos);
    // Boolean columns use Yes/No
    EXPECT_NE(csv.find("Yes"), std::string::npos);
}

TEST_F(ComplianceReportingTest, CsvExport_AccessControlMatrix_IsIComplianceReport) {
    ComplianceReporter reporter;
    ComplianceReporter::AccessControlMatrix matrix = reporter.generateAccessControlMatrix(*policy_mgr);
    const IComplianceReport& iface = matrix;
    auto csv = iface.toCSV();
    EXPECT_FALSE(csv.empty());
    EXPECT_NE(csv.find("Role"), std::string::npos);
}

// ---------------------------------------------------------------------------
// ChangeHistoryReport::toCSV() — column headers and delimiter escaping
// ---------------------------------------------------------------------------

TEST_F(ComplianceReportingTest, CsvExport_ChangeHistoryReport_ColumnHeaders) {
    ComplianceReporter reporter;
    int64_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    auto report = reporter.generateChangeHistory(*policy_mgr, 0, now + 1000);
    auto csv = report.toCSV();

    EXPECT_FALSE(csv.empty());
    EXPECT_NE(csv.find("Version"), std::string::npos);
    EXPECT_NE(csv.find("Timestamp"), std::string::npos);
    EXPECT_NE(csv.find("Modified By"), std::string::npos);
    EXPECT_NE(csv.find("Description"), std::string::npos);
}

TEST_F(ComplianceReportingTest, CsvExport_ChangeHistoryReport_DelimiterEscaping) {
    ComplianceReporter::ChangeHistoryReport report;

    // Add a change with a description that contains a comma (requires quoting)
    PolicyRuleVersion v;
    v.version = 1;
    v.timestamp = 1700000000;
    v.author = "alice";
    v.change_description = "Updated rule, added encryption requirement";
    v.rule_id = "rule1";
    report.changes.push_back(v);
    report.total_changes = 1;
    report.changes_by_user["alice"] = 1;

    auto csv = report.toCSV();
    // Description with a comma must be wrapped in double-quotes
    EXPECT_NE(csv.find("\"Updated rule, added encryption requirement\""),
              std::string::npos);
    EXPECT_NE(csv.find("alice"), std::string::npos);
    EXPECT_NE(csv.find("Total Changes"), std::string::npos);
}

TEST_F(ComplianceReportingTest, CsvExport_ChangeHistoryReport_IsIComplianceReport) {
    ComplianceReporter reporter;
    int64_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    ComplianceReporter::ChangeHistoryReport report =
        reporter.generateChangeHistory(*policy_mgr, 0, now + 1000);
    const IComplianceReport& iface = report;
    auto csv = iface.toCSV();
    EXPECT_FALSE(csv.empty());
    EXPECT_NE(csv.find("Version"), std::string::npos);
}

// ---------------------------------------------------------------------------
// exportReport(json, ReportFormat::CSV) — generic JSON → CSV path
// ---------------------------------------------------------------------------

TEST_F(ComplianceReportingTest, CsvExport_ExportReportCSV_BasicFields) {
    ComplianceReporter reporter;

    nlohmann::json test_report;
    test_report["report_type"] = "risk_assessment";
    test_report["total_rules"] = 15;
    test_report["compliance_score"] = 88.5;
    test_report["generated_at"] = static_cast<int64_t>(
        std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()));

    auto csv = reporter.exportReport(test_report, ComplianceReporter::ReportFormat::CSV);

    EXPECT_FALSE(csv.empty());
    // Must contain header row
    EXPECT_NE(csv.find("Field"), std::string::npos);
    EXPECT_NE(csv.find("Value"), std::string::npos);
    // Must contain the scalar fields
    EXPECT_NE(csv.find("report_type"), std::string::npos);
    EXPECT_NE(csv.find("risk_assessment"), std::string::npos);
    EXPECT_NE(csv.find("total_rules"), std::string::npos);
    EXPECT_NE(csv.find("compliance_score"), std::string::npos);
    EXPECT_NE(csv.find("generated_at"), std::string::npos);
}

TEST_F(ComplianceReportingTest, CsvExport_ExportReportCSV_NestedArraysEscaped) {
    ComplianceReporter reporter;

    nlohmann::json test_report;
    test_report["framework"] = "GDPR";
    test_report["controls"] = nlohmann::json::array({"ctrl-1", "ctrl-2", "ctrl-3"});

    auto csv = reporter.exportReport(test_report, ComplianceReporter::ReportFormat::CSV);

    EXPECT_FALSE(csv.empty());
    EXPECT_NE(csv.find("framework"), std::string::npos);
    EXPECT_NE(csv.find("GDPR"), std::string::npos);
    EXPECT_NE(csv.find("controls"), std::string::npos);
    // The array value must be present (JSON-serialised and quoted because it contains commas)
    EXPECT_NE(csv.find("ctrl-1"), std::string::npos);
}

TEST_F(ComplianceReportingTest, CsvExport_ExportReportCSV_CommaInValueIsQuoted) {
    ComplianceReporter reporter;

    nlohmann::json test_report;
    test_report["description"] = "Rule A, Rule B";

    auto csv = reporter.exportReport(test_report, ComplianceReporter::ReportFormat::CSV);

    EXPECT_FALSE(csv.empty());
    // Value with embedded comma must be double-quoted
    EXPECT_NE(csv.find("\"Rule A, Rule B\""), std::string::npos);
}
