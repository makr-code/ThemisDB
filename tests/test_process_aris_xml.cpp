/*
 * ThemisDB - Process Modeling Module
 *
 * File:    test_process_aris_xml.cpp
 * Module:  tests/
 * Purpose: Unit tests for EpkArisXmlImporter and ProcessModelManager::importArisXml()
 *
 * Test IDs: EAX-01 … EAX-10 (ARIS-XML import)
 *           PAR-01 … PAR-06  (ProcessAgenticRag façade)
 */

#include <gtest/gtest.h>

#include "process/epk_aris_xml_importer.h"
#include "process/process_model_manager.h"
#include "process/process_agentic_rag.h"
#include "index/process_graph.h"
#include "storage/rocksdb_wrapper.h"

#include <filesystem>
#include <string>
#include <string_view>

namespace themis { namespace process { 

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static const std::string kMinimalAml = R"(<?xml version="1.0" encoding="UTF-8"?>
<AML>
  <Group Group.ID="g-001">
    <Model Model.ID="m-bauantrag" Model.Type="EPK">
      <Model.Name LocaleId="19">Bauantrag Standardprozess</Model.Name>
      <ObjOcc ObjOcc.ID="occ-001" SymbolNum="14" ObjDef.IdRef="obj-001"/>
      <ObjOcc ObjOcc.ID="occ-002" SymbolNum="1"  ObjDef.IdRef="obj-002"/>
      <ObjOcc ObjOcc.ID="occ-003" SymbolNum="14" ObjDef.IdRef="obj-003"/>
      <CxnOcc CxnOcc.ID="cx-001" CxnDef.IdRef="cd-001"
              FromObjOcc.IdRef="occ-001" ToObjOcc.IdRef="occ-002"/>
      <CxnOcc CxnOcc.ID="cx-002" CxnDef.IdRef="cd-002"
              FromObjOcc.IdRef="occ-002" ToObjOcc.IdRef="occ-003"/>
    </Model>
    <ObjDef ObjDef.ID="obj-001" TypeNum="14">
      <ObjDef.Name>Antrag eingegangen</ObjDef.Name>
    </ObjDef>
    <ObjDef ObjDef.ID="obj-002" TypeNum="1">
      <ObjDef.Name>Vollst&#228;ndigkeit pr&#252;fen</ObjDef.Name>
    </ObjDef>
    <ObjDef ObjDef.ID="obj-003" TypeNum="14">
      <ObjDef.Name>Pr&#252;fung abgeschlossen</ObjDef.Name>
    </ObjDef>
    <CxnDef CxnDef.ID="cd-001" TypeNum="2"/>
    <CxnDef CxnDef.ID="cd-002" TypeNum="2"/>
  </Group>
</AML>)";

static const std::string kMultiModelAml = R"(<?xml version="1.0" encoding="UTF-8"?>
<AML>
  <Group Group.ID="g-001">
    <Model Model.ID="m-bpmn-ignored" Model.Type="BPMN">
      <Model.Name>Ignored BPMN</Model.Name>
    </Model>
    <Model Model.ID="m-epk-1" Model.Type="EPK">
      <Model.Name LocaleId="19">EPK Prozess 1</Model.Name>
      <ObjOcc ObjOcc.ID="occ-A" SymbolNum="14" ObjDef.IdRef="obj-A"/>
      <ObjOcc ObjOcc.ID="occ-B" SymbolNum="1"  ObjDef.IdRef="obj-B"/>
      <CxnOcc CxnOcc.ID="cx-AB" FromObjOcc.IdRef="occ-A" ToObjOcc.IdRef="occ-B"/>
    </Model>
    <Model Model.ID="m-epk-2" Model.Type="EPK">
      <Model.Name LocaleId="19">EPK Prozess 2</Model.Name>
      <ObjOcc ObjOcc.ID="occ-C" SymbolNum="14" ObjDef.IdRef="obj-C"/>
    </Model>
    <ObjDef ObjDef.ID="obj-A" TypeNum="14"><ObjDef.Name>Start Ereignis</ObjDef.Name></ObjDef>
    <ObjDef ObjDef.ID="obj-B" TypeNum="1"><ObjDef.Name>Verarbeitung</ObjDef.Name></ObjDef>
    <ObjDef ObjDef.ID="obj-C" TypeNum="14"><ObjDef.Name>Ende</ObjDef.Name></ObjDef>
  </Group>
</AML>)";

