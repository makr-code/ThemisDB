/**
 * @file test_sd_plugin.cpp
 * @brief Unit tests for the Stable Diffusion image generation plugin
 *
 * Test suite: SDPluginFocusedTests (30 tests)
 *   Group A (3)  – SDConfig: fromJson defaults, custom, clamping
 *   Group B (3)  – SDConfig: toJson round-trip, keys present, float round-trip
 *   Group C (3)  – SDPromptSanitizer: isAllowed, blocked keyword, case-insensitive
 *   Group D (3)  – SDPromptSanitizer: sanitize removes keyword, multiple, empty list
 *   Group E (3)  – InMemorySDGenerator: initialize, generate returns preset
 *   Group F (3)  – SDPlugin injection ctor: initialize, generate, isPromptAllowed
 *   Group G (3)  – SDPlugin: provenance fields (timestamp, prompt_hash, plugin_version)
 *   Group H (3)  – SDPlugin: content-policy blocks generate, blocked_count increments
 *   Group I (3)  – SDPlugin: getStatistics keys, plugin name, version
 *   Group J (3)  – SDPlugin: error paths (uninit, generator throws, double-init safe)
 */

#include <gtest/gtest.h>
#include "stable_diffusion/sd_config.h"
#include "stable_diffusion/sd_prompt_sanitizer.h"
#include "stable_diffusion/sd_generator.h"
#include "stable_diffusion/sd_plugin.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <stdexcept>

using namespace themis::imggen;
using json = nlohmann::json;

// ── helpers ───────────────────────────────────────────────────────────────────

class ThrowingGenerator : public ISDGenerator {
public:
    bool initialize(const SDConfig& cfg) override {
        model_id_ = cfg.model_path; initialized_ = true; return true;
    }
    bool isInitialized() const override { return initialized_; }
    std::vector<uint8_t> generate(const std::string&, const SDGenerationConfig&,
                                   int& w, int& h, uint64_t& seed) override {
        throw std::runtime_error("generator error");
    }
    std::string getModelId() const override { return model_id_; }
private:
    bool initialized_ = false;
    std::string model_id_;
};

// ── Group A – SDConfig::fromJson ──────────────────────────────────────────────

TEST(SDPluginFocusedTests, A1_FromJsonDefaults) {
    const SDConfig cfg = SDConfig::fromJson({});
    EXPECT_EQ(cfg.width,  512);
    EXPECT_EQ(cfg.height, 512);
    EXPECT_EQ(cfg.steps,  20);
    EXPECT_FLOAT_EQ(cfg.cfg_scale, 7.0f);
    EXPECT_EQ(cfg.sampler, "euler_a");
}

TEST(SDPluginFocusedTests, A2_FromJsonCustomValues) {
    const json j = {{"model_path","/m.gguf"},{"width",768},{"height",512},
                    {"steps",30},{"cfg_scale",8.5f},{"sampler","dpm++"},{"seed",42}};
    const SDConfig cfg = SDConfig::fromJson(j);
    EXPECT_EQ(cfg.model_path, "/m.gguf");
    EXPECT_EQ(cfg.width, 768);
    EXPECT_EQ(cfg.steps, 30);
    EXPECT_FLOAT_EQ(cfg.cfg_scale, 8.5f);
    EXPECT_EQ(cfg.sampler, "dpm++");
    EXPECT_EQ(cfg.seed, 42);
}

TEST(SDPluginFocusedTests, A3_FromJsonClampsInvalidDimensions) {
    const SDConfig cfg = SDConfig::fromJson({{"width",0},{"height",-1},{"steps",0}});
    EXPECT_GE(cfg.width,  1);
    EXPECT_GE(cfg.height, 1);
    EXPECT_GE(cfg.steps,  1);
}

// ── Group B – SDConfig::toJson round-trip ─────────────────────────────────────

TEST(SDPluginFocusedTests, B1_ToJsonRoundTrip) {
    SDConfig orig;
    orig.model_path = "/models/sd.gguf"; orig.width = 768; orig.height = 512;
    orig.steps = 30; orig.sampler = "dpm++"; orig.seed = 99;
    const SDConfig r = SDConfig::fromJson(orig.toJson());
    EXPECT_EQ(r.model_path, orig.model_path);
    EXPECT_EQ(r.width,  orig.width);
    EXPECT_EQ(r.steps,  orig.steps);
    EXPECT_EQ(r.seed,   orig.seed);
}

TEST(SDPluginFocusedTests, B2_ToJsonContainsAllKeys) {
    const json j = SDConfig{}.toJson();
    EXPECT_TRUE(j.contains("model_path"));
    EXPECT_TRUE(j.contains("width"));
    EXPECT_TRUE(j.contains("height"));
    EXPECT_TRUE(j.contains("steps"));
    EXPECT_TRUE(j.contains("cfg_scale"));
    EXPECT_TRUE(j.contains("sampler"));
    EXPECT_TRUE(j.contains("seed"));
}

