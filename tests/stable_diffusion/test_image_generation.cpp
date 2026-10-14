#include <gtest/gtest.h>

#include "stable_diffusion/sd_generator.h"
#include "stable_diffusion/sd_plugin.h"

#include <vector>

using namespace themis::imggen;

TEST(StableDiffusionImageGeneration, GeneratesNonEmptyPngForSimplePrompt) {
    auto generator = std::make_unique<InMemorySDGenerator>();
    generator->setNextPixels({255, 0, 0, 0, 255, 0, 0, 0, 255}, 1, 1, 7);

    SDPlugin plugin(std::move(generator), SDPromptSanitizer({}));
    ASSERT_TRUE(plugin.initialize("stub", {}));

    const auto image = plugin.generate("a warm sunset", SDGenerationConfig{});
    EXPECT_TRUE(image.success);
    EXPECT_FALSE(image.png_data.empty());
    EXPECT_EQ(image.width, 1);
    EXPECT_EQ(image.height, 1);
    EXPECT_EQ(image.seed_used, 7ULL);
    EXPECT_FALSE(image.prompt_hash.empty());
}

TEST(StableDiffusionImageGeneration, GenerateBatchReturnsSameNumberOfResultsAsInputs) {
    SDPlugin plugin(std::make_unique<InMemorySDGenerator>(), SDPromptSanitizer({}));
    ASSERT_TRUE(plugin.initialize("stub", {}));

    std::vector<std::string> prompts{"first prompt", "second prompt", "third prompt"};
    const auto results = plugin.generateBatch(prompts, SDGenerationConfig{});

    EXPECT_EQ(results.size(), prompts.size());
    for (const auto& image : results) {
        EXPECT_TRUE(image.success);
        EXPECT_FALSE(image.png_data.empty());
    }
}

TEST(StableDiffusionImageGeneration, ExactSeedIsPreservedAcrossGeneration) {
    auto generator = std::make_unique<InMemorySDGenerator>();
    generator->setNextPixels({0, 0, 0, 0, 0, 0, 0, 0, 0}, 2, 2, 1234);

    SDPlugin plugin(std::move(generator), SDPromptSanitizer({}));
    ASSERT_TRUE(plugin.initialize("stub", {}));

    const auto image = plugin.generate("seeded prompt", SDGenerationConfig{.seed = 1234});
    EXPECT_TRUE(image.success);
    EXPECT_EQ(image.seed_used, 1234ULL);
}

TEST(StableDiffusionImageGeneration, PromptHashIsDeterministicForSamePrompt) {
    SDPlugin plugin(std::make_unique<InMemorySDGenerator>(), SDPromptSanitizer({}));
    ASSERT_TRUE(plugin.initialize("stub", {}));

    const auto first = plugin.generate("same prompt", SDGenerationConfig{});
    const auto second = plugin.generate("same prompt", SDGenerationConfig{});

    EXPECT_EQ(first.prompt_hash, second.prompt_hash);
}

TEST(StableDiffusionImageGeneration, NegativePromptIsValidatedBeforeGeneration) {
    SDPlugin plugin(std::make_unique<InMemorySDGenerator>(), SDPromptSanitizer({"unsafe"}));
    ASSERT_TRUE(plugin.initialize("stub", {}));

    SDGenerationConfig cfg;
    cfg.negative_prompt = "unsafe portrait";
    const auto image = plugin.generate("calm ocean", cfg);

    EXPECT_FALSE(image.success);
    EXPECT_EQ(image.error_message, "negative_prompt blocked by content policy");
}

TEST(StableDiffusionImageGeneration, InvalidDimensionsAreRejectedBeforeEncoding) {
    auto generator = std::make_unique<InMemorySDGenerator>();
    generator->setNextPixels({0, 0, 0}, 1, 1, 10);

    SDPlugin plugin(std::move(generator), SDPromptSanitizer({}));
    ASSERT_TRUE(plugin.initialize("stub", {}));

    SDGenerationConfig cfg;
    cfg.width = 0;
    cfg.height = 0;
    const auto image = plugin.generate("bad dimensions", cfg);
    EXPECT_FALSE(image.success);
    EXPECT_EQ(image.error_message, "invalid image dimensions");
}

TEST(StableDiffusionImageGeneration, Img2ImgInputUsesExistingImageWhenProvided) {
    auto generator = std::make_unique<InMemorySDGenerator>();
    generator->setNextPixels({10, 20, 30, 40, 50, 60}, 2, 1, 90);

    SDPlugin plugin(std::move(generator), SDPromptSanitizer({}));
    ASSERT_TRUE(plugin.initialize("stub", {}));

    Img2ImgConfig cfg;
    cfg.input_image_rgb = {10, 20, 30, 40, 50, 60};
    cfg.input_width = 2;
    cfg.input_height = 1;
    cfg.strength = 0.9f;

    const auto image = plugin.generateImg2Img("refine image", cfg);
    EXPECT_TRUE(image.success);
    EXPECT_EQ(image.width, 2);
    EXPECT_EQ(image.height, 1);
    EXPECT_EQ(image.seed_used, 90ULL);
}

TEST(StableDiffusionImageGeneration, ControlNetSettingsFlowToGenerator) {
    auto generator = std::make_unique<InMemorySDGenerator>();
    generator->setNextPixels({1, 2, 3, 4, 5, 6}, 2, 1, 4);

    SDPlugin plugin(std::move(generator), SDPromptSanitizer({}));
    ASSERT_TRUE(plugin.initialize("stub", {}));

    SDGenerationConfig cfg;
    cfg.control_image_rgb = {7, 8, 9, 10, 11, 12};
    cfg.control_width = 2;
    cfg.control_height = 1;
    cfg.control_strength = 0.4f;
    cfg.control_model_path = "/models/controlnet/diffusion-controlnet";

    const auto image = plugin.generate("controlled image", cfg);
    EXPECT_TRUE(image.success);
    EXPECT_EQ(image.width, 2);
    EXPECT_EQ(image.height, 1);
}
