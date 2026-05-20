/*
 * ThemisDB | File: test_plugin_registrar_fail_closed.cpp | Version: 0.0.1
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include <gtest/gtest.h>

#include "llama_cpp/llama_cpp_registrar.h"
#include "stable_diffusion/sd_plugin_registrar.h"
#include "whisper/whisper_plugin_registrar.h"

namespace {

using themis::imggen::SDPlugin;
using themis::imggen::SDPluginAdapter;
using themis::imggen::SDPluginRegistrar;
using themis::llamacpp::LlamaCppPlugin;
using themis::llamacpp::LlamaCppPluginRegistrar;
using themis::whisper::WhisperPlugin;
using themis::whisper::WhisperPluginAdapter;
using themis::whisper::WhisperPluginRegistrar;

TEST(PluginRegistrarFailClosedTest, StableDiffusionInitializeFailsWithoutModelPath) {
    SDPluginAdapter adapter(std::make_unique<SDPlugin>());
    EXPECT_FALSE(adapter.initialize(nullptr));
    EXPECT_FALSE(adapter.initialize(""));
    EXPECT_FALSE(adapter.initialize("{}"));
    EXPECT_FALSE(adapter.initialize(R"({"model_path":""})"));
}

TEST(PluginRegistrarFailClosedTest, StableDiffusionReloadFailsWithoutModelPath) {
    SDPlugin plugin;
    auto reload = SDPluginRegistrar::defaultReloadCallback();
    EXPECT_FALSE(reload(plugin, nlohmann::json::object()));
    EXPECT_FALSE(reload(plugin, nlohmann::json{{"model_path", ""}}));
}

TEST(PluginRegistrarFailClosedTest, WhisperInitializeFailsWithoutModelPath) {
    WhisperPluginAdapter adapter(std::make_unique<WhisperPlugin>());
    EXPECT_FALSE(adapter.initialize(nullptr));
    EXPECT_FALSE(adapter.initialize(""));
    EXPECT_FALSE(adapter.initialize("{}"));
    EXPECT_FALSE(adapter.initialize(R"({"model_path":""})"));
}

TEST(PluginRegistrarFailClosedTest, WhisperReloadFailsWithoutModelPath) {
    WhisperPlugin plugin;
    auto reload = WhisperPluginRegistrar::defaultReloadCallback();
    EXPECT_FALSE(reload(plugin, nlohmann::json::object()));
    EXPECT_FALSE(reload(plugin, nlohmann::json{{"model_path", ""}}));
}

TEST(PluginRegistrarFailClosedTest, LlamaReloadFailsWithoutModelPath) {
    LlamaCppPlugin plugin;
    auto reload = LlamaCppPluginRegistrar::defaultReloadCallback();
    EXPECT_FALSE(reload(plugin, nlohmann::json::object()));
    EXPECT_FALSE(reload(plugin, nlohmann::json{{"model_path", ""}}));
}

} // namespace
