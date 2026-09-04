#include <gtest/gtest.h>
#include "governance/pci_dss_rules.h"
#include "governance/policy_manager.h"
#include "governance/policy_validation.h"

using namespace themis::governance;

// ============================================================================
// Helpers
// ============================================================================

namespace {

PolicyRule makeRule(const std::string& id,
                    bool require_encryption    = false,
                    bool allow_export          = true,
                    bool audit_access          = false,
                    bool audit_changes         = false,
                    int  retention_days        = 365,
                    const std::vector<std::string>& required_roles = {},
                    const std::string& classification = "offen") {
    PolicyRule r;
    r.id                   = id;
    r.name                 = "Rule " + id;
    r.enabled              = true;
    r.resources            = {"cardholder/" + id};
    r.actions              = {"read", "write"};
    r.classification_level = classification;
    r.require_encryption   = require_encryption;
    r.allow_export         = allow_export;
    r.audit_access         = audit_access;
    r.audit_changes        = audit_changes;
    r.retention_days       = retention_days;
    r.required_roles       = required_roles;
    r.require_signature    = false;
    return r;
}

} // anonymous namespace

// ============================================================================
// CardholderDataIsolation – PCI-DSS Req 1
// ============================================================================

TEST(CardholderDataIsolation, CompliantWhenRolesSpecified) {
    CardholderDataIsolation rule;
    PolicyRule pr = makeRule("r1", false, true, false, false, 365, {"pci_analyst"});
    EXPECT_TRUE(rule.evaluate(pr));
}

TEST(CardholderDataIsolation, CompliantWhenEncryptionRequired) {
    CardholderDataIsolation rule;
    PolicyRule pr = makeRule("r1", /*require_encryption=*/true);
    EXPECT_TRUE(rule.evaluate(pr));
}

TEST(CardholderDataIsolation, NonCompliantWhenNoRolesAndNoEncryption) {
    CardholderDataIsolation rule;
    PolicyRule pr = makeRule("r1", /*require_encryption=*/false,
                             true, false, false, 365, /*required_roles=*/{});
    EXPECT_FALSE(rule.evaluate(pr));
}

TEST(CardholderDataIsolation, DisabledRuleAlwaysCompliant) {
    CardholderDataIsolation rule;
    PolicyRule pr = makeRule("r1");
    pr.enabled = false;
    EXPECT_TRUE(rule.evaluate(pr));
}

TEST(CardholderDataIsolation, FrameworkAndId) {
    CardholderDataIsolation rule;
    EXPECT_EQ(rule.id(), "pci_dss_req_1_isolation");
    EXPECT_EQ(rule.framework(), "PCI-DSS");
    EXPECT_FALSE(rule.description().empty());
}

// ============================================================================
// CardholderDataEncryption – PCI-DSS Req 3
// ============================================================================

TEST(CardholderDataEncryption, CompliantWhenEncryptionRequired) {
    CardholderDataEncryption rule;
    PolicyRule pr = makeRule("r1", /*require_encryption=*/true);
    EXPECT_TRUE(rule.evaluate(pr));
}

TEST(CardholderDataEncryption, NonCompliantWhenEncryptionMissing) {
    CardholderDataEncryption rule;
    PolicyRule pr = makeRule("r1", /*require_encryption=*/false);
    EXPECT_FALSE(rule.evaluate(pr));
}

TEST(CardholderDataEncryption, DisabledRuleAlwaysCompliant) {
    CardholderDataEncryption rule;
    PolicyRule pr = makeRule("r1", false);
    pr.enabled = false;
    EXPECT_TRUE(rule.evaluate(pr));
}

TEST(CardholderDataEncryption, FrameworkAndId) {
    CardholderDataEncryption rule;
    EXPECT_EQ(rule.id(), "pci_dss_req_3_encryption");
    EXPECT_EQ(rule.framework(), "PCI-DSS");
    EXPECT_FALSE(rule.description().empty());
}

// ============================================================================
// TransmissionEncryption – PCI-DSS Req 4
// ============================================================================

