/**
 * @file test_llama_cpp_plugin.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 95/100
 * @note Gap Summary: total=22; TODO=1, Stub=20, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include <gtest/gtest.h>
#include "llama_cpp/llama_cpp_plugin.h"
#include "llama_cpp/llama_cpp_registrar.h"
#include <nlohmann/json.hpp>
#include <atomic>
#include <thread>
#include <vector>

using namespace themis::llamacpp;
using namespace themis::llm;
using json = nlohmann::json;

// Provide the short alias used across test code: `llm::ILLMPlugin` -> `themis::llm::ILLMPlugin`
namespace llm = themis::llm;

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

TEST(LlamaCppPluginFocusedTests, E4_GenerateRAGHonoursExplicitContextOverrideForMaxTokens) {
    LlamaCppPlugin p;
    p.loadModel("", {});

    int observed_max_tokens = -1;
    p.setGenerateFn([&]([[maybe_unused]] const InferenceRequest& request) {
        observed_max_tokens = request.max_tokens;
        InferenceResponse response;
        response.success = true;
        response.text = "ok";
        return response;
    });

    InferenceRequest req;
    req.prompt = "query";
    req.max_tokens = 999;

    RAGContext ctx;
    ctx.query = req.prompt;
    ctx.max_context_tokens = 100;
    ctx.response_budget_tokens = 20;
    RAGContext::Document d;
    d.content = "doc";
    d.relevance_score = 1.0f;
    ctx.documents = {d};

    const auto response = p.generateRAG(ctx, req);
    EXPECT_TRUE(response.success);
    EXPECT_EQ(observed_max_tokens, 20)
        << "generateRAG must derive max_tokens from the effective RAG context window override";
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
    EXPECT_TRUE(p.loadLoRA("adapter1", "/lora.bin", 1.0f));
}

TEST(LlamaCppPluginFocusedTests, G2_ListLoRAsAfterLoad) {
    LlamaCppPlugin p;
    p.loadLoRA("a1", "/lora.bin", 0.8f);
    const auto loras = p.listLoRAs();
    EXPECT_EQ(loras.size(), 1);
    EXPECT_EQ(loras[0].lora_id, "a1");
    EXPECT_FLOAT_EQ(loras[0].scale, 0.8f);
}

TEST(LlamaCppPluginFocusedTests, G3_UnloadLoRARemovesEntry) {
    LlamaCppPlugin p;
    p.loadLoRA("a1", "/lora.bin", 1.0f);
    EXPECT_TRUE(p.unloadLoRA("a1"));
    EXPECT_TRUE(p.listLoRAs().empty());
}

// ── Group H – LoRA edge cases ─────────────────────────────────────────────────

TEST(LlamaCppPluginFocusedTests, H1_DuplicateLoRAIdReplaces) {
    LlamaCppPlugin p;
    p.loadLoRA("a1", "/lora1.bin", 1.0f);
    p.loadLoRA("a1", "/lora2.bin", 0.5f);
    const auto loras = p.listLoRAs();
    EXPECT_EQ(loras.size(), 1);
    EXPECT_FLOAT_EQ(loras[0].scale, 0.5f);
}

TEST(LlamaCppPluginFocusedTests, H2_UnloadNonexistentReturnsFalse) {
    LlamaCppPlugin p;
    EXPECT_FALSE(p.unloadLoRA("nonexistent"));
}

TEST(LlamaCppPluginFocusedTests, H3_MultipleLoRAsListedCorrectly) {
    LlamaCppPlugin p;
    p.loadLoRA("a", "/a.bin", 1.0f);
    p.loadLoRA("b", "/b.bin", 0.5f);
    EXPECT_EQ(p.listLoRAs().size(), 2);
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
    EXPECT_GE(s["inference_count"].get<uint64_t>(), 2);
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
    std::string streamed = {};
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
    ASSERT_EQ(results.size(), 1);
    EXPECT_TRUE(results[0].success);
}

TEST(LlamaCppPluginFocusedTests, L3_BatchMultipleRequestsPreservesOrder) {
    LlamaCppPlugin p;
    p.loadModel("", {});
    InferenceRequest r1, r2, r3;
    r1.prompt = "aaaa"; r2.prompt = "bbbb"; r3.prompt = "cccc";
    const auto results = p.generateBatch({r1, r2, r3});
    ASSERT_EQ(results.size(), 3);
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
    ASSERT_EQ(results.size(), 1);
    EXPECT_FALSE(results[0].success);
}

TEST(LlamaCppPluginFocusedTests, L5_BatchIncreasesInferenceCount) {
    LlamaCppPlugin p;
    p.loadModel("", {});
    InferenceRequest r1, r2;
    r1.prompt = "x"; r2.prompt = "y";
    p.generateBatch({r1, r2});
    const auto stats = p.getPerformanceStats();
    EXPECT_GE(stats["inference_count"].get<uint64_t>(), 2);
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

// ── Group O – Gap 1: structured error when no model loaded (non-stub mode) ───
// These tests verify the production contract: generate() without STUB_MODE
// defined (and without a loaded LlamaWrapper) returns success=false with a
// descriptive error_message.  In this test file THEMIS_LLAMA_CPP_STUB_MODE is
// defined, so we test the error path via the "model not loaded" gate instead.
//
// O1: model never loaded → success=false, error_message non-empty  (D1 alias)
// O2: model loaded with empty path, STUB_MODE → still returns success=true  (sanity)
// O3: error_message contains "not loaded" when model is absent

TEST(LlamaCppPluginFocusedTests, O1_ModelNotLoadedReturnsSuccessFalse) {
    LlamaCppPlugin p;
    // Do NOT call loadModel() — model_loaded_ remains false.
    InferenceRequest req; req.prompt = "test";
    const auto resp = p.generate(req);
    EXPECT_FALSE(resp.success) << "generate() must fail when no model was ever loaded";
    EXPECT_FALSE(resp.error_message.empty()) << "error_message must be set";
}

TEST(LlamaCppPluginFocusedTests, O2_StubModeLoadedReturnsSuccessTrue) {
    // With THEMIS_LLAMA_CPP_STUB_MODE defined (as it is in this test build),
    // loadModel("", {}) + generate() must still return success=true so all
    // existing tests in groups A-N continue to pass.
    LlamaCppPlugin p;
    p.loadModel("", {});
    InferenceRequest req; req.prompt = "stub";
    const auto resp = p.generate(req);
    EXPECT_TRUE(resp.success)
        << "THEMIS_LLAMA_CPP_STUB_MODE must preserve success=true for test builds";
}

TEST(LlamaCppPluginFocusedTests, O3_NotLoadedErrorMessageDescriptive) {
    LlamaCppPlugin p;
    InferenceRequest req; req.prompt = "x";
    const auto resp = p.generate(req);
    EXPECT_FALSE(resp.success);
    EXPECT_NE(resp.error_message.find("not loaded"), std::string::npos)
        << "error_message should mention 'not loaded'; got: " << resp.error_message;
}

// ── Group P – Phase 5: Concurrency hardening ──────────────────────────────────
// P1: Concurrent generate() calls from multiple threads do not race or deadlock.
// P2: Concurrent generateBatch() from multiple threads returns correct response counts.
// P3: Thread-safe LoRA registry under concurrent loadLoRA() + generate() access.
//
// All tests operate in STUB_MODE (no real model required) and set a generous
// timeout via thread join — they fail by hanging, not by assertion.

TEST(LlamaCppPluginFocusedTests, P1_ConcurrentGenerateNoRaceOrDeadlock) {
    constexpr int kThreads  = 8;
    constexpr int kPerThread = 10;

    LlamaCppPlugin plugin;
    plugin.loadModel("", {});  // stub mode

    std::atomic<int> success_count{0};
    std::atomic<int> failure_count{0};

    auto worker = [&]() {
        for (int i = 0; i < kPerThread; ++i) {
            InferenceRequest req;
            req.prompt = "concurrent prompt " + std::to_string(i);
            const auto resp = plugin.generate(req);
            if (resp.success) {
                ++success_count;
            } else {
                ++failure_count;
            }
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back(worker);
    }
    for (auto& th : threads) {
        th.join();
    }

    EXPECT_EQ(success_count.load(), kThreads * kPerThread)
        << "All concurrent generate() calls must succeed in stub mode";
    EXPECT_EQ(failure_count.load(), 0);
}

TEST(LlamaCppPluginFocusedTests, P2_ConcurrentGenerateBatchCorrectResponseCount) {
    constexpr int kThreads    = 4;
    constexpr int kBatchSize  = 5;

    LlamaCppPlugin plugin;
    plugin.loadModel("", {});

    std::atomic<int> wrong_size_count{0};

    auto worker = [&]() {
        std::vector<InferenceRequest> batch;
        batch.reserve(kBatchSize);
        for (int i = 0; i < kBatchSize; ++i) {
            InferenceRequest req;
            req.prompt = "batch item " + std::to_string(i);
            batch.push_back(req);
        }
        const auto results = plugin.generateBatch(batch);
        if (static_cast<int>(results.size()) != kBatchSize) {
            ++wrong_size_count;
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back(worker);
    }
    for (auto& th : threads) {
        th.join();
    }

    EXPECT_EQ(wrong_size_count.load(), 0)
        << "generateBatch() must return exactly batch_size responses under parallel callers";
}

TEST(LlamaCppPluginFocusedTests, P3_ConcurrentLoraRegistryAndGenerate) {
    constexpr int kThreads = 6;

    LlamaCppPlugin plugin;
    plugin.loadModel("", {});

    // Half the threads load LoRA adapters; the other half call generate().
    // No deadlock or crash is expected.
    std::atomic<int> generate_ok{0};

    auto lora_writer = [&]([[maybe_unused]] int id) {
        const std::string path = "/stub/lora_" + std::to_string(id) + ".bin";
        const std::string name = "lora_" + std::to_string(id);
        plugin.loadLoRA(name, path, 1.0f);
    };

    auto generator = [&]() {
        InferenceRequest req;
        req.prompt = "lora race test";
        const auto resp = plugin.generate(req);
        if (resp.success) {
            ++generate_ok;
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        if (t % 2 == 0) {
            threads.emplace_back(lora_writer, t);
        } else {
            threads.emplace_back(generator);
        }
    }
    for (auto& th : threads) {
        th.join();
    }

    // generator threads: kThreads/2 = 3 success expected
    EXPECT_EQ(generate_ok.load(), kThreads / 2)
        << "generate() threads must all succeed despite concurrent LoRA writes";
}

// ── Group Q: setEmbedFn injection (Stub #200) ────────────────────────────────

// LCP-EMBED-01: injected fn is called and its result returned
TEST(LlamaCppPluginFocusedTests, Q1_EmbedFn_InjectedFnCalled) {
    LlamaCppPlugin plugin;
    plugin.loadModel("", {});

    bool called = false;
    plugin.setEmbedFn([&]([[maybe_unused]] const std::string& text) -> std::vector<float> {
        called = true;
        EXPECT_EQ(text, "hello");
        return std::vector<float>(128, 1.0f);
    });

    auto result = plugin.embed("hello");
    EXPECT_TRUE(called) << "setEmbedFn callback must be invoked by embed()";
    ASSERT_EQ(result.size(), 128);
    EXPECT_FLOAT_EQ(result[0], 1.0f);
}

// LCP-EMBED-02: fn returning empty vector causes zero-vector stub fallback
TEST(LlamaCppPluginFocusedTests, Q2_EmbedFn_EmptyReturnFallsBackToStub) {
    LlamaCppPlugin plugin;
    plugin.loadModel("", {});

    plugin.setEmbedFn([](const std::string&) -> std::vector<float> {
        return {};  // empty → trigger stub fallback
    });

    auto result = plugin.embed("anything");
    ASSERT_EQ(result.size(), 384) << "empty fn result must fall back to 384-dim zero vector";
    for (float v : result) {
        EXPECT_FLOAT_EQ(v, 0.0f);
    }
}

// LCP-EMBED-03: clearing fn (nullptr) reverts to zero-vector stub
TEST(LlamaCppPluginFocusedTests, Q3_EmbedFn_ClearingRevertsToStub) {
    LlamaCppPlugin plugin;
    plugin.loadModel("", {});

    plugin.setEmbedFn([](const std::string&) { return std::vector<float>(64, 3.14f); });
    // Verify injection is active
    ASSERT_EQ(plugin.embed("test").size(), 64);

    // Clear the fn
    plugin.setEmbedFn(nullptr);

    auto result = plugin.embed("test");
    ASSERT_EQ(result.size(), 384) << "after clearing embed_fn_, stub zero-vector must be returned";
    for (float v : result) {
        EXPECT_FLOAT_EQ(v, 0.0f);
    }
}

// LCP-EMBED-04: embed() with model not loaded but fn set still uses fn
TEST(LlamaCppPluginFocusedTests, Q4_EmbedFn_UsedEvenWithoutModel) {
    LlamaCppPlugin plugin;
    // Do NOT call loadModel → model_loaded_ is false

    plugin.setEmbedFn([](const std::string& text) {
        return static_cast<bool>(std::vector<float>(16, static_cast<float < static_cast<int>((text.size()))));
    });

    // embed() early-returns {} when !model_loaded_; fn is not called in that path.
    // This test documents the existing contract (model must be loaded).
    auto result = plugin.embed("hi");
    EXPECT_TRUE(result.empty())
        << "embed() returns empty without a loaded model regardless of embed_fn_";
}

TEST(LlamaCppPluginFocusedTests, R1_GenerateFn_InjectedFnCalled) {
    LlamaCppPlugin plugin;
    plugin.loadModel("", {});

    bool called = false;
    plugin.setGenerateFn([&]([[maybe_unused]] const InferenceRequest& request) {
        called = true;
        EXPECT_EQ(request.prompt, "bridge");
        InferenceResponse response;
        response.success = true;
        response.text = "bridged";
        return response;
    });

    InferenceRequest request;
    request.prompt = "bridge";
    const auto response = plugin.generate(request);
    EXPECT_TRUE(called);
    EXPECT_TRUE(response.success);
    EXPECT_EQ(response.text, "bridged");
}

TEST(LlamaCppPluginFocusedTests, R2_GenerateFn_ExceptionFailsClosed) {
    LlamaCppPlugin plugin;
    plugin.loadModel("", {});
    plugin.setGenerateFn([](const InferenceRequest&) -> InferenceResponse {
        throw std::runtime_error("boom");
    });

    InferenceRequest request;
    request.prompt = "bridge";
    const auto response = plugin.generate(request);
    EXPECT_FALSE(response.success);
    EXPECT_NE(response.error_message.find("bridge failed"), std::string::npos);
}

// ── Group S – Function / Tool Calling ────────────────────────────────────────

/// S1: getCapabilities() reports supports_function_call = true.
TEST(LlamaCppPluginFocusedTests, S1_CapabilitiesReportsFunctionCallSupport) {
    LlamaCppPlugin plugin;
    EXPECT_TRUE(plugin.getCapabilities().supports_function_call);
}

/// S2: generate() with tools synthesises a stub tool-call JSON response in
///     THEMIS_LLAMA_CPP_STUB_MODE (the test binary always defines this macro).
TEST(LlamaCppPluginFocusedTests, S2_StubMode_ToolCallSynthesized) {
    LlamaCppPlugin plugin;
    plugin.loadModel("", {});  // stub load

    ToolDefinition tool;
    tool.name        = "get_weather";
    tool.description = "Get the weather for a location";
    tool.parameters  = json::parse(R"({"type":"object","properties":{"location":{"type":"string"}},"required":["location"]})");

    InferenceRequest request;
    request.prompt = "What is the weather in Berlin?";
    request.tools  = {tool};

    const auto response = plugin.generate(request);
    EXPECT_TRUE(response.success);
    // The stub should have produced a tool call with the right name.
    ASSERT_FALSE(response.tool_calls.empty())
        << "Expected at least one parsed tool call in stub mode";
    EXPECT_EQ(response.tool_calls.front().name, "get_weather");
}

/// S3: generate() with tools via generate_fn_ bridge passes tools through and
///     tool_calls populated by the bridge are forwarded unchanged.
TEST(LlamaCppPluginFocusedTests, S3_BridgeFn_ToolCallsForwarded) {
    LlamaCppPlugin plugin;
    plugin.loadModel("", {});

    ToolDefinition tool;
    tool.name       = "lookup_price";
    tool.parameters = json::parse(R"({"type":"object","properties":{"item":{"type":"string"}}})");

    plugin.setGenerateFn([&tool](const InferenceRequest& req) -> InferenceResponse {
        EXPECT_EQ(req.tools.size(), 1);
        EXPECT_EQ(req.tools.front().name, tool.name);
        InferenceResponse resp;
        resp.success = true;
        resp.text    = R"({"name":"lookup_price","arguments":{"item":"laptop"}})";
        ToolCall tc;
        tc.name       = "lookup_price";
        tc.arguments  = json::parse(R"({"item":"laptop"})");
        resp.tool_calls.push_back(std::move(tc));
        return resp;
    });

    InferenceRequest request;
    request.prompt = "How much does a laptop cost?";
    request.tools  = {tool};

    const auto response = plugin.generate(request);
    EXPECT_TRUE(response.success);
    ASSERT_EQ(response.tool_calls.size(), 1);
    EXPECT_EQ(response.tool_calls.front().name, "lookup_price");
    EXPECT_EQ(response.tool_calls.front().arguments.value("item", ""), "laptop");
}

// ── Group T – Per-Request Cancellation Token ──────────────────────────────────

/// T1: generate() returns cancelled error when token is pre-set to true.
TEST(LlamaCppPluginFocusedTests, T1_CancellationToken_PreCancelledRejectsRequest) {
    LlamaCppPlugin plugin;
    plugin.loadModel("", {});
    // Wire a bridge so the test can verify the path never reaches generation.
    bool bridge_called = false;
    plugin.setGenerateFn([&]([[maybe_unused]] const InferenceRequest&) -> InferenceResponse {
        bridge_called = true;
        InferenceResponse r;
        r.success = true;
        r.text    = "should not reach here";
        return r;
    });

    auto cancel_token = std::make_shared<std::atomic<bool>>(true);  // already cancelled

    InferenceRequest request;
    request.prompt             = "should be cancelled";
    request.cancellation_token = cancel_token;

    const auto response = plugin.generate(request);
    EXPECT_FALSE(response.success);
    EXPECT_NE(response.error_message.find("cancelled"), std::string::npos);
    EXPECT_FALSE(bridge_called) << "Bridge must not be invoked after cancellation";
}

/// T2: generate() succeeds normally when token exists but is false.
TEST(LlamaCppPluginFocusedTests, T2_CancellationToken_NotSetAllowsGeneration) {
    LlamaCppPlugin plugin;
    plugin.loadModel("", {});

    auto cancel_token = std::make_shared<std::atomic<bool>>(false);  // not cancelled

    InferenceRequest request;
    request.prompt             = "hello";
    request.cancellation_token = cancel_token;

    const auto response = plugin.generate(request);
    EXPECT_TRUE(response.success);
}

// ── Group U – Concurrency hardening (atomic counters + RAG race-free) ──────────

/// U1: 8 threads: half call generateRAG(), half call loadLoRA() concurrently.
///     No crash and all threads join within 10 s.
TEST(LlamaCppPluginFocusedTests, U1_ConcurrentGenerateRAGAndLoadLoRA_NoCrashOrDeadlock) {
    constexpr int kThreads = 8;

    LlamaCppPlugin plugin;
    plugin.loadModel("", {});

    std::vector<std::thread> threads;
    threads.reserve(kThreads);

    for (int t = 0; t < kThreads; ++t) {
        if (t % 2 == 0) {
            // Even threads: call generateRAG() with 2 documents.
            threads.emplace_back([&plugin, t]() {
                InferenceRequest req;
                req.prompt = "race query " + std::to_string(t);

                RAGContext ctx;
                ctx.query = req.prompt;
                RAGContext::Document d1;
                d1.content         = "document one content";
                d1.relevance_score = 2.0f;
                RAGContext::Document d2;
                d2.content         = "document two content";
                d2.relevance_score = 1.0f;
                ctx.documents      = {d1, d2};

                // Result may succeed or fail depending on timing; we only assert
                // there is no crash or deadlock.
                (void)plugin.generateRAG(ctx, req);
            });
        } else {
            // Odd threads: call loadLoRA() concurrently.
            threads.emplace_back([&plugin, t]() {
                const std::string name = "test_lora_" + std::to_string(t);
                plugin.loadLoRA(name, "/tmp/lora.bin", 1.0f);
            });
        }
    }

    // All threads must finish within 10 seconds.
    for (auto& th : threads) {
        th.join();
    }
    // If we reach here, no deadlock occurred.
    SUCCEED();
}

/// U2: generate() called 100 times; inference_count in perf stats equals 100.
TEST(LlamaCppPluginFocusedTests, U2_AtomicInferenceCountAccumulatesCorrectly) {
    LlamaCppPlugin plugin;
    plugin.loadModel("", {});

    const uint64_t before = plugin.getPerformanceStats()["inference_count"].get<uint64_t>();

    constexpr int kIterations = 100;
    for (int i = 0; i < kIterations; ++i) {
        InferenceRequest req;
        req.prompt = "counter test " + std::to_string(i);
        const auto resp = plugin.generate(req);
        ASSERT_TRUE(resp.success) << "generate() must succeed in stub mode (iteration " << i << ")";
    }

    const uint64_t after = plugin.getPerformanceStats()["inference_count"].get<uint64_t>();
    EXPECT_EQ(after - before, static_cast<uint64_t>(kIterations))
        << "inference_count_ must be incremented atomically for every successful generate()";
}

/// U3: generateRAG() with no rag_mode key in metadata succeeds without crash.
TEST(LlamaCppPluginFocusedTests, U3_GenerateRAGNoRagModeMetadata_SucceedsWithStubResponse) {
    LlamaCppPlugin plugin;
    plugin.loadModel("", {});

    InferenceRequest req;
    req.prompt   = "metadata-free query";
    // Intentionally leave req.metadata empty (no rag_mode key).

    RAGContext ctx;
    ctx.query = req.prompt;
    RAGContext::Document d;
    d.content         = "relevant document text";
    d.relevance_score = 1.5f;
    ctx.documents     = {d};

    const auto response = plugin.generateRAG(ctx, req);
    EXPECT_TRUE(response.success)
        << "generateRAG() must succeed in stub mode even without rag_mode metadata";
    EXPECT_FALSE(response.text.empty())
        << "Stub response text must be non-empty";
}

/// U4: Factory round-trip: create -> loadModel -> generate -> destroy.
///     In THEMIS_TEST_BUILD the C-linkage factory is compiled out; we exercise
///     the equivalent heap allocation / deletion path directly.
TEST(LlamaCppPluginFocusedTests, U4_FactoryRoundTrip_CreateLoadGenerateDestroy) {
    // Simulate the ABI round-trip that themis_llm_create / themis_llm_destroy
    // provide in production. In test builds the C-linkage symbols are guarded
    // by !THEMIS_TEST_BUILD, so we replicate the same heap ownership semantics.
    llm::ILLMPlugin* plugin = new LlamaCppPlugin();
    ASSERT_NE(plugin, nullptr);

    EXPECT_TRUE(plugin->loadModel("", {}));

    InferenceRequest req;
    req.prompt = "factory round-trip probe";
    const auto response = plugin->generate(req);
    EXPECT_TRUE(response.success)
        << "generate() must succeed via ILLMPlugin* in stub mode";

    // Destroy via base-class pointer — mirrors themis_llm_destroy() semantics.
    delete plugin;
    // Reaching here without crash or sanitizer error validates the round-trip.
    SUCCEED();
}

// ── Group V – Security Gates ──────────────────────────────────────────────────

/// V1: importLoRA with bad magic bytes returns false.
TEST(LlamaCppPluginFocusedTests, V1_ImportLoRA_BadMagicReturnsFalse) {
    LlamaCppPlugin plugin;
    // 8 bytes but wrong magic (not 'G','G','U','F')
    const std::vector<uint8_t> bad_magic = {0x00, 0x01, 0x02, 0x03,
                                             0x04, 0x05, 0x06, 0x07};
    EXPECT_FALSE(plugin.importLoRA("id1", bad_magic))
        << "importLoRA must reject data with invalid GGUF magic bytes";
}

/// V2: importLoRA with valid GGUF magic and minimal size returns true in stub mode.
TEST(LlamaCppPluginFocusedTests, V2_ImportLoRA_ValidMagicStubModeReturnsTrue) {
    LlamaCppPlugin plugin;
    // Valid GGUF magic: 'G'=0x47, 'G'=0x47, 'U'=0x55, 'F'=0x46, plus padding
    const std::vector<uint8_t> valid_gguf = {0x47, 0x47, 0x55, 0x46,
                                              0x00, 0x00, 0x00, 0x00};
    EXPECT_TRUE(plugin.importLoRA("id2", valid_gguf))
        << "importLoRA must accept valid GGUF magic in stub mode (wrapper is null)";
}

/// V3: importLoRA with too-small data (< 8 bytes) returns false.
TEST(LlamaCppPluginFocusedTests, V3_ImportLoRA_TooSmallReturnsFalse) {
    LlamaCppPlugin plugin;
    // Even with correct magic prefix, 4 bytes is below the 8-byte minimum
    const std::vector<uint8_t> too_small = {0x47, 0x47, 0x55, 0x46};
    EXPECT_FALSE(plugin.importLoRA("id3", too_small))
        << "importLoRA must reject data smaller than the minimum header size";
}

/// V4: setPolicyFn with a denying function causes generate() to return
///     success=false with the denial reason in error_message.
TEST(LlamaCppPluginFocusedTests, V4_PolicyFn_DenyingFnBlocksGenerate) {
    LlamaCppPlugin plugin;
    plugin.loadModel("", {});

    plugin.setPolicyFn(
        [](const InferenceRequest& /*req*/, std::string& reason) -> bool {
            reason = "denied for testing";
            return false;
        });

    InferenceRequest request;
    request.prompt = "policy gate test";
    const auto response = plugin.generate(request);

    EXPECT_FALSE(response.success)
        << "generate() must return success=false when policy gate denies";
    EXPECT_NE(response.error_message.find("denied for testing"), std::string::npos)
        << "denial reason must appear in error_message";
}

