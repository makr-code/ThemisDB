/**
 * @file test_compliance_security_governance.cpp
 * @brief Compliance governance integration tests using governance module APIs
 *
 * Tests compliance enforcement for:
 * - GDPR (General Data Protection Regulation)
 * - HIPAA (Health Insurance Portability and Accountability Act)
 * - SOC 2 (Service Organization Control 2)
 * - PCI-DSS (Payment Card Industry Data Security Standard)
 * - CCPA/CPRA (California Consumer Privacy Act)
 * - Cross-compliance scenarios
 *
 * All tests use only governance module APIs (PolicyManager, PolicyEngine,
 * Soc2Controls, PciDssRules, CcpaRules) – no external security module
 * dependencies are required.
 *
 * @author ThemisDB Team
 */

#include <gtest/gtest.h>
#include "governance/policy_manager.h"
#include "governance/policy_engine.h"
#include "governance/soc2_controls.h"
#include "governance/pci_dss_rules.h"
#include "governance/ccpa_rules.h"

#include <memory>
#include <string>
#include <vector>

using namespace themis::governance;

// ============================================================================
// Helper functions
// ============================================================================

namespace {

/// Build a PolicyRule representing personal data with GDPR-required controls.
static PolicyRule makeGdprRule(const std::string& id) {
    PolicyRule r;
    r.id                   = id;
    r.name                 = "GDPR Personal Data Rule";
    r.enabled              = true;
    r.resources            = {"users/" + id, "personal_data/" + id};
    r.actions              = {"read", "write", "export"};
    r.required_roles       = {"data_controller", "data_processor"};
    r.classification_level = "vs-nfd";
    r.require_encryption   = true;
    r.require_signature    = false;
    r.audit_access         = true;
    r.audit_changes        = true;
    r.allow_export         = true;
    r.allow_cache          = false;
    r.redaction_level      = "standard";
    r.retention_days       = 365;
    return r;
}

/// Build a PolicyRule representing PHI data with HIPAA-required controls.
static PolicyRule makeHipaaRule(const std::string& id) {
    PolicyRule r;
    r.id                   = id;
    r.name                 = "HIPAA PHI Rule";
    r.enabled              = true;
    r.resources            = {"patients/" + id, "health_records/" + id};
    r.actions              = {"read", "write"};
    r.required_roles       = {"healthcare_provider", "authorized_user"};
    r.classification_level = "geheim";
    r.require_encryption   = true;
    r.require_signature    = true;
    r.audit_access         = true;
    r.audit_changes        = true;
    r.allow_export         = false;
    r.allow_cache          = false;
    r.redaction_level      = "strict";
    r.retention_days       = 2555; // 2555 days (7 years, exceeds 6-year HIPAA federal minimum)
    return r;
}

/// Build a minimally compliant SOC2 rule.
static PolicyRule makeSoc2CompliantRule(const std::string& id) {
    PolicyRule r;
    r.id                   = id;
    r.name                 = "SOC2 Compliant Rule";
    r.enabled              = true;
    r.resources            = {"sensitive/" + id};
    r.actions              = {"read", "write"};
    r.required_roles       = {"soc2_auditor", "data_owner"};
    r.classification_level = "geheim";
    r.require_encryption   = true;
    r.require_signature    = true;
    r.audit_access         = true;
    r.audit_changes        = true;
    r.allow_export         = false;
    r.redaction_level      = "standard";
    r.retention_days       = 365;
    return r;
}

} // anonymous namespace

// ============================================================================
// Test fixture
// ============================================================================

class ComplianceGovernanceTest : public ::testing::Test {
protected:
    void SetUp() override {
        manager_ = std::make_unique<PolicyManager>();
    }

    std::unique_ptr<PolicyManager> manager_;
};

// ============================================================================
// GDPR Compliance Tests – enforce encryption and audit on personal data rules
// ============================================================================

