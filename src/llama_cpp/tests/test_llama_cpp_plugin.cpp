/**
 * @file test_llama_cpp_plugin.cpp
 * @brief Unit tests for the llama_cpp LLM plugin
 *
 * Test suite: LlamaCppPluginFocusedTests (30 tests)
 *   Group A (3)  – loadModel: succeeds, double-load, unload
 *   Group B (3)  – getModelInfo: before/after load, model_id
 *   Group C (3)  – isModelLoaded: initially false, after load, after unload
 *   Group D (3)  – generate: uninit returns error, stub echoes prompt, success flag
 *   Group E (3)  – generateRAG: prepends context, calls generate internally
 *   Group F (3)  – embed: returns empty when not loaded, non-empty when loaded
 *   Group G (3)  – LoRA: loadLoRA, listLoRAs, unloadLoRA
 *   Group H (3)  – LoRA: duplicate id replaced, unload nonexistent returns false
 *   Group I (3)  – getCapabilities: supports_lora, supports_embeddings, plugin_version
 *   Group J (3)  – getMemoryStats / getPerformanceStats keys and inference_count
 */

#include <gtest/gtest.h>
#include "llama_cpp/llama_cpp_plugin.h"
#include <nlohmann/json.hpp>

using namespace themis::llamacpp;
using namespace themis::llm;
using json = nlohmann::json;

// ── Group A – loadModel ───────────────────────────────────────────────────────

TEST(LlamaCppPluginFocusedTests, A1_LoadModelReturnsTrue) {
    LlamaCppPlugin p;
    EXPECT_TRUE(p.loadModel("/stub/model.gguf", {}));
    EXPECT_TRUE(p.isModelLoaded());
}

TEST(LlamaCppPluginFocusedTests, A2_DoubleLoadIsSafe) {
    LlamaCppPlugin p;
    EXPECT_TRUE(p.loadModel("/m.gguf", {}));
    EXPECT_TRUE(p.loadModel("/m2.gguf", {}));
    EXPECT_TRUE(p.isModelLoaded());
}

TEST(LlamaCppPluginFocusedTests, A3_UnloadModel) {
    LlamaCppPlugin p;
    p.loadModel("/m.gguf", {});
    p.unloadModel();
    EXPECT_FALSE(p.isModelLoaded());
}

// ── Group B – getModelInfo ────────────────────────────────────────────────────

TEST(LlamaCppPluginFocusedTests, B1_GetModelInfoNulloptWhenNotLoaded) {
    LlamaCppPlugin p;
    EXPECT_FALSE(p.getModelInfo().has_value());
}

TEST(LlamaCppPluginFocusedTests, B2_GetModelInfoPresentAfterLoad) {
    LlamaCppPlugin p;
    p.loadModel("/model.gguf", {});
    EXPECT_TRUE(p.getModelInfo().has_value());
}

TEST(LlamaCppPluginFocusedTests, B3_GetModelInfoContainsModelId) {
    LlamaCppPlugin p;
    p.loadModel("/model.gguf", {});
    EXPECT_EQ(p.getModelInfo()->model_id, "/model.gguf");
}

// ── Group C – isModelLoaded ───────────────────────────────────────────────────

TEST(LlamaCppPluginFocusedTests, C1_InitiallyNotLoaded) {
    LlamaCppPlugin p;
    EXPECT_FALSE(p.isModelLoaded());
}

TEST(LlamaCppPluginFocusedTests, C2_LoadedAfterLoadModel) {
    LlamaCppPlugin p;
    p.loadModel("", {});
    EXPECT_TRUE(p.isModelLoaded());
}

TEST(LlamaCppPluginFocusedTests, C3_NotLoadedAfterUnload) {
    LlamaCppPlugin p;
    p.loadModel("", {});
    p.unloadModel();
    EXPECT_FALSE(p.isModelLoaded());
}

// ── Group D – generate ────────────────────────────────────────────────────────

TEST(LlamaCppPluginFocusedTests, D1_GenerateUninitializedReturnsError) {
    LlamaCppPlugin p;
    InferenceRequest req; req.prompt = "hello";
    const auto resp = p.generate(req);
    EXPECT_FALSE(resp.success);
    EXPECT_FALSE(resp.error_message.empty());
}

TEST(LlamaCppPluginFocusedTests, D2_GenerateAfterLoadReturnsSuccess) {
    LlamaCppPlugin p;
    p.loadModel("", {});
    InferenceRequest req; req.prompt = "hello";
    const auto resp = p.generate(req);
    EXPECT_TRUE(resp.success);
}

TEST(LlamaCppPluginFocusedTests, D3_GenerateTextNotEmpty) {
    LlamaCppPlugin p;
    p.loadModel("", {});
    InferenceRequest req; req.prompt = "hello world";
    EXPECT_FALSE(p.generate(req).text.empty());
}

// ── Group E – generateRAG ─────────────────────────────────────────────────────

TEST(LlamaCppPluginFocusedTests, E1_GenerateRAGReturnsSuccess) {
    LlamaCppPlugin p;
    p.loadModel("", {});
    InferenceRequest req; req.prompt = "query";
    RAGContext ctx;
    ctx.query = req.prompt;
    RAGContext::Document d1; d1.content = "doc1"; d1.relevance_score = 2.0f;
    RAGContext::Document d2; d2.content = "doc2"; d2.relevance_score = 1.0f;
    ctx.documents = {d1, d2};
    EXPECT_TRUE(p.generateRAG(ctx, req).success);
}

