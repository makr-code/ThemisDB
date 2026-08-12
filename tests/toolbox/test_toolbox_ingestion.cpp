/*
 * ThemisDB — Toolbox + AQLIngestionBridge unit tests
 *
 * Tests:
 *   IT-01  IngestionToolbox: default constructor creates non-null instance
 *   IT-02  IngestionToolbox::createDefault() returns valid shared_ptr
 *   IT-03  IngestionToolbox::workflowEngine() never null after construction
 *   IT-04  IngestionToolbox::textBackend() returns NullTextGenerationBackend by default
 *   IT-05  IngestionToolbox::setTextBackend(nullptr) reinstates NullTextGenerationBackend
 *   IT-06  IngestionToolbox::setWorkflowEngine(null) throws
 *   IT-07  IngestionToolbox::setWorkflowEngine(real) replaces engine
 *   IT-08  IngestionToolbox::stepRegistry() accessible (list returns vector)
 *   IT-09  IngestionToolbox::extractEntities("") returns empty vector
 *   IT-10  IngestionToolbox::extractEntities(text) returns vector (no crash)
 *   AB-01  AQLIngestionBridge: construction with valid toolbox succeeds
 *   AB-02  AQLIngestionBridge: construction with null toolbox throws
 *   AB-03  AQLIngestionBridge::toolbox() returns injected toolbox
 *   AB-04  AQLIngestionBridge::graphWriter() returns nullptr when not injected
 *   AB-05  AQLIngestionBridge::enrichInsertPayload() non-object JSON is no-op
 *   AB-06  AQLIngestionBridge::enrichInsertPayload() object without text field is no-op
 *   AB-07  AQLIngestionBridge::enrichInsertPayload() with text field does not crash
 *   AB-08  AQLIngestionBridge::extractEntitiesForContext("") returns empty
 *   AB-09  AQLIngestionBridge::buildEntityContext({}) returns empty string
 *   AB-10  AQLIngestionBridge::buildEntityContext(entities) contains "Extracted entities:"
 *   QB-01  AQLQueryBuilder::withIngestionEnrichment() defaults to false
 *   QB-02  AQLQueryBuilder::withIngestionEnrichment(true) sets flag
 *   QB-03  AQLQueryBuilder::withIngestionEnrichment(false) unsets flag
 *   QB-04  withIngestionEnrichment() is fluent (returns *this)
 *   LH-01  LLMAQLHandler::setIngestionBridge(nullptr) accepted
 *   LH-02  LLMAQLHandler::ingestionBridge() returns nullptr when not set
 *   LH-03  LLMAQLHandler::setIngestionBridge(bridge) stored and retrievable
 */

#include <gtest/gtest.h>

#include "toolbox/ingestion_toolbox.h"
#include "aql/aql_ingestion_bridge.h"
#include "aql/aql_query_builder.h"
#include "aql/llm_aql_handler.h"
#include "ingestion/ingestion_sinks.h"

#include <memory>
#include <string>
#include <vector>

using namespace themis;
using namespace themis::toolbox;
using namespace themis::aql;
using namespace themis::ingestion;

// ─────────────────────────────────────────────────────────────────────────────
// IngestionToolbox tests (IT-*)
// ─────────────────────────────────────────────────────────────────────────────

TEST(IngestionToolbox, IT01_DefaultConstruct) {
    IngestionToolbox toolbox;
    EXPECT_NE(toolbox.workflowEngine(), nullptr);
}

TEST(IngestionToolbox, IT02_CreateDefault) {
    auto tb = IngestionToolbox::createDefault();
    ASSERT_NE(tb, nullptr);
    EXPECT_NE(tb->workflowEngine(), nullptr);
}

TEST(IngestionToolbox, IT03_WorkflowEngineNeverNull) {
    IngestionToolbox tb;
    EXPECT_NE(tb.workflowEngine(), nullptr);
}

TEST(IngestionToolbox, IT04_DefaultTextBackendIsNull) {
    IngestionToolbox tb;
    auto backend = tb.textBackend();
    ASSERT_NE(backend, nullptr);
    // The default is NullTextGenerationBackend — it is unavailable
    EXPECT_FALSE(backend->isAvailable());
}

TEST(IngestionToolbox, IT05_SetTextBackendNullReinstatesNull) {
    auto tb = IngestionToolbox::createDefault();
    // Inject a real-ish backend, then reset with nullptr
    auto dummy = std::make_shared<NullTextGenerationBackend>();
    tb->setTextBackend(dummy);
    tb->setTextBackend(nullptr); // must reinstate NullTextGenerationBackend
    EXPECT_FALSE(tb->textBackend()->isAvailable());
}