TEST_F(ComplianceGovernanceTest, GDPR_PersonalDataRuleRequiresEncryption) {
    // GDPR Art. 32: appropriate technical security measures include encryption.
    PolicyRule rule = makeGdprRule("gdpr-1");
    manager_->addRule(rule);

    auto stored = manager_->getRule("gdpr-1");
    ASSERT_TRUE(stored.has_value());
    EXPECT_TRUE(stored->require_encryption)
        << "GDPR: personal data rules must require encryption";
}

TEST_F(ComplianceGovernanceTest, GDPR_PersonalDataRuleRequiresAudit) {
    // GDPR Art. 30: records of processing activities – audit access and changes.
    PolicyRule rule = makeGdprRule("gdpr-2");
    manager_->addRule(rule);

    auto stored = manager_->getRule("gdpr-2");
    ASSERT_TRUE(stored.has_value());
    EXPECT_TRUE(stored->audit_access)   << "GDPR: audit_access must be enabled";
    EXPECT_TRUE(stored->audit_changes)  << "GDPR: audit_changes must be enabled";
}

TEST_F(ComplianceGovernanceTest, GDPR_PersonalDataRuleRequiresRoles) {
    // GDPR Art. 28: processor accountability – roles must be defined.
    PolicyRule rule = makeGdprRule("gdpr-3");
    manager_->addRule(rule);

    auto stored = manager_->getRule("gdpr-3");
    ASSERT_TRUE(stored.has_value());
    EXPECT_FALSE(stored->required_roles.empty())
        << "GDPR: required_roles must not be empty";
}

TEST_F(ComplianceGovernanceTest, GDPR_PersonalDataRuleHasRetentionPeriod) {
    // GDPR Art. 5(1)(e): storage limitation – data not kept longer than necessary.
    PolicyRule rule = makeGdprRule("gdpr-4");
    manager_->addRule(rule);

    auto stored = manager_->getRule("gdpr-4");
    ASSERT_TRUE(stored.has_value());
    EXPECT_GT(stored->retention_days, 0)
        << "GDPR: retention_days must be set";
}

TEST_F(ComplianceGovernanceTest, GDPR_PolicyValidationPassesForCompliantRule) {
    // A GDPR-compliant rule set should pass policy validation.
    PolicyRule rule = makeGdprRule("gdpr-5");
    manager_->addRule(rule);

    auto validation = manager_->validateRules();
    EXPECT_TRUE(validation.valid)
        << "GDPR: compliant rule set should pass validation";
}

TEST_F(ComplianceGovernanceTest, GDPR_EvaluatePolicyReflectsEncryptionWhenRoleMatches) {
    // GDPR: when the user has the required role, the decision must reflect
    // the rule's encryption requirement.
    PolicyRule rule = makeGdprRule("gdpr-6");
    manager_->addRule(rule);

    auto decision = manager_->evaluatePolicy(
        "users/gdpr-6", "read", {"data_controller"});
    EXPECT_TRUE(decision.require_encryption)
        << "GDPR: encryption requirement must be reflected in the policy decision";
}

TEST_F(ComplianceGovernanceTest, GDPR_EvaluatePolicyDefaultsPermissiveWithoutMatchingRole) {
    // GDPR: when no rules apply (no matching role), the PolicyManager returns
    // the default permissive decision (allowed=true, no extra restrictions).
    // Note: PolicyManager uses a default-allow model – rules without a matching
    // role are simply skipped rather than resulting in an explicit deny. In a
    // production deployment, per-request authentication and authorization layers
    // upstream of the PolicyManager enforce deny-by-default behaviour.
    PolicyRule rule = makeGdprRule("gdpr-7");
    manager_->addRule(rule);

    auto decision = manager_->evaluatePolicy(
        "users/gdpr-7", "read", {"unrelated_role"});
    // Default permissive: allowed=true, require_encryption=false (no rule applied)
    EXPECT_TRUE(decision.allowed)
        << "GDPR: default decision with no matching rules must be permissive";
    EXPECT_FALSE(decision.require_encryption)
        << "GDPR: encryption requirement must not apply when no rules match";
}

