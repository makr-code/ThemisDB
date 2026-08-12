/**
 * @file test_soc2_controls.cpp
 * @brief Unit tests for SOC 2 compliance controls and evidence collection.
 *
 * Tests cover:
 * - CC6.1: Field-level encryption enforcement for sensitive resources
 * - CC7.2: System operations – access and change audit logging
 * - CC8.1: Change management – signature and change audit
 * - A1.1:  Availability – retention period definition
 * - C1.1:  Confidentiality – data classification and export restriction
 * - PI1.2: Processing integrity – audit trail completeness
 * - Soc2ControlSet: aggregate evaluation and evidence collection
 * - Soc2AuditReport: full report generation via PolicyManager
 */

#include <gtest/gtest.h>
#include "governance/soc2_controls.h"
#include "governance/policy_manager.h"
#include "governance/policy_template.h"

#include <memory>
#include <string>

using namespace themis::governance;

// ---------------------------------------------------------------------------
// Fixture helpers
// ---------------------------------------------------------------------------

/// Build a minimally compliant PolicyRule for a sensitive resource.
static PolicyRule makeSensitiveRule(const std::string& id = "rule-1") {
    PolicyRule r;
    r.id                   = id;
    r.name                 = "Sensitive Data Rule";
    r.enabled              = true;
    r.resources            = {"data/users"};
    r.actions              = {"read", "write"};
    r.required_roles       = {"data_admin"};
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

/// Build a PolicyRule that violates all SOC 2 controls.
static PolicyRule makeNonCompliantRule(const std::string& id = "rule-nc") {
    PolicyRule r;
    r.id                   = id;
    r.name                 = "Non-Compliant Rule";
    r.enabled              = true;
    r.resources            = {"data/users"};
    r.actions              = {"*"};
    r.required_roles       = {};           // CC6.1 gap: no roles
    r.classification_level = "geheim";
    r.require_encryption   = false;        // CC6.1 gap
    r.require_signature    = false;        // CC8.1 gap
    r.audit_access         = false;        // CC7.2 + PI1.2 gap
    r.audit_changes        = false;        // CC7.2 + CC8.1 gap
    r.allow_export         = true;         // C1.1 gap
    r.redaction_level      = "none";       // C1.1 gap
    r.retention_days       = 0;            // A1.1 gap
    return r;
}

// ===========================================================================
// CC6.1 – Logical Access Controls
// ===========================================================================

TEST(Soc2Cc6Control, CompliantSensitiveRule) {
    Soc2Cc6Control ctrl;
    auto result = ctrl.evaluate(makeSensitiveRule());

    EXPECT_EQ(result.control_id, "CC6.1");
    EXPECT_EQ(result.criteria,   "CC6");
    EXPECT_TRUE(result.compliant);
    EXPECT_TRUE(result.missing_controls.empty());
    EXPECT_FALSE(result.evidence.empty());
}

TEST(Soc2Cc6Control, NonCompliantMissingEncryption) {
    PolicyRule rule = makeSensitiveRule();
    rule.require_encryption = false;

    Soc2Cc6Control ctrl;
    auto result = ctrl.evaluate(rule);

    EXPECT_FALSE(result.compliant);
    EXPECT_FALSE(result.missing_controls.empty());
    EXPECT_FALSE(result.recommendation.empty());
}

TEST(Soc2Cc6Control, NonCompliantMissingRoles) {
    PolicyRule rule = makeSensitiveRule();
    rule.required_roles.clear();

    Soc2Cc6Control ctrl;
    auto result = ctrl.evaluate(rule);

    EXPECT_FALSE(result.compliant);
    ASSERT_FALSE(result.missing_controls.empty());
}

TEST(Soc2Cc6Control, DisabledRuleIsCompliant) {
    PolicyRule rule = makeNonCompliantRule();
    rule.enabled = false;

    Soc2Cc6Control ctrl;
    auto result = ctrl.evaluate(rule);
    EXPECT_TRUE(result.compliant);
}

TEST(Soc2Cc6Control, NonSensitiveResourceNoEncryptionRequired) {
    PolicyRule rule = makeSensitiveRule();
    rule.classification_level = "offen";  // not sensitive by level
    rule.resources            = {"data/public"};
    rule.require_encryption   = false;
    // required_roles still present → CC6.1 passes for non-sensitive resource

    Soc2Cc6Control ctrl;
    auto result = ctrl.evaluate(rule);
    EXPECT_TRUE(result.compliant);
}

// ===========================================================================
// CC7.2 – System Operations
// ===========================================================================

TEST(Soc2Cc7Control, CompliantFullAudit) {
    Soc2Cc7Control ctrl;
    auto result = ctrl.evaluate(makeSensitiveRule());

    EXPECT_EQ(result.control_id, "CC7.2");
    EXPECT_TRUE(result.compliant);
}

TEST(Soc2Cc7Control, NonCompliantNoAuditAccess) {
    PolicyRule rule = makeSensitiveRule();
    rule.audit_access = false;

    Soc2Cc7Control ctrl;
    EXPECT_FALSE(ctrl.evaluate(rule).compliant);
}

TEST(Soc2Cc7Control, NonCompliantNoAuditChanges) {
    PolicyRule rule = makeSensitiveRule();
    rule.audit_changes = false;

    Soc2Cc7Control ctrl;
    EXPECT_FALSE(ctrl.evaluate(rule).compliant);
}

// ===========================================================================
// CC8.1 – Change Management
// ===========================================================================

TEST(Soc2Cc8Control, CompliantWithSignatureAndAudit) {
    Soc2Cc8Control ctrl;
    auto result = ctrl.evaluate(makeSensitiveRule());

    EXPECT_EQ(result.control_id, "CC8.1");
    EXPECT_TRUE(result.compliant);
}

TEST(Soc2Cc8Control, NonCompliantNoSignature) {
    PolicyRule rule = makeSensitiveRule();
    rule.require_signature = false;

    Soc2Cc8Control ctrl;
    EXPECT_FALSE(ctrl.evaluate(rule).compliant);
}

TEST(Soc2Cc8Control, NonCompliantNoChangeAudit) {
    PolicyRule rule = makeSensitiveRule();
    rule.audit_changes = false;

    Soc2Cc8Control ctrl;
    EXPECT_FALSE(ctrl.evaluate(rule).compliant);
}

// ===========================================================================
// A1.1 – Availability
// ===========================================================================

TEST(Soc2A1Control, CompliantPositiveRetention) {
    Soc2A1Control ctrl;
    auto result = ctrl.evaluate(makeSensitiveRule());

    EXPECT_EQ(result.control_id, "A1.1");
    EXPECT_TRUE(result.compliant);
}

TEST(Soc2A1Control, NonCompliantZeroRetention) {
    PolicyRule rule = makeSensitiveRule();
    rule.retention_days = 0;

    Soc2A1Control ctrl;
    EXPECT_FALSE(ctrl.evaluate(rule).compliant);
}

TEST(Soc2A1Control, NonCompliantNegativeRetention) {
    PolicyRule rule = makeSensitiveRule();
    rule.retention_days = -1;

    Soc2A1Control ctrl;
    EXPECT_FALSE(ctrl.evaluate(rule).compliant);
}

// ===========================================================================
// C1.1 – Confidentiality
// ===========================================================================

TEST(Soc2C1Control, CompliantSensitiveWithEncryptionNoExport) {
    Soc2C1Control ctrl;
    auto result = ctrl.evaluate(makeSensitiveRule());

    EXPECT_EQ(result.control_id, "C1.1");
    EXPECT_TRUE(result.compliant);
}

TEST(Soc2C1Control, NonCompliantExportAllowedOnSensitive) {
    PolicyRule rule = makeSensitiveRule();
    rule.allow_export = true;

    Soc2C1Control ctrl;
    EXPECT_FALSE(ctrl.evaluate(rule).compliant);
}

TEST(Soc2C1Control, NonCompliantNoRedactionOnSensitive) {
    PolicyRule rule = makeSensitiveRule();
    rule.redaction_level = "none";

    Soc2C1Control ctrl;
    EXPECT_FALSE(ctrl.evaluate(rule).compliant);
}

TEST(Soc2C1Control, PublicResourcePassesWithoutRestrictions) {
    PolicyRule rule = makeSensitiveRule();
    rule.classification_level = "offen";
    rule.resources            = {"data/public"};
    rule.require_encryption   = false;
    rule.allow_export         = true;
    rule.redaction_level      = "none";

    Soc2C1Control ctrl;
    // Non-sensitive resource: C1.1 is not applicable
    EXPECT_TRUE(ctrl.evaluate(rule).compliant);
}

// ===========================================================================
// PI1.2 – Processing Integrity
// ===========================================================================

TEST(Soc2Pi1Control, CompliantWithAuditAccess) {
    Soc2Pi1Control ctrl;
    auto result = ctrl.evaluate(makeSensitiveRule());

    EXPECT_EQ(result.control_id, "PI1.2");
    EXPECT_TRUE(result.compliant);
}

TEST(Soc2Pi1Control, NonCompliantNoAuditAccess) {
    PolicyRule rule = makeSensitiveRule();
    rule.audit_access = false;

    Soc2Pi1Control ctrl;
    EXPECT_FALSE(ctrl.evaluate(rule).compliant);
}

// ===========================================================================
// Soc2ControlSet – aggregate evaluation
// ===========================================================================

TEST(Soc2ControlSet, FullyCompliantRule) {
    Soc2ControlSet cs;
    EXPECT_TRUE(cs.isRuleCompliant(makeSensitiveRule()));

    auto results = cs.evaluateRule(makeSensitiveRule());
    EXPECT_EQ(static_cast<int>(results.size()), 6); // One per control
    for (const auto& r : results) {
        EXPECT_TRUE(r.compliant) << "Control " << r.control_id << " failed";
    }
}

TEST(Soc2ControlSet, NonCompliantRuleDetectsAllGaps) {
    Soc2ControlSet cs;
    PolicyRule rule = makeNonCompliantRule();
    EXPECT_FALSE(cs.isRuleCompliant(rule));

    auto results = cs.evaluateRule(rule);
    // At least CC6.1, CC7.2, CC8.1, A1.1, C1.1, PI1.2 should all fail
    int failed = 0;
    for (const auto& r : results) {
        if (!r.compliant) failed++;
    }
    EXPECT_GE(failed, 5) << "Expected at least 5 control failures for fully non-compliant rule";
}

// ===========================================================================
// Soc2ControlSet – evidence collection
// ===========================================================================

TEST(Soc2ControlSet, EvidenceCollectionRecordsItems) {
    Soc2ControlSet cs;
    cs.clearEvidence();

    cs.collectEvidence("data/users", "read", "alice", true, true);
    cs.collectEvidence("data/keys",  "write", "bob",  true, false);

    auto ev = cs.getEvidence();
    ASSERT_EQ(static_cast<int>(ev.size()), 2);
    EXPECT_EQ(ev[0].resource,  "data/users");
    EXPECT_EQ(ev[0].principal, "alice");
    EXPECT_TRUE(ev[0].control_met);   // granted + encrypted → met

    EXPECT_EQ(ev[1].resource, "data/keys");
    EXPECT_FALSE(ev[1].control_met);  // granted but not encrypted → not met
}

TEST(Soc2ControlSet, ClearEvidenceEmptiesCollection) {
    Soc2ControlSet cs;
    cs.collectEvidence("data/x", "read", "carol", true, true);
    EXPECT_FALSE(cs.getEvidence().empty());

    cs.clearEvidence();
    EXPECT_TRUE(cs.getEvidence().empty());
}

TEST(Soc2ControlSet, EvidenceItemsHaveUniqueIds) {
    Soc2ControlSet cs;
    cs.clearEvidence();
    cs.collectEvidence("r1", "read",  "u1", true, true);
    cs.collectEvidence("r2", "write", "u2", true, true);

    auto ev = cs.getEvidence();
    ASSERT_EQ(static_cast<int>(ev.size()), 2);
    EXPECT_NE(ev[0].evidence_id, ev[1].evidence_id);
}

// ===========================================================================
// Soc2ControlSet – audit report generation
// ===========================================================================

TEST(Soc2ControlSet, GenerateReportOnCompliantPolicyManager) {
    auto pm = std::make_shared<PolicyManager>();
    pm->addRule(makeSensitiveRule("r1"));
    pm->addRule(makeSensitiveRule("r2"));

    Soc2ControlSet cs;
    auto report = cs.generateReport(*pm, "unit test scope");

    EXPECT_FALSE(report.report_id.empty());
    EXPECT_GT(report.generated_at_ms, 0LL);
    EXPECT_EQ(report.scope, "unit test scope");
    EXPECT_EQ(report.total_controls, 12);   // 2 rules × 6 controls
    EXPECT_EQ(report.controls_met, 12);
    EXPECT_DOUBLE_EQ(report.compliance_score, 100.0);
    EXPECT_EQ(static_cast<int>(report.results.size()), 12);
}

TEST(Soc2ControlSet, GenerateReportOnNonCompliantPolicyManager) {
    auto pm = std::make_shared<PolicyManager>();
    pm->addRule(makeNonCompliantRule("nc1"));

    Soc2ControlSet cs;
    auto report = cs.generateReport(*pm, "non-compliant scope");

    EXPECT_LT(report.compliance_score, 100.0);
    EXPECT_LT(report.controls_met, report.total_controls);
}

TEST(Soc2ControlSet, GenerateReportSkipsDisabledRules) {
    auto pm = std::make_shared<PolicyManager>();
    PolicyRule disabled = makeNonCompliantRule("disabled-1");
    disabled.enabled = false;
    pm->addRule(disabled);

    Soc2ControlSet cs;
    auto report = cs.generateReport(*pm, "disabled rules test");

    // Disabled rules are skipped entirely
    EXPECT_EQ(report.total_controls, 0);
    EXPECT_DOUBLE_EQ(report.compliance_score, 100.0);
}

// ===========================================================================
// Soc2AuditReport – JSON serialization
// ===========================================================================

TEST(Soc2AuditReport, ToJsonContainsExpectedKeys) {
    Soc2ControlSet cs;
    auto pm = std::make_shared<PolicyManager>();
    pm->addRule(makeSensitiveRule());
    auto report = cs.generateReport(*pm);

    auto j = report.toJson();
    EXPECT_TRUE(j.contains("report_id"));
    EXPECT_TRUE(j.contains("generated_at_ms"));
    EXPECT_TRUE(j.contains("scope"));
    EXPECT_TRUE(j.contains("total_controls"));
    EXPECT_TRUE(j.contains("controls_met"));
    EXPECT_TRUE(j.contains("compliance_score"));
    EXPECT_TRUE(j.contains("results"));
    EXPECT_TRUE(j.contains("evidence_items"));
}

TEST(Soc2ControlResult, ToJsonContainsExpectedKeys) {
    Soc2Cc6Control ctrl;
    auto result = ctrl.evaluate(makeSensitiveRule());
    auto j      = result.toJson();

    EXPECT_TRUE(j.contains("control_id"));
    EXPECT_TRUE(j.contains("criteria"));
    EXPECT_TRUE(j.contains("title"));
    EXPECT_TRUE(j.contains("compliant"));
    EXPECT_TRUE(j.contains("description"));
    EXPECT_TRUE(j.contains("missing_controls"));
    EXPECT_TRUE(j.contains("evidence"));
}

TEST(Soc2EvidenceItem, ToJsonContainsExpectedKeys) {
    Soc2ControlSet cs;
    cs.clearEvidence();
    cs.collectEvidence("data/test", "read", "tester", true, true);
    auto ev = cs.getEvidence();
    ASSERT_FALSE(ev.empty());

    auto j = ev.front().toJson();
    EXPECT_TRUE(j.contains("evidence_id"));
    EXPECT_TRUE(j.contains("control_id"));
    EXPECT_TRUE(j.contains("evidence_type"));
    EXPECT_TRUE(j.contains("timestamp_ms"));
    EXPECT_TRUE(j.contains("resource"));
    EXPECT_TRUE(j.contains("control_met"));
}

// ===========================================================================
// Soc2ComplianceTemplate
// ===========================================================================

TEST(Soc2ComplianceTemplate, InstantiateProducesCompliantRule) {
    PolicyTemplateManager mgr;
    auto tmpl = mgr.getTemplate("soc2_compliance");
    ASSERT_TRUE(tmpl.has_value()) << "soc2_compliance template should be registered";

    nlohmann::json params = {
        {"resource_pattern", "data/patients/*"},
        {"required_role",    "medical_staff"}
    };
    auto rule = (*tmpl)->instantiate(params, "soc2-test-001");

    // Verify all SOC 2 controls are pre-configured
    EXPECT_TRUE(rule.require_encryption);
    EXPECT_TRUE(rule.require_signature);
    EXPECT_TRUE(rule.audit_access);
    EXPECT_TRUE(rule.audit_changes);
    EXPECT_FALSE(rule.allow_export);
    EXPECT_NE(rule.redaction_level, "none");
    EXPECT_GT(rule.retention_days, 0);

    // Validate with Soc2ControlSet
    Soc2ControlSet cs;
    EXPECT_TRUE(cs.isRuleCompliant(rule));
}

TEST(Soc2ComplianceTemplate, TemplateIsListedInManager) {
    PolicyTemplateManager mgr;
    auto templates = mgr.listTemplates();
    bool found = false;
    for (const auto& t : templates) {
        if (t->id == "soc2_compliance") {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found) << "soc2_compliance template should appear in listTemplates()";
}

TEST(Soc2ComplianceTemplate, ComplianceTemplateAcceptsSoc2Framework) {
    PolicyTemplateManager mgr;
    auto tmpl = mgr.getTemplate("compliance");
    ASSERT_TRUE(tmpl.has_value());

    nlohmann::json params = {
        {"resource_pattern",    "audit/logs/*"},
        {"compliance_framework","SOC2"}
    };
    // Should not throw – SOC2 is now in the allowed_values list
    EXPECT_NO_THROW({
        auto rule = (*tmpl)->instantiate(params, "compliance-soc2-001");
        EXPECT_TRUE(rule.require_encryption);
    });
}
