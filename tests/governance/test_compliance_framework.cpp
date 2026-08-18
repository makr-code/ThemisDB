/**
 * @file test_compliance_framework.cpp
 * @brief Comprehensive tests for compliance framework integration
 * @version 1.0.0
 * @date 2026-08-18
 * 
 * Test coverage:
 * - GOV-Compliance-01: Framework loading and initialization
 * - GOV-Compliance-02: Requirement and control mapping
 * - GOV-Compliance-03: Compliance validation
 * - GOV-Compliance-04: Violation detection
 * - GOV-Compliance-05: Report generation
 * - GOV-Compliance-06: Evidence collection
 */

#include <gtest/gtest.h>
#include "governance/compliance_framework.h"
#include "governance/compliance_validator.h"
#include "governance/compliance_mapping.h"
#include <nlohmann/json.hpp>
#include <chrono>

using namespace themis::governance;
using json = nlohmann::json;

// ============================================================================
// GOV-Compliance-01: Framework Loading and Initialization
// ============================================================================

class ComplianceFrameworkTest : public ::testing::Test {
protected:
    std::shared_ptr<ComplianceFrameworkRegistry> registry;
    
    void SetUp() override {
        registry = std::make_shared<ComplianceFrameworkRegistry>();
    }
};

TEST_F(ComplianceFrameworkTest, LoadIso27001Framework) {
    auto iso_registry = ComplianceFrameworkLoader::loadFramework(
        ComplianceFramework::kIso27001);
    
    ASSERT_NE(iso_registry, nullptr);
    
    // Should have loaded requirements
    int req_count = iso_registry->getRequirementCount(ComplianceFramework::kIso27001);
    EXPECT_GT(req_count, 0);
    
    // Should have loaded controls
    int ctl_count = iso_registry->getControlCount(ComplianceFramework::kIso27001);
    EXPECT_GT(ctl_count, 0);
}

TEST_F(ComplianceFrameworkTest, GetFrameworkVersion) {
    std::string version = ComplianceFrameworkLoader::getFrameworkVersion(
        ComplianceFramework::kIso27001);
    EXPECT_EQ(version, "ISO 27001:2022");
    
    version = ComplianceFrameworkLoader::getFrameworkVersion(
        ComplianceFramework::kGdpr);
    EXPECT_EQ(version, "GDPR (2016/679/EU)");
    
    version = ComplianceFrameworkLoader::getFrameworkVersion(
        ComplianceFramework::kSoc2TypeI);
    EXPECT_EQ(version, "SOC 2 Type I");
}

// ============================================================================
// GOV-Compliance-02: Requirement and Control Mapping
// ============================================================================

TEST_F(ComplianceFrameworkTest, AddAndRetrieveRequirement) {
    ComplianceRequirement req;
    req.requirement_id = "TEST-REQ-001";
    req.framework = ComplianceFramework::kIso27001;
    req.requirement_text = "Test requirement";
    req.regulatory_section = "Article 1";
    req.severity = ComplianceSeverity::kCritical;
    req.control_ids = {"CTL-001"};
    req.category = "test";
    
    EXPECT_TRUE(registry->addRequirement(req));
    
    auto retrieved = registry->getRequirement("TEST-REQ-001");
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->requirement_id, "TEST-REQ-001");
    EXPECT_EQ(retrieved->framework, ComplianceFramework::kIso27001);
}

TEST_F(ComplianceFrameworkTest, DuplicateRequirementRejected) {
    ComplianceRequirement req;
    req.requirement_id = "DUP-REQ-001";
    req.framework = ComplianceFramework::kIso27001;
    req.requirement_text = "Test requirement";
    
    EXPECT_TRUE(registry->addRequirement(req));
    EXPECT_FALSE(registry->addRequirement(req));  // Should fail on duplicate
}

TEST_F(ComplianceFrameworkTest, AddAndRetrieveControl) {
    ComplianceControl ctl;
    ctl.control_id = "CTL-TEST-001";
    ctl.framework = ComplianceFramework::kIso27001;
    ctl.control_name = "Test Control";
    ctl.description = "A test control";
    ctl.policy_rules = {"rule-001"};
    ctl.evidence_types = {"policy_rule"};
    
    EXPECT_TRUE(registry->addControl(ctl));
    
    auto retrieved = registry->getControl("CTL-TEST-001");
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->control_id, "CTL-TEST-001");
    EXPECT_EQ(retrieved->control_name, "Test Control");
}

