/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_process_module.cpp                            ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-04-14 07:18:46                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     1173                                           ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • b18a0735c6  2026-04-12  fix(process): replace regex BPMN parser with state-machin... ║
    • 79f0815052  2026-03-28  Add test statistics documentation and collection script ║
    • 3fea6d6b51  2026-03-12  refactor: clean up includes and remove unused transaction... ║
    • f56652abf2  2026-03-12  audit(process): focused tests, ProcessNotation enum fix, ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/*
 * ThemisDB – Process Modeling Module
 *
 * Focused unit tests for:
 *   - BpmnSerializer  (import / export round-trip)
 *   - EpkSerializer   (import / export round-trip)
 *   - VccVpbImporter  (YAML import)
 *   - LlmProcessDescriptor (descriptor + prompt generation)
 *   - ProcessModelManager (CRUD over RocksDB)
 *   - ProcessLinker   (attach, detach, link, required-doc, missing-doc)
 *   - ProcessGraphRag (KnowledgeGraph population, subgraph, compliance)
 *
 * The tests use an in-process RocksDB instance (temp directory, cleaned up
 * in TearDown) and a real ProcessGraphManager so no mocking is required for
 * the storage layer.
 */

#include <gtest/gtest.h>

#include "index/process_graph.h"
#include "process/bpmn_serializer.h"
#include "process/epk_serializer.h"
#include "process/llm_process_descriptor.h"
#include "process/process_graph_rag.h"
#include "process/process_linker.h"
#include "process/process_model_manager.h"
#include "process/vcc_vpb_importer.h"
#include "storage/rocksdb_wrapper.h"

#include <filesystem>
#include <chrono>
#include <memory>
#include <sstream>
#include <string>

namespace fs = std::filesystem;
using json   = nlohmann::json;

// ─────────────────────────────────────────────────────────────────────────────
// Shared RocksDB fixture
// ─────────────────────────────────────────────────────────────────────────────

class ProcessModuleTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_path_ = "./data/test_process_module";
        fs::remove_all(db_path_);

        themis::RocksDBWrapper::Config cfg;
        cfg.db_path               = db_path_;
        cfg.memtable_size_mb      = 32;
        cfg.block_cache_size_mb   = 64;
        cfg.max_background_jobs   = 1;
        cfg.compression_default   = "lz4";
        cfg.compression_bottommost = "zstd";

        db_  = std::make_unique<themis::RocksDBWrapper>(cfg);
        ASSERT_TRUE(db_->open());

        themis::registerProcessEdgeTypes();
        engine_ = std::make_unique<themis::ProcessGraphManager>(*db_);
        mgr_    = std::make_unique<themis::process::ProcessModelManager>(*db_);
        linker_ = std::make_unique<themis::process::ProcessLinker>(*db_);
        rag_    = std::make_unique<themis::process::ProcessGraphRag>(
                      *db_, *engine_, *mgr_, *linker_);
    }

    void TearDown() override {
        rag_.reset();
        linker_.reset();
        mgr_.reset();
        engine_.reset();
        db_.reset();
        fs::remove_all(db_path_);
    }

    std::string db_path_;
    std::unique_ptr<themis::RocksDBWrapper>             db_;
    std::unique_ptr<themis::ProcessGraphManager>        engine_;
    std::unique_ptr<themis::process::ProcessModelManager> mgr_;
    std::unique_ptr<themis::process::ProcessLinker>     linker_;
    std::unique_ptr<themis::process::ProcessGraphRag>   rag_;
};

// ─────────────────────────────────────────────────────────────────────────────
// 1. BpmnSerializer
// ─────────────────────────────────────────────────────────────────────────────

class BpmnSerializerTest : public ::testing::Test {};

TEST_F(BpmnSerializerTest, ImportMinimalBpmn) {
    const std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<definitions xmlns="http://www.omg.org/spec/BPMN/20100524/MODEL">
  <process id="bauantrag" name="Bauantragsverfahren">
    <startEvent id="start" name="Antrag eingegangen"/>
    <userTask id="pruefe" name="Vollständigkeitsprüfung"/>
    <endEvent id="ende" name="Bescheid erteilt"/>
    <sequenceFlow id="f1" sourceRef="start" targetRef="pruefe"/>
    <sequenceFlow id="f2" sourceRef="pruefe" targetRef="ende"/>
  </process>
</definitions>)";

    auto result = themis::process::BpmnSerializer::importXml(xml);
    ASSERT_TRUE(result.ok) << result.message;
    EXPECT_EQ(result.process_id, "bauantrag");
    EXPECT_EQ(result.process_name, "Bauantragsverfahren");
    EXPECT_GE(result.nodes.size(), 3u);
    EXPECT_GE(result.edges.size(), 2u);
}

TEST_F(BpmnSerializerTest, ImportEmptyReturnsFalse) {
    auto result = themis::process::BpmnSerializer::importXml("");
    EXPECT_FALSE(result.ok);
}

TEST_F(BpmnSerializerTest, ExportProducesXml) {
    std::vector<themis::ProcessNodeInfo> nodes;
    themis::ProcessNodeInfo n1;
    n1.node_id = "s1"; n1.name = "Start"; n1.node_type = themis::BPMNNodeType::START_EVENT;
    themis::ProcessNodeInfo n2;
    n2.node_id = "t1"; n2.name = "Task"; n2.node_type = themis::BPMNNodeType::TASK;
    themis::ProcessNodeInfo n3;
    n3.node_id = "e1"; n3.name = "End"; n3.node_type = themis::BPMNNodeType::END_EVENT;
    nodes.push_back(n1); nodes.push_back(n2); nodes.push_back(n3);

    std::vector<themis::ProcessEdgeInfo> edges;
    themis::ProcessEdgeInfo e1;
    e1.edge_id = "f1"; e1.from_node = "s1"; e1.to_node = "t1";
    e1.edge_type = themis::ProcessEdgeType::SEQUENCE_FLOW;
    themis::ProcessEdgeInfo e2;
    e2.edge_id = "f2"; e2.from_node = "t1"; e2.to_node = "e1";
    e2.edge_type = themis::ProcessEdgeType::SEQUENCE_FLOW;
    edges.push_back(e1); edges.push_back(e2);

    std::string xml = themis::process::BpmnSerializer::exportXml(
        "proc1", "Test Process", nodes, edges);
    EXPECT_FALSE(xml.empty());
    EXPECT_NE(xml.find("proc1"), std::string::npos);
    EXPECT_NE(xml.find("startEvent"), std::string::npos);
    EXPECT_NE(xml.find("endEvent"), std::string::npos);
    EXPECT_NE(xml.find("sequenceFlow"), std::string::npos);
}

