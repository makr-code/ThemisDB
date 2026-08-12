/**
 * @file test_iso27001_rules.cpp
 * @brief Unit tests for ISO 27001 Annex A compliance controls and evidence collection.
 *
 * Tests cover:
 * - A.9.1.2:  Access Control Policy – least privilege
 * - A.10.1.1: Cryptography Policy – encryption of sensitive resources
 * - A.12.4.1: Event Logging – access and change audit trail
 * - A.12.4.2: Protection of Log Information – retention period
 * - A.13.2.3: Electronic Messaging – transmission security
 * - A.18.1.3: Protection of Records – retention and availability
 * - Iso27001ControlSet: aggregate evaluation, compliance check, report generation
 */

#include <gtest/gtest.h>
#include "governance/iso27001_rules.h"
#include "governance/policy_manager.h"

#include <memory>
#include <string>

using namespace themis::governance;

// ---------------------------------------------------------------------------
// Fixture helpers
// ---------------------------------------------------------------------------

/// Build a fully compliant PolicyRule for an ISO 27001 audit.
static PolicyRule makeCompliantRule(const std::string& id = "rule-iso-1") {
    PolicyRule r;
    r.id                   = id;
    r.name                 = "ISO 27001 Compliant Rule";
    r.enabled              = true;
    r.resources            = {"data/phi"};
    r.actions              = {"read", "write"};
    r.required_roles       = {"security_admin"};
    r.classification_level = "geheim";
    r.require_encryption   = true;
    r.require_signature    = true;
    r.audit_access         = true;
    r.audit_changes        = true;
    r.allow_export         = false;
    r.redaction_level      = "strict";
    r.retention_days       = 365;
    return r;
}

/// Build a PolicyRule that violates all ISO 27001 controls.
static PolicyRule makeNonCompliantRule(const std::string& id = "rule-iso-nc") {
    PolicyRule r;
    r.id                   = id;
    r.name                 = "ISO 27001 Non-Compliant Rule";
    r.enabled              = true;
    r.resources            = {"data/phi"};
    r.actions              = {"*"};
    r.required_roles       = {};          // A.9.1.2 gap
    r.classification_level = "geheim";
    r.require_encryption   = false;       // A.10.1.1 gap
    r.require_signature    = false;
    r.audit_access         = false;       // A.12.4.1 gap
    r.audit_changes        = false;       // A.12.4.1 gap
    r.allow_export         = true;        // A.13.2.3 gap
    r.redaction_level      = "none";
    r.retention_days       = 0;           // A.12.4.2 + A.18.1.3 gap
    return r;
}

// ===========================================================================
// A.9.1.2 – Access Control Policy
// ===========================================================================

TEST(Iso27001A912Control, CompliantWhenRolesSpecified) {
    Iso27001A912Control ctrl;
    auto result = ctrl.evaluate(makeCompliantRule());

    EXPECT_EQ(result.control_id,    "A.9.1.2");
    EXPECT_EQ(result.annex_section, "A.9");
    EXPECT_TRUE(result.compliant);
    EXPECT_TRUE(result.missing_controls.empty());
    EXPECT_FALSE(result.evidence.empty());
}

TEST(Iso27001A912Control, NonCompliantWhenNoRoles) {
    PolicyRule rule = makeCompliantRule();
    rule.required_roles.clear();

    Iso27001A912Control ctrl;
    auto result = ctrl.evaluate(rule);

    EXPECT_FALSE(result.compliant);
    ASSERT_FALSE(result.missing_controls.empty());
    EXPECT_FALSE(result.recommendation.empty());
}

TEST(Iso27001A912Control, DisabledRuleIsCompliant) {
    PolicyRule rule = makeNonCompliantRule();
    rule.enabled = false;

    Iso27001A912Control ctrl;
    auto result = ctrl.evaluate(rule);
    EXPECT_TRUE(result.compliant);
}

// ===========================================================================
// A.10.1.1 – Cryptography Policy
// ===========================================================================

TEST(Iso27001A1011Control, CompliantWhenEncryptionRequired) {
    Iso27001A1011Control ctrl;
    auto result = ctrl.evaluate(makeCompliantRule());

    EXPECT_EQ(result.control_id,    "A.10.1.1");
    EXPECT_EQ(result.annex_section, "A.10");
    EXPECT_TRUE(result.compliant);
    EXPECT_TRUE(result.missing_controls.empty());
}

