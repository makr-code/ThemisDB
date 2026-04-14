/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_llama_cpp_plugin.cpp                          ║
  Version:         0.0.4                                              ║
  Last Modified:   2026-04-14 11:34:24                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   82.0/100                                       ║
    • Total Lines:     472                                            ║
    • Open Issues:     TODOs: 0, Stubs: 8                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • f0f3ecebde  2026-04-11  feat(llama_cpp): v2.1.0 — streaming, batch inference, Plu... ║
    • 7b80a66e02  2026-04-07  fix(llama_cpp): align LlamaCppPlugin with ILLMPlugin inte... ║
    • 1e348484ec  2026-04-07  feat(plugins): add stable_diffusion + llama_cpp plugins, ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file test_llama_cpp_plugin.cpp
 * @brief Unit tests for the llama_cpp LLM plugin
 *
 * Test suite: LlamaCppPluginFocusedTests (50 tests)
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
 *   Group K (5)  – generateStream: callback invoked, no callback path, uninit error,
 *                  callback exception swallowed, response text matches
 *   Group L (5)  – generateBatch: empty input, single request, multiple requests,
 *                  error propagation, order preserved
 *   Group M (4)  – capabilities v2.1.0: supports_streaming, supports_batching,
 *                  plugin_version, getPluginVersion()
 *   Group N (6)  – registrar: createPlugin stub/config, defaultReloadCallback,
 *                  reload with empty path, generate after registrar load,
 *                  InferenceResponse trace_id/span_id echo
 */

#include <gtest/gtest.h>
#include "llama_cpp/llama_cpp_plugin.h"
#include "llama_cpp/llama_cpp_registrar.h"
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
    EXPECT_EQ(p.getCapabilities().plugin_version, "2.1.0");
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

// ── Group K – generateStream ──────────────────────────────────────────────────

TEST(LlamaCppPluginFocusedTests, K1_StreamCallbackInvokedOnSuccess) {
    LlamaCppPlugin p;
    p.loadModel("", {});
    InferenceRequest req;
    req.prompt = "stream test";
    std::vector<std::string> tokens;
    const auto resp = p.generateStream(req,
        [&tokens](const std::string& t) { tokens.push_back(t); });
    EXPECT_TRUE(resp.success);
    EXPECT_FALSE(tokens.empty());
}

TEST(LlamaCppPluginFocusedTests, K2_GenerateStreamResponseTextNotEmpty) {
    LlamaCppPlugin p;
    p.loadModel("", {});
    InferenceRequest req;
    req.prompt = "hello";
    const auto resp = p.generateStream(req, [](const std::string&) {});
    EXPECT_FALSE(resp.text.empty());
}

TEST(LlamaCppPluginFocusedTests, K3_GenerateStreamUninitReturnsError) {
    LlamaCppPlugin p;
    InferenceRequest req;
    req.prompt = "test";
    bool cb_called = false;
    const auto resp = p.generateStream(req,
        [&cb_called](const std::string&) { cb_called = true; });
    EXPECT_FALSE(resp.success);
    EXPECT_FALSE(cb_called);
}

TEST(LlamaCppPluginFocusedTests, K4_StreamCallbackExceptionSwallowed) {
    LlamaCppPlugin p;
    p.loadModel("", {});
    InferenceRequest req;
    req.prompt = "boom";
    // Callback that throws — should not propagate to caller
    EXPECT_NO_THROW({
        p.generateStream(req, [](const std::string&) {
            throw std::runtime_error("intentional");
        });
    });
}

TEST(LlamaCppPluginFocusedTests, K5_StreamCallbackTokenMatchesGenerateText) {
    LlamaCppPlugin p;
    p.loadModel("", {});
    InferenceRequest req;
    req.prompt = "compare";
    std::string streamed;
    const auto stream_resp = p.generateStream(req,
        [&streamed](const std::string& t) { streamed += t; });
    const auto direct_resp = p.generate(req);
    EXPECT_EQ(stream_resp.text, direct_resp.text);
    EXPECT_EQ(streamed, direct_resp.text);
}

// ── Group L – generateBatch ───────────────────────────────────────────────────

TEST(LlamaCppPluginFocusedTests, L1_BatchEmptyInputReturnsEmpty) {
    LlamaCppPlugin p;
    p.loadModel("", {});
    EXPECT_TRUE(p.generateBatch({}).empty());
}

TEST(LlamaCppPluginFocusedTests, L2_BatchSingleRequestReturnsOneResponse) {
    LlamaCppPlugin p;
    p.loadModel("", {});
    InferenceRequest req;
    req.prompt = "single";
    const auto results = p.generateBatch({req});
    ASSERT_EQ(results.size(), 1u);
    EXPECT_TRUE(results[0].success);
}