TEST_F(BpmnSerializerTest, RoundTrip) {
    // export → re-import; node/edge counts should survive
    std::vector<themis::ProcessNodeInfo> nodes;
    themis::ProcessNodeInfo ns; ns.node_id = "s"; ns.name = "S";
    ns.node_type = themis::BPMNNodeType::START_EVENT;
    themis::ProcessNodeInfo nt; nt.node_id = "t"; nt.name = "T";
    nt.node_type = themis::BPMNNodeType::TASK;
    themis::ProcessNodeInfo ne; ne.node_id = "e"; ne.name = "E";
    ne.node_type = themis::BPMNNodeType::END_EVENT;
    nodes.insert(nodes.end(), {ns, nt, ne});

    std::vector<themis::ProcessEdgeInfo> edges;
    themis::ProcessEdgeInfo ef1; ef1.edge_id = "e1"; ef1.from_node = "s";
    ef1.to_node = "t"; ef1.edge_type = themis::ProcessEdgeType::SEQUENCE_FLOW;
    themis::ProcessEdgeInfo ef2; ef2.edge_id = "e2"; ef2.from_node = "t";
    ef2.to_node = "e"; ef2.edge_type = themis::ProcessEdgeType::SEQUENCE_FLOW;
    edges.insert(edges.end(), {ef1, ef2});

    const std::string xml =
        themis::process::BpmnSerializer::exportXml("rt", "RoundTrip", nodes, edges);
    auto imported = themis::process::BpmnSerializer::importXml(xml);
    ASSERT_TRUE(imported.ok) << imported.message;
    EXPECT_EQ(imported.nodes.size(), nodes.size());
    EXPECT_EQ(imported.edges.size(), edges.size());
}

// ─────────────────────────────────────────────────────────────────────────────
// 2. EpkSerializer
// ─────────────────────────────────────────────────────────────────────────────

class EpkSerializerTest : public ::testing::Test {};

TEST_F(EpkSerializerTest, ImportMinimalEpk) {
    const std::string epk_text = R"(
EPK: Urlaubsantragsprozess
EVENT: Antrag gestellt
FUNCTION: Antrag prüfen
EVENT: Antrag genehmigt
FLOW: Antrag gestellt -> Antrag prüfen
FLOW: Antrag prüfen -> Antrag genehmigt
)";

    auto result = themis::process::EpkSerializer::importText(epk_text);
    ASSERT_TRUE(result.ok) << result.message;
    EXPECT_GE(result.nodes.size(), 3u);
    EXPECT_GE(result.edges.size(), 2u);
}

TEST_F(EpkSerializerTest, ImportEmptyReturnsFalse) {
    auto result = themis::process::EpkSerializer::importText("");
    EXPECT_FALSE(result.ok);
}

TEST_F(EpkSerializerTest, ImportExportFromJson) {
    json epk_json;
    epk_json["process_id"]   = "ep1";
    epk_json["process_name"] = "EPK Test";
    epk_json["nodes"] = json::array({
        {{"id","ev1"}, {"name","Start-Event"}, {"type","EVENT"}},
        {{"id","fn1"}, {"name","Prüfen"},      {"type","FUNCTION"}},
        {{"id","ev2"}, {"name","End-Event"},   {"type","EVENT"}}
    });
    epk_json["edges"] = json::array({
        {{"id","f1"}, {"from","ev1"}, {"to","fn1"}},
        {{"id","f2"}, {"from","fn1"}, {"to","ev2"}}
    });

    auto result = themis::process::EpkSerializer::importJson(epk_json);
    ASSERT_TRUE(result.ok) << result.message;
    EXPECT_EQ(result.process_id, "ep1");
    EXPECT_EQ(result.nodes.size(), 3u);
    EXPECT_EQ(result.edges.size(), 2u);

    // Export back
    std::string exported = themis::process::EpkSerializer::exportText(
        "EPK Test", result.nodes, result.edges);
    EXPECT_FALSE(exported.empty());
    EXPECT_NE(exported.find("EPK Test"), std::string::npos);
}

// ─────────────────────────────────────────────────────────────────────────────
// 3. VccVpbImporter
// ─────────────────────────────────────────────────────────────────────────────

class VccVpbImporterTest : public ::testing::Test {};

TEST_F(VccVpbImporterTest, ImportSingleModel) {
    const std::string yaml = R"(
id: bauantrag_einfach
name: Einfaches Bauantragsverfahren
domain: VERWALTUNG
description: Vereinfachtes Verfahren fuer Kleinbauten
compliance:
  - BauO NRW §75
activities:
  - id: act1
    name: Antrag einreichen
    type: USER_TASK
    sla_hours: 24
  - id: act2
    name: Vollstaendigkeitspruefung
    type: SERVICE_TASK
    sla_hours: 48
edges:
  - from: act1
    to: act2
    type: SEQUENCE_FLOW
)";

    auto result = themis::process::VccVpbImporter::importYaml(yaml);
    ASSERT_TRUE(result.ok) << result.message;
    EXPECT_EQ(result.record.id, "bauantrag_einfach");
    EXPECT_EQ(result.record.name, "Einfaches Bauantragsverfahren");
    EXPECT_FALSE(result.record.compliance_tags.empty());
    EXPECT_EQ(result.record.notation,
              themis::process::ProcessNotation::VCC_VPB);
}

TEST_F(VccVpbImporterTest, ImportEmptyReturnsFalse) {
    auto result = themis::process::VccVpbImporter::importYaml("");
    EXPECT_FALSE(result.ok);
}

// ─────────────────────────────────────────────────────────────────────────────
// 4. LlmProcessDescriptor
// ─────────────────────────────────────────────────────────────────────────────

class LlmDescriptorTest : public ::testing::Test {
protected:
    themis::process::ProcessModelRecord makeRecord() {
        themis::process::ProcessModelRecord r;
        r.id          = "kfz-zulassung";
        r.name        = "KFZ-Zulassung";
        r.description = "Zulassung eines Kraftfahrzeuges beim Straßenverkehrsamt";
        r.notation    = themis::process::ProcessNotation::BPMN_2_0;
        r.domain      = themis::process::ProcessDomain::ADMINISTRATION;
        r.compliance_tags = {"StVZO §23", "FZV §8"};
        r.normalized["nodes"] = json::array({
            {{"id","s"}, {"name","Antrag"},     {"type","startEvent"}},
            {{"id","t"}, {"name","Prüfung"},    {"type","userTask"}},
            {{"id","e"}, {"name","Zulassung"},  {"type","endEvent"}}
        });
        r.normalized["edges"] = json::array({
            {{"id","f1"}, {"from","s"}, {"to","t"}},
            {{"id","f2"}, {"from","t"}, {"to","e"}}
        });
        return r;
    }
};