TEST(SDPluginFocusedTests, B3_CfgScaleRoundTrip) {
    SDConfig cfg; cfg.cfg_scale = 12.5f;
    EXPECT_FLOAT_EQ(SDConfig::fromJson(cfg.toJson()).cfg_scale, 12.5f);
}

// ── Group C – SDPromptSanitizer::isAllowed ────────────────────────────────────

TEST(SDPluginFocusedTests, C1_AllowsCleanPrompt) {
    SDPromptSanitizer s({"violence"});
    EXPECT_TRUE(s.isAllowed("a beautiful mountain landscape"));
}

TEST(SDPluginFocusedTests, C2_BlocksMatchingKeyword) {
    SDPromptSanitizer s({"explicit"});
    EXPECT_FALSE(s.isAllowed("an explicit image of something"));
}

TEST(SDPluginFocusedTests, C3_CaseInsensitiveBlocking) {
    SDPromptSanitizer s({"nsfw"});
    EXPECT_FALSE(s.isAllowed("This is NSFW content"));
    EXPECT_FALSE(s.isAllowed("this is nsfw content"));
}

// ── Group D – SDPromptSanitizer::sanitize ────────────────────────────────────

TEST(SDPluginFocusedTests, D1_SanitizeRemovesKeyword) {
    SDPromptSanitizer s({"bad"});
    const std::string result = s.sanitize("a bad word here");
    EXPECT_EQ(result.find("bad"), std::string::npos);
}

TEST(SDPluginFocusedTests, D2_SanitizeMultipleOccurrences) {
    SDPromptSanitizer s({"x"});
    const std::string result = s.sanitize("axbxcx");
    EXPECT_EQ(result.find('x'), std::string::npos);
}

TEST(SDPluginFocusedTests, D3_SanitizeEmptyListReturnsUnchanged) {
    SDPromptSanitizer s({});
    EXPECT_EQ(s.sanitize("hello world"), "hello world");
}

// ── Group E – InMemorySDGenerator ────────────────────────────────────────────

TEST(SDPluginFocusedTests, E1_InitializeReturnsTrue) {
    InMemorySDGenerator g;
    EXPECT_FALSE(g.isInitialized());
    EXPECT_TRUE(g.initialize(SDConfig{}));
    EXPECT_TRUE(g.isInitialized());
}

TEST(SDPluginFocusedTests, E2_GenerateReturnsPresetPixels) {
    InMemorySDGenerator g;
    const std::vector<uint8_t> px = {255, 0, 0};
    g.setNextPixels(px, 1, 1, 7);
    g.initialize(SDConfig{});
    int w = 0, h = 0; uint64_t seed = 0;
    const auto result = g.generate("test", SDGenerationConfig{}, w, h, seed);
    EXPECT_EQ(w, 1); EXPECT_EQ(h, 1); EXPECT_EQ(seed, 7u);
    EXPECT_EQ(result, px);
}

TEST(SDPluginFocusedTests, E3_GenerateDefaultDimensions) {
    InMemorySDGenerator g;
    g.initialize(SDConfig{});
    int w = 0, h = 0; uint64_t seed = 0;
    g.generate("test", SDGenerationConfig{}, w, h, seed);
    EXPECT_EQ(w, 512); EXPECT_EQ(h, 512);
}

// ── Group F – SDPlugin injection ctor ────────────────────────────────────────

TEST(SDPluginFocusedTests, F1_InitializeViaDI) {
    SDPlugin p(std::make_unique<InMemorySDGenerator>(), SDPromptSanitizer{});
    EXPECT_FALSE(p.isInitialized());
    EXPECT_TRUE(p.initialize("", {}));
    EXPECT_TRUE(p.isInitialized());
}

TEST(SDPluginFocusedTests, F2_GenerateAfterInit) {
    auto g = std::make_unique<InMemorySDGenerator>();
    g->setNextPixels({0,0,0}, 1, 1);
    SDPlugin p(std::move(g), SDPromptSanitizer{});
    p.initialize("", {});
    const auto img = p.generate("a cat", SDGenerationConfig{});
    EXPECT_TRUE(img.success);
    EXPECT_FALSE(img.png_data.empty());
}

TEST(SDPluginFocusedTests, F3_IsPromptAllowedDelegates) {
    SDPlugin p(std::make_unique<InMemorySDGenerator>(), SDPromptSanitizer({"bad"}));
    EXPECT_TRUE(p.isPromptAllowed("a nice landscape"));
    EXPECT_FALSE(p.isPromptAllowed("a bad prompt"));
}

// ── Group G – Provenance fields ───────────────────────────────────────────────