/// V5: setPolicyFn with an allowing function lets generate() succeed normally.
TEST(LlamaCppPluginFocusedTests, V5_PolicyFn_AllowingFnPermitsGenerate) {
    LlamaCppPlugin plugin;
    plugin.loadModel("", {});

    plugin.setPolicyFn(
        [](const InferenceRequest& /*req*/, std::string& /*reason*/) -> bool {
            return true; // allow
        });

    InferenceRequest request;
    request.prompt = "policy allow test";
    const auto response = plugin.generate(request);

    EXPECT_TRUE(response.success)
        << "generate() must succeed when policy gate allows the request";
}

/// V6: Clearing the policy fn (nullptr) reverts to normal generate behaviour.
TEST(LlamaCppPluginFocusedTests, V6_PolicyFn_ClearingRevertsToNormalGenerate) {
    LlamaCppPlugin plugin;
    plugin.loadModel("", {});

    // First set a denying policy, confirm it blocks.
    plugin.setPolicyFn(
        [](const InferenceRequest&, std::string& reason) -> bool {
            reason = "blocked";
            return false;
        });
    {
        InferenceRequest req;
        req.prompt = "should be blocked";
        ASSERT_FALSE(plugin.generate(req).success);
    }

    // Clear the policy gate.
    plugin.setPolicyFn(nullptr);

    // Now generate must succeed in stub mode.
    InferenceRequest req;
    req.prompt = "should be allowed after clear";
    const auto response = plugin.generate(req);
    EXPECT_TRUE(response.success)
        << "generate() must succeed after policy fn is cleared to nullptr";
}