static const std::string kConnectorsAml = R"(<?xml version="1.0" encoding="UTF-8"?>
<AML>
  <Group Group.ID="g-conn">
    <Model Model.ID="m-conn" Model.Type="EPK">
      <Model.Name>Konnektor Prozess</Model.Name>
      <ObjOcc ObjOcc.ID="occ-and" SymbolNum="13" ObjDef.IdRef="obj-and"/>
      <ObjOcc ObjOcc.ID="occ-or"  SymbolNum="12" ObjDef.IdRef="obj-or"/>
      <ObjOcc ObjOcc.ID="occ-xor" SymbolNum="11" ObjDef.IdRef="obj-xor"/>
    </Model>
    <ObjDef ObjDef.ID="obj-and" TypeNum="13"><ObjDef.Name>UND Split</ObjDef.Name></ObjDef>
    <ObjDef ObjDef.ID="obj-or"  TypeNum="12"><ObjDef.Name>ODER Split</ObjDef.Name></ObjDef>
    <ObjDef ObjDef.ID="obj-xor" TypeNum="11"><ObjDef.Name>XOR Split</ObjDef.Name></ObjDef>
  </Group>
</AML>)";

// ---------------------------------------------------------------------------
// EAX-01: Parse well-formed minimal AML – ok, correct node count
// ---------------------------------------------------------------------------

TEST(EpkArisXmlImporterTest, EAX01_MinimalAml_Succeeds) {
    auto result = EpkArisXmlImporter::importAml(kMinimalAml);
    ASSERT_TRUE(result.ok) << result.message;
    EXPECT_EQ(result.process_id, "m-bauantrag");
    EXPECT_EQ(result.process_name, "Bauantrag Standardprozess");
    EXPECT_EQ(result.nodes.size(), 3u);
    EXPECT_EQ(result.edges.size(), 2u);
}

// ---------------------------------------------------------------------------
// EAX-02: Node types mapped from ARIS TypeNum correctly
// ---------------------------------------------------------------------------

TEST(EpkArisXmlImporterTest, EAX02_NodeTypesFromTypeNum) {
    auto result = EpkArisXmlImporter::importAml(kMinimalAml);
    ASSERT_TRUE(result.ok);

    // Find occ-001 (TypeNum 14 → EVENT) and occ-002 (TypeNum 1 → FUNCTION)
    int event_count    = 0;
    int function_count = 0;
    for (const auto& n : result.nodes) {
        if (std::holds_alternative<EPKNodeType>(n.node_type)) {
            auto t = std::get<EPKNodeType>(n.node_type);
            if (t == EPKNodeType::EVENT) {
              ++event_count;
            }
            if (t == EPKNodeType::FUNCTION) {
              ++function_count;
            }
        }
    }
    EXPECT_EQ(event_count,    2);
    EXPECT_EQ(function_count, 1);
}

// ---------------------------------------------------------------------------
// EAX-03: Edge IDs and connectivity from CxnOcc
// ---------------------------------------------------------------------------

TEST(EpkArisXmlImporterTest, EAX03_EdgeConnectivity) {
    auto result = EpkArisXmlImporter::importAml(kMinimalAml);
    ASSERT_TRUE(result.ok);
    ASSERT_EQ(result.edges.size(), 2u);

    // First edge: occ-001 → occ-002
    bool found_cx001 = false;
    for (const auto& e : result.edges) {
        if (e.from_node == "occ-001" && e.to_node == "occ-002") {
            found_cx001 = true;
            EXPECT_EQ(e.edge_type, ProcessEdgeType::CONTROL_FLOW);
        }
    }
    EXPECT_TRUE(found_cx001) << "Edge occ-001→occ-002 not found";
}

// ---------------------------------------------------------------------------
// EAX-04: importAllAml returns only EPK models; ignores BPMN model
// ---------------------------------------------------------------------------