TEST(Iso27001A1011Control, NonCompliantSensitiveResourceWithoutEncryption) {
    PolicyRule rule = makeCompliantRule();
    rule.require_encryption = false;
    // classification_level = "geheim" → sensitive

    Iso27001A1011Control ctrl;
    auto result = ctrl.evaluate(rule);

    EXPECT_FALSE(result.compliant);
    ASSERT_FALSE(result.missing_controls.empty());
    EXPECT_FALSE(result.recommendation.empty());
}

TEST(Iso27001A1011Control, CompliantNonSensitiveResourceWithoutEncryption) {
    PolicyRule rule = makeCompliantRule();
    rule.classification_level = "offen";
    rule.resources            = {"data/public"};
    rule.require_encryption   = false;

    Iso27001A1011Control ctrl;
    auto result = ctrl.evaluate(rule);
    EXPECT_TRUE(result.compliant);
}

TEST(Iso27001A1011Control, DisabledRuleIsCompliant) {
    PolicyRule rule = makeNonCompliantRule();
    rule.enabled = false;

    Iso27001A1011Control ctrl;
    auto result = ctrl.evaluate(rule);
    EXPECT_TRUE(result.compliant);
}

// ===========================================================================
// A.12.4.1 – Event Logging
// ===========================================================================

TEST(Iso27001A1241Control, CompliantWhenBothAuditFlagsSet) {
    Iso27001A1241Control ctrl;
    auto result = ctrl.evaluate(makeCompliantRule());

    EXPECT_EQ(result.control_id,    "A.12.4.1");
    EXPECT_EQ(result.annex_section, "A.12");
    EXPECT_TRUE(result.compliant);
    EXPECT_TRUE(result.missing_controls.empty());
}

TEST(Iso27001A1241Control, NonCompliantWhenAuditAccessMissing) {
    PolicyRule rule = makeCompliantRule();
    rule.audit_access = false;

    Iso27001A1241Control ctrl;
    auto result = ctrl.evaluate(rule);

    EXPECT_FALSE(result.compliant);
    ASSERT_FALSE(result.missing_controls.empty());
}

TEST(Iso27001A1241Control, NonCompliantWhenAuditChangesMissing) {
    PolicyRule rule = makeCompliantRule();
    rule.audit_changes = false;

    Iso27001A1241Control ctrl;
    auto result = ctrl.evaluate(rule);

    EXPECT_FALSE(result.compliant);
}

TEST(Iso27001A1241Control, DisabledRuleIsCompliant) {
    PolicyRule rule = makeNonCompliantRule();
    rule.enabled = false;

    Iso27001A1241Control ctrl;
    EXPECT_TRUE(ctrl.evaluate(rule).compliant);
}

// ===========================================================================
// A.12.4.2 – Protection of Log Information
// ===========================================================================

TEST(Iso27001A1242Control, CompliantWhenRetentionSufficient) {
    Iso27001A1242Control ctrl;
    auto result = ctrl.evaluate(makeCompliantRule()); // retention_days=365 >= 90

    EXPECT_EQ(result.control_id,    "A.12.4.2");
    EXPECT_EQ(result.annex_section, "A.12");
    EXPECT_TRUE(result.compliant);
    EXPECT_TRUE(result.missing_controls.empty());
}

TEST(Iso27001A1242Control, NonCompliantWhenRetentionBelowMinimum) {
    PolicyRule rule = makeCompliantRule();
    rule.retention_days = 30; // below 90

    Iso27001A1242Control ctrl;
    auto result = ctrl.evaluate(rule);

    EXPECT_FALSE(result.compliant);
    ASSERT_FALSE(result.missing_controls.empty());
    EXPECT_FALSE(result.recommendation.empty());
}

TEST(Iso27001A1242Control, DisabledRuleIsCompliant) {
    PolicyRule rule = makeNonCompliantRule();
    rule.enabled = false;

    Iso27001A1242Control ctrl;
    EXPECT_TRUE(ctrl.evaluate(rule).compliant);
}

// ===========================================================================
// A.13.2.3 – Electronic Messaging
// ===========================================================================