TEST(SDPluginFocusedTests, G1_GenerationTimestampPositive) {
    auto g = std::make_unique<InMemorySDGenerator>();
    g->setNextPixels({}, 1, 1);
    SDPlugin p(std::move(g), SDPromptSanitizer{});
    p.initialize("", {});
    EXPECT_GT(p.generate("test", {}).generation_timestamp, 0);
}

TEST(SDPluginFocusedTests, G2_PluginVersionIs2_0_0) {
    SDPlugin p(std::make_unique<InMemorySDGenerator>(), SDPromptSanitizer{});
    p.initialize("", {});
    EXPECT_EQ(p.generate("test", {}).plugin_version, "2.0.0");
}

TEST(SDPluginFocusedTests, G3_PromptHashNotEmpty) {
    auto g = std::make_unique<InMemorySDGenerator>();
    g->setNextPixels({}, 1, 1);
    SDPlugin p(std::move(g), SDPromptSanitizer{});
    p.initialize("", {});
    EXPECT_FALSE(p.generate("test prompt", {}).prompt_hash.empty());
}

// ── Group H – Content policy ──────────────────────────────────────────────────

TEST(SDPluginFocusedTests, H1_BlockedPromptReturnsError) {
    SDPlugin p(std::make_unique<InMemorySDGenerator>(), SDPromptSanitizer({"nsfw"}));
    p.initialize("", {});
    const auto img = p.generate("nsfw content", {});
    EXPECT_FALSE(img.success);
    EXPECT_FALSE(img.error_message.empty());
}

TEST(SDPluginFocusedTests, H2_BlockedCountIncrements) {
    SDPlugin p(std::make_unique<InMemorySDGenerator>(), SDPromptSanitizer({"evil"}));
    p.initialize("", {});
    p.generate("evil thing", {});
    p.generate("evil again", {});
    EXPECT_GE(p.getStatistics()["blocked_count"].get<uint64_t>(), 2u);
}

TEST(SDPluginFocusedTests, H3_AllowedPromptDoesNotIncrementBlockedCount) {
    SDPlugin p(std::make_unique<InMemorySDGenerator>(), SDPromptSanitizer({"nsfw"}));
    p.initialize("", {});
    p.generate("a nice landscape", {});
    EXPECT_EQ(p.getStatistics()["blocked_count"].get<uint64_t>(), 0u);
}

// ── Group I – getStatistics ───────────────────────────────────────────────────

TEST(SDPluginFocusedTests, I1_StatisticsContainsRequiredKeys) {
    SDPlugin p(std::make_unique<InMemorySDGenerator>(), SDPromptSanitizer{});
    const auto s = p.getStatistics();
    EXPECT_TRUE(s.contains("plugin"));
    EXPECT_TRUE(s.contains("plugin_version"));
    EXPECT_TRUE(s.contains("generation_count"));
    EXPECT_TRUE(s.contains("error_count"));
    EXPECT_TRUE(s.contains("blocked_count"));
}

TEST(SDPluginFocusedTests, I2_StatisticsPluginName) {
    SDPlugin p(std::make_unique<InMemorySDGenerator>(), SDPromptSanitizer{});
    EXPECT_EQ(p.getStatistics()["plugin"].get<std::string>(), "stable_diffusion");
}

TEST(SDPluginFocusedTests, I3_GenerationCountIncrements) {
    auto g = std::make_unique<InMemorySDGenerator>();
    g->setNextPixels({}, 1, 1);
    SDPlugin p(std::move(g), SDPromptSanitizer{});
    p.initialize("", {});
    p.generate("a", {}); p.generate("b", {});
    EXPECT_GE(p.getStatistics()["generation_count"].get<uint64_t>(), 2u);
}

// ── Group J – Error paths ─────────────────────────────────────────────────────

TEST(SDPluginFocusedTests, J1_UninitializedGenerateReturnsError) {
    SDPlugin p(std::make_unique<InMemorySDGenerator>(), SDPromptSanitizer{});
    const auto img = p.generate("test", {});
    EXPECT_FALSE(img.success);
    EXPECT_FALSE(img.error_message.empty());
}

TEST(SDPluginFocusedTests, J2_GeneratorThrowsReturnsError) {
    SDPlugin p(std::make_unique<ThrowingGenerator>(), SDPromptSanitizer{});
    p.initialize("", {});
    const auto img = p.generate("test", {});
    EXPECT_FALSE(img.success);
    EXPECT_EQ(img.error_message, "generator error");
}

TEST(SDPluginFocusedTests, J3_DoubleInitIsSafe) {
    SDPlugin p(std::make_unique<InMemorySDGenerator>(), SDPromptSanitizer{});
    EXPECT_TRUE(p.initialize("", {}));
    EXPECT_TRUE(p.initialize("", {}));
    EXPECT_TRUE(p.isInitialized());
}