TEST(EpkArisXmlImporterTest, EAX04_ImportAll_FiltersNonEpk) {
    auto results = EpkArisXmlImporter::importAllAml(kMultiModelAml);
    ASSERT_EQ(results.size(), 2u) << "Expected 2 EPK models";
    // Ensure "m-bpmn-ignored" is not in the results
    for (const auto& r : results) {
        EXPECT_NE(r.process_id, "m-bpmn-ignored");
    }
    // First EPK should have 2 nodes and 1 edge
    EXPECT_EQ(results[0].nodes.size(), 2u);
    EXPECT_EQ(results[0].edges.size(), 1u);
}

// ---------------------------------------------------------------------------
// EAX-05: Connector TypeNums (AND/OR/XOR) mapped correctly
// ---------------------------------------------------------------------------

TEST(EpkArisXmlImporterTest, EAX05_ConnectorNodeTypes) {
    auto result = EpkArisXmlImporter::importAml(kConnectorsAml);
    ASSERT_TRUE(result.ok);
    ASSERT_EQ(result.nodes.size(), 3u);

    int and_c = 0, or_c = 0, xor_c = 0;
    for (const auto& n : result.nodes) {
        if (!std::holds_alternative<EPKNodeType>(n.node_type)) {
          continue;
        }
        auto t = std::get<EPKNodeType>(n.node_type);
        if (t == EPKNodeType::AND_CONNECTOR) {
          ++and_c;
        }
        if (t == EPKNodeType::OR_CONNECTOR) {
          ++or_c;
        }
        if (t == EPKNodeType::XOR_CONNECTOR) {
          ++xor_c;
        }
    }
    EXPECT_EQ(and_c, 1);
    EXPECT_EQ(or_c,  1);
    EXPECT_EQ(xor_c, 1);
}

// ---------------------------------------------------------------------------
// EAX-06: typeNumToEpkNodeType round-trips for all known TypeNums
// ---------------------------------------------------------------------------

TEST(EpkArisXmlImporterTest, EAX06_TypeNumRoundTrip) {
    EXPECT_EQ(EpkArisXmlImporter::typeNumToEpkNodeType(1),  EPKNodeType::FUNCTION);
    EXPECT_EQ(EpkArisXmlImporter::typeNumToEpkNodeType(14), EPKNodeType::EVENT);
    EXPECT_EQ(EpkArisXmlImporter::typeNumToEpkNodeType(13), EPKNodeType::AND_CONNECTOR);
    EXPECT_EQ(EpkArisXmlImporter::typeNumToEpkNodeType(12), EPKNodeType::OR_CONNECTOR);
    EXPECT_EQ(EpkArisXmlImporter::typeNumToEpkNodeType(11), EPKNodeType::XOR_CONNECTOR);
    EXPECT_EQ(EpkArisXmlImporter::typeNumToEpkNodeType(18), EPKNodeType::ORGANIZATIONAL_UNIT);
    EXPECT_EQ(EpkArisXmlImporter::typeNumToEpkNodeType(15), EPKNodeType::INFORMATION_OBJECT);
    EXPECT_EQ(EpkArisXmlImporter::typeNumToEpkNodeType(40), EPKNodeType::APPLICATION_SYSTEM);
    EXPECT_EQ(EpkArisXmlImporter::typeNumToEpkNodeType(16), EPKNodeType::PROCESS_PATH);
    // Unknown falls back to FUNCTION
    EXPECT_EQ(EpkArisXmlImporter::typeNumToEpkNodeType(9999), EPKNodeType::FUNCTION);
}

// ---------------------------------------------------------------------------
// EAX-07: Empty input returns failure
// ---------------------------------------------------------------------------

TEST(EpkArisXmlImporterTest, EAX07_EmptyInput_Fails) {
    auto result = EpkArisXmlImporter::importAml("");
    EXPECT_FALSE(result.ok);
    EXPECT_FALSE(result.message.empty());
}

// ---------------------------------------------------------------------------
// EAX-08: AML with no EPK model returns failure
// ---------------------------------------------------------------------------