TEST(Iso27001A1323Control, CompliantWhenExportDisabled) {
    Iso27001A1323Control ctrl;
    auto result = ctrl.evaluate(makeCompliantRule()); // allow_export=false

    EXPECT_EQ(result.control_id,    "A.13.2.3");
    EXPECT_EQ(result.annex_section, "A.13");
    EXPECT_TRUE(result.compliant);
}

TEST(Iso27001A1323Control, CompliantWhenExportEnabledWithEncryption) {
    PolicyRule rule = makeCompliantRule();
    rule.allow_export        = true;
    rule.require_encryption  = true;

    Iso27001A1323Control ctrl;
    EXPECT_TRUE(ctrl.evaluate(rule).compliant);
}

TEST(Iso27001A1323Control, NonCompliantWhenExportWithoutEncryption) {
    PolicyRule rule = makeCompliantRule();
    rule.allow_export       = true;
    rule.require_encryption = false;

    Iso27001A1323Control ctrl;
    auto result = ctrl.evaluate(rule);

    EXPECT_FALSE(result.compliant);
    ASSERT_FALSE(result.missing_controls.empty());
    EXPECT_FALSE(result.recommendation.empty());
}

TEST(Iso27001A1323Control, DisabledRuleIsCompliant) {
    PolicyRule rule = makeNonCompliantRule();
    rule.enabled = false;

    Iso27001A1323Control ctrl;
    EXPECT_TRUE(ctrl.evaluate(rule).compliant);
}

// ===========================================================================
// A.18.1.3 – Protection of Records
// ===========================================================================

TEST(Iso27001A1813Control, CompliantWhenRetentionPositive) {
    Iso27001A1813Control ctrl;
    auto result = ctrl.evaluate(makeCompliantRule()); // retention_days=365

    EXPECT_EQ(result.control_id,    "A.18.1.3");
    EXPECT_EQ(result.annex_section, "A.18");
    EXPECT_TRUE(result.compliant);
    EXPECT_TRUE(result.missing_controls.empty());
}

TEST(Iso27001A1813Control, NonCompliantWhenRetentionZero) {
    PolicyRule rule = makeCompliantRule();
    rule.retention_days = 0;

    Iso27001A1813Control ctrl;
    auto result = ctrl.evaluate(rule);

    EXPECT_FALSE(result.compliant);
    ASSERT_FALSE(result.missing_controls.empty());
    EXPECT_FALSE(result.recommendation.empty());
}

TEST(Iso27001A1813Control, DisabledRuleIsCompliant) {
    PolicyRule rule = makeNonCompliantRule();
    rule.enabled = false;

    Iso27001A1813Control ctrl;
    EXPECT_TRUE(ctrl.evaluate(rule).compliant);
}

// ===========================================================================
// Iso27001ControlSet – aggregate evaluation
// ===========================================================================

TEST(Iso27001ControlSet, EvaluateRuleReturnsAllSixControls) {
    Iso27001ControlSet cs;
    auto results = cs.evaluateRule(makeCompliantRule());
    EXPECT_EQ(static_cast<int>(results.size()), 6);
}

TEST(Iso27001ControlSet, IsRuleCompliantReturnsTrueForFullyCompliantRule) {
    Iso27001ControlSet cs;
    EXPECT_TRUE(cs.isRuleCompliant(makeCompliantRule()));
}

TEST(Iso27001ControlSet, IsRuleCompliantReturnsFalseForNonCompliantRule) {
    Iso27001ControlSet cs;
    EXPECT_FALSE(cs.isRuleCompliant(makeNonCompliantRule()));
}

TEST(Iso27001ControlSet, GenerateReportWithPolicyManager) {
    PolicyManager pm;
    PolicyRule r1 = makeCompliantRule("r1");
    PolicyRule r2 = makeCompliantRule("r2");
    pm.addRule(r1);
    pm.addRule(r2);

    Iso27001ControlSet cs;
    auto report = cs.generateReport(pm, "Test scope");

    EXPECT_FALSE(report.report_id.empty());
    EXPECT_EQ(report.scope, "Test scope");
    EXPECT_GT(report.total_controls, 0);
    EXPECT_EQ(report.controls_met, report.total_controls);
    EXPECT_DOUBLE_EQ(report.compliance_score, 100.0);
    EXPECT_FALSE(report.results.empty());
}
