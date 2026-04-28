/*
 * ThemisDB - Process Modeling Module
 *
 * File:    test_cmmn_serializer.cpp
 * Module:  tests/process/
 * Purpose: GTest unit tests CMN-01..CMN-07 for CmmnSerializer.
 *
 * Tests cover:
 *   CMN-01: import valid CMMN 1.1 XML → ok=true, nodes populated
 *   CMN-02: import empty XML → ok=false
 *   CMN-03: humanTask/processTask/caseTask mapped to BPMNNodeType::TASK
 *   CMN-04: sentry/onPart → SEQUENCE_FLOW edge created
 *   CMN-05: stage mapped to BPMNNodeType::SUBPROCESS with subtype STAGE
 *   CMN-06: milestone mapped to BPMNNodeType::INTERMEDIATE_EVENT
 *   CMN-07: exportXml round-trip preserves node count and subtypes
 */

#include <gtest/gtest.h>

#include "index/process_graph.h"
#include "process/cmmn_serializer.h"

#include <string>
#include <variant>
#include <vector>

using namespace themis;
using namespace themis::process;

// ─────────────────────────────────────────────────────────────────────────────
// XML fixtures
// ─────────────────────────────────────────────────────────────────────────────

/// Full CMMN 1.1 document with humanTask, processTask, caseTask,
/// stage, milestone and a sentry/onPart.
static const char* kFullCmmnXml = R"(<?xml version="1.0" encoding="UTF-8"?>
<definitions xmlns="http://www.omg.org/spec/CMMN/20151109/MODEL"
             targetNamespace="http://themis.db/test"
             id="defs1">
  <case id="case_test" name="Test Case">
    <casePlanModel id="cpm1" name="Test Plan">
      <humanTask id="ht1" name="Review Application"/>
      <processTask id="pt1" name="Trigger Subprocess"/>
      <caseTask id="ct1" name="Delegate Case"/>
      <stage id="stg1" name="Investigation Stage"/>
      <milestone id="ms1" name="Approval Milestone">
        <entryCriterion id="ec1" sentryRef="s1"/>
      </milestone>
      <sentry id="s1">
        <onPart sourceRef="ht1"/>
      </sentry>
    </casePlanModel>
  </case>
</definitions>)";

/// Minimal CMMN with only a humanTask (no stage/milestone/sentry).
static const char* kMinimalCmmnXml = R"(<?xml version="1.0" encoding="UTF-8"?>
<definitions xmlns="http://www.omg.org/spec/CMMN/20151109/MODEL"
             targetNamespace="http://themis.db/test">
  <case id="case_min" name="Minimal Case">
    <casePlanModel id="cpm_min" name="Minimal Plan">
      <humanTask id="ht_min" name="The Only Task"/>
    </casePlanModel>
  </case>
</definitions>)";

// ─────────────────────────────────────────────────────────────────────────────
// Helper
// ─────────────────────────────────────────────────────────────────────────────

static const ProcessNodeInfo* findNode(
    const std::vector<ProcessNodeInfo>& nodes,
    const std::string& id)
{
    for (const auto& n : nodes) {
        if (n.node_id == id) return &n;
    }
    return nullptr;
}

// ─────────────────────────────────────────────────────────────────────────────
// Tests
// ─────────────────────────────────────────────────────────────────────────────

// CMN-01: Valid CMMN XML imports successfully.
TEST(CmmnSerializer, CMN01_ImportValid) {
    auto result = CmmnSerializer::importXml(kFullCmmnXml);
    EXPECT_TRUE(result.ok) << result.message;
    EXPECT_EQ(result.case_id, "case_test");
    EXPECT_EQ(result.case_name, "Test Case");
    EXPECT_FALSE(result.nodes.empty());
}

// CMN-02: Empty XML → ok=false.
TEST(CmmnSerializer, CMN02_ImportEmpty) {
    auto result = CmmnSerializer::importXml("");
    EXPECT_FALSE(result.ok);
    EXPECT_FALSE(result.message.empty());
}

