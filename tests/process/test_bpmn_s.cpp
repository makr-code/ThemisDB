/*
 * ThemisDB - Process Modeling Module
 *
 * File:    test_bpmn_s.cpp
 * Module:  tests/process/
 * Purpose: GTest unit tests BMS-01..BMS-08 for BPMN-S DSGVO security profile
 *          annotations (Task 1: Q4-2026 feature).
 *
 * Tests cover:
 *   BMS-01: import BPMN-S XML with SecurityAnnotation → dsgvo_annotation set
 *   BMS-02: importXml on BPMN without SecurityAnnotation → dsgvo_annotation null
 *   BMS-03: dataCategory="personal", legalBasis="" → DSGVO violation detected
 *   BMS-04: dataCategory="sensitive", legalBasis="Art. 6(1)(b)" → no violation
 *   BMS-05: requiresConsent=true, legalBasis missing Art.6(1)(a) → violation
 *   BMS-06: requiresConsent=true, legalBasis="Art. 6(1)(a) DSGVO" → no violation
 *   BMS-07: retentionDays attribute parsed correctly
 *   BMS-08: exportXml round-trip preserves DSGVO annotation
 */

#include <gtest/gtest.h>

#include "index/process_graph.h"
#include "process/bpmn_serializer.h"

#include <string>
#include <vector>

using namespace themis;
using namespace themis::process;

// ─────────────────────────────────────────────────────────────────────────────
// DSGVO compliance check helper (unit-test local — tests compliance logic
// on ProcessNodeInfo structs directly without needing a live ProcessGraphRag
// instance or RocksDB).
// ─────────────────────────────────────────────────────────────────────────────

struct DsgvoViolation {
    std::string message = {};
};

static std::vector<DsgvoViolation> checkDsgvoNodes(
    const std::vector<ProcessNodeInfo>& nodes)
{
    std::vector<DsgvoViolation> violations = {};

    for (const auto& node : nodes) {
        if (!node.dsgvo_annotation.has_value()) {
          continue;
        }
        const auto& ann = *node.dsgvo_annotation;

        if ((ann.data_category == "personal" || ann.data_category == "sensitive")
            && ann.legal_basis.empty()) {
            violations.push_back(
                {"Node '" + node.name +
                 "' handles personal data but has no DSGVO legal basis"});
        }
        if (ann.requires_consent &&
            ann.legal_basis.find("Art. 6(1)(a)") == std::string::npos) {
            violations.push_back(
                {"Node '" + node.name +
                 "' requires consent but legal_basis does not cite Art. 6(1)(a)"});
        }
    }
    return violations;
}

// ─────────────────────────────────────────────────────────────────────────────
// Test XML fixtures
// ─────────────────────────────────────────────────────────────────────────────

/// Minimal BPMN 2.0 with one userTask carrying a BPMN-S SecurityAnnotation.
static const char* kBpmnSWithAnnotation = R"(<?xml version="1.0" encoding="UTF-8"?>
<definitions xmlns="http://www.omg.org/spec/BPMN/20100524/MODEL"
             targetNamespace="http://themis.db/test">
  <process id="proc_dsgvo" name="DSGVO Test Process">
    <startEvent id="start" name="Start"/>
    <userTask id="ht1" name="Collect Personal Data">
      <extensionElements>
        <bpmns:SecurityAnnotation
            xmlns:bpmns="http://bpmn-s.org/schema"
            dataCategory="personal"
            legalBasis="Art. 6(1)(e) DSGVO"
            retentionDays="365"
            requiresConsent="false"/>
      </extensionElements>
    </userTask>
    <endEvent id="end" name="End"/>
    <sequenceFlow id="sf1" sourceRef="start" targetRef="ht1"/>
    <sequenceFlow id="sf2" sourceRef="ht1" targetRef="end"/>
  </process>
</definitions>)";

/// BPMN 2.0 without any SecurityAnnotation.
static const char* kBpmnNoAnnotation = R"(<?xml version="1.0" encoding="UTF-8"?>
<definitions xmlns="http://www.omg.org/spec/BPMN/20100524/MODEL"
             targetNamespace="http://themis.db/test">
  <process id="proc_plain" name="Plain Process">
    <startEvent id="s1" name="Start"/>
    <task id="t1" name="Do Something"/>
    <endEvent id="e1" name="End"/>
    <sequenceFlow id="sf1" sourceRef="s1" targetRef="t1"/>
    <sequenceFlow id="sf2" sourceRef="t1" targetRef="e1"/>
  </process>
</definitions>)";