// ============================================================================
// HIPAA Compliance Tests – PHI access controls and audit requirements
// ============================================================================

TEST_F(ComplianceGovernanceTest, HIPAA_PhiRuleRequiresEncryption) {
    // HIPAA Security Rule §164.312(a)(2)(iv): encryption of ePHI.
    PolicyRule rule = makeHipaaRule("hipaa-1");
    manager_->addRule(rule);

    auto stored = manager_->getRule("hipaa-1");
    ASSERT_TRUE(stored.has_value());
    EXPECT_TRUE(stored->require_encryption)
        << "HIPAA: PHI rules must require encryption";
}

TEST_F(ComplianceGovernanceTest, HIPAA_PhiRuleRequiresSignature) {
    // HIPAA Security Rule §164.312(c): integrity controls.
    PolicyRule rule = makeHipaaRule("hipaa-2");
    manager_->addRule(rule);

    auto stored = manager_->getRule("hipaa-2");
    ASSERT_TRUE(stored.has_value());
    EXPECT_TRUE(stored->require_signature)
        << "HIPAA: PHI integrity control requires signature";
}

TEST_F(ComplianceGovernanceTest, HIPAA_PhiRuleDisallowsExport) {
    // HIPAA minimum necessary principle: PHI should not be freely exported.
    PolicyRule rule = makeHipaaRule("hipaa-3");
    manager_->addRule(rule);

    auto stored = manager_->getRule("hipaa-3");
    ASSERT_TRUE(stored.has_value());
    EXPECT_FALSE(stored->allow_export)
        << "HIPAA: PHI export must be disabled";
}

TEST_F(ComplianceGovernanceTest, HIPAA_PhiRuleRequiresStrictRedaction) {
    // HIPAA: PHI must be strictly redacted.
    PolicyRule rule = makeHipaaRule("hipaa-4");
    manager_->addRule(rule);

    auto stored = manager_->getRule("hipaa-4");
    ASSERT_TRUE(stored.has_value());
    EXPECT_EQ(stored->redaction_level, "strict")
        << "HIPAA: PHI must use strict redaction";
}

TEST_F(ComplianceGovernanceTest, HIPAA_PhiRetentionMeetsRequirement) {
    // HIPAA: medical records must be retained for at least 6 years (2190 days).
    // Actual requirements may vary by jurisdiction and record type; this test
    // uses the federal HIPAA minimum as the lower bound.
    PolicyRule rule = makeHipaaRule("hipaa-5");
    manager_->addRule(rule);

    auto stored = manager_->getRule("hipaa-5");
    ASSERT_TRUE(stored.has_value());
    // Our test fixture uses 2555 days (7 years), exceeding the 6-year federal minimum.
    EXPECT_GE(stored->retention_days, 2190)
        << "HIPAA: retention must be at least 6 years (federal minimum)";
}

TEST_F(ComplianceGovernanceTest, HIPAA_PhiClassificationIsGeheimOrHigher) {
    // HIPAA PHI should be classified at the sensitive level.
    PolicyRule rule = makeHipaaRule("hipaa-6");
    manager_->addRule(rule);

    auto stored = manager_->getRule("hipaa-6");
    ASSERT_TRUE(stored.has_value());
    std::string cl = stored->classification_level;
    EXPECT_TRUE(cl == "geheim" || cl == "streng-geheim")
        << "HIPAA: PHI classification must be geheim or streng-geheim";
}

// ============================================================================
// SOC 2 Compliance Tests – Trust Services Criteria via Soc2ControlSet
// ============================================================================

TEST(Soc2ComplianceTest, CC61_CompliantRulePassesCheck) {
    // CC6.1: Logical access controls – encryption + roles required.
    Soc2Cc6Control control;
    PolicyRule rule = makeSoc2CompliantRule("soc2-cc61");
    auto result = control.evaluate(rule);
    EXPECT_TRUE(result.compliant)
        << "SOC2 CC6.1: compliant rule must pass the check";
}