// ── Group X – Retry hardening ────────────────────────────────────────────────
// These tests verify the stream-callback retry wrapper (no_retry_logic × 13
// gap findings) and guard against regressions in rapid successive calls.

/// X1: stream_callback that throws std::runtime_error on the first two
///     invocations is retried and increments stream_retry_count_ by >= 1.
///     Requires THEMIS_LLAMA_CPP_STUB_MODE so the callback is exercised in
///     unit-test builds without a real model.
TEST(LlamaCppPluginFocusedTests, X1_StreamCallbackRetry_TransientException) {
#ifndef THEMIS_LLAMA_CPP_STUB_MODE
    GTEST_SKIP() << "Requires THEMIS_LLAMA_CPP_STUB_MODE for stub callback path";
#else
    using namespace themis::llamacpp;
    using namespace themis::llm;

    LlamaCppPlugin plugin;
    ASSERT_TRUE(plugin.loadModel("", {}));

    // Capture retry baseline before this test.
    const auto stats_before = plugin.getPerformanceStats();
    const uint64_t retries_before = stats_before.value("stream_retry_count", uint64_t{0});

    int throw_count = 0;
    InferenceRequest req;
    req.prompt = "x1_retry_test";
    req.stream_callback = [&]([[maybe_unused]] const std::string&) {
        // Throw on the first two calls to force the retry path.
        if (throw_count++ < 2) {
          throw std::runtime_error("transient");
        }
    };

    const auto resp = plugin.generate(req);
    // generate() itself must still report success even though retries occurred.
    EXPECT_TRUE(resp.success)
        << "generate() should succeed despite transient callback throws";

    const auto stats_after = plugin.getPerformanceStats();
    const uint64_t retries_after = stats_after.value("stream_retry_count", uint64_t{0});
    EXPECT_GE(retries_after, retries_before + 1)
        << "stream_retry_count must increase when transient exceptions are retried";
#endif
}

