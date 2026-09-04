/**
 * @file test_llm_vision_integration.cpp
 * @brief Unit tests for multi-modal InferenceRequest, LLMCapabilities
 *        (supports_multimodal), and integration of image_paths with the
 *        standard inference API.  No model files required.
 */

#include <gtest/gtest.h>
#include <sstream>
#include "llm/llm_plugin_interface.h"
#include "llm/vision_encoder.h"

using namespace themis::llm;

// ============================================================================
// LLMCapabilities — supports_multimodal
// ============================================================================

TEST(LLMCapabilitiesMultiModal, DefaultIsFalse) {
    LLMCapabilities caps;
    EXPECT_FALSE(caps.supports_multimodal);
}

TEST(LLMCapabilitiesMultiModal, CanBeSetTrue) {
    LLMCapabilities caps;
    caps.supports_multimodal = true;
    EXPECT_TRUE(caps.supports_multimodal);
}

TEST(LLMCapabilitiesMultiModal, IndependentOfOtherFlags) {
    LLMCapabilities caps;
    caps.supports_multimodal = true;
    // Setting multimodal does not affect other capability flags
    EXPECT_FALSE(caps.supports_lora);
    EXPECT_FALSE(caps.supports_streaming);
    EXPECT_FALSE(caps.gpu_accelerated);
}

// ============================================================================
// InferenceRequest — image_paths field
// ============================================================================

TEST(InferenceRequestMultiModal, DefaultImagePathsEmpty) {
    InferenceRequest req;
    EXPECT_TRUE(req.image_paths.empty());
}

TEST(InferenceRequestMultiModal, CanAttachSingleImage) {
    InferenceRequest req;
    req.prompt = "Describe this image.";
    req.image_paths.push_back("/path/to/image.jpg");

    EXPECT_EQ(req.image_paths.size(), 1u);
    EXPECT_EQ(req.image_paths[0], "/path/to/image.jpg");
}

TEST(InferenceRequestMultiModal, CanAttachMultipleImages) {
    InferenceRequest req;
    req.prompt = "Compare these two photos.";
    req.image_paths = {"/img/before.jpg", "/img/after.jpg"};

    EXPECT_EQ(req.image_paths.size(), 2u);
    EXPECT_EQ(req.image_paths[0], "/img/before.jpg");
    EXPECT_EQ(req.image_paths[1], "/img/after.jpg");
}

TEST(InferenceRequestMultiModal, ImagePathsAreIndependentOfPrompt) {
    InferenceRequest req;
    req.prompt = "What do you see?";
    req.image_paths = {"/photo.png"};
    req.max_tokens  = 128;

    EXPECT_EQ(req.prompt, "What do you see?");
    EXPECT_EQ(req.max_tokens, 128);
    EXPECT_EQ(req.image_paths.size(), 1u);
}

TEST(InferenceRequestMultiModal, TextOnlyRequestHasEmptyImagePaths) {
    InferenceRequest req;
    req.prompt = "Translate this text.";
    req.image_paths.clear();

    EXPECT_TRUE(req.image_paths.empty());
    EXPECT_FALSE(req.prompt.empty());
}

// ============================================================================
// VisionRequest / InferenceRequest type compatibility
// ============================================================================

TEST(MultiModalTypeTest, VisionRequestImagePathsMatchInferenceRequestImagePaths) {
    // Verify that image_paths from InferenceRequest can populate a VisionRequest
    InferenceRequest inf_req;
    inf_req.prompt      = "Describe the scene.";
    inf_req.image_paths = {"/scene1.jpg", "/scene2.png"};

    VisionRequest vis_req;
    vis_req.text_prompt = inf_req.prompt;
    vis_req.image_paths = inf_req.image_paths;

    EXPECT_EQ(vis_req.text_prompt, inf_req.prompt);
    ASSERT_EQ(vis_req.image_paths.size(), inf_req.image_paths.size());
    for (size_t i = 0; i < inf_req.image_paths.size(); ++i) {
        EXPECT_EQ(vis_req.image_paths[i], inf_req.image_paths[i]);
    }
}

TEST(MultiModalTypeTest, GenerationParamsPropagate) {
    InferenceRequest inf_req;
    inf_req.prompt      = "What is in this picture?";
    inf_req.max_tokens  = 256;
    inf_req.temperature = 0.5f;
    inf_req.top_p       = 0.8f;
    inf_req.top_k       = 30;
    inf_req.image_paths = {"/test.jpg"};

    VisionRequest vis_req;
    vis_req.text_prompt = inf_req.prompt;
    vis_req.image_paths = inf_req.image_paths;
    vis_req.max_tokens  = inf_req.max_tokens;
    vis_req.temperature = inf_req.temperature;
    vis_req.top_p       = inf_req.top_p;
    vis_req.top_k       = inf_req.top_k;

    EXPECT_EQ(vis_req.max_tokens,  256);
    EXPECT_FLOAT_EQ(vis_req.temperature, 0.5f);
    EXPECT_FLOAT_EQ(vis_req.top_p, 0.8f);
    EXPECT_EQ(vis_req.top_k, 30);
}

// ============================================================================
// Cache key uniqueness — requests with different images must not share a key
// ============================================================================

// Mirrors InferenceEngineEnhanced::generateCacheKey() input construction
// (prompt | max_tokens | temperature | top_p | img:<path>…).
static std::string makeCacheKeyInput(const InferenceRequest& r) {
    std::ostringstream oss;
    oss << r.prompt << "|" << r.max_tokens << "|" << r.temperature << "|" << r.top_p;
    for (const auto& p : r.image_paths) {
      oss << "|img:" << p;
    }
    return oss.str();
}

TEST(MultiModalCacheKeyTest, DifferentImagePathsProduceDifferentKeys) {
    // Two requests: same text, different images → different cache keys.
    InferenceRequest req_a;
    req_a.prompt      = "Describe the image.";
    req_a.max_tokens  = 256;
    req_a.temperature = 0.7f;
    req_a.top_p       = 0.9f;
    req_a.image_paths = {"/photo_a.jpg"};

    InferenceRequest req_b = req_a;
    req_b.image_paths = {"/photo_b.jpg"};

    EXPECT_NE(makeCacheKeyInput(req_a), makeCacheKeyInput(req_b));
}

TEST(MultiModalCacheKeyTest, SameImagePathsSameKey) {
    InferenceRequest req_a;
    req_a.prompt      = "Describe the image.";
    req_a.max_tokens  = 256;
    req_a.temperature = 0.7f;
    req_a.top_p       = 0.9f;
    req_a.image_paths = {"/same.jpg"};

    InferenceRequest req_b = req_a;  // Exact copy → same key.

    EXPECT_EQ(makeCacheKeyInput(req_a), makeCacheKeyInput(req_b));
}

TEST(MultiModalCacheKeyTest, TextOnlyVsMultiModalDiffer) {
    InferenceRequest text_req;
    text_req.prompt      = "What is 2+2?";
    text_req.max_tokens  = 128;
    text_req.temperature = 0.5f;
    text_req.top_p       = 0.9f;

    InferenceRequest mm_req = text_req;
    mm_req.image_paths = {"/diagram.png"};

    EXPECT_NE(makeCacheKeyInput(text_req), makeCacheKeyInput(mm_req));
}