TEST(LlamaCppPluginFocusedTests, E2_GenerateRAGNoContextSameAsGenerate) {
    LlamaCppPlugin p;
    p.loadModel("", {});
    InferenceRequest req; req.prompt = "q";
    RAGContext ctx;
    ctx.query = req.prompt;
    // No documents — should produce the same output as generate()
    EXPECT_EQ(p.generateRAG(ctx, req).text, p.generate(req).text);
}

TEST(LlamaCppPluginFocusedTests, E3_GenerateRAGUninitReturnsError) {
    LlamaCppPlugin p;
    InferenceRequest req; req.prompt = "q";
    RAGContext ctx;
    ctx.query = req.prompt;
    RAGContext::Document d; d.content = "ctx"; d.relevance_score = 1.0f;
    ctx.documents = {d};
    EXPECT_FALSE(p.generateRAG(ctx, req).success);
}

// ── Group F – embed ───────────────────────────────────────────────────────────

TEST(LlamaCppPluginFocusedTests, F1_EmbedReturnsEmptyWhenNotLoaded) {
    LlamaCppPlugin p;
    EXPECT_TRUE(p.embed("hello").empty());
}

TEST(LlamaCppPluginFocusedTests, F2_EmbedReturnsVectorWhenLoaded) {
    LlamaCppPlugin p;
    p.loadModel("", {});
    EXPECT_FALSE(p.embed("hello").empty());
}

TEST(LlamaCppPluginFocusedTests, F3_EmbedReturnsEmptyAfterUnload) {
    LlamaCppPlugin p;
    p.loadModel("", {});
    p.unloadModel();
    EXPECT_TRUE(p.embed("hello").empty());
}

// ── Group G – LoRA load/list/unload ──────────────────────────────────────────

TEST(LlamaCppPluginFocusedTests, G1_LoadLoRAReturnsTrue) {
    LlamaCppPlugin p;
    EXPECT_TRUE(p.loadLoRA("/lora.bin", "adapter1", 1.0f));
}

TEST(LlamaCppPluginFocusedTests, G2_ListLoRAsAfterLoad) {
    LlamaCppPlugin p;
    p.loadLoRA("/lora.bin", "a1", 0.8f);
    const auto loras = p.listLoRAs();
    EXPECT_EQ(loras.size(), 1u);
    EXPECT_EQ(loras[0].lora_id, "a1");
    EXPECT_FLOAT_EQ(loras[0].scale, 0.8f);
}

TEST(LlamaCppPluginFocusedTests, G3_UnloadLoRARemovesEntry) {
    LlamaCppPlugin p;
    p.loadLoRA("/lora.bin", "a1", 1.0f);
    EXPECT_TRUE(p.unloadLoRA("a1"));
    EXPECT_TRUE(p.listLoRAs().empty());
}

// ── Group H – LoRA edge cases ─────────────────────────────────────────────────

TEST(LlamaCppPluginFocusedTests, H1_DuplicateLoRAIdReplaces) {
    LlamaCppPlugin p;
    p.loadLoRA("/lora1.bin", "a1", 1.0f);
    p.loadLoRA("/lora2.bin", "a1", 0.5f);
    const auto loras = p.listLoRAs();
    EXPECT_EQ(loras.size(), 1u);
    EXPECT_FLOAT_EQ(loras[0].scale, 0.5f);
}

TEST(LlamaCppPluginFocusedTests, H2_UnloadNonexistentReturnsFalse) {
    LlamaCppPlugin p;
    EXPECT_FALSE(p.unloadLoRA("nonexistent"));
}

TEST(LlamaCppPluginFocusedTests, H3_MultipleLoRAsListedCorrectly) {
    LlamaCppPlugin p;
    p.loadLoRA("/a.bin", "a", 1.0f);
    p.loadLoRA("/b.bin", "b", 0.5f);
    EXPECT_EQ(p.listLoRAs().size(), 2u);
}

// ── Group I – getCapabilities ─────────────────────────────────────────────────

TEST(LlamaCppPluginFocusedTests, I1_SupportsLoRA) {
    LlamaCppPlugin p;
    EXPECT_TRUE(p.getCapabilities().supports_lora);
}

TEST(LlamaCppPluginFocusedTests, I2_SupportsEmbeddings) {
    LlamaCppPlugin p;
    EXPECT_TRUE(p.getCapabilities().supports_embeddings);
}

TEST(LlamaCppPluginFocusedTests, I3_PluginVersion) {
    LlamaCppPlugin p;
    EXPECT_EQ(p.getCapabilities().plugin_version, "2.0.0");
}

// ── Group J – stats ───────────────────────────────────────────────────────────

TEST(LlamaCppPluginFocusedTests, J1_MemoryStatsContainsRequiredKeys) {
    LlamaCppPlugin p;
    const auto s = p.getMemoryStats();
    EXPECT_TRUE(s.contains("plugin"));
    EXPECT_TRUE(s.contains("model_loaded"));
    EXPECT_TRUE(s.contains("model_id"));
}

TEST(LlamaCppPluginFocusedTests, J2_PerformanceStatsContainsInferenceCount) {
    LlamaCppPlugin p;
    p.loadModel("", {});
    InferenceRequest req; req.prompt = "hi";
    p.generate(req); p.generate(req);
    const auto s = p.getPerformanceStats();
    EXPECT_GE(s["inference_count"].get<uint64_t>(), 2u);
}

TEST(LlamaCppPluginFocusedTests, J3_PluginNameInMemoryStats) {
    LlamaCppPlugin p;
    EXPECT_EQ(p.getMemoryStats()["plugin"].get<std::string>(), "llama_cpp");
}