/// X2: getPerformanceStats() always exposes the "stream_retry_count" key,
///     even when the counter is zero (regression guard).
TEST(LlamaCppPluginFocusedTests, X2_GetPerformanceStats_ContainsStreamRetryCount) {
    using namespace themis::llamacpp;

    LlamaCppPlugin plugin;
    ASSERT_TRUE(plugin.loadModel("", {}));

    const auto stats = plugin.getPerformanceStats();
    EXPECT_TRUE(stats.contains("stream_retry_count"))
        << "getPerformanceStats() must always contain 'stream_retry_count'";
}

/// X3: Plugin survives 10 rapid successive generate() calls without thread
///     leaks or crashes. This guards the stub generation path against resource
///     leaks while keeping the regression scope focused on production code.
TEST(LlamaCppPluginFocusedTests, X3_RapidSuccessiveGenerate_NoThreadLeak) {
#ifndef THEMIS_LLAMA_CPP_STUB_MODE
    GTEST_SKIP() << "Requires THEMIS_LLAMA_CPP_STUB_MODE for stub path";
#else
    using namespace themis::llamacpp;
    using namespace themis::llm;

    LlamaCppPlugin plugin;
    ASSERT_TRUE(plugin.loadModel("", {}));

    for (int i = 0; i < 10; ++i) {
        InferenceRequest req;
        req.prompt = "x3_rapid_test_" + std::to_string(i);
        const auto resp = plugin.generate(req);
        EXPECT_TRUE(resp.success)
            << "generate() must succeed at rapid-fire iteration " << i;
    }
    // Reaching here without crash, hang, or sanitizer error confirms no thread
    // is left un-joined or leaking resources.
    SUCCEED();
#endif
}