TEST(Soc2ComplianceTest, CC61_NonCompliantRuleFailsCheck) {
    // CC6.1: A rule with no roles and no encryption must fail.
    Soc2Cc6Control control;
    PolicyRule rule;
    rule.id                 = "non-compliant";
    rule.name               = "Non-compliant";
    rule.enabled            = true;
    rule.require_encryption = false;
    rule.required_roles     = {};
    auto result = control.evaluate(rule);
    EXPECT_FALSE(result.compliant)
        << "SOC2 CC6.1: rule with no roles and no encryption must fail";
}

TEST(Soc2ComplianceTest, CC72_AuditLoggingRequired) {
    // CC7.2: system operations – access must be audited.
    Soc2Cc7Control control;
    PolicyRule rule = makeSoc2CompliantRule("soc2-cc72");
    auto result = control.evaluate(rule);
    EXPECT_TRUE(result.compliant)
        << "SOC2 CC7.2: compliant rule must audit access/changes";
}

TEST(Soc2ComplianceTest, CC72_MissingAuditFails) {
    Soc2Cc7Control control;
    PolicyRule rule = makeSoc2CompliantRule("soc2-cc72-fail");
    rule.audit_access  = false;
    rule.audit_changes = false;
    auto result = control.evaluate(rule);
    EXPECT_FALSE(result.compliant)
        << "SOC2 CC7.2: rule without audit must fail";
}

TEST(Soc2ComplianceTest, CC81_ChangeManagementRequiresSignature) {
    // CC8.1: change management – changes must be signed.
    Soc2Cc8Control control;
    PolicyRule rule = makeSoc2CompliantRule("soc2-cc81");
    auto result = control.evaluate(rule);
    EXPECT_TRUE(result.compliant)
        << "SOC2 CC8.1: compliant rule must pass change management check";
}

TEST(Soc2ComplianceTest, A11_AvailabilityRequiresRetention) {
    // A1.1: availability – data retention must be defined.
    Soc2A1Control control;
    PolicyRule rule = makeSoc2CompliantRule("soc2-a11");
    auto result = control.evaluate(rule);
    EXPECT_TRUE(result.compliant)
        << "SOC2 A1.1: compliant rule with retention_days must pass";
}

TEST(Soc2ComplianceTest, C11_ConfidentialityRequiresExportRestriction) {
    // C1.1: confidentiality – confidential data must not be exportable.
    Soc2C1Control control;
    PolicyRule rule = makeSoc2CompliantRule("soc2-c11");
    auto result = control.evaluate(rule);
    EXPECT_TRUE(result.compliant)
        << "SOC2 C1.1: compliant rule with export disabled must pass";
}

TEST(Soc2ComplianceTest, ControlSet_FullyCompliantRulePasses) {
    // All SOC 2 controls should pass for a fully-compliant rule.
    Soc2ControlSet cs;
    PolicyRule rule = makeSoc2CompliantRule("soc2-full");
    EXPECT_TRUE(cs.isRuleCompliant(rule))
        << "SOC2 ControlSet: fully-compliant rule must pass all checks";
}

TEST(Soc2ComplianceTest, ControlSet_NonCompliantRuleFails) {
    Soc2ControlSet cs;
    PolicyRule rule;
    rule.id             = "soc2-bad";
    rule.name           = "Bad Rule";
    rule.enabled        = true;
    rule.require_encryption = false;
    rule.required_roles = {};
    rule.audit_access   = false;
    rule.audit_changes  = false;
    rule.allow_export   = true;
    rule.retention_days = 0;
    EXPECT_FALSE(cs.isRuleCompliant(rule))
        << "SOC2 ControlSet: non-compliant rule must fail";
}