/// Task with personal data but no legalBasis (should trigger violation).
static const char* kBpmnPersonalNoLegalBasis = R"(<?xml version="1.0" encoding="UTF-8"?>
<definitions xmlns="http://www.omg.org/spec/BPMN/20100524/MODEL"
             targetNamespace="http://themis.db/test">
  <process id="proc_violation" name="Violation Test">
    <userTask id="ht2" name="Process Personal Data">
      <extensionElements>
        <bpmns:SecurityAnnotation xmlns:bpmns="http://bpmn-s.org/schema"
            dataCategory="personal"
            legalBasis=""
            requiresConsent="false"/>
      </extensionElements>
    </userTask>
  </process>
</definitions>)";

/// Task with sensitive data and a valid legalBasis (no violation).
static const char* kBpmnSensitiveWithBasis = R"(<?xml version="1.0" encoding="UTF-8"?>
<definitions xmlns="http://www.omg.org/spec/BPMN/20100524/MODEL"
             targetNamespace="http://themis.db/test">
  <process id="proc_ok" name="OK Test">
    <userTask id="ht3" name="Handle Sensitive Data">
      <extensionElements>
        <bpmns:SecurityAnnotation xmlns:bpmns="http://bpmn-s.org/schema"
            dataCategory="sensitive"
            legalBasis="Art. 6(1)(b) DSGVO"
            requiresConsent="false"/>
      </extensionElements>
    </userTask>
  </process>
</definitions>)";

/// requiresConsent=true but no Art.6(1)(a) in legalBasis → violation.
static const char* kBpmnConsentNoArt6a = R"(<?xml version="1.0" encoding="UTF-8"?>
<definitions xmlns="http://www.omg.org/spec/BPMN/20100524/MODEL"
             targetNamespace="http://themis.db/test">
  <process id="proc_consent_viol" name="Consent Violation">
    <userTask id="ht4" name="Consent Required Task">
      <extensionElements>
        <bpmns:SecurityAnnotation xmlns:bpmns="http://bpmn-s.org/schema"
            dataCategory="personal"
            legalBasis="Art. 6(1)(b) DSGVO"
            requiresConsent="true"/>
      </extensionElements>
    </userTask>
  </process>
</definitions>)";

/// requiresConsent=true with Art.6(1)(a) → no violation.
static const char* kBpmnConsentWithArt6a = R"(<?xml version="1.0" encoding="UTF-8"?>
<definitions xmlns="http://www.omg.org/spec/BPMN/20100524/MODEL"
             targetNamespace="http://themis.db/test">
  <process id="proc_consent_ok" name="Consent OK">
    <userTask id="ht5" name="Consent OK Task">
      <extensionElements>
        <bpmns:SecurityAnnotation xmlns:bpmns="http://bpmn-s.org/schema"
            dataCategory="personal"
            legalBasis="Art. 6(1)(a) DSGVO"
            requiresConsent="true"/>
      </extensionElements>
    </userTask>
  </process>
</definitions>)";

// ─────────────────────────────────────────────────────────────────────────────
// Tests
// ─────────────────────────────────────────────────────────────────────────────

// BMS-01: SecurityAnnotation is parsed and dsgvo_annotation is populated.
TEST(BpmnS, BMS01_AnnotationParsed) {
    auto result = BpmnSerializer::importXml(kBpmnSWithAnnotation);
    ASSERT_TRUE(result.ok) << result.message;

    // Find the userTask node.
    const ProcessNodeInfo* ht1 = nullptr;
    for (const auto& n : result.nodes) {
        if (n.node_id == "ht1") { ht1 = &n; break; }
    }
    ASSERT_NE(ht1, nullptr) << "Node 'ht1' not found";
    ASSERT_TRUE(ht1->dsgvo_annotation.has_value())
        << "Expected dsgvo_annotation to be populated";

    const auto& ann = *ht1->dsgvo_annotation;
    EXPECT_EQ(ann.data_category, "personal");
    EXPECT_EQ(ann.legal_basis,   "Art. 6(1)(e) DSGVO");
    EXPECT_FALSE(ann.requires_consent);
}

// BMS-02: BPMN without SecurityAnnotation → dsgvo_annotation is null.
TEST(BpmnS, BMS02_NoAnnotationIsNull) {
    auto result = BpmnSerializer::importXml(kBpmnNoAnnotation);
    ASSERT_TRUE(result.ok) << result.message;

    for (const auto& n : result.nodes) {
        EXPECT_FALSE(n.dsgvo_annotation.has_value())
            << "Node '" << n.node_id << "' should have no dsgvo_annotation";
    }
}

// BMS-03: personal data + empty legal basis → violation detected.
TEST(BpmnS, BMS03_PersonalDataNoLegalBasisIsViolation) {
    auto result = BpmnSerializer::importXml(kBpmnPersonalNoLegalBasis);
    ASSERT_TRUE(result.ok) << result.message;

    auto violations = checkDsgvoNodes(result.nodes);
    EXPECT_FALSE(violations.empty())
        << "Expected at least one DSGVO violation for personal data without legal basis";

    bool found = false;
    for (const auto& v : violations) {
        if (v.message.find("personal data but has no DSGVO legal basis") != std::string::npos) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found) << "Expected 'personal data but has no DSGVO legal basis' violation";
}