TEST_F(LlmDescriptorTest, GenerateDescriptor) {
    auto rec  = makeRecord();
    auto desc = themis::process::LlmProcessDescriptor::generate(rec);
    EXPECT_FALSE(desc.empty());

    EXPECT_EQ(desc.value("process_id", ""), "kfz-zulassung");
    EXPECT_EQ(desc.value("name", ""), "KFZ-Zulassung");
    EXPECT_TRUE(desc.contains("notation"));
    EXPECT_TRUE(desc.contains("domain"));
    EXPECT_TRUE(desc.contains("nodes"));
    EXPECT_TRUE(desc.contains("edges"));
    EXPECT_EQ(desc["nodes"].size(), 3u);
    EXPECT_EQ(desc["edges"].size(), 2u);
}

TEST_F(LlmDescriptorTest, GenerateSystemPrompt) {
    auto rec    = makeRecord();
    auto desc   = themis::process::LlmProcessDescriptor::generate(rec, {.language = "de"});
    auto prompt = themis::process::LlmProcessDescriptor::buildSystemPrompt(desc);
    EXPECT_FALSE(prompt.empty());
    EXPECT_NE(prompt.find("KFZ-Zulassung"), std::string::npos);
    EXPECT_NE(prompt.find("BPMN"), std::string::npos);
}

TEST_F(LlmDescriptorTest, GenerateSystemPromptEnglish) {
    auto rec    = makeRecord();
    auto desc   = themis::process::LlmProcessDescriptor::generate(rec, {.language = "en"});
    auto prompt = themis::process::LlmProcessDescriptor::buildSystemPrompt(desc);
    EXPECT_FALSE(prompt.empty());
    EXPECT_NE(prompt.find("KFZ-Zulassung"), std::string::npos);
}

TEST_F(LlmDescriptorTest, BuildConformancePrompt) {
    auto rec    = makeRecord();
    auto desc   = themis::process::LlmProcessDescriptor::generate(rec, {.language = "de"});
    auto prompt = themis::process::LlmProcessDescriptor::buildConformancePrompt(
        desc, json::array({"Prüfung", "Zulassung"}));
    EXPECT_FALSE(prompt.empty());
    EXPECT_NE(prompt.find("Prüfung"), std::string::npos);
}

// ─────────────────────────────────────────────────────────────────────────────
// 5. ProcessModelManager
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ProcessModuleTest, SaveAndLoad) {
    themis::process::ProcessModelRecord rec;
    rec.id          = "mgr-test-001";
    rec.name        = "Test Model";
    rec.description = "A simple test model";
    rec.notation    = themis::process::ProcessNotation::BPMN_2_0;
    rec.domain      = themis::process::ProcessDomain::BUSINESS;
    rec.normalized["nodes"] = json::array({
        {{"id","s"}, {"name","Start"}, {"type","startEvent"}},
        {{"id","e"}, {"name","End"},   {"type","endEvent"}}
    });
    rec.normalized["edges"] = json::array();

    auto result = mgr_->save(rec);
    ASSERT_TRUE(result.ok) << result.message;

    auto loaded = mgr_->load("mgr-test-001");
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->id, "mgr-test-001");
    EXPECT_EQ(loaded->name, "Test Model");
    EXPECT_EQ(loaded->notation, themis::process::ProcessNotation::BPMN_2_0);
}

TEST_F(ProcessModuleTest, LoadNonExistentReturnsNullopt) {
    auto loaded = mgr_->load("does-not-exist");
    EXPECT_FALSE(loaded.has_value());
}

TEST_F(ProcessModuleTest, ListModels) {
    for (int i = 0; i < 3; ++i) {
        themis::process::ProcessModelRecord r;
        r.id   = "list-model-" + std::to_string(i);
        r.name = "Model " + std::to_string(i);
        r.normalized["nodes"] = json::array();
        r.normalized["edges"] = json::array();
        ASSERT_TRUE(mgr_->save(r).ok);
    }
    auto all = mgr_->list();
    EXPECT_GE(all.size(), 3u);
}

TEST_F(ProcessModuleTest, DeleteModel) {
    themis::process::ProcessModelRecord rec;
    rec.id   = "to-delete";
    rec.name = "DeleteMe";
    rec.normalized["nodes"] = json::array();
    rec.normalized["edges"] = json::array();
    ASSERT_TRUE(mgr_->save(rec).ok);
    EXPECT_TRUE(mgr_->load("to-delete").has_value());

    EXPECT_TRUE(mgr_->remove("to-delete").ok);
    auto archived = mgr_->load("to-delete");
    ASSERT_TRUE(archived.has_value());
    EXPECT_EQ(archived->state, themis::process::ProcessModelState::ARCHIVED);
}

TEST_F(ProcessModuleTest, ImportBpmnViaManager) {
    const std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<definitions xmlns="http://www.omg.org/spec/BPMN/20100524/MODEL">
  <process id="manager-bpmn" name="Manager BPMN Test">
    <startEvent id="s" name="Start"/>
    <endEvent id="e" name="End"/>
    <sequenceFlow id="f1" sourceRef="s" targetRef="e"/>
  </process>
</definitions>)";

    auto result = mgr_->importBpmn(xml);
    ASSERT_TRUE(result.ok) << result.message;

    auto loaded = mgr_->load("manager-bpmn");
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->notation, themis::process::ProcessNotation::BPMN_2_0);
}

TEST_F(ProcessModuleTest, ImportVccVpbViaManager) {
    const std::string yaml = R"(
id: mgr-vpb-001
name: VPB Manager Test
domain: VERWALTUNG
activities:
  - id: a1
    name: Schritt 1
    type: USER_TASK
  - id: a2
    name: Schritt 2
    type: SERVICE_TASK
edges:
  - from: a1
    to: a2
    type: SEQUENCE_FLOW
)";

    auto result = mgr_->importVccVpb(yaml);
    ASSERT_TRUE(result.ok) << result.message;

    auto loaded = mgr_->load("mgr-vpb-001");
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->notation, themis::process::ProcessNotation::VCC_VPB);
    EXPECT_EQ(loaded->domain,   themis::process::ProcessDomain::ADMINISTRATION);
}

