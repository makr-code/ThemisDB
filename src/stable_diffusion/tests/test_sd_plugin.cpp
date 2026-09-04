/**
 * @file test_sd_plugin.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include <gtest/gtest.h>
#include "stable_diffusion/sd_config.h"
#include "stable_diffusion/sd_prompt_sanitizer.h"
#include "stable_diffusion/sd_generator.h"
#include "stable_diffusion/sd_plugin.h"
#include "utils/checksum_utils.h"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <stdexcept>

using namespace themis::imggen;
using json = nlohmann::json;

// ── helpers ───────────────────────────────────────────────────────────────────

/** @brief ── helpers ───────────────────────────────────────────────────────────────────. */
class ThrowingGenerator : public ISDGenerator {
public:
    bool initialize(const SDConfig& cfg) override {
        model_id_ = cfg.model_path; initialized_ = true; return true;
    }
    bool isInitialized() const override { return initialized_; }
    std::vector<uint8_t> generate(const std::string&, const SDGenerationConfig&,
                                   [[maybe_unused]] int& w,
                                   [[maybe_unused]] int& h,
                                   [[maybe_unused]] uint64_t& seed) override {
        throw std::runtime_error("generator error");
    }
    std::string getModelId() const override { return model_id_; }
private:
    bool initialized_ = false;
    std::string model_id_;
};