TEST_F(ComplianceFrameworkTest, GetRequirementsByFramework) {
    ComplianceRequirement req1, req2;
    req1.requirement_id = "REQ-001";
    req1.framework = ComplianceFramework::kIso27001;
    req2.requirement_id = "REQ-002";
    req2.framework = ComplianceFramework::kGdpr;
    
    registry->addRequirement(req1);
    registry->addRequirement(req2);
    
    auto iso_reqs = registry->getRequirements(ComplianceFramework::kIso27001);
    EXPECT_EQ(iso_reqs.size(), 1);
    EXPECT_EQ(iso_reqs[0].requirement_id, "REQ-001");
    
    auto gdpr_reqs = registry->getRequirements(ComplianceFramework::kGdpr);
    EXPECT_EQ(gdpr_reqs.size(), 1);
    EXPECT_EQ(gdpr_reqs[0].requirement_id, "REQ-002");
}

TEST_F(ComplianceFrameworkTest, GetRequirementsByCategory) {
    ComplianceRequirement req1, req2;
    req1.requirement_id = "REQ-CAT-001";
    req1.framework = ComplianceFramework::kIso27001;
    req1.category = "encryption";
    req2.requirement_id = "REQ-CAT-002";
    req2.framework = ComplianceFramework::kIso27001;
    req2.category = "access-control";
    
    registry->addRequirement(req1);
    registry->addRequirement(req2);
    
    auto enc_reqs = registry->getRequirementsByCategory(
        ComplianceFramework::kIso27001, "encryption");
    EXPECT_EQ(enc_reqs.size(), 1);
    EXPECT_EQ(enc_reqs[0].requirement_id, "REQ-CAT-001");
}

TEST_F(ComplianceFrameworkTest, ExportToJson) {
    ComplianceRequirement req;
    req.requirement_id = "JSON-REQ-001";
    req.framework = ComplianceFramework::kIso27001;
    req.requirement_text = "JSON test";
    registry->addRequirement(req);
    
    auto json_export = registry->exportToJson(ComplianceFramework::kIso27001);
    EXPECT_TRUE(json_export.contains("requirements"));
    EXPECT_TRUE(json_export["requirements"].is_array());
    EXPECT_GT(json_export["requirements"].size(), 0);
}

// ============================================================================
// GOV-Compliance-03: Compliance Validation
// ============================================================================

class ComplianceValidationTest : public ::testing::Test {
protected:
    std::shared_ptr<ComplianceFrameworkRegistry> registry;
    ComplianceValidationEngine engine;
    ComplianceContext ctx;
    
    void SetUp() override {
        registry = std::make_shared<ComplianceFrameworkRegistry>();
        
        // Setup context
        ctx.system_id = "test-system";
        ctx.enabled_frameworks = {"ISO27001"};
        ctx.validation_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }
};

TEST_F(ComplianceValidationTest, ValidateCompliantRequirement) {
    // Create requirement and control
    ComplianceRequirement req;
    req.requirement_id = "VAL-REQ-001";
    req.framework = ComplianceFramework::kIso27001;
    req.control_ids = {"VAL-CTL-001"};
    registry->addRequirement(req);
    
    // Mark control as implemented
    ctx.control_status["VAL-CTL-001"] = true;
    
    auto status = engine.validateRequirement(req, ctx);
    EXPECT_EQ(status, ComplianceStatus::kCompliant);
}

TEST_F(ComplianceValidationTest, ValidateNonCompliantRequirement) {
    ComplianceRequirement req;
    req.requirement_id = "VAL-REQ-002";
    req.framework = ComplianceFramework::kIso27001;
    req.control_ids = {"VAL-CTL-002"};
    registry->addRequirement(req);
    
    // Control NOT implemented
    ctx.control_status["VAL-CTL-002"] = false;
    
    auto status = engine.validateRequirement(req, ctx);
    EXPECT_EQ(status, ComplianceStatus::kNonCompliant);
}

