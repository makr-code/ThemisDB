/**
 * @file tests/move_semantics/test_type_c_moves.cpp
 * @brief Sprint 8 Phase 3 — Type C Complex Move Semantics Tests
 *
 * Verifies that all Type C move semantics remediations are correct:
 *   - Polymorphic base types carry noexcept move constructors / assignments
 *   - Concrete derived classes transfer all data members
 *   - Classes with non-moveable internals (mutex) are explicitly deleted
 *   - Moved-from objects remain in a valid (empty/default) state
 *
 * CWE coverage: CWE-415 (double-free), CWE-416 (use-after-free), CWE-763
 * (release of invalid pointer)
 *
 * @author ThemisDB Team
 * @date 2026-07-06
 * @version 1.0.0
 * @license Apache 2.0
 */

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

// ── Module headers under test ────────────────────────────────────────────────
#include "llm/llm_plugin_interface.h"
#include "llm/gguf_st_adapter.h"
#include "graph/distributed_graph.h"
#include "analytics/analytics_export.h"
#include "analytics/arrow_flight.h"
#include "temporal/bi_temporal.h"

namespace themis { namespace test { namespace move_semantics { 

// ============================================================================
// LLM Module — Type C Gap 1-2: ModelInfo / LoRAInfo polymorphic structs
// ============================================================================

class ModelInfoMoveTest : public ::testing::Test {};

TEST_F(ModelInfoMoveTest, MoveConstructorTransfersAllFields) {
    using namespace themis::llm;

    ModelInfo src;
    src.name          = "mistral-7b";
    src.path          = "/models/mistral-7b.gguf";
    src.format        = "gguf";
    src.architecture  = "llama";
    src.model_id      = "m1";
    src.is_loaded     = true;
    src.size_bytes    = 8'000'000;
    src.quantization  = "q4_k_m";
    src.parameter_count   = 7'000'000'000ULL;
    src.context_length    = 4096;
    src.vocab_size        = 32000;
    src.vram_required_mb  = 6000;
    src.ram_required_mb   = 2000;

    ModelInfo dst(std::move(src));

    EXPECT_EQ(dst.name,         "mistral-7b");
    EXPECT_EQ(dst.path,         "/models/mistral-7b.gguf");
    EXPECT_EQ(dst.format,       "gguf");
    EXPECT_EQ(dst.architecture, "llama");
    EXPECT_EQ(dst.model_id,     "m1");
    EXPECT_TRUE(dst.is_loaded);
    EXPECT_EQ(dst.size_bytes,   8'000'000u);
    EXPECT_EQ(dst.quantization, "q4_k_m");
    EXPECT_EQ(dst.parameter_count,  7'000'000'000ULL);
    EXPECT_EQ(dst.context_length,   4096u);
    EXPECT_EQ(dst.vocab_size,       32000u);
    EXPECT_EQ(dst.vram_required_mb, 6000u);
    EXPECT_EQ(dst.ram_required_mb,  2000u);
}

TEST_F(ModelInfoMoveTest, SourceIsValidAfterMove) {
    using namespace themis::llm;
    ModelInfo src;
    src.name = "test-model";

    ModelInfo dst(std::move(src));

    // Moved-from std::string is in valid-empty state
    EXPECT_TRUE(src.name.empty());
    EXPECT_FALSE(src.is_loaded);
}

TEST_F(ModelInfoMoveTest, MoveAssignmentTransfersAllFields) {
    using namespace themis::llm;
    ModelInfo src;
    src.name    = "gpt-4o";
    src.model_id = "gpt4";
    src.size_bytes = 999;

    ModelInfo dst;
    dst = std::move(src);

    EXPECT_EQ(dst.name,      "gpt-4o");
    EXPECT_EQ(dst.model_id,  "gpt4");
    EXPECT_EQ(dst.size_bytes, 999u);
    EXPECT_TRUE(src.name.empty());
}

TEST_F(ModelInfoMoveTest, IsNoexcept) {
    using T = themis::llm::ModelInfo;
    EXPECT_TRUE(std::is_nothrow_move_constructible_v<T>);
    EXPECT_TRUE(std::is_nothrow_move_assignable_v<T>);
}

// ── LoRAInfo ─────────────────────────────────────────────────────────────────

class LoRAInfoMoveTest : public ::testing::Test {};

TEST_F(LoRAInfoMoveTest, MoveConstructorTransfersFields) {
    using namespace themis::llm;
    LoRAInfo src;
    src.id          = "lora-001";
    src.name        = "code-lora";
    src.path        = "/loras/code.safetensors";
    src.base_model  = "mistral-7b";
    src.is_loaded   = true;
    src.size_bytes  = 50'000;
    src.scale       = 0.8f;

    LoRAInfo dst(std::move(src));

    EXPECT_EQ(dst.id,         "lora-001");
    EXPECT_EQ(dst.name,       "code-lora");
    EXPECT_EQ(dst.path,       "/loras/code.safetensors");
    EXPECT_EQ(dst.base_model, "mistral-7b");
    EXPECT_TRUE(dst.is_loaded);
    EXPECT_EQ(dst.size_bytes, 50'000u);
    EXPECT_FLOAT_EQ(dst.scale, 0.8f);
}

TEST_F(LoRAInfoMoveTest, SourceIsValidAfterMove) {
    using namespace themis::llm;
    LoRAInfo src;
    src.id = "lora-x";
    LoRAInfo dst(std::move(src));
    EXPECT_TRUE(src.id.empty());
}

TEST_F(LoRAInfoMoveTest, MoveAssignmentTransfersFields) {
    using namespace themis::llm;
    LoRAInfo src;
    src.id = "a";
    src.scale = 1.5f;

    LoRAInfo dst;
    dst = std::move(src);
    EXPECT_EQ(dst.id, "a");
    EXPECT_FLOAT_EQ(dst.scale, 1.5f);
    EXPECT_TRUE(src.id.empty());
}

TEST_F(LoRAInfoMoveTest, IsNoexcept) {
    using T = themis::llm::LoRAInfo;
    EXPECT_TRUE(std::is_nothrow_move_constructible_v<T>);
    EXPECT_TRUE(std::is_nothrow_move_assignable_v<T>);
}

// ============================================================================
// LLM Module — Type C Gap 3: InferenceResponse polymorphic struct
// ============================================================================

class InferenceResponseMoveTest : public ::testing::Test {};

TEST_F(InferenceResponseMoveTest, MoveConstructorTransfersText) {
    using namespace themis::llm;
    InferenceResponse src;
    src.text             = "Hello world";
    src.model_id         = "m1";
    src.request_id       = "req-42";
    src.success          = true;
    src.tokens_generated = 42;
    src.inference_time_ms = 123.4f;
    src.tool_calls.push_back({"fn", nlohmann::json::object()});
    src.logprobs = {-0.1f, -0.2f, -0.3f};

    InferenceResponse dst(std::move(src));

    EXPECT_EQ(dst.text,             "Hello world");
    EXPECT_EQ(dst.model_id,         "m1");
    EXPECT_EQ(dst.request_id,       "req-42");
    EXPECT_TRUE(dst.success);
    EXPECT_EQ(dst.tokens_generated, 42);
    EXPECT_FLOAT_EQ(dst.inference_time_ms, 123.4f);
    EXPECT_EQ(dst.tool_calls.size(), 1u);
    EXPECT_EQ(dst.logprobs.size(),   3u);
}

TEST_F(InferenceResponseMoveTest, SourceContainersEmptyAfterMove) {
    using namespace themis::llm;
    InferenceResponse src;
    src.text = "test";
    src.logprobs = {1.0f, 2.0f};
    src.tool_calls.push_back({"foo", nlohmann::json::object()});

    InferenceResponse dst(std::move(src));

    EXPECT_TRUE(src.text.empty());
    EXPECT_TRUE(src.logprobs.empty());
    EXPECT_TRUE(src.tool_calls.empty());
}

TEST_F(InferenceResponseMoveTest, MoveAssignment) {
    using namespace themis::llm;
    InferenceResponse src;
    src.text    = "assigned";
    src.success = true;

    InferenceResponse dst;
    dst = std::move(src);
    EXPECT_EQ(dst.text, "assigned");
    EXPECT_TRUE(dst.success);
    EXPECT_TRUE(src.text.empty());
}

TEST_F(InferenceResponseMoveTest, IsNoexcept) {
    using T = themis::llm::InferenceResponse;
    EXPECT_TRUE(std::is_nothrow_move_constructible_v<T>);
    EXPECT_TRUE(std::is_nothrow_move_assignable_v<T>);
}

// ============================================================================
// LLM Module — Type C Gap 4: RAGContext polymorphic struct
// ============================================================================

class RAGContextMoveTest : public ::testing::Test {};

TEST_F(RAGContextMoveTest, MoveConstructorTransfersDocuments) {
    using namespace themis::llm;
    RAGContext src;
    src.query           = "What is ThemisDB?";
    src.collection_name = "docs";
    src.top_k           = 5;
    src.max_context_tokens = 4096;

    RAGContext::Document doc;
    doc.content          = "ThemisDB is a temporal graph database.";
    doc.source           = "README";
    doc.relevance_score  = 0.95f;
    src.documents.push_back(std::move(doc));

    RAGContext dst(std::move(src));

    EXPECT_EQ(dst.query,           "What is ThemisDB?");
    EXPECT_EQ(dst.collection_name, "docs");
    EXPECT_EQ(dst.top_k,           5);
    EXPECT_EQ(dst.max_context_tokens, 4096);
    ASSERT_EQ(dst.documents.size(),   1u);
    EXPECT_EQ(dst.documents[0].content, "ThemisDB is a temporal graph database.");
    EXPECT_FLOAT_EQ(dst.documents[0].relevance_score, 0.95f);
}

TEST_F(RAGContextMoveTest, SourceEmptyAfterMove) {
    using namespace themis::llm;
    RAGContext src;
    src.query = "query";
    src.documents.push_back({"content", "src", 1.0f, {}});

    RAGContext dst(std::move(src));
    EXPECT_TRUE(src.query.empty());
    EXPECT_TRUE(src.documents.empty());
}

TEST_F(RAGContextMoveTest, IsNoexcept) {
    using T = themis::llm::RAGContext;
    EXPECT_TRUE(std::is_nothrow_move_constructible_v<T>);
    EXPECT_TRUE(std::is_nothrow_move_assignable_v<T>);
}

// ============================================================================
// LLM Module — Type C Gap 5: ILLMPlugin abstract base noexcept move traits
// ============================================================================

class ILLMPluginMoveTraitsTest : public ::testing::Test {};

TEST_F(ILLMPluginMoveTraitsTest, AbstractBaseIsNothrowMoveConstructible) {
    // Abstract class cannot be instantiated, but we can check the trait.
    EXPECT_TRUE(std::is_nothrow_move_constructible_v<themis::llm::ILLMPlugin>);
}

TEST_F(ILLMPluginMoveTraitsTest, AbstractBaseIsNothrowMoveAssignable) {
    EXPECT_TRUE(std::is_nothrow_move_assignable_v<themis::llm::ILLMPlugin>);
}

TEST_F(ILLMPluginMoveTraitsTest, AbstractBaseIsNotCopyConstructible) {
    EXPECT_FALSE(std::is_copy_constructible_v<themis::llm::ILLMPlugin>);
}

// ============================================================================
// LLM Module — Type C Gap 6: LLMPluginAdapter move semantics
// ============================================================================

/// Minimal concrete ILLMPlugin for testing adapter ownership transfer
class MinimalLLMPlugin : public themis::llm::ILLMPlugin {
public:
    explicit MinimalLLMPlugin(int id) : id_(id) {}

    bool loadModel(const std::string&, const nlohmann::json&) override { return true; }
    void unloadModel() override {}
    std::optional<themis::llm::ModelInfo> getModelInfo() const override { return std::nullopt; }
    bool isModelLoaded() const override { return false; }
    bool loadLoRA(const std::string&, const std::string&, float) override { return true; }
    bool unloadLoRA(const std::string&) override { return true; }
    std::vector<themis::llm::LoRAInfo> listLoRAs() const override { return {}; }
    themis::llm::InferenceResponse generate(const themis::llm::InferenceRequest&) override {
        return {};
    }
    themis::llm::InferenceResponse generateRAG(
        const themis::llm::RAGContext&,
        const themis::llm::InferenceRequest&) override { return {}; }
    std::vector<float> embed(const std::string&) override { return {}; }
    themis::llm::LLMCapabilities getCapabilities() const override { return {}; }
    nlohmann::json getMemoryStats() const override { return {}; }
    nlohmann::json getPerformanceStats() const override { return {}; }
    std::vector<uint8_t> exportLoRA(const std::string&) override { return {}; }
    bool importLoRA(const std::string&, const std::vector<uint8_t>&) override { return true; }

    int id() const { return id_; }
private:
    int id_;
};

class LLMPluginAdapterMoveTest : public ::testing::Test {};

TEST_F(LLMPluginAdapterMoveTest, MoveConstructorTransfersOwnership) {
    using namespace themis::llm;
    auto plugin = std::make_unique<MinimalLLMPlugin>(42);
    MinimalLLMPlugin* raw = plugin.get();

    LLMPluginAdapter adapter(std::move(plugin));
    LLMPluginAdapter moved(std::move(adapter));

    // Pointer identity preserved
    EXPECT_EQ(moved.getLLMPlugin(), raw);
}

TEST_F(LLMPluginAdapterMoveTest, SourceNullAfterMove) {
    using namespace themis::llm;
    auto plugin = std::make_unique<MinimalLLMPlugin>(7);
    LLMPluginAdapter src(std::move(plugin));
    LLMPluginAdapter dst(std::move(src));

    EXPECT_EQ(src.getLLMPlugin(), nullptr);
    EXPECT_NE(dst.getLLMPlugin(), nullptr);
}

TEST_F(LLMPluginAdapterMoveTest, MoveAssignmentTransfersOwnership) {
    using namespace themis::llm;
    auto p1 = std::make_unique<MinimalLLMPlugin>(1);
    auto p2 = std::make_unique<MinimalLLMPlugin>(2);
    MinimalLLMPlugin* raw2 = p2.get();

    LLMPluginAdapter a1(std::move(p1));
    LLMPluginAdapter a2(std::move(p2));

    a1 = std::move(a2);
    EXPECT_EQ(a1.getLLMPlugin(), raw2);
    EXPECT_EQ(a2.getLLMPlugin(), nullptr);
}

TEST_F(LLMPluginAdapterMoveTest, IsNoexcept) {
    using T = themis::llm::LLMPluginAdapter;
    EXPECT_TRUE(std::is_nothrow_move_constructible_v<T>);
    EXPECT_TRUE(std::is_nothrow_move_assignable_v<T>);
}

// ============================================================================
// LLM Module — Type C Gap 7: SectionHeader polymorphic struct
// ============================================================================

class SectionHeaderMoveTest : public ::testing::Test {};

TEST_F(SectionHeaderMoveTest, MoveConstructorTransfersFields) {
    using namespace themis::llm;
    SectionHeader src;
    src.magic[0] = 'S'; src.magic[1] = 'T'; src.magic[2] = 'N'; src.magic[3] = 'S';
    src.version   = 2;
    src.data_size = 1024;
    src.flags     = 0x0001;
    src.reserved  = 0;

    SectionHeader dst(std::move(src));
    EXPECT_EQ(dst.magic[0], 'S');
    EXPECT_EQ(dst.magic[1], 'T');
    EXPECT_EQ(dst.version,   2u);
    EXPECT_EQ(dst.data_size, 1024u);
    EXPECT_EQ(dst.flags,     0x0001u);
}

TEST_F(SectionHeaderMoveTest, IsNoexcept) {
    using T = themis::llm::SectionHeader;
    EXPECT_TRUE(std::is_nothrow_move_constructible_v<T>);
    EXPECT_TRUE(std::is_nothrow_move_assignable_v<T>);
}

// ============================================================================
// LLM Module — Type C Gap 8: GGUFSTAdapter move-only semantics
// ============================================================================

class GGUFSTAdapterMoveTest : public ::testing::Test {};

TEST_F(GGUFSTAdapterMoveTest, IsNothrowMoveConstructible) {
    EXPECT_TRUE(std::is_nothrow_move_constructible_v<themis::llm::GGUFSTAdapter>);
}

TEST_F(GGUFSTAdapterMoveTest, IsNothrowMoveAssignable) {
    EXPECT_TRUE(std::is_nothrow_move_assignable_v<themis::llm::GGUFSTAdapter>);
}

TEST_F(GGUFSTAdapterMoveTest, IsNotCopyConstructible) {
    EXPECT_FALSE(std::is_copy_constructible_v<themis::llm::GGUFSTAdapter>);
}

// ============================================================================
// Graph Module — Type C Gap 9: ShardGraphExecutor polymorphic base
// ============================================================================

class ShardGraphExecutorMoveTest : public ::testing::Test {};

TEST_F(ShardGraphExecutorMoveTest, AbstractBaseIsNothrowMoveConstructible) {
    EXPECT_TRUE(std::is_nothrow_move_constructible_v<themis::graph::ShardGraphExecutor>);
}

TEST_F(ShardGraphExecutorMoveTest, AbstractBaseIsNothrowMoveAssignable) {
    EXPECT_TRUE(std::is_nothrow_move_assignable_v<themis::graph::ShardGraphExecutor>);
}

TEST_F(ShardGraphExecutorMoveTest, AbstractBaseIsNotCopyConstructible) {
    EXPECT_FALSE(std::is_copy_constructible_v<themis::graph::ShardGraphExecutor>);
}

// ============================================================================
// Graph Module — Type C Gap 10: LocalShardGraphExecutor concrete move
// ============================================================================

class LocalShardGraphExecutorMoveTest : public ::testing::Test {};

TEST_F(LocalShardGraphExecutorMoveTest, IsNothrowMoveConstructible) {
    EXPECT_TRUE(std::is_nothrow_move_constructible_v<themis::graph::LocalShardGraphExecutor>);
}

TEST_F(LocalShardGraphExecutorMoveTest, IsNothrowMoveAssignable) {
    EXPECT_TRUE(std::is_nothrow_move_assignable_v<themis::graph::LocalShardGraphExecutor>);
}

TEST_F(LocalShardGraphExecutorMoveTest, IsNotCopyConstructible) {
    EXPECT_FALSE(std::is_copy_constructible_v<themis::graph::LocalShardGraphExecutor>);
}

// ============================================================================
// Analytics Module — Type C Gap 11: IAnalyticsExporter abstract base
// ============================================================================

class IAnalyticsExporterMoveTest : public ::testing::Test {};

TEST_F(IAnalyticsExporterMoveTest, AbstractBaseIsNothrowMoveConstructible) {
    EXPECT_TRUE(std::is_nothrow_move_constructible_v<themis::analytics::IAnalyticsExporter>);
}

TEST_F(IAnalyticsExporterMoveTest, AbstractBaseIsNothrowMoveAssignable) {
    EXPECT_TRUE(std::is_nothrow_move_assignable_v<themis::analytics::IAnalyticsExporter>);
}

TEST_F(IAnalyticsExporterMoveTest, AbstractBaseIsNotCopyConstructible) {
    EXPECT_FALSE(std::is_copy_constructible_v<themis::analytics::IAnalyticsExporter>);
}

// ============================================================================
// Analytics Module — Type C Gap 12-13: ArrowFlightServer / ArrowFlightClient
// ============================================================================

class ArrowFlightServerMoveTest : public ::testing::Test {};

TEST_F(ArrowFlightServerMoveTest, AbstractBaseIsNothrowMoveConstructible) {
    EXPECT_TRUE(std::is_nothrow_move_constructible_v<themis::analytics::ArrowFlightServer>);
}

TEST_F(ArrowFlightServerMoveTest, AbstractBaseIsNothrowMoveAssignable) {
    EXPECT_TRUE(std::is_nothrow_move_assignable_v<themis::analytics::ArrowFlightServer>);
}

TEST_F(ArrowFlightServerMoveTest, AbstractBaseIsNotCopyConstructible) {
    EXPECT_FALSE(std::is_copy_constructible_v<themis::analytics::ArrowFlightServer>);
}

class ArrowFlightClientMoveTest : public ::testing::Test {};

TEST_F(ArrowFlightClientMoveTest, AbstractBaseIsNothrowMoveConstructible) {
    EXPECT_TRUE(std::is_nothrow_move_constructible_v<themis::analytics::ArrowFlightClient>);
}

TEST_F(ArrowFlightClientMoveTest, AbstractBaseIsNothrowMoveAssignable) {
    EXPECT_TRUE(std::is_nothrow_move_assignable_v<themis::analytics::ArrowFlightClient>);
}

TEST_F(ArrowFlightClientMoveTest, AbstractBaseIsNotCopyConstructible) {
    EXPECT_FALSE(std::is_copy_constructible_v<themis::analytics::ArrowFlightClient>);
}

// ============================================================================
// Temporal Module — Type C Gap 14: BiTemporalTable explicit move delete
// ============================================================================

class BiTemporalTableMoveTest : public ::testing::Test {};

TEST_F(BiTemporalTableMoveTest, IsNotMoveConstructible) {
    // BiTemporalTable holds std::mutex — explicitly deleted to prevent data races.
    EXPECT_FALSE(std::is_move_constructible_v<themisdb::temporal::BiTemporalTable>);
}

TEST_F(BiTemporalTableMoveTest, IsNotMoveAssignable) {
    EXPECT_FALSE(std::is_move_assignable_v<themisdb::temporal::BiTemporalTable>);
}

TEST_F(BiTemporalTableMoveTest, IsNotCopyConstructible) {
    EXPECT_FALSE(std::is_copy_constructible_v<themisdb::temporal::BiTemporalTable>);
}

TEST_F(BiTemporalTableMoveTest, IsNotCopyAssignable) {
    EXPECT_FALSE(std::is_copy_assignable_v<themisdb::temporal::BiTemporalTable>);
}

TEST_F(BiTemporalTableMoveTest, CanBeWrappedInSharedPtr) {
    // Verify the workaround: shared ownership via std::shared_ptr.
    auto tbl = std::make_shared<themisdb::temporal::BiTemporalTable>("test_table");
    EXPECT_NE(tbl, nullptr);
    EXPECT_EQ(tbl->tableName(), "test_table");

    // Shared pointer transfer (not a move of the table itself).
    auto tbl2 = tbl;
    EXPECT_EQ(tbl2->tableName(), "test_table");
    EXPECT_EQ(tbl.get(), tbl2.get()); // same underlying object
}

// ============================================================================
// Cross-cutting: noexcept trait verification summary
// ============================================================================

class NoexceptMoveTraitSummaryTest : public ::testing::Test {};

TEST_F(NoexceptMoveTraitSummaryTest, AllRemediatedTypesAreNothrowMoveOrDeleted) {
    using namespace themis::llm;
    using namespace themis::graph;
    using namespace themis::analytics;

    // LLM module
    EXPECT_TRUE(std::is_nothrow_move_constructible_v<ModelInfo>)       << "ModelInfo";
    EXPECT_TRUE(std::is_nothrow_move_assignable_v<ModelInfo>)          << "ModelInfo";
    EXPECT_TRUE(std::is_nothrow_move_constructible_v<LoRAInfo>)        << "LoRAInfo";
    EXPECT_TRUE(std::is_nothrow_move_assignable_v<LoRAInfo>)           << "LoRAInfo";
    EXPECT_TRUE(std::is_nothrow_move_constructible_v<InferenceResponse>) << "InferenceResponse";
    EXPECT_TRUE(std::is_nothrow_move_assignable_v<InferenceResponse>)  << "InferenceResponse";
    EXPECT_TRUE(std::is_nothrow_move_constructible_v<RAGContext>)      << "RAGContext";
    EXPECT_TRUE(std::is_nothrow_move_assignable_v<RAGContext>)         << "RAGContext";
    EXPECT_TRUE(std::is_nothrow_move_constructible_v<ILLMPlugin>)      << "ILLMPlugin";
    EXPECT_TRUE(std::is_nothrow_move_constructible_v<LLMPluginAdapter>) << "LLMPluginAdapter";
    EXPECT_TRUE(std::is_nothrow_move_assignable_v<LLMPluginAdapter>)   << "LLMPluginAdapter";
    EXPECT_TRUE(std::is_nothrow_move_constructible_v<SectionHeader>)   << "SectionHeader";
    EXPECT_TRUE(std::is_nothrow_move_assignable_v<SectionHeader>)      << "SectionHeader";
    EXPECT_TRUE(std::is_nothrow_move_constructible_v<GGUFSTAdapter>)   << "GGUFSTAdapter";
    EXPECT_TRUE(std::is_nothrow_move_assignable_v<GGUFSTAdapter>)      << "GGUFSTAdapter";

    // Graph module
    EXPECT_TRUE(std::is_nothrow_move_constructible_v<ShardGraphExecutor>)      << "ShardGraphExecutor";
    EXPECT_TRUE(std::is_nothrow_move_constructible_v<LocalShardGraphExecutor>) << "LocalShardGraphExecutor";
    EXPECT_TRUE(std::is_nothrow_move_assignable_v<LocalShardGraphExecutor>)    << "LocalShardGraphExecutor";

    // Analytics module
    EXPECT_TRUE(std::is_nothrow_move_constructible_v<IAnalyticsExporter>) << "IAnalyticsExporter";
    EXPECT_TRUE(std::is_nothrow_move_constructible_v<ArrowFlightServer>)  << "ArrowFlightServer";
    EXPECT_TRUE(std::is_nothrow_move_constructible_v<ArrowFlightClient>)  << "ArrowFlightClient";

    // Temporal module — explicit delete (not movable)
    EXPECT_FALSE(std::is_move_constructible_v<themisdb::temporal::BiTemporalTable>) << "BiTemporalTable";
    EXPECT_FALSE(std::is_move_assignable_v<themisdb::temporal::BiTemporalTable>)    << "BiTemporalTable";
}
} } } // namespace themis::test::move_semantics
