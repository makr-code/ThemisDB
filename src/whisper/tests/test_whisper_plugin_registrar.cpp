/**
 * @file test_whisper_plugin_registrar.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 95/100
 * @note Gap Summary: total=13; TODO=1, Stub=11, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include <gtest/gtest.h>
#include "whisper/whisper_plugin_registrar.h"
#include "whisper/whisper_plugin.h"
#include "plugins/plugin_interface.h"
#include <nlohmann/json.hpp>
#include <string>

using namespace themis::whisper;
using namespace themis::plugins;
using json = nlohmann::json;

// ── Group A – WhisperPluginRegistrar::createPlugin ────────────────────────────

TEST(WhisperPluginRegistrarTests, A1_CreatePluginStubMode) {
    auto plugin = WhisperPluginRegistrar::createPlugin({});
    ASSERT_NE(plugin, nullptr);
    // Stub mode: no model_path → plugin not initialized
    EXPECT_FALSE(plugin->isInitialized());
}

TEST(WhisperPluginRegistrarTests, A2_CreatePluginEmptyModelPath) {
    json cfg; cfg["model_path"] = "";
    auto plugin = WhisperPluginRegistrar::createPlugin(cfg);
    ASSERT_NE(plugin, nullptr);
    // Empty path is treated as stub mode
    EXPECT_FALSE(plugin->isInitialized());
}

TEST(WhisperPluginRegistrarTests, A3_CreatePluginWithModelPath) {
    json cfg; cfg["model_path"] = "/stub/ggml-base.bin";
    auto plugin = WhisperPluginRegistrar::createPlugin(cfg);
    ASSERT_NE(plugin, nullptr);
    // WhisperStubTranscriber always succeeds on initialize
    EXPECT_TRUE(plugin->isInitialized());
}

// ── Group B – WhisperPluginRegistrar::createAdapter ──────────────────────────

TEST(WhisperPluginRegistrarTests, B1_CreateAdapterNotNull) {
    auto adapter = WhisperPluginRegistrar::createAdapter({});
    ASSERT_NE(adapter, nullptr);
}

TEST(WhisperPluginRegistrarTests, B2_AdapterTypeIsAudioProcessing) {
    auto adapter = WhisperPluginRegistrar::createAdapter({});
    ASSERT_NE(adapter, nullptr);
    EXPECT_EQ(adapter->getType(), PluginType::AUDIO_PROCESSING);
}

TEST(WhisperPluginRegistrarTests, B3_AdapterGetInstanceReturnsWhisperPlugin) {
    auto adapter = WhisperPluginRegistrar::createAdapter({});
    ASSERT_NE(adapter, nullptr);
    void* instance = adapter->getInstance();
    ASSERT_NE(instance, nullptr);
    auto* plugin = static_cast<WhisperPlugin*>(instance);
    EXPECT_NE(plugin, nullptr);
}

// ── Group C – WhisperPluginAdapter IThemisPlugin ─────────────────────────────

TEST(WhisperPluginRegistrarTests, C1_AdapterNameAndVersion) {
    auto adapter = WhisperPluginRegistrar::createAdapter({});
    ASSERT_NE(adapter, nullptr);
    EXPECT_STREQ(adapter->getName(), "whisper");
    EXPECT_STREQ(adapter->getVersion(), "2.3.0");
}

TEST(WhisperPluginRegistrarTests, C2_AdapterCapabilitiesThreadSafe) {
    auto adapter = WhisperPluginRegistrar::createAdapter({});
    ASSERT_NE(adapter, nullptr);
    const auto caps = adapter->getCapabilities();
    EXPECT_TRUE(caps.thread_safe);
    EXPECT_TRUE(caps.supports_streaming);
    EXPECT_FALSE(caps.supports_batching);
}

TEST(WhisperPluginRegistrarTests, C3_AdapterInitializeAndShutdownCycle) {
    auto adapter = WhisperPluginRegistrar::createAdapter({});
    ASSERT_NE(adapter, nullptr);

    // initialize with a stub model path
    json cfg; cfg["model_path"] = "/stub/ggml-base.bin";
    EXPECT_TRUE(adapter->initialize(cfg.dump().c_str()));

    // after initialization the underlying plugin should be initialized
    auto* plugin = static_cast<WhisperPlugin*>(adapter->getInstance());
    ASSERT_NE(plugin, nullptr);
    EXPECT_TRUE(plugin->isInitialized());

    // shutdown resets state
    adapter->shutdown();
    plugin = static_cast<WhisperPlugin*>(adapter->getInstance());
    ASSERT_NE(plugin, nullptr);
    EXPECT_FALSE(plugin->isInitialized());
}

// ── Group D – WhisperPluginRegistrar::defaultReloadCallback ──────────────────

TEST(WhisperPluginRegistrarTests, D1_DefaultReloadCallbackStubMode) {
    auto cb = WhisperPluginRegistrar::defaultReloadCallback();
    WhisperPlugin plugin;
    // No model_path → stub success, plugin not initialized
    EXPECT_TRUE(cb(plugin, {}));
    EXPECT_FALSE(plugin.isInitialized());
}

TEST(WhisperPluginRegistrarTests, D2_DefaultReloadCallbackWithPath) {
    auto cb = WhisperPluginRegistrar::defaultReloadCallback();
    WhisperPlugin plugin;
    json cfg; cfg["model_path"] = "/stub/ggml-small.bin";
    EXPECT_TRUE(cb(plugin, cfg));
    EXPECT_TRUE(plugin.isInitialized());
}

TEST(WhisperPluginRegistrarTests, D3_DefaultReloadCallbackEmptyPath) {
    auto cb = WhisperPluginRegistrar::defaultReloadCallback();
    WhisperPlugin plugin;
    json cfg; cfg["model_path"] = "";
    // Empty path → stub mode, no initialization
    EXPECT_TRUE(cb(plugin, cfg));
    EXPECT_FALSE(plugin.isInitialized());
}