TEST_F(ComplianceValidationTest, ValidatePartiallyCompliantRequirement) {
    ComplianceRequirement req;
    req.requirement_id = "VAL-REQ-003";
    req.framework = ComplianceFramework::kIso27001;
    req.control_ids = {"VAL-CTL-003", "VAL-CTL-004"};
    registry->addRequirement(req);
    
    // Only one control implemented
    ctx.control_status["VAL-CTL-003"] = true;
    ctx.control_status["VAL-CTL-004"] = false;
    
    auto status = engine.validateRequirement(req, ctx);
    EXPECT_EQ(status, ComplianceStatus::kPartiallyCompliant);
}

TEST_F(ComplianceValidationTest, ValidateSingleControl) {
    ComplianceControl ctl;
    ctl.control_id = "CTL-VAL-001";
    ctl.framework = ComplianceFramework::kIso27001;
    ctl.policy_rules = {"policy-001"};
    
    // Control not satisfied
    auto status = engine.validateControl(ctl, ctx);
    EXPECT_EQ(status, ComplianceStatus::kNonCompliant);
    
    // Make policy satisfied
    ctx.policy_state["policy-001"] = "active";
    status = engine.validateControl(ctl, ctx);
    EXPECT_EQ(status, ComplianceStatus::kCompliant);
}

// ============================================================================
// GOV-Compliance-04: Violation Detection
// ============================================================================

TEST_F(ComplianceValidationTest, DetectViolations) {
    ComplianceRequirement req1, req2;
    req1.requirement_id = "VIOL-REQ-001";
    req1.framework = ComplianceFramework::kIso27001;
    req1.control_ids = {"VIOL-CTL-001"};
    req1.severity = ComplianceSeverity::kCritical;
    
    req2.requirement_id = "VIOL-REQ-002";
    req2.framework = ComplianceFramework::kIso27001;
    req2.control_ids = {"VIOL-CTL-002"};
    req2.severity = ComplianceSeverity::kHigh;
    
    registry->addRequirement(req1);
    registry->addRequirement(req2);
    
    // Both controls NOT implemented
    ctx.control_status["VIOL-CTL-001"] = false;
    ctx.control_status["VIOL-CTL-002"] = false;
    
    auto result = engine.validateAll(
        {ComplianceFramework::kIso27001}, registry, ctx);
    
    auto violations = engine.detectViolations(result);
    EXPECT_EQ(violations.size(), 2);
    EXPECT_EQ(violations[0].severity, ComplianceSeverity::kCritical);
}

// ============================================================================
// GOV-Compliance-05: Report Generation
// ============================================================================

TEST_F(ComplianceValidationTest, GenerateFrameworkReport) {
    ComplianceRequirement req1, req2;
    req1.requirement_id = "RPT-REQ-001";
    req1.framework = ComplianceFramework::kIso27001;
    req1.control_ids = {"RPT-CTL-001"};
    
    req2.requirement_id = "RPT-REQ-002";
    req2.framework = ComplianceFramework::kIso27001;
    req2.control_ids = {"RPT-CTL-002"};
    
    registry->addRequirement(req1);
    registry->addRequirement(req2);
    
    // One control compliant, one non-compliant
    ctx.control_status["RPT-CTL-001"] = true;
    ctx.control_status["RPT-CTL-002"] = false;
    
    auto report = engine.validateFramework(
        ComplianceFramework::kIso27001, *registry, ctx);
    
    EXPECT_EQ(report.total_requirements, 2);
    EXPECT_EQ(report.compliant_requirements, 1);
    EXPECT_EQ(report.non_compliant_requirements, 1);
    EXPECT_DOUBLE_EQ(report.compliance_score, 50.0);
    EXPECT_EQ(report.overall_status, ComplianceStatus::kPartiallyCompliant);
    EXPECT_EQ(report.violations.size(), 1);
}

TEST_F(ComplianceValidationTest, ReportHasValidId) {
    ComplianceRequirement req;
    req.requirement_id = "RPT-ID-001";
    req.framework = ComplianceFramework::kIso27001;
    req.control_ids = {"RPT-ID-CTL"};
    registry->addRequirement(req);
    
    ctx.control_status["RPT-ID-CTL"] = true;
    
    auto report = engine.validateFramework(
        ComplianceFramework::kIso27001, *registry, ctx);
    
    EXPECT_FALSE(report.report_id.empty());
    EXPECT_GT(report.generated_at_ms, 0);
}