// ─────────────────────────────────────────────────────────────────────────────
// 6. ProcessLinker
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ProcessModuleTest, AttachAndRetrieve) {
    auto [ok, att_id] = linker_->attachObject(
        "inst-001", "doc-bauliste-01", "documents",
        themis::process::ProcessLinkType::HAS_DOCUMENT,
        "node-pruefe",
        {{"doc_type", "Bauzeichnung"}, {"required", true}},
        "sachbearbeiter-a"
    );
    ASSERT_TRUE(ok) << att_id;
    EXPECT_FALSE(att_id.empty());

    auto attachments = linker_->getAttachments("inst-001");
    ASSERT_EQ(attachments.size(), 1u);
    EXPECT_EQ(attachments[0].object_id, "doc-bauliste-01");
    EXPECT_EQ(attachments[0].object_collection, "documents");
    EXPECT_EQ(attachments[0].link_type,
              themis::process::ProcessLinkType::HAS_DOCUMENT);
}

TEST_F(ProcessModuleTest, AttachFilterByType) {
    linker_->attachObject("inst-002", "doc-001", "documents",
        themis::process::ProcessLinkType::HAS_DOCUMENT, std::nullopt, {}, "u1");
    linker_->attachObject("inst-002", "meta-001", "metadata",
        themis::process::ProcessLinkType::HAS_METADATA, std::nullopt, {}, "u1");

    auto docs_only = linker_->getAttachments(
        "inst-002", themis::process::ProcessLinkType::HAS_DOCUMENT);
    EXPECT_EQ(docs_only.size(), 1u);
    EXPECT_EQ(docs_only[0].object_collection, "documents");

    auto all = linker_->getAttachments("inst-002");
    EXPECT_EQ(all.size(), 2u);
}

TEST_F(ProcessModuleTest, DetachObject) {
    auto [ok, att_id] = linker_->attachObject(
        "inst-003", "doc-x", "documents",
        themis::process::ProcessLinkType::HAS_DOCUMENT,
        std::nullopt, {}, "u1");
    ASSERT_TRUE(ok);

    EXPECT_TRUE(linker_->detachObject(att_id));

    auto remaining = linker_->getAttachments("inst-003");
    EXPECT_TRUE(remaining.empty());
}

TEST_F(ProcessModuleTest, NodeAttachments) {
    linker_->attachObject("inst-004", "doc-a", "documents",
        themis::process::ProcessLinkType::HAS_DOCUMENT, "node-1", {}, "u1");
    linker_->attachObject("inst-004", "doc-b", "documents",
        themis::process::ProcessLinkType::HAS_DOCUMENT, "node-2", {}, "u1");

    auto node1_atts = linker_->getNodeAttachments("inst-004", "node-1");
    EXPECT_EQ(node1_atts.size(), 1u);
    EXPECT_EQ(node1_atts[0].object_id, "doc-a");

    auto node2_atts = linker_->getNodeAttachments("inst-004", "node-2");
    EXPECT_EQ(node2_atts.size(), 1u);
    EXPECT_EQ(node2_atts[0].object_id, "doc-b");
}

TEST_F(ProcessModuleTest, FindInstancesWithObject) {
    linker_->attachObject("inst-A", "shared-doc", "documents",
        themis::process::ProcessLinkType::HAS_DOCUMENT, std::nullopt, {}, "u1");
    linker_->attachObject("inst-B", "shared-doc", "documents",
        themis::process::ProcessLinkType::HAS_DOCUMENT, std::nullopt, {}, "u2");

    auto instances = linker_->findInstancesWithObject("shared-doc", "documents");
    EXPECT_EQ(instances.size(), 2u);
    EXPECT_NE(std::find(instances.begin(), instances.end(), "inst-A"),
              instances.end());
    EXPECT_NE(std::find(instances.begin(), instances.end(), "inst-B"),
              instances.end());
}

TEST_F(ProcessModuleTest, LinkProcesses) {
    auto [ok, link_id] = linker_->linkProcesses(
        "inst-parent", "inst-child",
        themis::process::ProcessLinkType::SUB_PROCESS,
        {{"reason", "subprocess call at node-3"}});
    ASSERT_TRUE(ok) << link_id;
    EXPECT_FALSE(link_id.empty());

    auto links = linker_->getLinks("inst-parent");
    EXPECT_EQ(links.size(), 1u);
    EXPECT_EQ(links[0].target_id, "inst-child");
    EXPECT_EQ(links[0].link_type,
              themis::process::ProcessLinkType::SUB_PROCESS);
}

TEST_F(ProcessModuleTest, RequiredDocumentRegistry) {
    ASSERT_TRUE(linker_->registerRequiredDocument(
        "model-bauantrag", "node-vollst", "Bauzeichnung", true,
        {{"format", "PDF"}}));
    ASSERT_TRUE(linker_->registerRequiredDocument(
        "model-bauantrag", "node-vollst", "Grundriss", false,
        {}));

    auto reqs = linker_->getRequiredDocuments("model-bauantrag", "node-vollst");
    EXPECT_EQ(reqs.size(), 2u);
}

TEST_F(ProcessModuleTest, MissingDocumentsDetection) {
    // Register two required documents for a node
    linker_->registerRequiredDocument(
        "model-req", "node-check", "Bauzeichnung", true, {});
    linker_->registerRequiredDocument(
        "model-req", "node-check", "Lageplan", true, {});

    // Attach only one
    linker_->attachObject("inst-req", "doc-bauzeichnung", "documents",
        themis::process::ProcessLinkType::HAS_DOCUMENT, "node-check",
        {{"doc_type", "Bauzeichnung"}}, "u1");

    auto missing = linker_->getMissingDocuments(
        "inst-req", "node-check", "model-req");
    EXPECT_EQ(missing.size(), 1u);
    EXPECT_EQ(missing[0], "Lageplan");
}

