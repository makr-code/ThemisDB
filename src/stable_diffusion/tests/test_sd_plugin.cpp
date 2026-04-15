/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_sd_plugin.cpp                                 ║
  Version:         0.0.8                                              ║
  Last Modified:   2026-04-15 07:14:31                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     672                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • b66a69c598  2026-04-12  feat(stable_diffusion): implement SDCppGenerator, real PN... ║
    • 75af53c598  2026-04-11  feat(stable_diffusion): v2.1.0 — batch generation, img2im... ║
    • 1e348484ec  2026-04-07  feat(plugins): add stable_diffusion + llama_cpp plugins, ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file test_sd_plugin.cpp
 * @brief Unit tests for the Stable Diffusion image generation plugin
 *
 * Test suite: SDPluginFocusedTests (51 tests)
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
 *   Group K (3)  – SDPlugin: negative_prompt content-policy enforcement
 *   Group L (3)  – SDPlugin::generateBatch: returns one result per prompt, blocked prompts
 *   Group M (3)  – SDPlugin::generateImg2Img: success, blocked prompt, uninit
 *   Group N (3)  – InMemorySDGenerator img2img: path taken, strength/dimensions recorded
 *   Group O (3)  – IImageGenerationBackend: Img2ImgConfig default, generateBatch default impl
 *   Group P (3)  – PNG encoder: signature, IDAT chunk present, IHDR dimensions correct
 *   Group Q (3)  – SDStubGenerator::generateImg2Img: input dimensions, data pass-through, fallback
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