TEST(LlamaCppPluginFocusedTests, L3_BatchMultipleRequestsPreservesOrder) {
    LlamaCppPlugin p;
    p.loadModel("", {});
    InferenceRequest r1, r2, r3;
    r1.prompt = "aaaa"; r2.prompt = "bbbb"; r3.prompt = "cccc";
    const auto results = p.generateBatch({r1, r2, r3});
    ASSERT_EQ(results.size(), 3u);
    // Each response text should contain part of the corresponding prompt
    EXPECT_NE(results[0].text.find("aaaa"), std::string::npos);
    EXPECT_NE(results[1].text.find("bbbb"), std::string::npos);
    EXPECT_NE(results[2].text.find("cccc"), std::string::npos);
}

TEST(LlamaCppPluginFocusedTests, L4_BatchErrorWhenNotLoaded) {
    LlamaCppPlugin p;
    InferenceRequest req;
    req.prompt = "fail";
    const auto results = p.generateBatch({req});
    ASSERT_EQ(results.size(), 1u);
    EXPECT_FALSE(results[0].success);
}

TEST(LlamaCppPluginFocusedTests, L5_BatchIncreasesInferenceCount) {
    LlamaCppPlugin p;
    p.loadModel("", {});
    InferenceRequest r1, r2;
    r1.prompt = "x"; r2.prompt = "y";
    p.generateBatch({r1, r2});
    const auto stats = p.getPerformanceStats();
    EXPECT_GE(stats["inference_count"].get<uint64_t>(), 2u);
}

// ── Group M – capabilities v2.1.0 ────────────────────────────────────────────

TEST(LlamaCppPluginFocusedTests, M1_CapabilitiesSupportsStreaming) {
    LlamaCppPlugin p;
    EXPECT_TRUE(p.getCapabilities().supports_streaming);
}

TEST(LlamaCppPluginFocusedTests, M2_CapabilitiesSupportsBatching) {
    LlamaCppPlugin p;
    EXPECT_TRUE(p.getCapabilities().supports_batching);
}

TEST(LlamaCppPluginFocusedTests, M3_CapabilitiesPluginVersion210) {
    LlamaCppPlugin p;
    EXPECT_EQ(p.getCapabilities().plugin_version, "2.1.0");
}

TEST(LlamaCppPluginFocusedTests, M4_GetPluginVersionMethod) {
    LlamaCppPlugin p;
    EXPECT_EQ(p.getPluginVersion(), "2.1.0");
}

// ── Group N – LlamaCppPluginRegistrar ────────────────────────────────────────

TEST(LlamaCppPluginFocusedTests, N1_RegistrarCreatePluginStubMode) {
    auto plugin = LlamaCppPluginRegistrar::createPlugin({});
    ASSERT_NE(plugin, nullptr);
    // Stub mode: no model_path in config → plugin not loaded
    EXPECT_FALSE(plugin->isModelLoaded());
}

TEST(LlamaCppPluginFocusedTests, N2_RegistrarCreatePluginWithEmptyPath) {
    json cfg; cfg["model_path"] = "";
    auto plugin = LlamaCppPluginRegistrar::createPlugin(cfg);
    ASSERT_NE(plugin, nullptr);
    // Empty path treated the same as stub mode
    EXPECT_FALSE(plugin->isModelLoaded());
}

TEST(LlamaCppPluginFocusedTests, N3_RegistrarDefaultReloadCallbackStubMode) {
    auto cb = LlamaCppPluginRegistrar::defaultReloadCallback();
    LlamaCppPlugin p;
    // No model_path → stub success
    EXPECT_TRUE(cb(p, {}));
}

TEST(LlamaCppPluginFocusedTests, N4_RegistrarDefaultReloadCallbackWithPath) {
    auto cb = LlamaCppPluginRegistrar::defaultReloadCallback();
    LlamaCppPlugin p;
    json cfg; cfg["model_path"] = "/some/model.gguf";
    // loadModel with any non-empty path always succeeds in stub mode
    EXPECT_TRUE(cb(p, cfg));
    EXPECT_TRUE(p.isModelLoaded());
}

TEST(LlamaCppPluginFocusedTests, N5_RegistrarCreatePluginSupportsGenerate) {
    json cfg; cfg["model_path"] = "/stub/m.gguf";
    auto plugin = LlamaCppPluginRegistrar::createPlugin(cfg);
    ASSERT_NE(plugin, nullptr);
    InferenceRequest req; req.prompt = "hello from registrar";
    const auto resp = plugin->generate(req);
    EXPECT_TRUE(resp.success);
}

TEST(LlamaCppPluginFocusedTests, N6_InferenceResponseEchoesTraceContext) {
    LlamaCppPlugin p;
    p.loadModel("", {});
    InferenceRequest req;
    req.prompt   = "trace test";
    req.trace_id = "abc123";
    req.span_id  = "span456";
    const auto resp = p.generate(req);
    EXPECT_TRUE(resp.success);
    EXPECT_EQ(resp.trace_id, "abc123");
    EXPECT_EQ(resp.span_id,  "span456");
}