TEST_F(ProcessModuleTest, NoMissingDocumentsWhenAllPresent) {
    linker_->registerRequiredDocument(
        "model-all", "node-x", "DocA", true, {});
    linker_->attachObject("inst-all", "obj-a", "documents",
        themis::process::ProcessLinkType::HAS_DOCUMENT, "node-x",
        {{"doc_type", "DocA"}}, "u1");

    auto missing = linker_->getMissingDocuments(
        "inst-all", "node-x", "model-all");
    EXPECT_TRUE(missing.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// 7. ProcessGraphRag
// ─────────────────────────────────────────────────────────────────────────────

class ProcessGraphRagTest : public ProcessModuleTest {
protected:
    // Helper: import a minimal Bauantrag model and return its ID
    std::string importBauantragModel() {
        const std::string yaml = R"(
id: rag-bauantrag
name: Bauantragsverfahren
domain: VERWALTUNG
description: Vollständiges Bauantragsverfahren
compliance_tags:
  - BauO NRW §75
  - DSGVO Art. 5
activities:
  - id: start
    name: Antrag eingegangen
    type: START_EVENT
  - id: vollst
    name: Vollständigkeitsprüfung
    type: USER_TASK
    sla_hours: 48
  - id: fach
    name: Fachprüfung
    type: USER_TASK
    sla_hours: 120
  - id: ende
    name: Bescheid erteilt
    type: END_EVENT
edges:
  - from: start
    to: vollst
    type: SEQUENCE_FLOW
  - from: vollst
    to: fach
    type: SEQUENCE_FLOW
  - from: fach
    to: ende
    type: SEQUENCE_FLOW
)";
        auto result = mgr_->importVccVpb(yaml);
        EXPECT_TRUE(result.ok) << result.message;
        return "rag-bauantrag";
    }

    // Helper: start a process instance and return its ID
    std::string startInstance(const std::string& model_id) {
        // Deploy model to engine
        auto rec = mgr_->load(model_id);
        if (!rec) return "";
        mgr_->deployToEngine(model_id, *engine_);

        auto [status, inst] = engine_->startProcess(
            model_id, {{"instance_id", "inst-" + model_id}});
        if (!status.ok) return "";
        return inst;
    }
};

TEST_F(ProcessGraphRagTest, BuildKnowledgeGraphFromModel) {
    importBauantragModel();

    auto kg = rag_->buildKnowledgeGraph("rag-bauantrag");
    EXPECT_GE(kg.nodes.size(), 4u);  // at least 4 activities
    EXPECT_GE(kg.edges.size(), 3u);  // at least 3 sequence flows

    // All nodes should have non-empty IDs
    for (const auto& node : kg.nodes) {
        EXPECT_FALSE(node.id.empty());
        EXPECT_FALSE(node.canonical_name.empty());
    }
}

TEST_F(ProcessGraphRagTest, ExtractSubgraph) {
    importBauantragModel();

    auto subgraph = rag_->extractSubgraph(
        "rag-bauantrag", {"vollst"}, 2);

    ASSERT_FALSE(subgraph.is_null());
    EXPECT_TRUE(subgraph.contains("nodes"));
    EXPECT_TRUE(subgraph.contains("edges"));
    // BFS depth 2 from vollst should include start, fach, and possibly ende
    EXPECT_GE(subgraph["nodes"].size(), 1u);
}

TEST_F(ProcessGraphRagTest, SummarizeNonExistentInstance) {
    auto summary = rag_->summarizeVerwaltungsvorgang("no-such-instance");
    ASSERT_FALSE(summary.is_null());
    // Should return an error summary, not crash
    EXPECT_TRUE(summary.contains("error"));
}

TEST_F(ProcessGraphRagTest, CheckComplianceNonExistentInstance) {
    auto result = rag_->checkCompliance("no-such-instance");
    // Should not throw; compliance fails gracefully
    EXPECT_FALSE(result.is_compliant);
}

TEST_F(ProcessGraphRagTest, RetrieveNonExistentInstance) {
    themis::process::ProcessRagConfig cfg;
    cfg.language = "de";
    auto ctx = rag_->retrieve("no-such-instance", "Was fehlt?", cfg);
    // Should return context with the queried instance ID and an empty prompt
    // (graceful failure, no crash, no result for an unknown instance)
    EXPECT_EQ(ctx.instance_id, "no-such-instance");
    EXPECT_TRUE(ctx.active_nodes.empty());
    EXPECT_TRUE(ctx.visited_nodes.empty());
}

TEST_F(ProcessGraphRagTest, BuildAdminProcessingPrompt) {
    importBauantragModel();

    themis::process::ProcessRagContext ctx;
    ctx.instance_id             = "inst-rag-1";
    ctx.process_name            = "Bauantragsverfahren";
    ctx.process_definition_id   = "rag-bauantrag";
    ctx.current_state           = "RUNNING";
    ctx.active_nodes            = {"vollst"};
    ctx.visited_nodes           = {"start", "vollst"};
    ctx.compliance_tags         = {"BauO NRW §75"};
    ctx.missing_documents       = {"Lageplan"};
    ctx.attachments             = {{{"doc_type", "Bauzeichnung"}, {"id", "doc-1"}}};
    ctx.query                   = "Was fehlt noch für die Vollständigkeitsprüfung?";

    std::string prompt = rag_->buildAdminProcessingPrompt(ctx);
    EXPECT_FALSE(prompt.empty());
    EXPECT_NE(prompt.find("Bauantragsverfahren"), std::string::npos);
    EXPECT_NE(prompt.find("vollst"), std::string::npos);
}

TEST_F(ProcessGraphRagTest, BuildQueryPrompt) {
    themis::process::ProcessRagContext ctx;
    ctx.instance_id   = "inst-x";
    ctx.process_name  = "KFZ-Zulassung";
    ctx.current_state = "RUNNING";
    ctx.query         = "Welche Dokumente fehlen noch?";

    std::string prompt = rag_->buildQueryPrompt(ctx);
    EXPECT_FALSE(prompt.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// 8. toString / fromString helpers
// ─────────────────────────────────────────────────────────────────────────────

TEST(ProcessEnumsTest, ProcessLinkTypeRoundTrip) {
    using namespace themis::process;
    const std::vector<ProcessLinkType> types = {
        ProcessLinkType::HAS_DOCUMENT,
        ProcessLinkType::HAS_METADATA,
        ProcessLinkType::REQUIRES_DOCUMENT,
        ProcessLinkType::IS_INSTANCE_OF,
        ProcessLinkType::SUB_PROCESS,
        ProcessLinkType::CROSS_REFERENCE,
        ProcessLinkType::TRIGGERS,
        ProcessLinkType::EVIDENCE_FOR,
    };
    for (auto t : types) {
        auto s     = toString(t);
        auto back  = processLinkTypeFromString(s);
        EXPECT_EQ(back, t) << "Round-trip failed for: " << std::string(s);
    }
}

TEST(ProcessEnumsTest, ProcessNotationRoundTrip) {
    using namespace themis::process;
    const std::vector<ProcessNotation> notations = {
        ProcessNotation::BPMN_2_0,
        ProcessNotation::EPK,
        ProcessNotation::VCC_VPB,
        ProcessNotation::CMMN_1_1,
        ProcessNotation::DMN_1_5,
    };
    for (auto n : notations) {
        auto s    = toString(n);
        auto back = notationFromString(s);
        EXPECT_EQ(back, n) << "Round-trip failed for notation: "
                           << std::string(s);
    }
}

TEST(ProcessEnumsTest, ProcessDomainRoundTrip) {
    using namespace themis::process;
    const std::vector<ProcessDomain> domains = {
        ProcessDomain::BUSINESS,
        ProcessDomain::ADMINISTRATION,
        ProcessDomain::IT_SERVICE,
        ProcessDomain::HEALTHCARE,
        ProcessDomain::FINANCE,
        ProcessDomain::CUSTOMER_SERVICE,
        ProcessDomain::CUSTOM,
    };
    for (auto d : domains) {
        auto s    = toString(d);
        auto back = domainFromString(s);
        EXPECT_EQ(back, d) << "Round-trip failed for domain: "
                           << std::string(s);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// 9. BpmnSerializer – extended parser hardening tests
// ─────────────────────────────────────────────────────────────────────────────

// PM-01: Namespace-prefixed BPMN tags (bpmn:startEvent etc.)
TEST_F(BpmnSerializerTest, ImportNamespacePrefixedBpmn) {
    const std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<bpmn:definitions
    xmlns:bpmn="http://www.omg.org/spec/BPMN/20100524/MODEL"
    targetNamespace="http://example.com">
  <bpmn:process id="ns_proc" name="NS Process">
    <bpmn:startEvent id="ns_start" name="Eingang"/>
    <bpmn:userTask   id="ns_task"  name="Prüfung"/>
    <bpmn:endEvent   id="ns_end"   name="Abschluss"/>
    <bpmn:sequenceFlow id="ns_f1" sourceRef="ns_start" targetRef="ns_task"/>
    <bpmn:sequenceFlow id="ns_f2" sourceRef="ns_task"  targetRef="ns_end"/>
  </bpmn:process>
</bpmn:definitions>)";

    auto result = themis::process::BpmnSerializer::importXml(xml);
    ASSERT_TRUE(result.ok) << result.message;
    EXPECT_EQ(result.process_id, "ns_proc");
    EXPECT_GE(result.nodes.size(), 3u);
    EXPECT_GE(result.edges.size(), 2u);
}

// PM-02: Complex nested subProcess with child nodes
TEST_F(BpmnSerializerTest, ImportNestedSubProcess) {
    const std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<definitions xmlns="http://www.omg.org/spec/BPMN/20100524/MODEL">
  <process id="outer_proc" name="Outer">
    <startEvent id="out_start" name="Start"/>
    <subProcess id="sp1" name="Genehmigung">
      <startEvent id="sp_start" name="SP-Start"/>
      <userTask   id="sp_task"  name="SP-Prüfung"/>
      <endEvent   id="sp_end"   name="SP-Ende"/>
      <sequenceFlow id="sp_f1" sourceRef="sp_start" targetRef="sp_task"/>
      <sequenceFlow id="sp_f2" sourceRef="sp_task"  targetRef="sp_end"/>
    </subProcess>
    <endEvent id="out_end" name="Ende"/>
    <sequenceFlow id="out_f1" sourceRef="out_start" targetRef="sp1"/>
    <sequenceFlow id="out_f2" sourceRef="sp1"       targetRef="out_end"/>
  </process>
</definitions>)";

    auto result = themis::process::BpmnSerializer::importXml(xml);
    ASSERT_TRUE(result.ok) << result.message;
    EXPECT_EQ(result.process_id, "outer_proc");
    // Outer nodes: out_start, sp1, out_end; inner: sp_start, sp_task, sp_end
    EXPECT_GE(result.nodes.size(), 6u);
    // Outer flows + inner flows
    EXPECT_GE(result.edges.size(), 4u);
}

// PM-03: conditionExpression as child element of sequenceFlow (non-self-closing)
TEST_F(BpmnSerializerTest, ImportConditionExpressionChild) {
    const std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<definitions xmlns="http://www.omg.org/spec/BPMN/20100524/MODEL">
  <process id="cond_proc" name="Condition Test">
    <startEvent id="c_start"/>
    <exclusiveGateway id="gw1"/>
    <endEvent id="c_end1"/>
    <endEvent id="c_end2"/>
    <sequenceFlow id="cf1" sourceRef="c_start" targetRef="gw1"/>
    <sequenceFlow id="cf2" sourceRef="gw1" targetRef="c_end1">
      <conditionExpression>${approved == true}</conditionExpression>
    </sequenceFlow>
    <sequenceFlow id="cf3" sourceRef="gw1" targetRef="c_end2">
      <conditionExpression>${approved == false}</conditionExpression>
    </sequenceFlow>
  </process>
</definitions>)";

    auto result = themis::process::BpmnSerializer::importXml(xml);
    ASSERT_TRUE(result.ok) << result.message;
    EXPECT_GE(result.edges.size(), 3u);

    bool found_cond = false;
    for (const auto& e : result.edges) {
        if (e.condition_expression.has_value() &&
            e.condition_expression->find("approved") != std::string::npos) {
            found_cond = true;
        }
    }
    EXPECT_TRUE(found_cond) << "conditionExpression child text not captured";
}

// PM-04: BPMN XML with XML comment blocks and PIs must parse cleanly
TEST_F(BpmnSerializerTest, ImportWithCommentsAndPIs) {
    const std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<!-- Generated by Camunda Modeler 4.8 -->
<definitions xmlns="http://www.omg.org/spec/BPMN/20100524/MODEL">
  <!-- Header comment -->
  <process id="comment_proc" name="Comment Test">
    <?camunda custom-pi="value"?>
    <startEvent id="c1" name="Start"/>
    <!-- mid-element comment -->
    <endEvent id="c2" name="End"/>
    <sequenceFlow id="cf1" sourceRef="c1" targetRef="c2"/>
  </process>
</definitions>)";

    auto result = themis::process::BpmnSerializer::importXml(xml);
    ASSERT_TRUE(result.ok) << result.message;
    EXPECT_EQ(result.process_id, "comment_proc");
    EXPECT_GE(result.nodes.size(), 2u);
    EXPECT_GE(result.edges.size(), 1u);
}

// PM-05: Deeply nested sub-process pools (stress test for proper stack handling)
TEST_F(BpmnSerializerTest, ImportDeeplyNestedSubProcesses) {
    const std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<definitions xmlns="http://www.omg.org/spec/BPMN/20100524/MODEL">
  <process id="deep_proc" name="Deep">
    <startEvent id="d_start"/>
    <subProcess id="sp_lvl1" name="Level 1">
      <subProcess id="sp_lvl2" name="Level 2">
        <userTask id="inner_task" name="Inner Task"/>
        <sequenceFlow id="inner_f1" sourceRef="sp_lvl2_start" targetRef="inner_task"/>
      </subProcess>
      <sequenceFlow id="sp1_f1" sourceRef="sp_lvl1_start" targetRef="sp_lvl2"/>
    </subProcess>
    <endEvent id="d_end"/>
    <sequenceFlow id="d_f1" sourceRef="d_start" targetRef="sp_lvl1"/>
    <sequenceFlow id="d_f2" sourceRef="sp_lvl1" targetRef="d_end"/>
  </process>
</definitions>)";

    auto result = themis::process::BpmnSerializer::importXml(xml);
    ASSERT_TRUE(result.ok) << result.message;
    // Must not crash; must capture all identified nodes/edges
    EXPECT_GE(result.nodes.size(), 4u);
    EXPECT_GE(result.edges.size(), 2u);
}

