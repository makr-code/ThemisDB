#include <gtest/gtest.h>

#include "stable_diffusion/sd_config.h"
#include "stable_diffusion/sd_generator.h"
#include "stable_diffusion/sd_plugin.h"
#include "stable_diffusion/sd_prompt_sanitizer.h"

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

using namespace themis::imggen;

namespace {

std::string writeBlockedKeywordsFile(const std::string& body) {
    const auto path = std::filesystem::temp_directory_path() /
        std::filesystem::path("themis_sd_blocked_keywords_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".txt");
    std::ofstream out(path);
    out << body;
    out.close();
    return path.string();
}

}  // namespace

TEST(StableDiffusionModelInitialization, DefaultConfigValuesAreValid) {
    const SDConfig cfg = SDConfig::fromJson({});
    EXPECT_EQ(cfg.width, 512);
    EXPECT_EQ(cfg.height, 512);
    EXPECT_EQ(cfg.steps, 1);
    EXPECT_FLOAT_EQ(cfg.cfg_scale, 7.0f);
    EXPECT_EQ(cfg.sampler, "euler_a");
}

TEST(StableDiffusionModelInitialization, InvalidDimensionsAreClampedToSafeDefaults) {
    const SDConfig cfg = SDConfig::fromJson({{"width", 0}, {"height", -4}, {"steps", 0}});
    EXPECT_GE(cfg.width, 1);
    EXPECT_GE(cfg.height, 1);
    EXPECT_GE(cfg.steps, 1);
    EXPECT_EQ(cfg.width, 512);
    EXPECT_EQ(cfg.height, 512);
}

TEST(StableDiffusionModelInitialization, PluginInitializesInStubModeWithoutModelFile) {
    SDPlugin plugin(std::make_unique<SDStubGenerator>(), SDPromptSanitizer({}));
    EXPECT_TRUE(plugin.initialize("", {}));
    EXPECT_TRUE(plugin.isInitialized());
    EXPECT_EQ(plugin.getModelId(), "stub");
}

TEST(StableDiffusionModelInitialization, PluginUsesIncomingModelIdDuringInitialization) {
    SDPlugin plugin(std::make_unique<SDStubGenerator>(), SDPromptSanitizer({}));
    EXPECT_TRUE(plugin.initialize("/models/safetensors/model.safetensors", {}));
    EXPECT_EQ(plugin.getModelId(), "/models/safetensors/model.safetensors");
}

TEST(StableDiffusionModelInitialization, BlockedKeywordsFileOverridesDefaultPolicy) {
    const auto keywords_path = writeBlockedKeywordsFile("unsafe\nforbidden\n");
    SDPlugin plugin(std::make_unique<InMemorySDGenerator>(), SDPromptSanitizer({"allowed"}));
    EXPECT_TRUE(plugin.initialize("stub", {{"blocked_keywords_file", keywords_path}}));
    EXPECT_FALSE(plugin.isPromptAllowed("unsafe art prompt"));
    EXPECT_FALSE(plugin.isPromptAllowed("forbidden composition"));
}

TEST(StableDiffusionModelInitialization, PluginStatisticsTrackInitializationAndGenerations) {
    SDPlugin plugin(std::make_unique<InMemorySDGenerator>(), SDPromptSanitizer({"blocked"}));
    EXPECT_TRUE(plugin.initialize("stub", {}));
    const auto image = plugin.generate("sunrise over the mountains", SDGenerationConfig{});
    EXPECT_TRUE(image.success);

    const auto stats = plugin.getStatistics();
    EXPECT_TRUE(stats["initialized"].get<bool>());
    EXPECT_GE(stats["generation_count"].get<uint64_t>(), 1ULL);
}

TEST(StableDiffusionModelInitialization, NegativePromptBlockedByPolicyDoesNotLeakThrough) {
    SDPlugin plugin(std::make_unique<InMemorySDGenerator>(), SDPromptSanitizer({"unsafe"}));
    EXPECT_TRUE(plugin.initialize("stub", {}));

    SDGenerationConfig cfg;
    cfg.negative_prompt = "unsafe portrait";
    const auto image = plugin.generate("peaceful landscape", cfg);

    EXPECT_FALSE(image.success);
    EXPECT_EQ(image.error_message, "negative_prompt blocked by content policy");
}