TEST_F(ComplianceValidationTest, GenerateJsonReport) {
    ComplianceRequirement req;
    req.requirement_id = "JSON-RPT-001";
    req.framework = ComplianceFramework::kIso27001;
    req.control_ids = {"JSON-RPT-CTL"};
    registry->addRequirement(req);
    
    ctx.control_status["JSON-RPT-CTL"] = true;
    
    auto report = engine.validateFramework(
        ComplianceFramework::kIso27001, *registry, ctx);
    
    auto json_report = report.toJson();
    EXPECT_TRUE(json_report.contains("report_id"));
    EXPECT_TRUE(json_report.contains("compliance_score"));
    EXPECT_TRUE(json_report.contains("violations"));
    EXPECT_DOUBLE_EQ(json_report["compliance_score"].get<double>(), 100.0);
}

// ============================================================================
// GOV-Compliance-06: Evidence Collection
// ============================================================================

class EvidenceCollectionTest : public ::testing::Test {
protected:
    ComplianceValidationEngine engine;
    ComplianceContext ctx;
    
    void SetUp() override {
        ctx.system_id = "evidence-test";
        ctx.validation_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }
};

TEST_F(EvidenceCollectionTest, CollectEvidenceFromValidator) {
    ComplianceControl ctl;
    ctl.control_id = "EVID-CTL-001";
    ctl.framework = ComplianceFramework::kIso27001;
    ctl.evidence_types = {"policy_rule", "audit_log", "encryption_status"};
    
    auto validator = std::make_unique<Iso27001Validator>();
    auto evidence = validator->collectEvidence(ctl, ctx);
    
    EXPECT_EQ(evidence.size(), 3);
    
    for (const auto& e : evidence) {
        EXPECT_FALSE(e.evidence_id.empty());
        EXPECT_EQ(e.control_id, "EVID-CTL-001");
        EXPECT_TRUE(e.satisfies_requirement);
        EXPECT_GT(e.timestamp_ms, 0);
    }
}

TEST_F(EvidenceCollectionTest, EvidenceHasCorrectType) {
    ComplianceControl ctl;
    ctl.control_id = "EVID-CTL-002";
    ctl.framework = ComplianceFramework::kIso27001;
    ctl.evidence_types = {"policy_rule", "access_log"};
    
    auto validator = std::make_unique<Iso27001Validator>();
    auto evidence = validator->collectEvidence(ctl, ctx);
    
    EXPECT_EQ(evidence[0].evidence_type, "policy_rule");
    EXPECT_EQ(evidence[1].evidence_type, "access_log");
}

TEST_F(EvidenceCollectionTest, EvidenceToJson) {
    ComplianceEvidence evidence;
    evidence.evidence_id = "EVID-001";
    evidence.control_id = "CTL-001";
    evidence.requirement_id = "REQ-001";
    evidence.evidence_type = "policy_rule";
    evidence.timestamp_ms = 1234567890;
    evidence.detail = "Test evidence";
    evidence.satisfies_requirement = true;
    
    auto json = evidence.toJson();
    EXPECT_TRUE(json.contains("evidence_id"));
    EXPECT_TRUE(json.contains("control_id"));
    EXPECT_TRUE(json.contains("evidence_type"));
    EXPECT_EQ(json["evidence_id"], "EVID-001");
    EXPECT_EQ(json["evidence_type"], "policy_rule");
}

// ============================================================================
// Builder Pattern Tests
// ============================================================================

TEST(BuilderPatternTest, RequirementBuilder) {
    auto req = ComplianceRequirementBuilder()
        .withId("BUILDER-REQ-001")
        .withFramework(ComplianceFramework::kIso27001)
        .withText("Test requirement")
        .withSection("Article 1")
        .withSeverity(ComplianceSeverity::kCritical)
        .withControl("CTL-001")
        .withCategory("security")
        .withMandatory(true)
        .build();
    
    EXPECT_EQ(req.requirement_id, "BUILDER-REQ-001");
    EXPECT_EQ(req.framework, ComplianceFramework::kIso27001);
    EXPECT_EQ(req.severity, ComplianceSeverity::kCritical);
    EXPECT_EQ(req.control_ids.size(), 1);
    EXPECT_EQ(req.control_ids[0], "CTL-001");
}