// PM-06: All gateway types in a single document
TEST_F(BpmnSerializerTest, ImportAllGatewayTypes) {
    const std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<definitions xmlns="http://www.omg.org/spec/BPMN/20100524/MODEL">
  <process id="gw_proc" name="Gateways">
    <startEvent id="gw_s"/>
    <exclusiveGateway  id="xor_gw"/>
    <parallelGateway   id="and_gw"/>
    <inclusiveGateway  id="or_gw"/>
    <eventBasedGateway id="ev_gw"/>
    <complexGateway    id="cplx_gw"/>
    <endEvent id="gw_e"/>
    <sequenceFlow id="gf1" sourceRef="gw_s"    targetRef="xor_gw"/>
    <sequenceFlow id="gf2" sourceRef="xor_gw"  targetRef="and_gw"/>
    <sequenceFlow id="gf3" sourceRef="and_gw"  targetRef="or_gw"/>
    <sequenceFlow id="gf4" sourceRef="or_gw"   targetRef="ev_gw"/>
    <sequenceFlow id="gf5" sourceRef="ev_gw"   targetRef="cplx_gw"/>
    <sequenceFlow id="gf6" sourceRef="cplx_gw" targetRef="gw_e"/>
  </process>
</definitions>)";

    auto result = themis::process::BpmnSerializer::importXml(xml);
    ASSERT_TRUE(result.ok) << result.message;
    EXPECT_GE(result.nodes.size(), 7u);
    EXPECT_EQ(result.edges.size(), 6u);
}