TEST(TransmissionEncryption, CompliantWhenExportDisabled) {
    TransmissionEncryption rule;
    PolicyRule pr = makeRule("r1", false, /*allow_export=*/false);
    EXPECT_TRUE(rule.evaluate(pr));
}

TEST(TransmissionEncryption, CompliantWhenExportEnabledAndEncryptionRequired) {
    TransmissionEncryption rule;
    PolicyRule pr = makeRule("r1", /*require_encryption=*/true, /*allow_export=*/true);
    EXPECT_TRUE(rule.evaluate(pr));
}

TEST(TransmissionEncryption, NonCompliantWhenExportEnabledWithoutEncryption) {
    TransmissionEncryption rule;
    PolicyRule pr = makeRule("r1", /*require_encryption=*/false, /*allow_export=*/true);
    EXPECT_FALSE(rule.evaluate(pr));
}

TEST(TransmissionEncryption, DisabledRuleAlwaysCompliant) {
    TransmissionEncryption rule;
    PolicyRule pr = makeRule("r1", false, true);
    pr.enabled = false;
    EXPECT_TRUE(rule.evaluate(pr));
}

TEST(TransmissionEncryption, FrameworkAndId) {
    TransmissionEncryption rule;
    EXPECT_EQ(rule.id(), "pci_dss_req_4_transmission");
    EXPECT_EQ(rule.framework(), "PCI-DSS");
    EXPECT_FALSE(rule.description().empty());
}

// ============================================================================
// AccessControlLeastPrivilege – PCI-DSS Req 7
// ============================================================================

TEST(AccessControlLeastPrivilege, CompliantWhenRolesDefined) {
    AccessControlLeastPrivilege rule;
    PolicyRule pr = makeRule("r1", false, true, false, false, 365,
                             {"admin", "pci_auditor"});
    EXPECT_TRUE(rule.evaluate(pr));
}

TEST(AccessControlLeastPrivilege, NonCompliantWhenRolesEmpty) {
    AccessControlLeastPrivilege rule;
    PolicyRule pr = makeRule("r1", false, true, false, false, 365, {});
    EXPECT_FALSE(rule.evaluate(pr));
}

TEST(AccessControlLeastPrivilege, DisabledRuleAlwaysCompliant) {
    AccessControlLeastPrivilege rule;
    PolicyRule pr = makeRule("r1");
    pr.enabled = false;
    EXPECT_TRUE(rule.evaluate(pr));
}

TEST(AccessControlLeastPrivilege, FrameworkAndId) {
    AccessControlLeastPrivilege rule;
    EXPECT_EQ(rule.id(), "pci_dss_req_7_least_privilege");
    EXPECT_EQ(rule.framework(), "PCI-DSS");
    EXPECT_FALSE(rule.description().empty());
}

// ============================================================================
// CardholderDataAuditTrail – PCI-DSS Req 10
// ============================================================================

TEST(CardholderDataAuditTrail, CompliantWhenAllAuditConditionsMet) {
    CardholderDataAuditTrail rule;
    PolicyRule pr = makeRule("r1", false, false,
                             /*audit_access=*/true, /*audit_changes=*/true,
                             /*retention_days=*/365);
    EXPECT_TRUE(rule.evaluate(pr));
}

TEST(CardholderDataAuditTrail, CompliantWithLongerRetention) {
    CardholderDataAuditTrail rule;
    PolicyRule pr = makeRule("r1", false, false, true, true, 730); // 2 years
    EXPECT_TRUE(rule.evaluate(pr));
}

TEST(CardholderDataAuditTrail, NonCompliantWhenAuditAccessMissing) {
    CardholderDataAuditTrail rule;
    PolicyRule pr = makeRule("r1", false, false,
                             /*audit_access=*/false, /*audit_changes=*/true, 365);
    EXPECT_FALSE(rule.evaluate(pr));
}

TEST(CardholderDataAuditTrail, NonCompliantWhenAuditChangesMissing) {
    CardholderDataAuditTrail rule;
    PolicyRule pr = makeRule("r1", false, false,
                             /*audit_access=*/true, /*audit_changes=*/false, 365);
    EXPECT_FALSE(rule.evaluate(pr));
}

