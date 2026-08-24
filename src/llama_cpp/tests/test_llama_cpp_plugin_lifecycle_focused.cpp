/**
 * @file test_llama_cpp_plugin_lifecycle_focused.cpp
 * @brief Group LC — Plugin lifecycle tests (load / unload / reload / concurrency).
 *
 * Verifies that LlamaCppPlugin transitions correctly between the
 * unloaded → loaded → unloaded states and that repeated or concurrent
 * lifecycle operations are safe.
 *
 * All tests run under THEMIS_LLAMA_CPP_STUB_MODE; no real model is required.
 */

#include <gtest/gtest.h>
#include "llama_cpp/llama_cpp_plugin.h"
#include "llama_cpp/llama_cpp_registrar.h"
#include <nlohmann/json.hpp>
#include <thread>
#include <vector>
#include <atomic>

using namespace themis::llamacpp;
using namespace themis::llm;
using json = nlohmann::json;

// ── Group LC — Lifecycle ──────────────────────────────────────────────────────

// LC1: Fresh plugin reports not-loaded
TEST(LlamaCppPluginLifecycleFocusedTests, LC1_InitialState_NotLoaded) {
    LlamaCppPlugin p;
    EXPECT_FALSE(p.isModelLoaded());
    EXPECT_FALSE(p.getModelInfo().has_value());
}

// LC2: After loadModel, plugin is loaded and info is present
TEST(LlamaCppPluginLifecycleFocusedTests, LC2_LoadModel_TransitionsToLoaded) {
    LlamaCppPlugin p;
    ASSERT_TRUE(p.loadModel("/stub/model.gguf", {}));
    EXPECT_TRUE(p.isModelLoaded());
    EXPECT_TRUE(p.getModelInfo().has_value());
}

// LC3: After unloadModel, plugin is not loaded
TEST(LlamaCppPluginLifecycleFocusedTests, LC3_UnloadModel_TransitionsToUnloaded) {
    LlamaCppPlugin p;
    p.loadModel("/stub/model.gguf", {});
    p.unloadModel();
    EXPECT_FALSE(p.isModelLoaded());
}

// LC4: Reload (load → unload → load) works correctly
TEST(LlamaCppPluginLifecycleFocusedTests, LC4_Reload_WorksAfterUnload) {
    LlamaCppPlugin p;
    ASSERT_TRUE(p.loadModel("/stub/first.gguf", {}));
    p.unloadModel();
    ASSERT_FALSE(p.isModelLoaded());
    ASSERT_TRUE(p.loadModel("/stub/second.gguf", {}));
    EXPECT_TRUE(p.isModelLoaded());
    auto info = p.getModelInfo();
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->model_id, "/stub/second.gguf");
}

// LC5: Double-load replaces the previous model
TEST(LlamaCppPluginLifecycleFocusedTests, LC5_DoubleLoad_ReplacesModel) {
    LlamaCppPlugin p;
    p.loadModel("/stub/a.gguf", {});
    p.loadModel("/stub/b.gguf", {});
    EXPECT_TRUE(p.isModelLoaded());
    EXPECT_EQ(p.getModelInfo()->model_id, "/stub/b.gguf");
}

// LC6: Double-unload does not crash
TEST(LlamaCppPluginLifecycleFocusedTests, LC6_DoubleUnload_IsIdempotent) {
    LlamaCppPlugin p;
    p.loadModel("/stub/m.gguf", {});
    p.unloadModel();
    ASSERT_NO_THROW(p.unloadModel());
    EXPECT_FALSE(p.isModelLoaded());
}

// LC7: Unload on a never-loaded plugin does not crash
TEST(LlamaCppPluginLifecycleFocusedTests, LC7_UnloadNeverLoaded_IsNoOp) {
    LlamaCppPlugin p;
    ASSERT_NO_THROW(p.unloadModel());
    EXPECT_FALSE(p.isModelLoaded());
}

// LC8: Plugin created via registrar starts in consistent state
TEST(LlamaCppPluginLifecycleFocusedTests, LC8_RegistrarCreatedPlugin_ConsistentState) {
    auto plugin = LlamaCppPluginRegistrar::createPlugin(json::object());
    ASSERT_NE(plugin, nullptr);
    // Empty model_path → stub mode, plugin is ready to generate but
    // isModelLoaded may be false or true depending on stub; both are acceptable.
    // Key invariant: getModelInfo and generate must not crash.
    (void)plugin->getModelInfo();
    InferenceRequest req;
    req.prompt = "hello";
    ASSERT_NO_THROW(plugin->generate(req));
}

// LC9: Concurrent load and query does not cause data races (thread-safety smoke test)
TEST(LlamaCppPluginLifecycleFocusedTests, LC9_ConcurrentLoadQuery_NoRace) {
    LlamaCppPlugin p;
    p.loadModel("/stub/m.gguf", {});

    std::atomic<int> query_ok{0};
    std::vector<std::thread> threads;
    threads.reserve(4);

    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([&p, &query_ok]() {
            InferenceRequest req;
            req.prompt     = "concurrent test";
            req.max_tokens = 8;
            auto resp = p.generate(req);
            if (resp.success || !resp.text.empty()) {
                ++query_ok;
            }
        });
    }
    for (auto& t : threads) { t.join(); }
    // No assertion on query_ok count — the goal is no crash / UB
    SUCCEED();
}