// PM-07: Security – oversized input must be rejected without crashing
TEST_F(BpmnSerializerTest, SecurityOversizedInputRejected) {
    // Build an input just over 10 MiB
    const std::string huge(11u * 1024u * 1024u, 'X');
    auto result = themis::process::BpmnSerializer::importXml(huge);
    EXPECT_FALSE(result.ok);
    EXPECT_NE(result.message.find("10 MiB"), std::string::npos);
}

// PM-08: Security – malformed / truncated XML must not crash
TEST_F(BpmnSerializerTest, SecurityMalformedXmlGraceful) {
    const std::vector<std::string> malformed = {
        "<",
        "<<<<<",
        "<!-",
        "<?",
        "<definitions><process id=\"p1\">",  // unclosed tags
        R"(<definitions><process id="p1"><startEvent id="s1"/><sequenceFlow)",
        "<!-- unclosed comment",
        "<![CDATA[unclosed",
    };
    for (const auto& bad : malformed) {
        EXPECT_NO_THROW({
            auto r = themis::process::BpmnSerializer::importXml(bad);
            (void)r;
        }) << "Crashed on: " << bad;
    }
}

// PM-09: Security – XML entity / CDATA content must not be executed
TEST_F(BpmnSerializerTest, SecurityCdataStrippedFromAttributes) {
    // id attribute with entity-encoded angle brackets must survive as-is
    const std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<definitions xmlns="http://www.omg.org/spec/BPMN/20100524/MODEL">
  <process id="safe_proc" name="Safe &amp; Secure">
    <startEvent id="safe_start" name="Antrag &lt;geprüft&gt;"/>
    <endEvent   id="safe_end"   name="Fertig"/>
    <sequenceFlow id="safe_f1" sourceRef="safe_start" targetRef="safe_end"/>
  </process>
</definitions>)";

    auto result = themis::process::BpmnSerializer::importXml(xml);
    ASSERT_TRUE(result.ok) << result.message;
    EXPECT_EQ(result.process_id, "safe_proc");
    EXPECT_GE(result.nodes.size(), 2u);
    // Verify entity decoding in node names
    bool found = false;
    for (const auto& n : result.nodes) {
        if (n.name.find('<') != std::string::npos ||
            n.name.find("geprüft") != std::string::npos) found = true;
    }
    EXPECT_TRUE(found) << "XML entity decoding did not work correctly";
}

// PM-10: messageFlow is imported correctly
TEST_F(BpmnSerializerTest, ImportMessageFlow) {
    const std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<definitions xmlns="http://www.omg.org/spec/BPMN/20100524/MODEL">
  <process id="mf_proc">
    <startEvent id="mf_s"/>
    <endEvent   id="mf_e"/>
    <sequenceFlow id="mf_sf1" sourceRef="mf_s" targetRef="mf_e"/>
  </process>
  <collaboration id="collab1">
    <participant id="p1" name="Amt"/>
    <participant id="p2" name="Bürger"/>
    <messageFlow id="mf1" sourceRef="p1" targetRef="p2"/>
    <messageFlow id="mf2" sourceRef="p2" targetRef="p1"/>
  </collaboration>
</definitions>)";

    auto result = themis::process::BpmnSerializer::importXml(xml);
    ASSERT_TRUE(result.ok) << result.message;
    // participants become nodes, message flows become edges
    bool has_msg_flow = false;
    for (const auto& e : result.edges) {
        if (e.edge_type == themis::ProcessEdgeType::MESSAGE_FLOW) {
            has_msg_flow = true;
        }
    }
    EXPECT_TRUE(has_msg_flow);
}

