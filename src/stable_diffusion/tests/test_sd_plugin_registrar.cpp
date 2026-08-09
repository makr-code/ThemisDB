/**
 * @file test_sd_plugin_registrar.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 95/100
 * @note Gap Summary: total=13; TODO=1, Stub=11, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include <gtest/gtest.h>
#include "stable_diffusion/sd_plugin_registrar.h"
#include "stable_diffusion/sd_plugin.h"
#include "plugins/plugin_interface.h"
#include <nlohmann/json.hpp>
#include <string>

using namespace themis::imggen;
using namespace themis::plugins;
using json = nlohmann::json;

// ── Group A – SDPluginRegistrar::createPlugin ─────────────────────────────────

TEST(SDPluginRegistrarTests, A1_CreatePluginStubMode) {
    auto plugin = SDPluginRegistrar::createPlugin({});
    ASSERT_NE(plugin, nullptr);
    // Stub mode: no model_path in config → plugin not initialized
    EXPECT_FALSE(plugin->isInitialized());
}

TEST(SDPluginRegistrarTests, A2_CreatePluginEmptyModelPath) {
    json cfg; cfg["model_path"] = "";
    auto plugin = SDPluginRegistrar::createPlugin(cfg);
    ASSERT_NE(plugin, nullptr);
    // Empty path is treated as stub mode
    EXPECT_FALSE(plugin->isInitialized());
}

TEST(SDPluginRegistrarTests, A3_CreatePluginWithModelPath) {
    json cfg; cfg["model_path"] = "/stub/model.ckpt";
    auto plugin = SDPluginRegistrar::createPlugin(cfg);
    ASSERT_NE(plugin, nullptr);
    // SDStubGenerator always succeeds on initialize
    EXPECT_TRUE(plugin->isInitialized());
}

// ── Group B – SDPluginRegistrar::createAdapter ────────────────────────────────

TEST(SDPluginRegistrarTests, B1_CreateAdapterNotNull) {
    auto adapter = SDPluginRegistrar::createAdapter({});
    ASSERT_NE(adapter, nullptr);
}

TEST(SDPluginRegistrarTests, B2_AdapterTypeIsImageGeneration) {
    auto adapter = SDPluginRegistrar::createAdapter({});
    ASSERT_NE(adapter, nullptr);
    EXPECT_EQ(adapter->getType(), PluginType::IMAGE_GENERATION);
}

TEST(SDPluginRegistrarTests, B3_AdapterGetInstanceReturnsSDPlugin) {
    auto adapter = SDPluginRegistrar::createAdapter({});
    ASSERT_NE(adapter, nullptr);
    void* instance = adapter->getInstance();
    ASSERT_NE(instance, nullptr);
    auto* plugin = static_cast<SDPlugin*>(instance);
    EXPECT_NE(plugin, nullptr);
}

// ── Group C – SDPluginAdapter IThemisPlugin ───────────────────────────────────

TEST(SDPluginRegistrarTests, C1_AdapterNameAndVersion) {
    auto adapter = SDPluginRegistrar::createAdapter({});
    ASSERT_NE(adapter, nullptr);
    EXPECT_STREQ(adapter->getName(), "stable_diffusion");
    EXPECT_STREQ(adapter->getVersion(), "2.3.0");
}

TEST(SDPluginRegistrarTests, C2_AdapterCapabilitiesBatchingAndThreadSafe) {
    auto adapter = SDPluginRegistrar::createAdapter({});
    ASSERT_NE(adapter, nullptr);
    const auto caps = adapter->getCapabilities();
    EXPECT_TRUE(caps.supports_batching);
    EXPECT_TRUE(caps.thread_safe);
    EXPECT_FALSE(caps.supports_streaming);
}

TEST(SDPluginRegistrarTests, C3_AdapterInitializeAndShutdownCycle) {
    auto adapter = SDPluginRegistrar::createAdapter({});
    ASSERT_NE(adapter, nullptr);

    // initialize with a stub model path
    json cfg; cfg["model_path"] = "/stub/sd.ckpt";
    EXPECT_TRUE(adapter->initialize(cfg.dump().c_str()));

    // after initialization the underlying plugin should be initialized
    auto* plugin = static_cast<SDPlugin*>(adapter->getInstance());
    ASSERT_NE(plugin, nullptr);
    EXPECT_TRUE(plugin->isInitialized());

    // shutdown resets state
    adapter->shutdown();
    plugin = static_cast<SDPlugin*>(adapter->getInstance());
    ASSERT_NE(plugin, nullptr);
    EXPECT_FALSE(plugin->isInitialized());
}

// ── Group D – SDPluginRegistrar::defaultReloadCallback ───────────────────────

TEST(SDPluginRegistrarTests, D1_DefaultReloadCallbackStubMode) {
    auto cb = SDPluginRegistrar::defaultReloadCallback();
    SDPlugin plugin;
    // No model_path → stub success, no initialization
    EXPECT_TRUE(cb(plugin, {}));
    EXPECT_FALSE(plugin.isInitialized());
}

TEST(SDPluginRegistrarTests, D2_DefaultReloadCallbackWithPath) {
    auto cb = SDPluginRegistrar::defaultReloadCallback();
    SDPlugin plugin;
    json cfg; cfg["model_path"] = "/stub/reload.ckpt";
    EXPECT_TRUE(cb(plugin, cfg));
    EXPECT_TRUE(plugin.isInitialized());
}

TEST(SDPluginRegistrarTests, D3_DefaultReloadCallbackEmptyPath) {
    auto cb = SDPluginRegistrar::defaultReloadCallback();
    SDPlugin plugin;
    json cfg; cfg["model_path"] = "";
    // Empty path → stub mode, no initialization
    EXPECT_TRUE(cb(plugin, cfg));
    EXPECT_FALSE(plugin.isInitialized());
}