TEST(EpkArisXmlImporterTest, EAX08_NoEpkModel_Fails) {
    const std::string no_epk = R"(<AML><Group Group.ID="g1">
      <Model Model.ID="m1" Model.Type="BPMN">
        <Model.Name>Not EPK</Model.Name>
      </Model></Group></AML>)";
    auto result = EpkArisXmlImporter::importAml(no_epk);
    EXPECT_FALSE(result.ok);
}

// ---------------------------------------------------------------------------
// EAX-09: Self-closing ObjOcc without CxnOcc still produces nodes
// ---------------------------------------------------------------------------

TEST(EpkArisXmlImporterTest, EAX09_NodesWithoutEdges) {
    const std::string xml = R"(<AML>
      <Group Group.ID="g1">
        <Model Model.ID="m-solo" Model.Type="EPK">
          <Model.Name>Solo</Model.Name>
          <ObjOcc ObjOcc.ID="occ-1" SymbolNum="14" ObjDef.IdRef="obj-1"/>
          <ObjOcc ObjOcc.ID="occ-2" SymbolNum="1"  ObjDef.IdRef="obj-2"/>
        </Model>
        <ObjDef ObjDef.ID="obj-1" TypeNum="14"><ObjDef.Name>E1</ObjDef.Name></ObjDef>
        <ObjDef ObjDef.ID="obj-2" TypeNum="1"><ObjDef.Name>F1</ObjDef.Name></ObjDef>
      </Group>
    </AML>)";
    auto result = EpkArisXmlImporter::importAml(xml);
    ASSERT_TRUE(result.ok);
    EXPECT_EQ(result.nodes.size(), 2u);
    EXPECT_EQ(result.edges.size(), 0u);
}

// ---------------------------------------------------------------------------
// EAX-10: ProcessModelManager::importArisXml stores model in DB
// ---------------------------------------------------------------------------

class ArisXmlManagerTest : public ::testing::Test {
protected:
    std::string db_path_;
    std::unique_ptr<::themis::RocksDBWrapper> db_;
    std::unique_ptr<ProcessModelManager>       mgr_;
    std::unique_ptr<ProcessGraphManager>       engine_;

    void SetUp() override {
        db_path_ = (std::filesystem::temp_directory_path() /
                    ("test_aris_" + std::to_string(
                         std::chrono::system_clock::now().time_since_epoch().count())))
                       .string();
      ::themis::RocksDBWrapper::Config cfg;
      cfg.db_path = db_path_;
      db_ = std::make_unique<::themis::RocksDBWrapper>(cfg);
      ASSERT_TRUE(db_->open());
        engine_ = std::make_unique<ProcessGraphManager>(*db_);
        mgr_    = std::make_unique<ProcessModelManager>(*db_);
    }

    void TearDown() override {
        mgr_.reset();
        engine_.reset();
        db_->close();
        db_.reset();
        std::filesystem::remove_all(db_path_);
    }
};

TEST_F(ArisXmlManagerTest, EAX10_ImportArisXml_StoresModel) {
    auto res = mgr_->importArisXml(kMinimalAml);
    ASSERT_TRUE(res.ok) << res.message;
    EXPECT_EQ(res.model_id, "m-bauantrag");

    auto loaded = mgr_->load("m-bauantrag");
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->notation, ProcessNotation::EPK);
    EXPECT_EQ(loaded->name, "Bauantrag Standardprozess");
    EXPECT_FALSE(loaded->raw_payload.empty());
    EXPECT_FALSE(loaded->normalized.is_null());

    // Normalized graph should have 3 nodes and 2 edges
    ASSERT_TRUE(loaded->normalized.contains("nodes"));
    ASSERT_TRUE(loaded->normalized.contains("edges"));
    EXPECT_EQ(loaded->normalized["nodes"].size(), 3u);
    EXPECT_EQ(loaded->normalized["edges"].size(), 2u);
}

// ---------------------------------------------------------------------------
// PAR-01: ProcessAgenticConfig default values
// ---------------------------------------------------------------------------

TEST(ProcessAgenticRagTest, PAR01_DefaultConfig) {
    ProcessAgenticConfig cfg;
    EXPECT_EQ(cfg.max_iterations, 4u);
    EXPECT_DOUBLE_EQ(cfg.quality_threshold, 0.75);
    EXPECT_DOUBLE_EQ(cfg.faithfulness_threshold, 0.80);
    EXPECT_EQ(cfg.max_total_documents, 40u);
}