TEST(SDPluginFocusedTests, G2_PluginVersionIs2_1_0) {
    SDPlugin p(std::make_unique<InMemorySDGenerator>(), SDPromptSanitizer{});
    p.initialize("", {});
    EXPECT_EQ(p.generate("test", {}).plugin_version, "2.1.0");
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

// ── Group K – negative_prompt content-policy enforcement ─────────────────────

TEST(SDPluginFocusedTests, K1_BlockedNegativePromptReturnsError) {
    SDPlugin p(std::make_unique<InMemorySDGenerator>(), SDPromptSanitizer({"nsfw"}));
    p.initialize("", {});
    SDGenerationConfig cfg;
    cfg.negative_prompt = "nsfw ugly distorted";
    const auto img = p.generate("a beautiful cat", cfg);
    EXPECT_FALSE(img.success);
    EXPECT_NE(img.error_message.find("negative_prompt"), std::string::npos);
}

TEST(SDPluginFocusedTests, K2_BlockedNegativePromptIncrementsBlockedCount) {
    SDPlugin p(std::make_unique<InMemorySDGenerator>(), SDPromptSanitizer({"banned"}));
    p.initialize("", {});
    SDGenerationConfig cfg;
    cfg.negative_prompt = "banned keyword";
    p.generate("safe prompt", cfg);
    EXPECT_GE(p.getStatistics()["blocked_count"].get<uint64_t>(), 1u);
}

TEST(SDPluginFocusedTests, K3_EmptyNegativePromptDoesNotBlock) {
    auto g = std::make_unique<InMemorySDGenerator>();
    g->setNextPixels({0}, 1, 1);
    SDPlugin p(std::move(g), SDPromptSanitizer({"nsfw"}));
    p.initialize("", {});
    SDGenerationConfig cfg;
    cfg.negative_prompt = "";  // empty negative_prompt: policy check skipped
    const auto img = p.generate("a nice landscape", cfg);
    EXPECT_TRUE(img.success);
}

// ── Group L – generateBatch ───────────────────────────────────────────────────

TEST(SDPluginFocusedTests, L1_BatchReturnsOneResultPerPrompt) {
    auto g = std::make_unique<InMemorySDGenerator>();
    g->setNextPixels({0}, 1, 1);
    SDPlugin p(std::move(g), SDPromptSanitizer{});
    p.initialize("", {});
    const auto results = p.generateBatch({"cat", "dog", "bird"}, SDGenerationConfig{});
    EXPECT_EQ(results.size(), 3u);
    for (const auto& img : results) {
        EXPECT_TRUE(img.success);
    }
}

TEST(SDPluginFocusedTests, L2_BatchBlockedPromptDoesNotBlockOthers) {
    auto g = std::make_unique<InMemorySDGenerator>();
    g->setNextPixels({0}, 1, 1);
    SDPlugin p(std::move(g), SDPromptSanitizer({"blocked"}));
    p.initialize("", {});
    const auto results = p.generateBatch({"safe prompt", "blocked content", "also safe"},
                                          SDGenerationConfig{});
    ASSERT_EQ(results.size(), 3u);
    EXPECT_TRUE(results[0].success);
    EXPECT_FALSE(results[1].success);
    EXPECT_TRUE(results[2].success);
}

TEST(SDPluginFocusedTests, L3_BatchEmptyPromptsListReturnsEmpty) {
    SDPlugin p(std::make_unique<InMemorySDGenerator>(), SDPromptSanitizer{});
    p.initialize("", {});
    const auto results = p.generateBatch({}, SDGenerationConfig{});
    EXPECT_TRUE(results.empty());
}

// ── Group M – generateImg2Img ─────────────────────────────────────────────────

TEST(SDPluginFocusedTests, M1_Img2ImgSuccessAfterInit) {
    auto g = std::make_unique<InMemorySDGenerator>();
    g->setNextPixels({0, 0, 0}, 1, 1);
    SDPlugin p(std::move(g), SDPromptSanitizer{});
    p.initialize("", {});
    Img2ImgConfig cfg;
    cfg.input_image_rgb = {255, 0, 0};
    cfg.input_width  = 1;
    cfg.input_height = 1;
    cfg.strength     = 0.8f;
    const auto img = p.generateImg2Img("refine the cat", cfg);
    EXPECT_TRUE(img.success);
    EXPECT_FALSE(img.png_data.empty());
}

TEST(SDPluginFocusedTests, M2_Img2ImgBlockedPromptReturnsError) {
    SDPlugin p(std::make_unique<InMemorySDGenerator>(), SDPromptSanitizer({"blocked"}));
    p.initialize("", {});
    Img2ImgConfig cfg;
    cfg.input_image_rgb = {0};
    cfg.input_width = 1; cfg.input_height = 1;
    const auto img = p.generateImg2Img("blocked content", cfg);
    EXPECT_FALSE(img.success);
}

TEST(SDPluginFocusedTests, M3_Img2ImgUninitializedReturnsError) {
    SDPlugin p(std::make_unique<InMemorySDGenerator>(), SDPromptSanitizer{});
    // Not initialized
    Img2ImgConfig cfg;
    const auto img = p.generateImg2Img("test", cfg);
    EXPECT_FALSE(img.success);
    EXPECT_FALSE(img.error_message.empty());
}

// ── Group N – InMemorySDGenerator img2img inspection ─────────────────────────

TEST(SDPluginFocusedTests, N1_Img2ImgGeneratorPathTaken) {
    auto* raw_gen = new InMemorySDGenerator();
    raw_gen->setNextPixels({0}, 1, 1);
    SDPlugin p(std::unique_ptr<ISDGenerator>(raw_gen), SDPromptSanitizer{});
    p.initialize("", {});
    Img2ImgConfig cfg;
    cfg.input_image_rgb = {100, 200, 50};
    cfg.input_width = 1; cfg.input_height = 1;
    cfg.strength = 0.6f;
    p.generateImg2Img("refine", cfg);
    EXPECT_TRUE(raw_gen->img2imgCalled());
}

TEST(SDPluginFocusedTests, N2_Img2ImgStrengthRecorded) {
    auto* raw_gen = new InMemorySDGenerator();
    raw_gen->setNextPixels({0}, 1, 1);
    SDPlugin p(std::unique_ptr<ISDGenerator>(raw_gen), SDPromptSanitizer{});
    p.initialize("", {});
    Img2ImgConfig cfg;
    cfg.input_width = 2; cfg.input_height = 3;
    cfg.strength = 0.45f;
    p.generateImg2Img("test", cfg);
    EXPECT_FLOAT_EQ(raw_gen->lastImg2ImgStrength(), 0.45f);
}

TEST(SDPluginFocusedTests, N3_Img2ImgInputDimensionsRecorded) {
    auto* raw_gen = new InMemorySDGenerator();
    raw_gen->setNextPixels({0}, 1, 1);
    SDPlugin p(std::unique_ptr<ISDGenerator>(raw_gen), SDPromptSanitizer{});
    p.initialize("", {});
    Img2ImgConfig cfg;
    cfg.input_width = 128; cfg.input_height = 64;
    p.generateImg2Img("test", cfg);
    EXPECT_EQ(raw_gen->lastImg2ImgInputWidth(),  128);
    EXPECT_EQ(raw_gen->lastImg2ImgInputHeight(),  64);
}

// ── Group O – IImageGenerationBackend / Img2ImgConfig ────────────────────────

TEST(SDPluginFocusedTests, O1_Img2ImgConfigDefaultStrength) {
    Img2ImgConfig cfg;
    EXPECT_FLOAT_EQ(cfg.strength, 0.75f);
    EXPECT_EQ(cfg.input_width,  0);
    EXPECT_EQ(cfg.input_height, 0);
    EXPECT_TRUE(cfg.mask_rgb.empty());
}

TEST(SDPluginFocusedTests, O2_Img2ImgConfigInheritsSdGenerationConfig) {
    Img2ImgConfig cfg;
    EXPECT_EQ(cfg.width,  512);
    EXPECT_EQ(cfg.height, 512);
    EXPECT_EQ(cfg.steps,  20);
    EXPECT_EQ(cfg.seed,  -1);
}

TEST(SDPluginFocusedTests, O3_GenerateBatchCountsGenerations) {
    auto g = std::make_unique<InMemorySDGenerator>();
    g->setNextPixels({0}, 1, 1);
    SDPlugin p(std::move(g), SDPromptSanitizer{});
    p.initialize("", {});
    p.generateBatch({"a", "b", "c"}, SDGenerationConfig{});
    EXPECT_EQ(p.getStatistics()["generation_count"].get<uint64_t>(), 3u);
}

// ── PNG parsing helpers ───────────────────────────────────────────────────────

namespace {

// Returns true when buf contains a PNG chunk with the given 4-byte type.
// Scans the entire buffer (post-signature) to find any matching chunk.
bool png_has_chunk(const std::vector<uint8_t>& buf, const char type[4]) {
    if (buf.size() < 8u) return false;
    size_t pos = 8u;  // skip PNG signature
    while (pos + 12u <= buf.size()) {
        const uint32_t len =
            (static_cast<uint32_t>(buf[pos])     << 24) |
            (static_cast<uint32_t>(buf[pos + 1]) << 16) |
            (static_cast<uint32_t>(buf[pos + 2]) <<  8) |
             static_cast<uint32_t>(buf[pos + 3]);
        if (buf.size() < pos + 12u + len) break;
        if (buf[pos + 4] == static_cast<uint8_t>(type[0]) &&
            buf[pos + 5] == static_cast<uint8_t>(type[1]) &&
            buf[pos + 6] == static_cast<uint8_t>(type[2]) &&
            buf[pos + 7] == static_cast<uint8_t>(type[3])) {
            return true;
        }
        pos += 12u + len;
    }
    return false;
}

// Read the IDAT chunk data (first occurrence) from a PNG buffer.
// Returns empty vector if not found.
std::vector<uint8_t> png_idat_data(const std::vector<uint8_t>& buf) {
    if (buf.size() < 8u) return {};
    size_t pos = 8u;
    while (pos + 12u <= buf.size()) {
        const uint32_t len =
            (static_cast<uint32_t>(buf[pos])     << 24) |
            (static_cast<uint32_t>(buf[pos + 1]) << 16) |
            (static_cast<uint32_t>(buf[pos + 2]) <<  8) |
             static_cast<uint32_t>(buf[pos + 3]);
        if (buf.size() < pos + 12u + len) break;
        if (buf[pos + 4] == 'I' && buf[pos + 5] == 'D' &&
            buf[pos + 6] == 'A' && buf[pos + 7] == 'T') {
            return std::vector<uint8_t>(buf.begin() + pos + 8,
                                        buf.begin() + pos + 8 + len);
        }
        pos += 12u + len;
    }
    return {};
}

// Read IHDR dimensions from a PNG buffer.
// Returns {-1,-1} on error.
std::pair<int,int> png_ihdr_dims(const std::vector<uint8_t>& buf) {
    if (buf.size() < 8u + 25u) return {-1, -1};
    const size_t pos = 8u;
    // Length of IHDR data must be 13
    const uint32_t len =
        (static_cast<uint32_t>(buf[pos])     << 24) |
        (static_cast<uint32_t>(buf[pos + 1]) << 16) |
        (static_cast<uint32_t>(buf[pos + 2]) <<  8) |
         static_cast<uint32_t>(buf[pos + 3]);
    if (len != 13u) return {-1, -1};
    if (buf[pos + 4] != 'I' || buf[pos + 5] != 'H' ||
        buf[pos + 6] != 'D' || buf[pos + 7] != 'R') return {-1, -1};
    const int w =
        (static_cast<int>(buf[pos + 8])  << 24) |
        (static_cast<int>(buf[pos + 9])  << 16) |
        (static_cast<int>(buf[pos + 10]) <<  8) |
         static_cast<int>(buf[pos + 11]);
    const int h =
        (static_cast<int>(buf[pos + 12]) << 24) |
        (static_cast<int>(buf[pos + 13]) << 16) |
        (static_cast<int>(buf[pos + 14]) <<  8) |
         static_cast<int>(buf[pos + 15]);
    return {w, h};
}

} // namespace

// ── Group P – PNG encoder ─────────────────────────────────────────────────────

TEST(SDPluginFocusedTests, P1_PngSignatureCorrect) {
    auto g = std::make_unique<InMemorySDGenerator>();
    g->setNextPixels({255, 0, 0}, 1, 1);   // 1×1 red pixel
    SDPlugin p(std::move(g), SDPromptSanitizer{});
    p.initialize("", {});
    SDGenerationConfig cfg; cfg.width = 1; cfg.height = 1;
    const auto img = p.generate("test", cfg);
    ASSERT_TRUE(img.success);
    ASSERT_GE(img.png_data.size(), 8u);
    // PNG magic: 89 50 4E 47 0D 0A 1A 0A
    EXPECT_EQ(img.png_data[0], 0x89u);
    EXPECT_EQ(img.png_data[1], static_cast<uint8_t>('P'));
    EXPECT_EQ(img.png_data[2], static_cast<uint8_t>('N'));
    EXPECT_EQ(img.png_data[3], static_cast<uint8_t>('G'));
    EXPECT_EQ(img.png_data[4], 0x0Du);
    EXPECT_EQ(img.png_data[5], 0x0Au);
    EXPECT_EQ(img.png_data[6], 0x1Au);
    EXPECT_EQ(img.png_data[7], 0x0Au);
}

TEST(SDPluginFocusedTests, P2_PngContainsIdatChunk) {
    auto g = std::make_unique<InMemorySDGenerator>();
    std::vector<uint8_t> px(2 * 3 * 3, 128u);  // 2×3 grey image
    g->setNextPixels(px, 2, 3);
    SDPlugin p(std::move(g), SDPromptSanitizer{});
    p.initialize("", {});
    SDGenerationConfig cfg; cfg.width = 2; cfg.height = 3;
    const auto img = p.generate("test", cfg);
    ASSERT_TRUE(img.success);
    EXPECT_TRUE(png_has_chunk(img.png_data, "IDAT"))
        << "PNG output must contain an IDAT chunk with pixel data";
    const auto idat = png_idat_data(img.png_data);
    EXPECT_FALSE(idat.empty());
    // zlib CMF byte 0x78 indicates deflate with 32 KB window
    EXPECT_EQ(idat[0], 0x78u);
}

TEST(SDPluginFocusedTests, P3_PngIhdrDimensionsMatchRequest) {
    auto g = std::make_unique<InMemorySDGenerator>();
    std::vector<uint8_t> px(4 * 7 * 3, 0u);
    g->setNextPixels(px, 4, 7);
    SDPlugin p(std::move(g), SDPromptSanitizer{});
    p.initialize("", {});
    SDGenerationConfig cfg; cfg.width = 4; cfg.height = 7;
    const auto img = p.generate("test", cfg);
    ASSERT_TRUE(img.success);
    const auto [w, h] = png_ihdr_dims(img.png_data);
    EXPECT_EQ(w, 4);
    EXPECT_EQ(h, 7);
}

// ── Group Q – SDStubGenerator::generateImg2Img ────────────────────────────────

TEST(SDPluginFocusedTests, Q1_StubImg2ImgUsesInputDimensions) {
    SDStubGenerator g;
    g.initialize(SDConfig{});
    Img2ImgConfig cfg;
    cfg.input_image_rgb = {100, 150, 200, 50, 75, 100};  // 2 pixels (2×1 RGB)
    cfg.input_width  = 2;
    cfg.input_height = 1;
    cfg.strength = 0.5f;
    int out_w = 0, out_h = 0;
    uint64_t out_seed = 0;
    g.generateImg2Img("prompt", cfg, out_w, out_h, out_seed);
    EXPECT_EQ(out_w, 2);
    EXPECT_EQ(out_h, 1);
}

TEST(SDPluginFocusedTests, Q2_StubImg2ImgReturnsInputData) {
    SDStubGenerator g;
    g.initialize(SDConfig{});
    const std::vector<uint8_t> input_rgb = {10, 20, 30, 40, 50, 60};
    Img2ImgConfig cfg;
    cfg.input_image_rgb = input_rgb;
    cfg.input_width  = 2;
    cfg.input_height = 1;
    int out_w = 0, out_h = 0;
    uint64_t out_seed = 0;
    const auto result = g.generateImg2Img("prompt", cfg, out_w, out_h, out_seed);
    EXPECT_EQ(result, input_rgb)
        << "SDStubGenerator::generateImg2Img must return the input pixel data";
}

TEST(SDPluginFocusedTests, Q3_StubImg2ImgFallsBackWhenInputEmpty) {
    SDStubGenerator g;
    SDConfig sc; sc.width = 8; sc.height = 8;
    g.initialize(sc);
    Img2ImgConfig cfg;
    cfg.width  = 8;
    cfg.height = 8;
    // No input_image_rgb provided → fall back to generate()
    int out_w = 0, out_h = 0;
    uint64_t out_seed = 0;
    const auto result = g.generateImg2Img("prompt", cfg, out_w, out_h, out_seed);
    EXPECT_EQ(out_w, 8);
    EXPECT_EQ(out_h, 8);
    EXPECT_EQ(result.size(), static_cast<size_t>(8 * 8 * 3));
}