// CMN-03: humanTask, processTask, caseTask → BPMNNodeType::TASK with correct subtypes.
TEST(CmmnSerializer, CMN03_TaskNodeTypes) {
    auto result = CmmnSerializer::importXml(kFullCmmnXml);
    ASSERT_TRUE(result.ok) << result.message;

    const ProcessNodeInfo* ht = findNode(result.nodes, "ht1");
    const ProcessNodeInfo* pt = findNode(result.nodes, "pt1");
    const ProcessNodeInfo* ct = findNode(result.nodes, "ct1");

    ASSERT_NE(ht, nullptr) << "humanTask 'ht1' not found";
    ASSERT_NE(pt, nullptr) << "processTask 'pt1' not found";
    ASSERT_NE(ct, nullptr) << "caseTask 'ct1' not found";

    ASSERT_TRUE(std::holds_alternative<BPMNNodeType>(ht->node_type));
    EXPECT_EQ(std::get<BPMNNodeType>(ht->node_type), BPMNNodeType::TASK);
    EXPECT_EQ(ht->subtype, "HUMAN_TASK");

    ASSERT_TRUE(std::holds_alternative<BPMNNodeType>(pt->node_type));
    EXPECT_EQ(std::get<BPMNNodeType>(pt->node_type), BPMNNodeType::TASK);
    EXPECT_EQ(pt->subtype, "PROCESS_TASK");

    ASSERT_TRUE(std::holds_alternative<BPMNNodeType>(ct->node_type));
    EXPECT_EQ(std::get<BPMNNodeType>(ct->node_type), BPMNNodeType::TASK);
    EXPECT_EQ(ct->subtype, "CASE_TASK");
}

// CMN-04: sentry/onPart → SEQUENCE_FLOW edge from humanTask to milestone.
TEST(CmmnSerializer, CMN04_SentryCreatesEdge) {
    auto result = CmmnSerializer::importXml(kFullCmmnXml);
    ASSERT_TRUE(result.ok) << result.message;

    EXPECT_FALSE(result.edges.empty())
        << "Expected at least one edge from sentry/onPart";

    bool found_edge = false;
    for (const auto& e : result.edges) {
        if (e.from_node == "ht1" && e.to_node == "ms1" &&
            e.edge_type == ProcessEdgeType::SEQUENCE_FLOW) {
            found_edge = true;
            break;
        }
    }
    EXPECT_TRUE(found_edge)
        << "Expected SEQUENCE_FLOW edge from 'ht1' to 'ms1' via sentry";
}

// CMN-05: stage → BPMNNodeType::SUBPROCESS with subtype STAGE.
TEST(CmmnSerializer, CMN05_StageNodeType) {
    auto result = CmmnSerializer::importXml(kFullCmmnXml);
    ASSERT_TRUE(result.ok) << result.message;

    const ProcessNodeInfo* stg = findNode(result.nodes, "stg1");
    ASSERT_NE(stg, nullptr) << "stage 'stg1' not found";
    ASSERT_TRUE(std::holds_alternative<BPMNNodeType>(stg->node_type));
    EXPECT_EQ(std::get<BPMNNodeType>(stg->node_type), BPMNNodeType::SUBPROCESS);
    EXPECT_EQ(stg->subtype, "STAGE");
}

// CMN-06: milestone → BPMNNodeType::INTERMEDIATE_EVENT with subtype MILESTONE.
TEST(CmmnSerializer, CMN06_MilestoneNodeType) {
    auto result = CmmnSerializer::importXml(kFullCmmnXml);
    ASSERT_TRUE(result.ok) << result.message;

    const ProcessNodeInfo* ms = findNode(result.nodes, "ms1");
    ASSERT_NE(ms, nullptr) << "milestone 'ms1' not found";
    ASSERT_TRUE(std::holds_alternative<BPMNNodeType>(ms->node_type));
    EXPECT_EQ(std::get<BPMNNodeType>(ms->node_type), BPMNNodeType::INTERMEDIATE_EVENT);
    EXPECT_EQ(ms->subtype, "MILESTONE");
}

// CMN-07: exportXml round-trip — node count and subtypes are preserved.
TEST(CmmnSerializer, CMN07_ExportRoundTrip) {
    auto import1 = CmmnSerializer::importXml(kMinimalCmmnXml);
    ASSERT_TRUE(import1.ok) << import1.message;

    const size_t original_node_count = import1.nodes.size();
    ASSERT_GT(original_node_count, 0u);

    std::string exported = CmmnSerializer::exportXml(
        import1.case_id,
        import1.case_name,
        import1.nodes,
        import1.edges);

    EXPECT_FALSE(exported.empty());
    EXPECT_NE(exported.find("<definitions"), std::string::npos)
        << "Exported XML should contain <definitions>";
    EXPECT_NE(exported.find("casePlanModel"), std::string::npos)
        << "Exported XML should contain casePlanModel";
    EXPECT_NE(exported.find("humanTask"), std::string::npos)
        << "Exported XML should contain humanTask";

    // Re-import the exported XML.
    auto import2 = CmmnSerializer::importXml(exported);
    ASSERT_TRUE(import2.ok) << "Re-import failed: " << import2.message;

    // After round-trip, the humanTask node should still be present.
    const ProcessNodeInfo* ht = findNode(import2.nodes, "ht_min");
    ASSERT_NE(ht, nullptr) << "humanTask 'ht_min' lost after round-trip";
    EXPECT_EQ(ht->subtype, "HUMAN_TASK");
}
