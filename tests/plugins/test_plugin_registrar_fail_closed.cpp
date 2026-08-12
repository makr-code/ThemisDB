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

TEST(PluginRegistrarFailClosedTest, LlamaDraftTokensCapsOversizedVocabHintInFallback) {
    LlamaCppPlugin plugin;
    themis::llm::InferenceRequest req;
    req.prompt = "x";

    constexpr size_t kHugeHint = 1000000000u;
    constexpr size_t kExpectedCappedVocab = 65536u;

    const auto result = plugin.generateDraftTokens(req, 1, kHugeHint);
    ASSERT_EQ(result.tokens.size(), 1u);
    ASSERT_EQ(result.logits.size(), 1u);
    EXPECT_EQ(result.vocab_size, kExpectedCappedVocab);
    EXPECT_EQ(result.logits[0].size(), kExpectedCappedVocab);
    EXPECT_LT(static_cast<size_t>(result.tokens[0]), result.vocab_size);
}

TEST(PluginRegistrarFailClosedTest, LlamaDraftTokensZeroKReturnsEmptyResult) {
    // k=0 is a valid (no-op) call: caller should receive an empty token/logit
    // list and a valid (non-zero) vocab_size without any allocation of logit rows.
    LlamaCppPlugin plugin;
    themis::llm::InferenceRequest req;
    req.prompt = "hello";

    const auto result = plugin.generateDraftTokens(req, 0, 1024);
    EXPECT_TRUE(result.tokens.empty());
    EXPECT_TRUE(result.logits.empty());
    EXPECT_GT(result.vocab_size, 0u);
}

} // namespace