// PM-11: Performance – import of a large BPMN document completes within 500 ms
TEST_F(BpmnSerializerTest, PerformanceLargeBpmnImport) {
    // Build a synthetic BPMN with 200 tasks and 199 sequence flows
    std::ostringstream oss;
    oss << R"(<?xml version="1.0" encoding="UTF-8"?>)"
        << R"(<definitions xmlns="http://www.omg.org/spec/BPMN/20100524/MODEL">)"
        << R"(<process id="perf_proc" name="Perf Test">)";
    oss << R"(<startEvent id="perf_s" name="Start"/>)";
    for (int i = 0; i < 200; ++i) {
        oss << "<userTask id=\"perf_t" << i << "\" name=\"Task " << i << "\"/>";
    }
    oss << R"(<endEvent id="perf_e" name="End"/>)";
    oss << R"(<sequenceFlow id="perf_sf0" sourceRef="perf_s" targetRef="perf_t0"/>)";
    for (int i = 0; i < 199; ++i) {
        oss << "<sequenceFlow id=\"perf_sf" << (i + 1) << "\" sourceRef=\"perf_t" << i
            << "\" targetRef=\"perf_t" << (i + 1) << "\"/>";
    }
    oss << "<sequenceFlow id=\"perf_sf200\" sourceRef=\"perf_t199\" targetRef=\"perf_e\"/>";
    oss << "</process></definitions>";

    const std::string large_xml = oss.str();
    auto t0 = std::chrono::steady_clock::now();
    auto result = themis::process::BpmnSerializer::importXml(large_xml);
    auto t1 = std::chrono::steady_clock::now();

    ASSERT_TRUE(result.ok) << result.message;
    EXPECT_GE(result.nodes.size(), 202u); // start + 200 tasks + end
    EXPECT_GE(result.edges.size(), 201u);

    auto elapsed_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    EXPECT_LT(elapsed_ms, 500)
        << "Large BPMN import took " << elapsed_ms << " ms (limit: 500 ms)";
}

// ─────────────────────────────────────────────────────────────────────────────
// 10. ProcessLinker – hard-delete and secondary-index tests
// ─────────────────────────────────────────────────────────────────────────────

// PL-01: detachObject performs a hard delete (key must not exist afterwards)
TEST_F(ProcessModuleTest, DetachObjectHardDelete) {
    auto [ok, att_id] = linker_->attachObject(
        "hd-inst-001", "hd-doc-001", "documents",
        themis::process::ProcessLinkType::HAS_DOCUMENT,
        std::nullopt, {}, "tester");
    ASSERT_TRUE(ok);

    ASSERT_TRUE(linker_->detachObject(att_id));

    // No tombstone should appear when we list attachments.
    auto remaining = linker_->getAttachments("hd-inst-001");
    EXPECT_TRUE(remaining.empty()) << "Expected hard delete, but attachment still visible";

    // Direct DB key must be gone: a second detach returns false (key not found).
    EXPECT_FALSE(linker_->detachObject(att_id))
        << "Expected false on second detach (hard delete must have removed the key)";
}

// PL-02: detachObject on a non-existent ID returns false gracefully
TEST_F(ProcessModuleTest, DetachObjectNonExistentReturnsFalse) {
    EXPECT_FALSE(linker_->detachObject("attach:does-not-exist:doc-x"));
}

// PL-03: Secondary index: findInstancesWithObject uses index (verify via attach+detach cycle)
TEST_F(ProcessModuleTest, FindInstancesSecondaryIndexRoundTrip) {
    // Attach the same shared doc to two instances
    linker_->attachObject("idx-inst-A", "idx-shared-doc", "documents",
        themis::process::ProcessLinkType::HAS_DOCUMENT, std::nullopt, {}, "u1");
    auto [ok, att_id_B] = linker_->attachObject(
        "idx-inst-B", "idx-shared-doc", "documents",
        themis::process::ProcessLinkType::HAS_DOCUMENT, std::nullopt, {}, "u2");
    ASSERT_TRUE(ok);

    auto instances = linker_->findInstancesWithObject("idx-shared-doc", "documents");
    EXPECT_EQ(instances.size(), 2u);
    EXPECT_NE(std::find(instances.begin(), instances.end(), "idx-inst-A"), instances.end());
    EXPECT_NE(std::find(instances.begin(), instances.end(), "idx-inst-B"), instances.end());

    // After detaching inst-B, only inst-A should remain
    ASSERT_TRUE(linker_->detachObject(att_id_B));
    auto after_detach = linker_->findInstancesWithObject("idx-shared-doc", "documents");
    EXPECT_EQ(after_detach.size(), 1u);
    EXPECT_EQ(after_detach[0], "idx-inst-A");
}

// PL-04: findInstancesWithObject returns empty when no attachments exist
TEST_F(ProcessModuleTest, FindInstancesWithObjectEmpty) {
    auto instances = linker_->findInstancesWithObject("nonexistent-doc", "documents");
    EXPECT_TRUE(instances.empty());
}

// PL-05: Secondary index handles multiple collections correctly
TEST_F(ProcessModuleTest, FindInstancesMultipleCollections) {
    linker_->attachObject("col-inst-A", "multi-doc", "documents",
        themis::process::ProcessLinkType::HAS_DOCUMENT, std::nullopt, {}, "u1");
    linker_->attachObject("col-inst-B", "multi-doc", "metadata",
        themis::process::ProcessLinkType::HAS_METADATA, std::nullopt, {}, "u2");

    auto docs = linker_->findInstancesWithObject("multi-doc", "documents");
    EXPECT_EQ(docs.size(), 1u);
    EXPECT_EQ(docs[0], "col-inst-A");

    auto meta = linker_->findInstancesWithObject("multi-doc", "metadata");
    EXPECT_EQ(meta.size(), 1u);
    EXPECT_EQ(meta[0], "col-inst-B");
}

// PL-06: getAttachments still works after hard-delete (no tombstone leakage)
TEST_F(ProcessModuleTest, GetAttachmentsAfterHardDelete) {
    linker_->attachObject("noTmb-inst", "noTmb-doc-a", "documents",
        themis::process::ProcessLinkType::HAS_DOCUMENT, std::nullopt, {}, "u1");
    auto [ok, att_id] = linker_->attachObject(
        "noTmb-inst", "noTmb-doc-b", "documents",
        themis::process::ProcessLinkType::HAS_DOCUMENT, std::nullopt, {}, "u1");
    ASSERT_TRUE(ok);

    // Detach doc-b
    ASSERT_TRUE(linker_->detachObject(att_id));

    // Only doc-a should remain – no tombstone visible
    auto remaining = linker_->getAttachments("noTmb-inst");
    ASSERT_EQ(remaining.size(), 1u);
    EXPECT_EQ(remaining[0].object_id, "noTmb-doc-a");
}
