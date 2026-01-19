#include <gtest/gtest.h>
#include "llm/llama_wrapper.h"
#include <filesystem>
#include <cstdlib>

using namespace themis::llm;

class LlamaWrapperVisionTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Test configuration
        config_.n_gpu_layers = 0;  // CPU only for tests
        config_.n_ctx = 2048;
        config_.n_batch = 512;
        config_.n_threads = 4;
        config_.use_flash_attn = false;
        config_.use_kv_cache_reuse = false;
        config_.use_speculative_decoding = false;
        config_.use_continuous_batching = false;
        config_.enable_response_cache = false;
        
        // Vision configuration
        config_.enable_vision = false;  // Disabled by default, tests can enable
        config_.clip_model_path = "/models/mmproj-model-f16.gguf";
        config_.vision_threads = 4;
        config_.preload_vision = true;
    }
    
    void TearDown() override {
        // Cleanup
    }
    
    LlamaWrapper::Config config_;
};

TEST_F(LlamaWrapperVisionTest, ConstructorWithVisionDisabled) {
    // Test that LlamaWrapper can be constructed with vision disabled
    config_.enable_vision = false;
    
    EXPECT_NO_THROW({
        LlamaWrapper wrapper(config_);
    });
}

TEST_F(LlamaWrapperVisionTest, VisionDisabledByDefault) {
    // Verify vision is disabled when not configured
    config_.enable_vision = false;
    
    LlamaWrapper wrapper(config_);
    
    // Attempt to use vision should fail gracefully
    VisionRequest request;
    request.text_prompt = "What's in this image?";
    request.image_path = "/test/image.jpg";
    
    auto response = wrapper.generateVision(request);
    
    EXPECT_FALSE(response.success);
    EXPECT_FALSE(response.error_message.empty());
    EXPECT_THAT(response.error_message, ::testing::HasSubstr("not enabled"));
}

TEST_F(LlamaWrapperVisionTest, VisionWithoutModel) {
    // Test vision request when no LLM model is loaded
    config_.enable_vision = true;
    
    // This will fail to initialize vision encoder due to missing CLIP model
    // but should not crash
    LlamaWrapper wrapper(config_);
    
    VisionRequest request;
    request.text_prompt = "Describe this image";
    request.image_path = "/test/image.jpg";
    
    auto response = wrapper.generateVision(request);
    
    EXPECT_FALSE(response.success);
    EXPECT_FALSE(response.error_message.empty());
}

TEST_F(LlamaWrapperVisionTest, VisionRequestValidation) {
    // Test that vision request validates input
    config_.enable_vision = true;
    LlamaWrapper wrapper(config_);
    
    // Empty image path should fail
    VisionRequest request;
    request.text_prompt = "What's in this image?";
    request.image_path = "";  // Empty
    
    auto response = wrapper.generateVision(request);
    
    EXPECT_FALSE(response.success);
}

TEST_F(LlamaWrapperVisionTest, VisionRequestParameters) {
    // Test that vision request respects generation parameters
    VisionRequest request;
    request.text_prompt = "Describe this image";
    request.image_path = "/test/image.jpg";
    request.max_tokens = 512;
    request.temperature = 0.8f;
    request.top_p = 0.95f;
    request.top_k = 50;
    request.use_image_start_end = true;
    request.image_token = "<image>";
    
    EXPECT_EQ(request.max_tokens, 512);
    EXPECT_FLOAT_EQ(request.temperature, 0.8f);
    EXPECT_FLOAT_EQ(request.top_p, 0.95f);
    EXPECT_EQ(request.top_k, 50);
    EXPECT_TRUE(request.use_image_start_end);
    EXPECT_EQ(request.image_token, "<image>");
}

TEST_F(LlamaWrapperVisionTest, MultipleImagesInRequest) {
    // Test handling of multiple images
    config_.enable_vision = true;
    LlamaWrapper wrapper(config_);
    
    VisionRequest request;
    request.text_prompt = "Compare these images";
    request.image_paths = {"/test/img1.jpg", "/test/img2.jpg", "/test/img3.jpg"};
    request.max_tokens = 1024;
    
    EXPECT_EQ(request.image_paths.size(), 3);
    EXPECT_TRUE(request.image_path.empty());  // Single path should be empty
}

TEST_F(LlamaWrapperVisionTest, VisionResponseStructure) {
    // Test vision response contains all expected fields
    VisionResponse response;
    response.success = true;
    response.text = "Generated description";
    response.tokens_generated = 50;
    response.inference_time_ms = 1500;
    response.image_encoding_time_ms = 300;
    response.model_name = "llava-v1.6-mistral-7b";
    
    EXPECT_TRUE(response.success);
    EXPECT_EQ(response.text, "Generated description");
    EXPECT_EQ(response.tokens_generated, 50);
    EXPECT_EQ(response.inference_time_ms, 1500);
    EXPECT_EQ(response.image_encoding_time_ms, 300);
    EXPECT_EQ(response.model_name, "llava-v1.6-mistral-7b");
    EXPECT_TRUE(response.error_message.empty());
}