TEST(Soc2ComplianceTest, ControlSet_EvaluateReturnsResultsForEachControl) {
    Soc2ControlSet cs;
    PolicyRule rule = makeSoc2CompliantRule("soc2-results");
    auto results = cs.evaluateRule(rule);
    EXPECT_FALSE(results.empty())
        << "SOC2 ControlSet: evaluateRule must return at least one result";
}

TEST(Soc2ComplianceTest, AuditReport_GeneratedFromPolicyManager) {
    PolicyManager pm;
    pm.addRule(makeSoc2CompliantRule("soc2-report-1"));
    pm.addRule(makeSoc2CompliantRule("soc2-report-2"));

    Soc2ControlSet cs;
    auto report = cs.generateReport(pm);
    EXPECT_FALSE(report.results.empty())
        << "SOC2: audit report must contain control results";
}

// ============================================================================
// PCI-DSS Compliance Tests – cardholder data protection
// ============================================================================

TEST(PciDssComplianceTest, Req1_IsolationCompliantWithRoles) {
    CardholderDataIsolation rule;
    PolicyRule pr;
    pr.id              = "pci-iso";
    pr.name            = "PCI Isolation";
    pr.enabled         = true;
    pr.required_roles  = {"pci_analyst"};
    pr.require_encryption = false;
    EXPECT_TRUE(rule.evaluate(pr))
        << "PCI-DSS Req 1: rule with roles must satisfy isolation";
}

TEST(PciDssComplianceTest, Req1_IsolationNonCompliantWithoutRoles) {
    CardholderDataIsolation rule;
    PolicyRule pr;
    pr.id              = "pci-no-roles";
    pr.name            = "PCI Bad";
    pr.enabled         = true;
    pr.required_roles  = {};
    pr.require_encryption = false;
    EXPECT_FALSE(rule.evaluate(pr))
        << "PCI-DSS Req 1: rule without roles or encryption must fail";
}

TEST(PciDssComplianceTest, Req3_EncryptionRequired) {
    CardholderDataEncryption rule;
    PolicyRule pr;
    pr.id              = "pci-enc";
    pr.name            = "PCI Encryption";
    pr.enabled         = true;
    pr.require_encryption = true;
    EXPECT_TRUE(rule.evaluate(pr))
        << "PCI-DSS Req 3: encrypted rule must pass";
}

TEST(PciDssComplianceTest, Req3_MissingEncryptionFails) {
    CardholderDataEncryption rule;
    PolicyRule pr;
    pr.id              = "pci-no-enc";
    pr.name            = "PCI No Enc";
    pr.enabled         = true;
    pr.require_encryption = false;
    EXPECT_FALSE(rule.evaluate(pr))
        << "PCI-DSS Req 3: rule without encryption must fail";
}

TEST(PciDssComplianceTest, Req7_LeastPrivilegeCompliant) {
    AccessControlLeastPrivilege rule;
    PolicyRule pr;
    pr.id             = "pci-lp";
    pr.name           = "PCI Least Privilege";
    pr.enabled        = true;
    pr.required_roles = {"pci_analyst"};
    pr.actions        = {"read"};
    EXPECT_TRUE(rule.evaluate(pr))
        << "PCI-DSS Req 7: rule with roles and non-wildcard actions must pass";
}

TEST(PciDssComplianceTest, Req10_AuditTrailRequired) {
    CardholderDataAuditTrail rule;
    PolicyRule pr;
    pr.id             = "pci-audit";
    pr.name           = "PCI Audit";
    pr.enabled        = true;
    pr.audit_access   = true;
    pr.audit_changes  = true;
    pr.retention_days = 365;
    EXPECT_TRUE(rule.evaluate(pr))
        << "PCI-DSS Req 10: rule with audit must pass";
}

