/**
 * @file test_hipaa_rules.cpp
 * @brief Unit tests for HIPAA Security Rule compliance evaluators.
 *
 * Tests cover:
 * - §164.312(a)(1):    Access Control – unique user identification
 * - §164.312(a)(2)(iv): Encryption and Decryption – ePHI at rest
 * - §164.312(b):       Audit Controls – recording access to ePHI
 * - §164.312(c)(1):    Integrity Controls – detecting improper alteration
 * - §164.312(e)(2)(ii): Transmission Security – encrypted export
 * - §164.530(j):       Documentation and Retention – 6-year minimum
 * - HipaaRuleSet: aggregate evaluation, compliance check, PolicyManager integration
 */

#include <gtest/gtest.h>
#include "governance/hipaa_rules.h"
#include "governance/policy_manager.h"

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
                    int  retention_days        = 2190,
                    const std::vector<std::string>& required_roles = {},
                    const std::string& classification = "offen") {
    PolicyRule r;
    r.id                   = id;
    r.name                 = "Rule " + id;
    r.enabled              = true;
    r.resources            = {"phi/" + id};
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
// HipaaAccessControl – §164.312(a)(1)
// ============================================================================

TEST(HipaaAccessControl, CompliantWhenRolesSpecified) {
    HipaaAccessControl rule;
    PolicyRule pr = makeRule("r1", false, true, false, false, 2190, {"phi_clinician"});
    EXPECT_TRUE(rule.evaluate(pr));
}

TEST(HipaaAccessControl, NonCompliantWhenNoRoles) {
    HipaaAccessControl rule;
    PolicyRule pr = makeRule("r1", false, true, false, false, 2190, {});
    EXPECT_FALSE(rule.evaluate(pr));
}

TEST(HipaaAccessControl, DisabledRuleAlwaysCompliant) {
    HipaaAccessControl rule;
    PolicyRule pr = makeRule("r1");
    pr.enabled = false;
    EXPECT_TRUE(rule.evaluate(pr));
}

TEST(HipaaAccessControl, FrameworkAndId) {
    HipaaAccessControl rule;
    EXPECT_EQ(rule.id(), "hipaa_164_312_a1_access_control");
    EXPECT_EQ(rule.framework(), "HIPAA");
    EXPECT_FALSE(rule.description().empty());
}

// ============================================================================
// HipaaEncryption – §164.312(a)(2)(iv)
// ============================================================================

TEST(HipaaEncryption, CompliantWhenEncryptionRequired) {
    HipaaEncryption rule;
    PolicyRule pr = makeRule("r1", /*require_encryption=*/true);
    EXPECT_TRUE(rule.evaluate(pr));
}

TEST(HipaaEncryption, NonCompliantWhenEncryptionMissing) {
    HipaaEncryption rule;
    PolicyRule pr = makeRule("r1", /*require_encryption=*/false);
    EXPECT_FALSE(rule.evaluate(pr));
}

TEST(HipaaEncryption, DisabledRuleAlwaysCompliant) {
    HipaaEncryption rule;
    PolicyRule pr = makeRule("r1", false);
    pr.enabled = false;
    EXPECT_TRUE(rule.evaluate(pr));
}

TEST(HipaaEncryption, FrameworkAndId) {
    HipaaEncryption rule;
    EXPECT_EQ(rule.id(), "hipaa_164_312_a2iv_encryption");
    EXPECT_EQ(rule.framework(), "HIPAA");
    EXPECT_FALSE(rule.description().empty());
}

// ============================================================================
// HipaaAuditControls – §164.312(b)
// ============================================================================

TEST(HipaaAuditControls, CompliantWhenAuditAccessEnabled) {
    HipaaAuditControls rule;
    PolicyRule pr = makeRule("r1", false, true, /*audit_access=*/true);
    EXPECT_TRUE(rule.evaluate(pr));
}

TEST(HipaaAuditControls, NonCompliantWhenAuditAccessDisabled) {
    HipaaAuditControls rule;
    PolicyRule pr = makeRule("r1", false, true, /*audit_access=*/false);
    EXPECT_FALSE(rule.evaluate(pr));
}

TEST(HipaaAuditControls, DisabledRuleAlwaysCompliant) {
    HipaaAuditControls rule;
    PolicyRule pr = makeRule("r1");
    pr.enabled = false;
    EXPECT_TRUE(rule.evaluate(pr));
}

TEST(HipaaAuditControls, FrameworkAndId) {
    HipaaAuditControls rule;
    EXPECT_EQ(rule.id(), "hipaa_164_312_b_audit_controls");
    EXPECT_EQ(rule.framework(), "HIPAA");
    EXPECT_FALSE(rule.description().empty());
}

// ============================================================================
// HipaaIntegrityControls – §164.312(c)(1)
// ============================================================================

TEST(HipaaIntegrityControls, CompliantWhenAuditChangesEnabled) {
    HipaaIntegrityControls rule;
    PolicyRule pr = makeRule("r1", false, true, false, /*audit_changes=*/true);
    EXPECT_TRUE(rule.evaluate(pr));
}

TEST(HipaaIntegrityControls, NonCompliantWhenAuditChangesDisabled) {
    HipaaIntegrityControls rule;
    PolicyRule pr = makeRule("r1", false, true, false, /*audit_changes=*/false);
    EXPECT_FALSE(rule.evaluate(pr));
}

TEST(HipaaIntegrityControls, DisabledRuleAlwaysCompliant) {
    HipaaIntegrityControls rule;
    PolicyRule pr = makeRule("r1");
    pr.enabled = false;
    EXPECT_TRUE(rule.evaluate(pr));
}

TEST(HipaaIntegrityControls, FrameworkAndId) {
    HipaaIntegrityControls rule;
    EXPECT_EQ(rule.id(), "hipaa_164_312_c1_integrity");
    EXPECT_EQ(rule.framework(), "HIPAA");
    EXPECT_FALSE(rule.description().empty());
}

// ============================================================================
// HipaaTransmissionSecurity – §164.312(e)(2)(ii)
// ============================================================================

TEST(HipaaTransmissionSecurity, CompliantWhenExportDisabled) {
    HipaaTransmissionSecurity rule;
    PolicyRule pr = makeRule("r1", false, /*allow_export=*/false);
    EXPECT_TRUE(rule.evaluate(pr));
}

TEST(HipaaTransmissionSecurity, CompliantWhenExportEnabledAndEncryptionRequired) {
    HipaaTransmissionSecurity rule;
    PolicyRule pr = makeRule("r1", /*require_encryption=*/true, /*allow_export=*/true);
    EXPECT_TRUE(rule.evaluate(pr));
}

TEST(HipaaTransmissionSecurity, NonCompliantWhenExportWithoutEncryption) {
    HipaaTransmissionSecurity rule;
    PolicyRule pr = makeRule("r1", /*require_encryption=*/false, /*allow_export=*/true);
    EXPECT_FALSE(rule.evaluate(pr));
}

TEST(HipaaTransmissionSecurity, DisabledRuleAlwaysCompliant) {
    HipaaTransmissionSecurity rule;
    PolicyRule pr = makeRule("r1", false, true);
    pr.enabled = false;
    EXPECT_TRUE(rule.evaluate(pr));
}

TEST(HipaaTransmissionSecurity, FrameworkAndId) {
    HipaaTransmissionSecurity rule;
    EXPECT_EQ(rule.id(), "hipaa_164_312_e2ii_transmission");
    EXPECT_EQ(rule.framework(), "HIPAA");
    EXPECT_FALSE(rule.description().empty());
}

// ============================================================================
// HipaaRetention – §164.530(j)
// ============================================================================

TEST(HipaaRetention, CompliantWhenRetentionMeetsSixYears) {
    HipaaRetention rule;
    PolicyRule pr = makeRule("r1", false, true, false, false, /*retention_days=*/2190);
    EXPECT_TRUE(rule.evaluate(pr));
}

TEST(HipaaRetention, NonCompliantWhenRetentionBelowSixYears) {
    HipaaRetention rule;
    PolicyRule pr = makeRule("r1", false, true, false, false, /*retention_days=*/365);
    EXPECT_FALSE(rule.evaluate(pr));
}

TEST(HipaaRetention, DisabledRuleAlwaysCompliant) {
    HipaaRetention rule;
    PolicyRule pr = makeRule("r1", false, true, false, false, 0);
    pr.enabled = false;
    EXPECT_TRUE(rule.evaluate(pr));
}

TEST(HipaaRetention, FrameworkAndId) {
    HipaaRetention rule;
    EXPECT_EQ(rule.id(), "hipaa_164_530_j_retention");
    EXPECT_EQ(rule.framework(), "HIPAA");
    EXPECT_FALSE(rule.description().empty());
}

// ============================================================================
// HipaaRuleSet – aggregate evaluation
// ============================================================================

TEST(HipaaRuleSet, EvaluateRuleReturnsSixResults) {
    HipaaRuleSet rs;
    PolicyRule pr = makeRule("r1", true, false, true, true, 2190, {"clinician"});
    auto results = rs.evaluateRule(pr);
    EXPECT_EQ(static_cast<int>(results.size()), 6);
}

TEST(HipaaRuleSet, IsRuleCompliantReturnsTrueForFullyCompliantRule) {
    HipaaRuleSet rs;
    PolicyRule pr = makeRule("r1",
        /*require_encryption=*/true,
        /*allow_export=*/false,
        /*audit_access=*/true,
        /*audit_changes=*/true,
        /*retention_days=*/2190,
        /*required_roles=*/{"phi_admin"}
    );
    EXPECT_TRUE(rs.isRuleCompliant(pr));
}

TEST(HipaaRuleSet, IsRuleCompliantReturnsFalseWhenRolesMissing) {
    HipaaRuleSet rs;
    PolicyRule pr = makeRule("r1", true, false, true, true, 2190, /*required_roles=*/{});
    EXPECT_FALSE(rs.isRuleCompliant(pr));
}

TEST(HipaaRuleSet, EvaluateRulePopulatesDescriptions) {
    HipaaRuleSet rs;
    // Non-compliant rule to check descriptions are populated
    PolicyRule pr = makeRule("r1", false, true, false, false, 100, {});
    auto results = rs.evaluateRule(pr);
    for (const auto& r : results) {
        EXPECT_FALSE(r.rule_id.empty());
        EXPECT_FALSE(r.hipaa_check_id.empty());
        EXPECT_FALSE(r.description.empty());
    }
}

TEST(HipaaRuleSet, WithPolicyManager) {
    PolicyManager pm;
    PolicyRule r1 = makeRule("r1", true, false, true, true, 2190, {"clinician"});
    PolicyRule r2 = makeRule("r2", true, false, true, true, 2190, {"nurse"});
    pm.addRule(r1);
    pm.addRule(r2);

    HipaaRuleSet rs;
    EXPECT_TRUE(rs.isRuleCompliant(r1));
    EXPECT_TRUE(rs.isRuleCompliant(r2));

    auto rules = pm.listRules();
    EXPECT_EQ(static_cast<int>(rules.size()), 2);
}