TEST(CardholderDataAuditTrail, NonCompliantWhenRetentionBelowMinimum) {
    CardholderDataAuditTrail rule;
    PolicyRule pr = makeRule("r1", false, false, true, true, /*retention_days=*/180);
    EXPECT_FALSE(rule.evaluate(pr));
}

TEST(CardholderDataAuditTrail, NonCompliantWhenRetentionZero) {
    CardholderDataAuditTrail rule;
    PolicyRule pr = makeRule("r1", false, false, true, true, /*retention_days=*/0);
    EXPECT_FALSE(rule.evaluate(pr));
}

TEST(CardholderDataAuditTrail, DisabledRuleAlwaysCompliant) {
    CardholderDataAuditTrail rule;
    PolicyRule pr = makeRule("r1");
    pr.enabled = false;
    EXPECT_TRUE(rule.evaluate(pr));
}

TEST(CardholderDataAuditTrail, FrameworkAndId) {
    CardholderDataAuditTrail rule;
    EXPECT_EQ(rule.id(), "pci_dss_req_10_audit");
    EXPECT_EQ(rule.framework(), "PCI-DSS");
    EXPECT_FALSE(rule.description().empty());
}

// ============================================================================
// PciDssRuleSet – aggregated evaluation
// ============================================================================

class PciDssRuleSetTest : public ::testing::Test {
protected:
    PciDssRuleSet pci;
};

TEST_F(PciDssRuleSetTest, RuleSetInitializedWithFiveEvaluators) {
    EXPECT_EQ(pci.rules().size(), 5u);
}

TEST_F(PciDssRuleSetTest, EvaluateFullyCompliantRule) {
    // A rule that satisfies all five PCI-DSS checks:
    //   - required_roles non-empty    → Req 1 + Req 7
    //   - require_encryption=true     → Req 3 + Req 4
    //   - allow_export=false          → Req 4
    //   - audit_access + audit_changes + retention >= 365 → Req 10
    PolicyRule rule = makeRule("pci_compliant",
        /*require_encryption=*/true,
        /*allow_export=*/false,
        /*audit_access=*/true,
        /*audit_changes=*/true,
        /*retention_days=*/365,
        /*required_roles=*/{"pci_analyst"});
    EXPECT_TRUE(pci.isRuleCompliant(rule));
}

TEST_F(PciDssRuleSetTest, EvaluateNonCompliantRuleMissingEncryption) {
    PolicyRule rule = makeRule("bad_rule",
        /*require_encryption=*/false, // violates Req 3 and potentially Req 4
        /*allow_export=*/false,
        /*audit_access=*/true,
        /*audit_changes=*/true,
        /*retention_days=*/365,
        /*required_roles=*/{"analyst"});
    EXPECT_FALSE(pci.isRuleCompliant(rule));
}

TEST_F(PciDssRuleSetTest, EvaluateRuleReturnsResultsForAllChecks) {
    PolicyRule rule = makeRule("r1");
    auto results = pci.evaluateRule(rule);
    EXPECT_EQ(results.size(), 5u); // 5 PCI-DSS rules
    for (const auto& r : results) {
        EXPECT_FALSE(r.pci_dss_check_id.empty());
        EXPECT_EQ(r.rule_id, "r1");
    }
}

TEST_F(PciDssRuleSetTest, EvaluateRuleNonCompliantHasRecommendation) {
    PolicyRule rule = makeRule("r1"); // default has require_encryption=false → non-compliant
    auto results = pci.evaluateRule(rule);
    bool found_non_compliant = false;
    for (const auto& r : results) {
        if (!r.compliant) {
            EXPECT_FALSE(r.recommendation.empty());
            found_non_compliant = true;
        }
    }
    EXPECT_TRUE(found_non_compliant);
}

TEST_F(PciDssRuleSetTest, EvaluateRuleCompliantHasEmptyRecommendation) {
    PolicyRule rule = makeRule("compliant",
        true, false, true, true, 365, {"pci_admin"});
    auto results = pci.evaluateRule(rule);
    for (const auto& r : results) {
        if (r.compliant) {
            EXPECT_TRUE(r.recommendation.empty());
        }
    }
}