TEST(PciDssComplianceTest, RuleSet_FullyCompliantPasses) {
    PciDssRuleSet rs;
    PolicyRule pr;
    pr.id              = "pci-full";
    pr.name            = "PCI Full";
    pr.enabled         = true;
    pr.required_roles  = {"pci_analyst"};
    pr.require_encryption = true;
    pr.allow_export    = false;
    pr.audit_access    = true;
    pr.audit_changes   = true;
    pr.retention_days  = 365;
    pr.actions         = {"read", "write"};
    EXPECT_TRUE(rs.isRuleCompliant(pr))
        << "PCI-DSS: fully-compliant rule must pass all checks";
}

TEST(PciDssComplianceTest, RuleSet_EvaluateRuleReturnsResults) {
    PciDssRuleSet rs;
    PolicyRule pr;
    pr.id             = "pci-eval";
    pr.name           = "PCI Eval";
    pr.enabled        = true;
    pr.required_roles = {"pci_analyst"};
    pr.require_encryption = true;
    pr.audit_access   = true;
    pr.audit_changes  = true;
    pr.retention_days = 365;
    pr.actions        = {"read"};
    auto results = rs.evaluateRule(pr);
    EXPECT_FALSE(results.empty())
        << "PCI-DSS: evaluateRule must return results";
}

// ============================================================================
// CCPA Compliance Tests – data subject rights
// ============================================================================

TEST(CcpaComplianceTest, RightToKnow_CompliantWhenAuditEnabled) {
    RightToKnow rule;
    PolicyRule pr;
    pr.id           = "ccpa-rk";
    pr.name         = "CCPA Right to Know";
    pr.enabled      = true;
    pr.audit_access = true;
    EXPECT_TRUE(rule.evaluate(pr))
        << "CCPA: right_to_know requires audit_access=true";
}

TEST(CcpaComplianceTest, RightToKnow_NonCompliantWithoutAudit) {
    RightToKnow rule;
    PolicyRule pr;
    pr.id           = "ccpa-rk-fail";
    pr.name         = "CCPA Right to Know Fail";
    pr.enabled      = true;
    pr.audit_access = false;
    EXPECT_FALSE(rule.evaluate(pr))
        << "CCPA: right_to_know without audit_access must fail";
}

TEST(CcpaComplianceTest, OptOutOfSale_CompliantWhenExportDisabled) {
    OptOutOfSale rule;
    PolicyRule pr;
    pr.id          = "ccpa-optout";
    pr.name        = "CCPA Opt Out";
    pr.enabled     = true;
    pr.allow_export = false;
    EXPECT_TRUE(rule.evaluate(pr))
        << "CCPA: opt_out_of_sale requires allow_export=false";
}

TEST(CcpaComplianceTest, OptOutOfSale_NonCompliantWhenExportEnabled) {
    OptOutOfSale rule;
    PolicyRule pr;
    pr.id          = "ccpa-optout-fail";
    pr.name        = "CCPA Opt Out Fail";
    pr.enabled     = true;
    pr.allow_export = true;
    EXPECT_FALSE(rule.evaluate(pr))
        << "CCPA: opt_out_of_sale with allow_export=true must fail";
}

TEST(CcpaComplianceTest, CcpaRuleSet_FullyCompliantPasses) {
    CcpaRuleSet rs;
    PolicyRule pr;
    pr.id           = "ccpa-full";
    pr.name         = "CCPA Full";
    pr.enabled      = true;
    pr.audit_access  = true;
    pr.audit_changes = true;   // Required by RightToDelete
    pr.allow_export  = false;
    pr.retention_days = 365;
    pr.classification_level = "vs-nfd";
    EXPECT_TRUE(rs.isRuleCompliant(pr))
        << "CCPA: fully-compliant rule must pass all checks";
}

TEST(CcpaComplianceTest, CcpaRuleSet_EvaluateReturnsResults) {
    CcpaRuleSet rs;
    PolicyRule pr;
    pr.id            = "ccpa-results";
    pr.name          = "CCPA Results";
    pr.enabled       = true;
    pr.audit_access  = true;
    pr.audit_changes = true;
    pr.allow_export  = false;
    auto results = rs.evaluateRule(pr);
    EXPECT_FALSE(results.empty())
        << "CCPA: evaluateRule must return results";
}