TEST(IngestionToolbox, IT06_SetWorkflowEngineNullThrows) {
    IngestionToolbox tb;
    EXPECT_THROW(tb.setWorkflowEngine(nullptr), std::invalid_argument);
}

TEST(IngestionToolbox, IT07_SetWorkflowEngineReplaces) {
    IngestionToolbox tb;
    auto old_engine = tb.workflowEngine();
    auto new_engine = std::make_shared<ingestion::WorkflowEngine>();
    tb.setWorkflowEngine(new_engine);
    EXPECT_EQ(tb.workflowEngine(), new_engine);
    EXPECT_NE(tb.workflowEngine(), old_engine);
}

TEST(IngestionToolbox, IT08_StepRegistryAccessible) {
    IngestionToolbox tb;
    auto& reg = tb.stepRegistry();
    // listSteps() should return a vector (possibly empty or pre-populated)
    auto steps = reg.listSteps();
    EXPECT_GE(steps.size(), 0u); // at least doesn't crash
}

TEST(IngestionToolbox, IT09_ExtractEntitiesEmptyTextReturnsEmpty) {
    auto tb = IngestionToolbox::createDefault();
    auto entities = tb->extractEntities("");
    EXPECT_TRUE(entities.empty());
}

TEST(IngestionToolbox, IT10_ExtractEntitiesTextNoCrash) {
    auto tb = IngestionToolbox::createDefault();
    // No profiles loaded → execute returns ERR_WORKFLOW_NO_MATCHING_PROFILE
    // → extractEntities() degrades gracefully and returns {}
    auto entities = tb->extractEntities("Der Antragsteller beantragt die Genehmigung.");
    // Result may be empty (no profiles loaded) or non-empty; must not crash
    SUCCEED();
}

TEST(IngestionToolbox, IT11_MetricsTrackEmptyExtractions) {
    auto tb = IngestionToolbox::createDefault();
    
    // Extract with empty result (null backend returns empty)
    auto result1 = tb->extractEntitySet("");
    EXPECT_TRUE(result1.nodes.empty() && result1.chunks.empty());
    
    // Metrics should reflect the call
    std::string metrics = tb->getMetricsText();
    EXPECT_TRUE(metrics.find("toolbox_extract_calls_total") != std::string::npos);
    EXPECT_TRUE(metrics.find("toolbox_extract_empty_results_total") != std::string::npos);
}