TEST_F(PciDssRuleSetTest, DisabledRuleIsCompliant) {
    PolicyRule rule = makeRule("disabled"); // non-compliant fields
    rule.enabled = false;
    EXPECT_TRUE(pci.isRuleCompliant(rule));
}

// ============================================================================
// PciDssRuleSet – GDPR conflict detection
// ============================================================================

TEST_F(PciDssRuleSetTest, NoGdprConflictForCompliantRule) {
    PolicyRule rule = makeRule("r1",
        /*require_encryption=*/true,
        /*allow_export=*/false,
        /*audit_access=*/true,
        /*audit_changes=*/true,
        /*retention_days=*/365,
        {"pci_analyst"});
    auto conflicts = pci.detectGdprConflicts(rule);
    EXPECT_TRUE(conflicts.empty());
}

TEST_F(PciDssRuleSetTest, GdprConflictWhenRetentionBelowPciMinimum) {
    PolicyRule rule = makeRule("r1",
        /*require_encryption=*/true,
        /*allow_export=*/false,
        /*audit_access=*/true,
        /*audit_changes=*/true,
        /*retention_days=*/180); // < 365 → PCI-DSS Req 10.7 violation
    auto conflicts = pci.detectGdprConflicts(rule);
    EXPECT_FALSE(conflicts.empty());
    EXPECT_NE(conflicts[0].find("PCI-DSS"), std::string::npos);
    EXPECT_NE(conflicts[0].find("GDPR"), std::string::npos);
}

TEST_F(PciDssRuleSetTest, GdprConflictWhenExportAllowedWithoutEncryption) {
    PolicyRule rule = makeRule("r1",
        /*require_encryption=*/false,
        /*allow_export=*/true, // violates both PCI-DSS Req 4 and GDPR Art. 32
        /*audit_access=*/true,
        /*audit_changes=*/true,
        /*retention_days=*/365);
    auto conflicts = pci.detectGdprConflicts(rule);
    EXPECT_FALSE(conflicts.empty());
    // Should flag both PCI-DSS and GDPR in the conflict description
    bool found_pci = false, found_gdpr = false;
    for (const auto& c : conflicts) {
        if (c.find("PCI-DSS") != std::string::npos) {
          found_pci = true;
        }
        if (c.find("GDPR")    != std::string::npos) {
          found_gdpr = true;
        }
    }
    EXPECT_TRUE(found_pci);
    EXPECT_TRUE(found_gdpr);
}

TEST_F(PciDssRuleSetTest, NoGdprConflictForDisabledRule) {
    PolicyRule rule = makeRule("r1",
        /*require_encryption=*/false,
        /*allow_export=*/true,
        /*audit_access=*/true,
        false, 100);
    rule.enabled = false;
    auto conflicts = pci.detectGdprConflicts(rule);
    EXPECT_TRUE(conflicts.empty());
}

TEST_F(PciDssRuleSetTest, TwoConflictsWhenBothIssuesPresent) {
    // retention_days < 365 AND allow_export without encryption
    PolicyRule rule = makeRule("r1",
        /*require_encryption=*/false,
        /*allow_export=*/true,
        /*audit_access=*/true,
        /*audit_changes=*/true,
        /*retention_days=*/90);
    auto conflicts = pci.detectGdprConflicts(rule);
    EXPECT_GE(conflicts.size(), 2u);
}

// ============================================================================
// PciDssRuleEvalResult JSON serialisation
// ============================================================================

TEST(PciDssRuleEvalResult, ToJson) {
    PciDssRuleEvalResult res;
    res.rule_id          = "pol-1";
    res.pci_dss_check_id = "pci_dss_req_3_encryption";
    res.requirement      = "pci_dss_req_3_encryption";
    res.compliant        = true;
    res.description      = "passes";
    res.recommendation   = "";

    auto j = res.toJson();
    EXPECT_EQ(j["rule_id"].get<std::string>(),          "pol-1");
    EXPECT_EQ(j["pci_dss_check_id"].get<std::string>(), "pci_dss_req_3_encryption");
    EXPECT_TRUE(j["compliant"].get<bool>());
    EXPECT_EQ(j["description"].get<std::string>(),      "passes");
    EXPECT_TRUE(j.contains("recommendation"));
}