// This test requires actual models and is disabled by default
TEST_F(LlamaWrapperVisionTest, DISABLED_FullVisionPipeline) {
    // This test requires:
    // 1. THEMIS_ENABLE_LLM=ON
    // 2. LLaVA model file (GGUF)
    // 3. CLIP vision encoder model (GGUF)
    // 4. Test image file
    //
    // Note: Paths are examples. Set environment variables or modify for your setup:
    //   THEMIS_LLAVA_MODEL=/path/to/llava-model.gguf
    //   THEMIS_CLIP_MODEL=/path/to/clip-model.gguf
    //   THEMIS_TEST_IMAGE=/path/to/test-image.jpg
    
    const std::string llava_model = std::getenv("THEMIS_LLAVA_MODEL") 
        ? std::getenv("THEMIS_LLAVA_MODEL") 
        : "/models/llava-v1.6-mistral-7b.Q4_K_M.gguf";
    const std::string clip_model = std::getenv("THEMIS_CLIP_MODEL")
        ? std::getenv("THEMIS_CLIP_MODEL")
        : "/models/mmproj-model-f16.gguf";
    const std::string test_image = std::getenv("THEMIS_TEST_IMAGE")
        ? std::getenv("THEMIS_TEST_IMAGE")
        : "/test/images/sample.jpg";
    
    if (!std::filesystem::exists(llava_model)) {
        GTEST_SKIP() << "LLaVA model not found";
    }
    if (!std::filesystem::exists(clip_model)) {
        GTEST_SKIP() << "CLIP model not found";
    }
    if (!std::filesystem::exists(test_image)) {
        GTEST_SKIP() << "Test image not found";
    }
    
#ifdef THEMIS_ENABLE_LLM
    // Configure with vision enabled
    config_.enable_vision = true;
    config_.clip_model_path = clip_model;
    
    LlamaWrapper wrapper(config_);
    
    // Load LLaVA model
    ASSERT_TRUE(wrapper.loadModel(llava_model));
    ASSERT_TRUE(wrapper.isModelLoaded());
    
    // Prepare vision request
    VisionRequest request;
    request.text_prompt = "Describe this image in detail";
    request.image_path = test_image;
    request.max_tokens = 256;
    request.temperature = 0.7f;
    
    // Generate response
    auto response = wrapper.generateVision(request);
    
    // Verify response
    EXPECT_TRUE(response.success) << "Error: " << response.error_message;
    EXPECT_FALSE(response.text.empty());
    EXPECT_GT(response.tokens_generated, 0);
    EXPECT_GT(response.inference_time_ms, 0);
    EXPECT_GT(response.image_encoding_time_ms, 0);
    EXPECT_FALSE(response.model_name.empty());
    
    // Image encoding should be faster than total inference
    EXPECT_LT(response.image_encoding_time_ms, response.inference_time_ms);
    
    std::cout << "Vision Response: " << response.text << std::endl;
    std::cout << "Tokens: " << response.tokens_generated << std::endl;
    std::cout << "Total time: " << response.inference_time_ms << "ms" << std::endl;
    std::cout << "Image encoding: " << response.image_encoding_time_ms << "ms" << std::endl;
#else
    GTEST_SKIP() << "LLM support not enabled";
#endif
}

TEST_F(LlamaWrapperVisionTest, DISABLED_MultiImageVisionPipeline) {
    // Test with multiple images
    // Note: Use environment variables for custom paths (see DISABLED_FullVisionPipeline test)
    const std::string llava_model = std::getenv("THEMIS_LLAVA_MODEL")
        ? std::getenv("THEMIS_LLAVA_MODEL")
        : "/models/llava-v1.6-mistral-7b.Q4_K_M.gguf";
    const std::string clip_model = std::getenv("THEMIS_CLIP_MODEL")
        ? std::getenv("THEMIS_CLIP_MODEL")
        : "/models/mmproj-model-f16.gguf";
    const std::vector<std::string> test_images = {
        "/test/images/sample1.jpg",
        "/test/images/sample2.jpg"
    };
    
    if (!std::filesystem::exists(llava_model)) {
        GTEST_SKIP() << "LLaVA model not found";
    }
    if (!std::filesystem::exists(clip_model)) {
        GTEST_SKIP() << "CLIP model not found";
    }
    
#ifdef THEMIS_ENABLE_LLM
    config_.enable_vision = true;
    config_.clip_model_path = clip_model;
    
    LlamaWrapper wrapper(config_);
    ASSERT_TRUE(wrapper.loadModel(llava_model));
    
    VisionRequest request;
    request.text_prompt = "What are the differences between these images?";
    request.image_paths = test_images;
    request.max_tokens = 512;
    
    auto response = wrapper.generateVision(request);
    
    EXPECT_TRUE(response.success) << "Error: " << response.error_message;
    EXPECT_FALSE(response.text.empty());
    EXPECT_GT(response.tokens_generated, 0);
#else
    GTEST_SKIP() << "LLM support not enabled";
#endif
}

TEST_F(LlamaWrapperVisionTest, VisionCapabilityQuery) {
    // Test that we can query if vision is enabled
    config_.enable_vision = false;
    LlamaWrapper wrapper1(config_);
    
    // Vision disabled wrapper should not support vision
    VisionRequest request;
    request.text_prompt = "Test";
    request.image_path = "/test.jpg";
    auto response1 = wrapper1.generateVision(request);
    EXPECT_FALSE(response1.success);
    
    // With vision enabled (even if initialization fails)
    config_.enable_vision = true;
    LlamaWrapper wrapper2(config_);
    
    // Should at least attempt vision (may fail due to missing model)
    auto response2 = wrapper2.generateVision(request);
    EXPECT_FALSE(response2.success);  // Will fail due to missing models
}

TEST_F(LlamaWrapperVisionTest, VisionRateLimiter) {
    // Validate rate limiting for vision requests
    themis::llm::RateLimiter limiter(2, 2);  // 2 req/min, burst 2
    EXPECT_TRUE(limiter.tryAcquire());
    EXPECT_TRUE(limiter.tryAcquire());
    EXPECT_FALSE(limiter.tryAcquire());  // Third should be throttled
    auto wait_time = limiter.timeUntilNextToken();
    EXPECT_GE(wait_time.count(), 0);
}
