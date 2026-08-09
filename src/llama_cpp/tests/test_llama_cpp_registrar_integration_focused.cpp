/**
 * @file test_llama_cpp_registrar_integration_focused.cpp
 * @brief Group W — Server-startup registrar integration tests.
 *
 * Validates LlamaCppPluginRegistrar::initFromServerConfig() and related
 * helpers that wire the LLM plugin subsystem to the server startup path.
 *
 * All tests run under THEMIS_LLAMA_CPP_STUB_MODE so no real model file is
 * required.
 */

#include <gtest/gtest.h>
#include "llama_cpp/llama_cpp_registrar.h"
#include "llama_cpp/llama_cpp_plugin.h"
#include "llm/llm_plugin_manager.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace themis::llamacpp;
using namespace themis::llm;

// ── Group W — Server-startup registrar integration ────────────────────────────

// W1: No "llm" key → no-op, must return true
TEST(LlamaCppRegistrarIntegrationTests, W1_InitFromServerConfig_NoLLMSection) {
    json config = {{"database", {{"path", "/tmp/db"}}}};
    EXPECT_TRUE(LlamaCppPluginRegistrar::initFromServerConfig(config));
}

// W2: Empty model_path → stub mode, must return true
TEST(LlamaCppRegistrarIntegrationTests, W2_InitFromServerConfig_EmptyModelPath) {
    json config = {{"llm", {{"model_path", ""}}}};
    EXPECT_TRUE(LlamaCppPluginRegistrar::initFromServerConfig(config));
}

// W3: "llm" section present but no "model_path" key → no-op, must return true
TEST(LlamaCppRegistrarIntegrationTests, W3_InitFromServerConfig_NoModelPathKey) {
    json config = {{"llm", {{"n_ctx", 4096}}}};
    EXPECT_TRUE(LlamaCppPluginRegistrar::initFromServerConfig(config));
}

// W4: defaultReloadCallback with empty config → stub mode returns true
TEST(LlamaCppRegistrarIntegrationTests, W4_DefaultReloadCallback_EmptyConfig) {
    auto cb = LlamaCppPluginRegistrar::defaultReloadCallback();
    LlamaCppPlugin plugin;
    EXPECT_TRUE(cb(plugin, json::object()));
}

// W5: registerWithLLMManager in stub mode → must return true
TEST(LlamaCppRegistrarIntegrationTests, W5_RegisterWithLLMManager_StubMode) {
    auto& mgr = LLMPluginManager::instance();
    bool ok = LlamaCppPluginRegistrar::registerWithLLMManager(
        mgr, "llama_cpp_test_w5", json::object());
    EXPECT_TRUE(ok);
}

// W6: Plugin created by the registrar can respond (stub mode)
TEST(LlamaCppRegistrarIntegrationTests, W6_RegisteredPlugin_CanGenerate) {
    auto plugin = LlamaCppPluginRegistrar::createPlugin(json::object());
    ASSERT_NE(plugin, nullptr);
    plugin->loadModel("", json::object());

    InferenceRequest req;
    req.prompt = "hello from W6";
    auto resp = plugin->generate(req);
    // In stub mode the plugin must either succeed or report an error message —
    // it must not throw or crash.
    EXPECT_TRUE(resp.success || !resp.error_message.empty());
}

// W7: initFromServerConfig with a non-empty model_path registers the plugin
TEST(LlamaCppRegistrarIntegrationTests, W7_InitFromServerConfig_WithModelPath) {
    json config = {{"llm", {{"model_path", "/stub/model.gguf"}, {"n_ctx", 512}}}};
    // In THEMIS_LLAMA_CPP_STUB_MODE loadModel() always succeeds, so this
    // must return true even though the path is fake.
    EXPECT_TRUE(LlamaCppPluginRegistrar::initFromServerConfig(config));
}

// W8: initFromServerConfig is idempotent (calling twice is safe)
TEST(LlamaCppRegistrarIntegrationTests, W8_InitFromServerConfig_Idempotent) {
    json config = {{"llm", {{"model_path", ""}}}};
    EXPECT_TRUE(LlamaCppPluginRegistrar::initFromServerConfig(config));
    EXPECT_TRUE(LlamaCppPluginRegistrar::initFromServerConfig(config));
}