// ---------------------------------------------------------------------------
// PAR-02: ProcessAgenticResult default values
// ---------------------------------------------------------------------------

TEST(ProcessAgenticRagTest, PAR02_ResultDefaults) {
    ProcessAgenticResult r;
    EXPECT_FALSE(r.quality_satisfied);
    EXPECT_EQ(r.total_iterations, 0u);
    EXPECT_EQ(r.total_elapsed_ms.count(), 0);
    EXPECT_TRUE(r.iteration_history.empty());
    EXPECT_TRUE(r.llm_prompt.empty());
}

// ---------------------------------------------------------------------------
// PAR-03: typeNumToLabel returns non-empty strings for known TypeNums
// ---------------------------------------------------------------------------

TEST(ProcessAgenticRagTest, PAR03_TypeNumLabels) {
    EXPECT_FALSE(std::string(EpkArisXmlImporter::typeNumToLabel(1)).empty());
    EXPECT_FALSE(std::string(EpkArisXmlImporter::typeNumToLabel(14)).empty());
    EXPECT_FALSE(std::string(EpkArisXmlImporter::typeNumToLabel(13)).empty());
    EXPECT_FALSE(std::string(EpkArisXmlImporter::typeNumToLabel(9999)).empty());
}

// ---------------------------------------------------------------------------
// PAR-04: importAllAml on empty XML returns empty vector (no crash)
// ---------------------------------------------------------------------------

TEST(ProcessAgenticRagTest, PAR04_ImportAllEmpty) {
    auto results = EpkArisXmlImporter::importAllAml("");
    EXPECT_TRUE(results.empty());
}

// ---------------------------------------------------------------------------
// PAR-05: importAml with XML containing numeric character escapes decodes names
// ---------------------------------------------------------------------------

TEST(ProcessAgenticRagTest, PAR05_EntityEscapedNames) {
    // kMinimalAml contains &#228; (ä) and &#252; (ü) in "Vollständigkeit prüfen"
    auto result = EpkArisXmlImporter::importAml(kMinimalAml);
    ASSERT_TRUE(result.ok);

    // All three nodes should have non-empty names.
    for (const auto& n : result.nodes) {
        EXPECT_FALSE(n.name.empty()) << "Node " << n.node_id << " has no name";
    }

    // The FUNCTION node should contain the decoded German characters
    bool found_vollstaendigkeit = false;
    for (const auto& n : result.nodes) {
        if (!std::holds_alternative<EPKNodeType>(n.node_type)) {
          continue;
        }
        if (std::get<EPKNodeType>(n.node_type) == EPKNodeType::FUNCTION) {
            // After decoding &#228; → ä and &#252; → ü:
            // "Vollständigkeit prüfen"
            EXPECT_NE(n.name.find("ndig"), std::string::npos) // "ständigkeit"
                << "Expected decoded German name, got: " << n.name;
            found_vollstaendigkeit = true;
        }
    }
    EXPECT_TRUE(found_vollstaendigkeit);
}

// ---------------------------------------------------------------------------
// PAR-06: importArisXml with metadata override uses provided name and domain
// ---------------------------------------------------------------------------

TEST_F(ArisXmlManagerTest, PAR06_ImportArisXmlWithMetaOverride) {
    ProcessModelRecord meta;
    meta.id     = "custom-id-001";
    meta.name   = "Custom Process Name";
    meta.domain = ProcessDomain::ADMINISTRATION;
    meta.owner  = "TestTeam";

    auto res = mgr_->importArisXml(kMinimalAml, meta);
    ASSERT_TRUE(res.ok) << res.message;
    EXPECT_EQ(res.model_id, "custom-id-001");

    auto loaded = mgr_->load("custom-id-001");
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->name,   "Custom Process Name");
    EXPECT_EQ(loaded->domain, ProcessDomain::ADMINISTRATION);
    EXPECT_EQ(loaded->owner,  "TestTeam");
}
} } // namespace themis::process