TEST(BuilderPatternTest, ControlBuilder) {
    auto ctl = ComplianceControlBuilder()
        .withId("BUILDER-CTL-001")
        .withFramework(ComplianceFramework::kIso27001)
        .withName("Test Control")
        .withDescription("A test control")
        .withImplementation("Implementation details")
        .automated(true)
        .withPolicyRule("policy-001")
        .withEvidenceType("policy_rule")
        .build();
    
    EXPECT_EQ(ctl.control_id, "BUILDER-CTL-001");
    EXPECT_EQ(ctl.control_name, "Test Control");
    EXPECT_EQ(ctl.is_automated, true);
    EXPECT_EQ(ctl.policy_rules.size(), 1);
    EXPECT_EQ(ctl.evidence_types.size(), 1);
}

// ============================================================================
// Performance Tests
// ============================================================================

class CompliancePerformanceTest : public ::testing::Test {
protected:
    std::shared_ptr<ComplianceFrameworkRegistry> registry;
    ComplianceValidationEngine engine;
    ComplianceContext ctx;
    
    void SetUp() override {
        registry = std::make_shared<ComplianceFrameworkRegistry>();
        ctx.system_id = "perf-test";
        ctx.validation_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }
};

TEST_F(CompliancePerformanceTest, SingleCheckLatency) {
    ComplianceRequirement req;
    req.requirement_id = "PERF-REQ-001";
    req.framework = ComplianceFramework::kIso27001;
    req.control_ids = {"PERF-CTL-001"};
    registry->addRequirement(req);
    
    ctx.control_status["PERF-CTL-001"] = true;
    
    auto start = std::chrono::high_resolution_clock::now();
    auto status = engine.validateRequirement(req, ctx);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        end - start).count();
    
    // Requirement: ≤1s per check
    EXPECT_LT(elapsed_ms, 1000);
    EXPECT_EQ(status, ComplianceStatus::kCompliant);
}

TEST_F(CompliancePerformanceTest, ReportGenerationLatency) {
    // Add 100 requirements
    for (int i = 0; i < 100; i++) {
        ComplianceRequirement req;
        req.requirement_id = "PERF-REQ-" + std::to_string(i);
        req.framework = ComplianceFramework::kIso27001;
        req.control_ids = {"PERF-CTL-" + std::to_string(i)};
        registry->addRequirement(req);
        ctx.control_status["PERF-CTL-" + std::to_string(i)] = (i % 2 == 0);
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    auto result = engine.validateAll(
        {ComplianceFramework::kIso27001}, registry, ctx);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        end - start).count();
    
    // Requirement: ≤5s for report generation
    EXPECT_LT(elapsed_ms, 5000);
    EXPECT_GT(result.framework_reports.size(), 0);
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST(IntegrationTest, MultiFrameworkValidation) {
    auto registry = std::make_shared<ComplianceFrameworkRegistry>();
    ComplianceValidationEngine engine;
    ComplianceContext ctx;
    
    ctx.system_id = "multi-fw-test";
    ctx.enabled_frameworks = {"ISO27001", "GDPR", "SOC2"};
    
    // Add requirements from multiple frameworks
    ComplianceRequirement iso_req, gdpr_req;
    iso_req.requirement_id = "MULTI-ISO-001";
    iso_req.framework = ComplianceFramework::kIso27001;
    iso_req.control_ids = {"MULTI-CTL-ISO"};
    
    gdpr_req.requirement_id = "MULTI-GDPR-001";
    gdpr_req.framework = ComplianceFramework::kGdpr;
    gdpr_req.control_ids = {"MULTI-CTL-GDPR"};
    
    registry->addRequirement(iso_req);
    registry->addRequirement(gdpr_req);
    
    ctx.control_status["MULTI-CTL-ISO"] = true;
    ctx.control_status["MULTI-CTL-GDPR"] = false;
    
    auto result = engine.validateAll(
        {ComplianceFramework::kIso27001, ComplianceFramework::kGdpr},
        registry, ctx);
    
    EXPECT_EQ(result.framework_reports.size(), 2);
    EXPECT_EQ(result.all_violations.size(), 1);  // One GDPR violation
}