TEST(IngestionToolbox, IT12_ExtractEntitySetWithEmptyText) {
    auto tb = IngestionToolbox::createDefault();
    auto result = tb->extractEntitySet("", "text/plain", "file.txt");
    
    // Empty text should produce empty entity set
    EXPECT_TRUE(result.nodes.empty());
    EXPECT_TRUE(result.chunks.empty());
    
    // Verify metrics track this as empty result
    std::string metrics = tb->getMetricsText();
    EXPECT_FALSE(metrics.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// AQLIngestionBridge tests (AB-*)
// ─────────────────────────────────────────────────────────────────────────────

namespace {
std::shared_ptr<IngestionToolbox> makeToolbox() {
    return IngestionToolbox::createDefault();
}
} // anonymous namespace

TEST(AQLIngestionBridge, AB01_ConstructWithValidToolbox) {
    auto tb = makeToolbox();
    EXPECT_NO_THROW(AQLIngestionBridge bridge(tb));
}

TEST(AQLIngestionBridge, AB02_ConstructWithNullToolboxThrows) {
    EXPECT_THROW(AQLIngestionBridge bridge(nullptr), std::invalid_argument);
}

TEST(AQLIngestionBridge, AB03_ToolboxAccessor) {
    auto tb = makeToolbox();
    AQLIngestionBridge bridge(tb);
    EXPECT_EQ(bridge.toolbox(), tb);
}

TEST(AQLIngestionBridge, AB04_GraphWriterNullByDefault) {
    auto tb = makeToolbox();
    AQLIngestionBridge bridge(tb);
    EXPECT_EQ(bridge.graphWriter(), nullptr);
}

TEST(AQLIngestionBridge, AB05_EnrichNonObjectIsNoop) {
    auto tb = makeToolbox();
    AQLIngestionBridge bridge(tb);

    nlohmann::json arr = nlohmann::json::array();
    auto ctx = bridge.enrichInsertPayload(arr);
    EXPECT_TRUE(ctx.empty());
    EXPECT_TRUE(arr.is_array()); // unchanged
}

TEST(AQLIngestionBridge, AB06_EnrichObjectWithoutTextField) {
    auto tb = makeToolbox();
    AQLIngestionBridge bridge(tb);

    nlohmann::json doc = {{"name", "Alice"}, {"age", 30}};
    auto ctx = bridge.enrichInsertPayload(doc);
    EXPECT_TRUE(ctx.empty());
    EXPECT_FALSE(doc.contains("_entities"));
}

TEST(AQLIngestionBridge, AB07_EnrichObjectWithTextFieldNoCrash) {
    auto tb = makeToolbox();
    AQLIngestionBridge bridge(tb);

    nlohmann::json doc = {{"text", "Some legal text about § 4 BImSchG."}};
    EXPECT_NO_THROW(bridge.enrichInsertPayload(doc));
    // _entities may or may not be added depending on workflow profiles loaded
    SUCCEED();
}

TEST(AQLIngestionBridge, AB08_ExtractEntitiesForContextEmpty) {
    auto tb = makeToolbox();
    AQLIngestionBridge bridge(tb);
    auto entities = bridge.extractEntitiesForContext("");
    EXPECT_TRUE(entities.empty());
}

TEST(AQLIngestionBridge, AB09_BuildEntityContextEmpty) {
    std::string ctx = AQLIngestionBridge::buildEntityContext({});
    EXPECT_TRUE(ctx.empty());
}

TEST(AQLIngestionBridge, AB10_BuildEntityContextNonEmpty) {
    BaseEntity e;
    e.id          = "law:BGB:§823";
    e.entity_type = EntityType::LEGAL_PROVISION;
    e.text        = "§ 823 BGB";

    auto ctx = AQLIngestionBridge::buildEntityContext({e});
    EXPECT_FALSE(ctx.empty());
    EXPECT_NE(ctx.find("Extracted entities:"), std::string::npos);
    EXPECT_NE(ctx.find("LEGAL_PROVISION"), std::string::npos);
    EXPECT_NE(ctx.find("law:BGB:§823"), std::string::npos);
}

// ─────────────────────────────────────────────────────────────────────────────
// AQLQueryBuilder enrichment flag tests (QB-*)
// ─────────────────────────────────────────────────────────────────────────────

TEST(AQLQueryBuilderEnrichment, QB01_DefaultFalse) {
    AQLQueryBuilder builder;
    EXPECT_FALSE(builder.hasIngestionEnrichment());
}

TEST(AQLQueryBuilderEnrichment, QB02_SetTrue) {
    AQLQueryBuilder builder;
    builder.withIngestionEnrichment(true);
    EXPECT_TRUE(builder.hasIngestionEnrichment());
}

TEST(AQLQueryBuilderEnrichment, QB03_SetFalse) {
    AQLQueryBuilder builder;
    builder.withIngestionEnrichment(true);
    builder.withIngestionEnrichment(false);
    EXPECT_FALSE(builder.hasIngestionEnrichment());
}

TEST(AQLQueryBuilderEnrichment, QB04_Fluent) {
    AQLQueryBuilder builder;
    // withIngestionEnrichment must return *this for fluent chaining
    auto& ref = builder.withIngestionEnrichment(true);
    EXPECT_EQ(&ref, &builder);
}

// ─────────────────────────────────────────────────────────────────────────────
// LLMAQLHandler bridge injection tests (LH-*)
// ─────────────────────────────────────────────────────────────────────────────

TEST(LLMAQLHandlerBridge, LH01_SetNullBridgeAccepted) {
    LLMAQLHandler handler;
    EXPECT_NO_THROW(handler.setIngestionBridge(nullptr));
}

TEST(LLMAQLHandlerBridge, LH02_BridgeNullByDefault) {
    LLMAQLHandler handler;
    EXPECT_EQ(handler.ingestionBridge(), nullptr);
}

TEST(LLMAQLHandlerBridge, LH03_SetBridgeStoredAndRetrievable) {
    LLMAQLHandler handler;
    auto tb     = makeToolbox();
    auto bridge = std::make_shared<AQLIngestionBridge>(tb);
    handler.setIngestionBridge(bridge);
    EXPECT_EQ(handler.ingestionBridge(), bridge);
}