// BMS-04: sensitive data + valid legal basis → no violation.
TEST(BpmnS, BMS04_SensitiveDataWithLegalBasisNoViolation) {
    auto result = BpmnSerializer::importXml(kBpmnSensitiveWithBasis);
    ASSERT_TRUE(result.ok) << result.message;

    auto violations = checkDsgvoNodes(result.nodes);
    EXPECT_TRUE(violations.empty())
        << "Expected no DSGVO violations for sensitive data with legal basis";
}

// BMS-05: requiresConsent=true without Art.6(1)(a) → violation.
TEST(BpmnS, BMS05_ConsentRequiredWithoutArt6aIsViolation) {
    auto result = BpmnSerializer::importXml(kBpmnConsentNoArt6a);
    ASSERT_TRUE(result.ok) << result.message;

    auto violations = checkDsgvoNodes(result.nodes);
    EXPECT_FALSE(violations.empty())
        << "Expected violation: requires_consent=true but legalBasis lacks Art.6(1)(a)";

    bool found = false;
    for (const auto& v : violations) {
        if (v.message.find("does not cite Art. 6(1)(a)") != std::string::npos) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found) << "Expected 'does not cite Art. 6(1)(a)' violation";
}

// BMS-06: requiresConsent=true with Art.6(1)(a) → no violation.
TEST(BpmnS, BMS06_ConsentRequiredWithArt6aNoViolation) {
    auto result = BpmnSerializer::importXml(kBpmnConsentWithArt6a);
    ASSERT_TRUE(result.ok) << result.message;

    auto violations = checkDsgvoNodes(result.nodes);
    // Only check consent violations (other violations for data_category are ok here).
    bool consent_violation = false;
    for (const auto& v : violations) {
        if (v.message.find("does not cite Art. 6(1)(a)") != std::string::npos) {
            consent_violation = true;
            break;
        }
    }
    EXPECT_FALSE(consent_violation)
        << "Expected no consent violation when Art. 6(1)(a) is cited";
}

// BMS-07: retentionDays attribute is parsed to optional<int>.
TEST(BpmnS, BMS07_RetentionDaysParsed) {
    auto result = BpmnSerializer::importXml(kBpmnSWithAnnotation);
    ASSERT_TRUE(result.ok) << result.message;

    const ProcessNodeInfo* ht1 = nullptr;
    for (const auto& n : result.nodes) {
        if (n.node_id == "ht1") { ht1 = &n; break; }
    }
    ASSERT_NE(ht1, nullptr);
    ASSERT_TRUE(ht1->dsgvo_annotation.has_value());
    ASSERT_TRUE(ht1->dsgvo_annotation->retention_days.has_value())
        << "Expected retention_days to be parsed";
    EXPECT_EQ(*ht1->dsgvo_annotation->retention_days, 365);
}

// BMS-08: exportXml emits <extensionElements><bpmns:SecurityAnnotation …/>
//         and the resulting XML can be re-imported with annotation intact.
TEST(BpmnS, BMS08_ExportRoundTrip) {
    // Import
    auto import1 = BpmnSerializer::importXml(kBpmnSWithAnnotation);
    ASSERT_TRUE(import1.ok) << import1.message;

    // Export
    std::string exported = BpmnSerializer::exportXml(
        import1.process_id,
        import1.process_name,
        import1.nodes,
        import1.edges);

    EXPECT_FALSE(exported.empty());
    // The export should contain BPMN-S namespace and SecurityAnnotation
    EXPECT_NE(exported.find("extensionElements"), std::string::npos)
        << "Exported XML should contain <extensionElements>";
    EXPECT_NE(exported.find("SecurityAnnotation"), std::string::npos)
        << "Exported XML should contain SecurityAnnotation";
    EXPECT_NE(exported.find("dataCategory"), std::string::npos)
        << "Exported XML should contain dataCategory attribute";

    // Re-import
    auto import2 = BpmnSerializer::importXml(exported);
    ASSERT_TRUE(import2.ok) << import2.message;

    const ProcessNodeInfo* ht1 = nullptr;
    for (const auto& n : import2.nodes) {
        if (n.node_id == "ht1") { ht1 = &n; break; }
    }
    ASSERT_NE(ht1, nullptr) << "Node 'ht1' not found after round-trip";
    ASSERT_TRUE(ht1->dsgvo_annotation.has_value())
        << "dsgvo_annotation lost after export/re-import";
    EXPECT_EQ(ht1->dsgvo_annotation->data_category, "personal");
    EXPECT_EQ(ht1->dsgvo_annotation->legal_basis, "Art. 6(1)(e) DSGVO");
}