// ============================================================================
// PolicyEngine – compliance-aware evaluate / simulate
// ============================================================================

TEST(PolicyEngineComplianceTest, EvaluateDefaultDecisionForUnknownRoute) {
    PolicyEngine engine;
    std::unordered_map<std::string, std::string> headers;
    // A default-constructed engine must not crash on evaluate().
    EXPECT_NO_THROW({
        auto decision = engine.evaluate(headers, "/unknown/route");
        (void)decision;
    });
}

TEST(PolicyEngineComplianceTest, SimulateDecisionIsDryRun) {
    PolicyEngine engine;
    SimulationRequest req;
    req.headers["X-Classification"] = "vs-nfd";
    req.route = "/api/data";
    auto result = engine.simulateDecision(req);
    EXPECT_TRUE(result.dry_run)
        << "simulateDecision must always return dry_run=true";
}

TEST(PolicyEngineComplianceTest, SimulateMatchesEvaluateForSameInput) {
    PolicyEngine engine;
    std::unordered_map<std::string, std::string> headers;
    headers["X-Governance-Mode"] = "observe";
    const std::string route = "/api/documents";

    auto eval_decision = engine.evaluate(headers, route);

    SimulationRequest req;
    req.headers = headers;
    req.route   = route;
    auto sim_result = engine.simulateDecision(req);

    EXPECT_EQ(sim_result.decision.mode, eval_decision.mode)
        << "simulateDecision must produce the same mode as evaluate()";
}

// ============================================================================
// Cross-compliance: PolicyManager stats reflect combined rules
// ============================================================================

TEST(CrossComplianceTest, PolicyManagerStatsShowAllRules) {
    PolicyManager pm;
    pm.addRule(makeGdprRule("cross-gdpr"));
    pm.addRule(makeHipaaRule("cross-hipaa"));
    pm.addRule(makeSoc2CompliantRule("cross-soc2"));

    auto stats = pm.getStats();
    EXPECT_EQ(stats.total_rules, 3)
        << "Cross-compliance: manager must hold all three rules";
    EXPECT_EQ(stats.enabled_rules, 3)
        << "Cross-compliance: all rules must be enabled";
}

TEST(CrossComplianceTest, JsonRoundTripPreservesEncryptionFlag) {
    PolicyManager pm;
    PolicyRule rule = makeGdprRule("round-trip");
    pm.addRule(rule);

    auto exported = pm.exportRules();
    PolicyManager pm2;
    ASSERT_TRUE(pm2.importRules(exported));

    auto imported = pm2.getRule("round-trip");
    ASSERT_TRUE(imported.has_value());
    EXPECT_TRUE(imported->require_encryption)
        << "Cross-compliance: encryption flag must survive JSON round-trip";
}

TEST(CrossComplianceTest, GdprAndHipaaRulesCoexistInManager) {
    PolicyManager pm;
    pm.addRule(makeGdprRule("coexist-gdpr"));
    pm.addRule(makeHipaaRule("coexist-hipaa"));

    auto validation = pm.validateRules();
    // Both rules are for different resources so no conflict is expected.
    EXPECT_TRUE(validation.valid)
        << "Cross-compliance: GDPR and HIPAA rules must coexist without conflict";
}

TEST(CrossComplianceTest, AllFrameworksRulesPassIndividualValidation) {
    // Each framework's rule must satisfy its own validation when added alone.
    const std::vector<PolicyRule> framework_rules = {
        makeGdprRule("v-gdpr"),
        makeHipaaRule("v-hipaa"),
        makeSoc2CompliantRule("v-soc2")};
    for (const auto& rule : framework_rules) {
        PolicyManager pm;
        pm.addRule(rule);
        auto res = pm.validateRules();
        EXPECT_TRUE(res.valid)
            << "Cross-compliance: rule '" << rule.id
            << "' must pass standalone validation";
    }
}