TEST(PciDssRuleEvalResult, ToJsonNonCompliant) {
    PciDssRuleEvalResult res;
    res.rule_id          = "bad-rule";
    res.pci_dss_check_id = "pci_dss_req_7_least_privilege";
    res.requirement      = "pci_dss_req_7_least_privilege";
    res.compliant        = false;
    res.description      = "missing roles";
    res.recommendation   = "Add required_roles";

    auto j = res.toJson();
    EXPECT_FALSE(j["compliant"].get<bool>());
    EXPECT_EQ(j["recommendation"].get<std::string>(), "Add required_roles");
}

// ============================================================================
// PolicyValidator PCI-DSS/GDPR cross-framework conflict detection
// ============================================================================

TEST(PolicyValidatorPciDssGdpr, NoConflictsForFullyCompliantRule) {
    PolicyManager mgr;
    PolicyRule rule = makeRule("r1",
        /*require_encryption=*/true,
        /*allow_export=*/false,
        /*audit_access=*/true,
        /*audit_changes=*/true,
        /*retention_days=*/365,
        {"pci_analyst"});
    mgr.addRule(rule);

    PolicyValidator validator;
    auto conflicts = validator.detectPciDssGdprConflicts(mgr);
    EXPECT_TRUE(conflicts.empty());
}

TEST(PolicyValidatorPciDssGdpr, DetectsExportWithoutEncryption) {
    PolicyManager mgr;
    PolicyRule rule = makeRule("r1",
        /*require_encryption=*/false,
        /*allow_export=*/true, // violates PCI-DSS Req 4 + GDPR Art. 32
        /*audit_access=*/true,
        /*audit_changes=*/true,
        /*retention_days=*/365);
    mgr.addRule(rule);

    PolicyValidator validator;
    auto conflicts = validator.detectPciDssGdprConflicts(mgr);
    EXPECT_FALSE(conflicts.empty());
    EXPECT_EQ(conflicts[0].conflict_type, "pci_dss_gdpr");
    EXPECT_FALSE(conflicts[0].description.empty());
    EXPECT_FALSE(conflicts[0].recommendation.empty());
}

TEST(PolicyValidatorPciDssGdpr, DetectsRetentionBelowPciMinimum) {
    PolicyManager mgr;
    PolicyRule rule = makeRule("r1",
        /*require_encryption=*/true,
        /*allow_export=*/false,
        /*audit_access=*/true,
        /*audit_changes=*/true,
        /*retention_days=*/90); // < 365 → PCI-DSS Req 10.7 conflict with GDPR
    mgr.addRule(rule);

    PolicyValidator validator;
    auto conflicts = validator.detectPciDssGdprConflicts(mgr);
    EXPECT_FALSE(conflicts.empty());
    EXPECT_EQ(conflicts[0].conflict_type, "pci_dss_gdpr");
}

TEST(PolicyValidatorPciDssGdpr, IntegratedIntoDetectConflicts) {
    PolicyManager mgr;
    PolicyRule rule = makeRule("r1",
        /*require_encryption=*/false,
        /*allow_export=*/true, // violates PCI-DSS Req 4 + GDPR Art. 32
        /*audit_access=*/true,
        /*audit_changes=*/true,
        /*retention_days=*/365);
    mgr.addRule(rule);

    PolicyValidator validator;
    auto all_conflicts = validator.detectConflicts(mgr);
    bool found_pci_dss_gdpr = false;
    for (const auto& c : all_conflicts) {
        if (c.conflict_type == "pci_dss_gdpr") {
            found_pci_dss_gdpr = true;
        }
    }
    EXPECT_TRUE(found_pci_dss_gdpr);
}

TEST(PolicyValidatorPciDssGdpr, DisabledRulesSkipped) {
    PolicyManager mgr;
    PolicyRule rule = makeRule("r1", false, true, true, false, 90);
    rule.enabled = false;
    mgr.addRule(rule);

    PolicyValidator validator;
    auto conflicts = validator.detectPciDssGdprConflicts(mgr);
    EXPECT_TRUE(conflicts.empty());
}
