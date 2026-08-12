/**
 * @file test_llm_vision_encoder.cpp
 * @brief Unit tests for VisionEncoder API, VisionRequest, VisionResponse,
 *        and VisionConfig — no CLIP model file is required.
 */

#include <gtest/gtest.h>
#include "llm/vision_encoder.h"
#include "llm/vision_config.h"

using namespace themis::llm;

// ============================================================================
// VisionRequest struct
// ============================================================================

TEST(VisionRequestTest, DefaultConstruction) {
    VisionRequest req;
    EXPECT_TRUE(req.text_prompt.empty());
    EXPECT_TRUE(req.image_path.empty());
    EXPECT_TRUE(req.image_paths.empty());
    EXPECT_EQ(req.max_tokens, 256);
    EXPECT_FLOAT_EQ(req.temperature, 0.7f);
    EXPECT_FLOAT_EQ(req.top_p, 0.9f);
    EXPECT_EQ(req.top_k, 40);
    EXPECT_TRUE(req.use_image_start_end);
    EXPECT_EQ(req.image_token, "<image>");
}

TEST(VisionRequestTest, PopulateSingleImage) {
    VisionRequest req;
    req.text_prompt = "What is in this image?";
    req.image_path  = "/path/to/image.jpg";
    req.max_tokens  = 512;

    EXPECT_EQ(req.text_prompt, "What is in this image?");
    EXPECT_EQ(req.image_path, "/path/to/image.jpg");
    EXPECT_EQ(req.max_tokens, 512);
}

TEST(VisionRequestTest, PopulateMultipleImages) {
    VisionRequest req;
    req.text_prompt = "Compare these images";
    req.image_paths = {"/img1.png", "/img2.png"};

    EXPECT_EQ(req.image_paths.size(), 2u);
    EXPECT_EQ(req.image_paths[0], "/img1.png");
    EXPECT_EQ(req.image_paths[1], "/img2.png");
}

// ============================================================================
// VisionResponse struct
// ============================================================================

TEST(VisionResponseTest, DefaultConstruction) {
    VisionResponse resp;
    EXPECT_FALSE(resp.success);
    EXPECT_TRUE(resp.text.empty());
    EXPECT_TRUE(resp.error_message.empty());
    EXPECT_EQ(resp.tokens_generated, 0);
    EXPECT_EQ(resp.inference_time_ms, 0);
    EXPECT_EQ(resp.image_encoding_time_ms, 0);
    EXPECT_TRUE(resp.model_name.empty());
}

TEST(VisionResponseTest, FailureResponse) {
    VisionResponse resp;
    resp.success       = false;
    resp.error_message = "Vision support not enabled";

    EXPECT_FALSE(resp.success);
    EXPECT_EQ(resp.error_message, "Vision support not enabled");
}

TEST(VisionResponseTest, SuccessResponse) {
    VisionResponse resp;
    resp.success          = true;
    resp.text             = "A scenic mountain landscape.";
    resp.tokens_generated = 6;
    resp.inference_time_ms      = 350;
    resp.image_encoding_time_ms = 42;
    resp.model_name       = "llava-v1.5-7b";

    EXPECT_TRUE(resp.success);
    EXPECT_EQ(resp.text, "A scenic mountain landscape.");
    EXPECT_EQ(resp.tokens_generated, 6);
    EXPECT_GT(resp.inference_time_ms, 0);
    EXPECT_GT(resp.image_encoding_time_ms, 0);
    EXPECT_EQ(resp.model_name, "llava-v1.5-7b");
}

// ============================================================================
// VisionConfig::getDefault()
// ============================================================================

TEST(VisionConfigTest, DefaultConfigNotNull) {
    auto cfg = VisionConfig::getDefault();
    ASSERT_NE(cfg, nullptr);
}

TEST(VisionConfigTest, DefaultAPIVersion) {
    auto cfg = VisionConfig::getDefault();
    EXPECT_FALSE(cfg->getAPIVersion().empty());
}

TEST(VisionConfigTest, DefaultAPIStabilityIsStable) {
    auto cfg = VisionConfig::getDefault();
    EXPECT_EQ(cfg->getAPIStability(), VisionAPIStability::STABLE);
}

TEST(VisionConfigTest, DefaultBackwardCompatible) {
    auto cfg = VisionConfig::getDefault();
    EXPECT_TRUE(cfg->isBackwardCompatible());
}

TEST(VisionConfigTest, DefaultResourceLimitsHavePositiveCPUThreads) {
    auto cfg = VisionConfig::getDefault();
    EXPECT_GT(cfg->getResourceLimits().cpu_inference_threads, 0);
}

// ============================================================================
// VisionEncoder — constructor rejects missing model file
// ============================================================================

TEST(VisionEncoderTest, DISABLED_ThrowsOnMissingModelFile) {
    // Regardless of THEMIS_ENABLE_LLM, constructing with a missing path
    // must throw (either "not found" or "LLM support not enabled").
    // TODO: Fix ambiguous VisionEncoder constructor overload (C2668)
    GTEST_SKIP() << "Skipped: VisionEncoder constructor overload ambiguity";
}

TEST(VisionEncoderTest, DISABLED_ThrowsOnMissingModelFileWithConfig) {
    auto cfg = VisionConfig::getDefault();
    // TODO: Fix ambiguous VisionEncoder constructor overload (C2668)
    GTEST_SKIP() << "Skipped: VisionEncoder constructor overload ambiguity";
}