/** @brief Lo ra failing generator. */
class LoRAFailingGenerator : public InMemorySDGenerator {
public:
    bool applyLoRA(const std::string&, float, std::string& error_out) override {
        error_out = "lora apply failed";
        return false;
    }
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

TEST(SDPluginFocusedTests, B4_ToJsonRoundTripNewFields) {
    SDConfig cfg;
    cfg.model_path = "/model.gguf";
    cfg.model_sha256 = "ABCDEF";
    const SDConfig r = SDConfig::fromJson(cfg.toJson());
    EXPECT_EQ(r.model_path, "/model.gguf");
    EXPECT_EQ(r.model_sha256, "ABCDEF");
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
    g->setNextPixels({0, 0, 0}, 1, 1);
    SDPlugin p(std::move(g), SDPromptSanitizer{});
    p.initialize("", {});
    EXPECT_GT(p.generate("test", {}).generation_timestamp, 0);
}

TEST(SDPluginFocusedTests, G2_PluginVersionIs2_1_0) {
    SDPlugin p(std::make_unique<InMemorySDGenerator>(), SDPromptSanitizer{});
    p.initialize("", {});
    EXPECT_EQ(p.generate("test", {}).plugin_version, "2.3.0");
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
    g->setNextPixels({0, 0, 0}, 1, 1);
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

TEST(SDPluginFocusedTests, J4_InitializeFailsOnModelShaMismatch) {
    const std::filesystem::path p = std::filesystem::temp_directory_path() / "sd_plugin_sha_mismatch.bin";
    {
        std::ofstream ofs(p, std::ios::binary);
        ofs << "themis-sd-test";
    }
    SDPlugin plugin(std::make_unique<InMemorySDGenerator>(), SDPromptSanitizer{});
    const bool ok = plugin.initialize(
        p.string(),
        json{{"model_sha256", "deadbeef"}}
    );
    EXPECT_FALSE(ok);
    std::error_code ec = {};
    std::filesystem::remove(p, ec);
}

TEST(SDPluginFocusedTests, J5_InitializeSucceedsOnModelShaMatch) {
    const std::filesystem::path p = std::filesystem::temp_directory_path() / "sd_plugin_sha_match.bin";
    {
        std::ofstream ofs(p, std::ios::binary);
        ofs << "themis-sd-test-match";
    }
    const std::string digest = themis::utils::calculateSHA256(p.string());
    ASSERT_FALSE(digest.empty());

    SDPlugin plugin(std::make_unique<InMemorySDGenerator>(), SDPromptSanitizer{});
    const bool ok = plugin.initialize(
        p.string(),
        json{{"model_sha256", digest}}
    );
    EXPECT_TRUE(ok);
    std::error_code ec = {};
    std::filesystem::remove(p, ec);
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
    g->setNextPixels({0, 0, 0}, 1, 1);
    SDPlugin p(std::move(g), SDPromptSanitizer({"nsfw"}));
    p.initialize("", {});
    SDGenerationConfig cfg;
    cfg.negative_prompt = "";  // empty negative_prompt: policy check skipped
    const auto img = p.generate("a nice landscape", cfg);
    EXPECT_TRUE(img.success);
}

TEST(SDPluginFocusedTests, K4_DimensionGuardRejectsInvalidSize) {
    SDPlugin p(std::make_unique<InMemorySDGenerator>(), SDPromptSanitizer{});
    p.initialize("", {});
    SDGenerationConfig cfg;
    cfg.width = 0;
    cfg.height = 256;
    const auto img = p.generate("safe prompt", cfg);
    EXPECT_FALSE(img.success);
    EXPECT_NE(img.error_message.find("dimensions"), std::string::npos);
}

// ── Group L – generateBatch ───────────────────────────────────────────────────

TEST(SDPluginFocusedTests, L1_BatchReturnsOneResultPerPrompt) {
    auto g = std::make_unique<InMemorySDGenerator>();
    g->setNextPixels({0, 0, 0}, 1, 1);
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
    g->setNextPixels({0, 0, 0}, 1, 1);
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
    cfg.input_image_rgb = std::vector<uint8_t>(2 * 3 * 3, 8u);
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
    cfg.input_image_rgb = std::vector<uint8_t>(128 * 64 * 3, 1u);
    p.generateImg2Img("test", cfg);
    EXPECT_EQ(raw_gen->lastImg2ImgInputWidth(),  128);
    EXPECT_EQ(raw_gen->lastImg2ImgInputHeight(),  64);
}

TEST(SDPluginFocusedTests, N4_ControlNetAndLoRAFieldsPassedToGenerator) {
    auto* raw_gen = new InMemorySDGenerator();
    raw_gen->setNextPixels(std::vector<uint8_t>(8 * 8 * 3, 77u), 8, 8);
    SDPlugin p(std::unique_ptr<ISDGenerator>(raw_gen), SDPromptSanitizer{});
    p.initialize("", {});
    SDGenerationConfig cfg;
    cfg.width = 8;
    cfg.height = 8;
    cfg.control_image_rgb = std::vector<uint8_t>(8 * 8 * 3, 11u);
    cfg.control_width = 8;
    cfg.control_height = 8;
    cfg.control_strength = 0.4f;
    cfg.control_model_path = "/tmp/controlnet.safetensors";
    cfg.lora_adapter_path = "/tmp/adapter.safetensors";
    cfg.lora_scale = 0.8f;

    const auto img = p.generate("safe", cfg);
    EXPECT_TRUE(img.success);
    EXPECT_EQ(raw_gen->lastControlWidth(), 8);
    EXPECT_EQ(raw_gen->lastControlHeight(), 8);
    EXPECT_FLOAT_EQ(raw_gen->lastControlStrength(), 0.4f);
    EXPECT_EQ(raw_gen->lastControlModelPath(), "/tmp/controlnet.safetensors");
    EXPECT_EQ(raw_gen->lastLoraAdapterPath(), "/tmp/adapter.safetensors");
    EXPECT_FLOAT_EQ(raw_gen->lastLoraScale(), 0.8f);
    EXPECT_EQ(raw_gen->lastAppliedLoraPath(), "/tmp/adapter.safetensors");
    EXPECT_FLOAT_EQ(raw_gen->lastAppliedLoraScale(), 0.8f);
}

TEST(SDPluginFocusedTests, N5_LoRAApplyFailureReturnsError) {
    auto g = std::make_unique<LoRAFailingGenerator>();
    g->setNextPixels(std::vector<uint8_t>(8 * 8 * 3, 3u), 8, 8);
    SDPlugin p(std::move(g), SDPromptSanitizer{});
    p.initialize("", {});
    SDGenerationConfig cfg;
    cfg.width = 8;
    cfg.height = 8;
    cfg.lora_adapter_path = "/tmp/missing.safetensors";
    cfg.lora_scale = 1.0f;
    const auto img = p.generate("safe", cfg);
    EXPECT_FALSE(img.success);
    EXPECT_EQ(img.error_message, "lora apply failed");
}

TEST(SDPluginFocusedTests, N6_ControlNetAndLoRAFieldsPassedToImg2Img) {
    auto* raw_gen = new InMemorySDGenerator();
    raw_gen->setNextPixels(std::vector<uint8_t>(8 * 8 * 3, 12u), 8, 8);
    SDPlugin p(std::unique_ptr<ISDGenerator>(raw_gen), SDPromptSanitizer{});
    p.initialize("", {});
    Img2ImgConfig cfg;
    cfg.width = 8;
    cfg.height = 8;
    cfg.input_width = 8;
    cfg.input_height = 8;
    cfg.input_image_rgb = std::vector<uint8_t>(8 * 8 * 3, 1u);
    cfg.control_image_rgb = std::vector<uint8_t>(8 * 8 * 3, 2u);
    cfg.control_width = 8;
    cfg.control_height = 8;
    cfg.control_strength = 0.25f;
    cfg.control_model_path = "/tmp/control-2.safetensors";
    cfg.lora_adapter_path = "/tmp/lora-2.safetensors";
    cfg.lora_scale = 1.2f;
    const auto img = p.generateImg2Img("safe", cfg);
    EXPECT_TRUE(img.success);
    EXPECT_EQ(raw_gen->lastControlWidth(), 8);
    EXPECT_EQ(raw_gen->lastControlHeight(), 8);
    EXPECT_FLOAT_EQ(raw_gen->lastControlStrength(), 0.25f);
    EXPECT_EQ(raw_gen->lastControlModelPath(), "/tmp/control-2.safetensors");
    EXPECT_EQ(raw_gen->lastLoraAdapterPath(), "/tmp/lora-2.safetensors");
    EXPECT_FLOAT_EQ(raw_gen->lastLoraScale(), 1.2f);
}

TEST(SDPluginFocusedTests, N7_EmptyLoraPathClearsPreviousStateOnGenerate) {
    auto* raw_gen = new InMemorySDGenerator();
    raw_gen->setNextPixels(std::vector<uint8_t>(8 * 8 * 3, 7u), 8, 8);
    SDPlugin p(std::unique_ptr<ISDGenerator>(raw_gen), SDPromptSanitizer{});
    p.initialize("", {});

    SDGenerationConfig first_cfg;
    first_cfg.width = 8;
    first_cfg.height = 8;
    first_cfg.lora_adapter_path = "/tmp/lora-first.safetensors";
    first_cfg.lora_scale = 1.1f;
    const auto first_img = p.generate("first", first_cfg);
    ASSERT_TRUE(first_img.success);
    EXPECT_EQ(raw_gen->lastAppliedLoraPath(), "/tmp/lora-first.safetensors");
    EXPECT_FLOAT_EQ(raw_gen->lastAppliedLoraScale(), 1.1f);

    SDGenerationConfig second_cfg;
    second_cfg.width = 8;
    second_cfg.height = 8;
    second_cfg.lora_adapter_path.clear();
    const auto second_img = p.generate("second", second_cfg);
    ASSERT_TRUE(second_img.success);
    EXPECT_TRUE(raw_gen->lastAppliedLoraPath().empty());
    EXPECT_FLOAT_EQ(raw_gen->lastAppliedLoraScale(), 1.0f);
}

TEST(SDPluginFocusedTests, N8_EmptyLoraPathClearsPreviousStateOnImg2Img) {
    auto* raw_gen = new InMemorySDGenerator();
    raw_gen->setNextPixels(std::vector<uint8_t>(8 * 8 * 3, 9u), 8, 8);
    SDPlugin p(std::unique_ptr<ISDGenerator>(raw_gen), SDPromptSanitizer{});
    p.initialize("", {});

    Img2ImgConfig first_cfg;
    first_cfg.width = 8;
    first_cfg.height = 8;
    first_cfg.input_width = 8;
    first_cfg.input_height = 8;
    first_cfg.input_image_rgb = std::vector<uint8_t>(8 * 8 * 3, 1u);
    first_cfg.lora_adapter_path = "/tmp/lora-img2img.safetensors";
    first_cfg.lora_scale = 1.3f;
    const auto first_img = p.generateImg2Img("first", first_cfg);
    ASSERT_TRUE(first_img.success);
    EXPECT_EQ(raw_gen->lastAppliedLoraPath(), "/tmp/lora-img2img.safetensors");
    EXPECT_FLOAT_EQ(raw_gen->lastAppliedLoraScale(), 1.3f);

    Img2ImgConfig second_cfg;
    second_cfg.width = 8;
    second_cfg.height = 8;
    second_cfg.input_width = 8;
    second_cfg.input_height = 8;
    second_cfg.input_image_rgb = std::vector<uint8_t>(8 * 8 * 3, 1u);
    second_cfg.lora_adapter_path.clear();
    const auto second_img = p.generateImg2Img("second", second_cfg);
    ASSERT_TRUE(second_img.success);
    EXPECT_TRUE(raw_gen->lastAppliedLoraPath().empty());
    EXPECT_FLOAT_EQ(raw_gen->lastAppliedLoraScale(), 1.0f);
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
    g->setNextPixels({0, 0, 0}, 1, 1);
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
    if (buf.size() < 8u) {
      return false;
    }
    size_t pos = 8u;  // skip PNG signature
    while (pos + 12u <= buf.size()) {
        const uint32_t len =
            (static_cast<uint32_t>(buf[pos])     << 24) |
            (static_cast<uint32_t>(buf[pos + 1]) << 16) |
            (static_cast<uint32_t>(buf[pos + 2]) <<  8) |
             static_cast<uint32_t>(buf[pos + 3]);
        if (buf.size() < pos + 12u + len) {
          break;
        }
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
        if (buf.size() < pos + 12u + len) {
          break;
        }
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

TEST(SDPluginFocusedTests, P4_PerceptualHashDeterministic) {
    auto g = std::make_unique<InMemorySDGenerator>();
    std::vector<uint8_t> px(8 * 8 * 3, 42u);
    g->setNextPixels(px, 8, 8);
    SDPlugin p(std::move(g), SDPromptSanitizer{});
    p.initialize("", {});
    SDGenerationConfig cfg;
    cfg.width = 8;
    cfg.height = 8;
    const auto img1 = p.generate("same prompt", cfg);
    const auto img2 = p.generate("same prompt", cfg);
    ASSERT_TRUE(img1.success);
    ASSERT_TRUE(img2.success);
    ASSERT_TRUE(img1.perceptual_hash.has_value());
    ASSERT_TRUE(img2.perceptual_hash.has_value());
    EXPECT_EQ(*img1.perceptual_hash, *img2.perceptual_hash);
}

TEST(SDPluginFocusedTests, P5_PerceptualHashNonFatalWhenUnavailable) {
    auto g = std::make_unique<InMemorySDGenerator>();
    g->setNextPixels({255, 0, 0}, 1, 1);
    SDPlugin p(std::move(g), SDPromptSanitizer{});
    p.initialize("", {});
    SDGenerationConfig cfg;
    cfg.width = 1;
    cfg.height = 1;
    const auto img = p.generate("tiny", cfg);
    EXPECT_TRUE(img.success);
    EXPECT_FALSE(img.perceptual_hash.has_value());
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
