#include <gtest/gtest.h>

#include "stable_diffusion/sd_generator.h"
#include "stable_diffusion/sd_plugin.h"

#include <vector>

using namespace themis::imggen;

TEST(StableDiffusionEdgeCases, EmptyPromptIsRejectedByPolicy) {
    SDPlugin plugin(std::make_unique<InMemorySDGenerator>(), SDPromptSanitizer({}));
    ASSERT_TRUE(plugin.initialize("stub", {}));

    const auto image = plugin.generate("", SDGenerationConfig{});
    EXPECT_FALSE(image.success);
    EXPECT_FALSE(image.error_message.empty());
}

TEST(StableDiffusionEdgeCases, NullSeedFallsBackToDeterministicStubValue) {
    auto generator = std::make_unique<InMemorySDGenerator>();
    generator->setNextPixels({0, 0, 0}, 1, 1, 42);

    SDPlugin plugin(std::move(generator), SDPromptSanitizer({}));
    ASSERT_TRUE(plugin.initialize("stub", {}));

    SDGenerationConfig cfg;
    cfg.seed = -1;
    const auto image = plugin.generate("seeded prompt", cfg);

    EXPECT_TRUE(image.success);
    EXPECT_EQ(image.seed_used, 42ULL);
}

TEST(StableDiffusionEdgeCases, LargeBatchGenerationIsSupportedWithOneResultPerPrompt) {
    SDPlugin plugin(std::make_unique<InMemorySDGenerator>(), SDPromptSanitizer({}));
    ASSERT_TRUE(plugin.initialize("stub", {}));

    std::vector<std::string> prompts(4, "prompt");
    const auto results = plugin.generateBatch(prompts, SDGenerationConfig{});

    EXPECT_EQ(results.size(), 4U);
    for (const auto& image : results) {
        EXPECT_TRUE(image.success);
        EXPECT_FALSE(image.png_data.empty());
    }
}

TEST(StableDiffusionEdgeCases, PromptBlockedByPolicyReturnsFailureWithoutCrash) {
    SDPlugin plugin(std::make_unique<InMemorySDGenerator>(), SDPromptSanitizer({"forbidden"}));
    ASSERT_TRUE(plugin.initialize("stub", {}));

    const auto image = plugin.generate("forbidden prompt", SDGenerationConfig{});
    EXPECT_FALSE(image.success);
    EXPECT_EQ(image.error_message, "prompt blocked by content policy");
}

TEST(StableDiffusionEdgeCases, GenerationAfterInitializationKeepsPluginStatisticsConsistent) {
    SDPlugin plugin(std::make_unique<InMemorySDGenerator>(), SDPromptSanitizer({}));
    ASSERT_TRUE(plugin.initialize("stub", {}));

    const auto first = plugin.generate("one", SDGenerationConfig{});
    const auto second = plugin.generate("two", SDGenerationConfig{});
    const auto stats = plugin.getStatistics();

    EXPECT_TRUE(first.success);
    EXPECT_TRUE(second.success);
    EXPECT_GE(stats["generation_count"].get<uint64_t>(), 2ULL);
    EXPECT_GE(stats["error_count"].get<uint64_t>(), 0ULL);
}
