#include <gtest/gtest.h>
#include "llm/vision_encoder.h"
#include <filesystem>
#include <fstream>

using namespace themis::llm;

class VisionEncoderTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Test will be skipped if LLM support is not enabled
        // or if CLIP model is not available
    }
    
    void TearDown() override {
        // Cleanup
    }
};

TEST_F(VisionEncoderTest, ConstructorWithoutLLMSupport) {
    // This test verifies that the encoder throws appropriate error
    // when LLM support is not enabled
    
#ifndef THEMIS_ENABLE_LLM
    EXPECT_THROW({
        VisionEncoder encoder("/nonexistent/model.gguf");
    }, std::runtime_error);
#else
    // When LLM is enabled, should throw file not found error
    EXPECT_THROW({
        VisionEncoder encoder("/nonexistent/model.gguf");
    }, std::runtime_error);
#endif
}

TEST_F(VisionEncoderTest, VisionRequestStructure) {
    // Test VisionRequest structure
    VisionRequest request;
    request.text_prompt = "What's in this image?";
    request.image_path = "/path/to/image.jpg";
    request.max_tokens = 256;
    request.temperature = 0.7f;
    
    EXPECT_EQ(request.text_prompt, "What's in this image?");
    EXPECT_EQ(request.image_path, "/path/to/image.jpg");
    EXPECT_EQ(request.max_tokens, 256);
    EXPECT_FLOAT_EQ(request.temperature, 0.7f);
}

TEST_F(VisionEncoderTest, VisionResponseStructure) {
    // Test VisionResponse structure
    VisionResponse response;
    response.success = true;
    response.text = "This is a test response";
    response.tokens_generated = 10;
    response.inference_time_ms = 100;
    response.image_encoding_time_ms = 50;
    
    EXPECT_TRUE(response.success);
    EXPECT_EQ(response.text, "This is a test response");
    EXPECT_EQ(response.tokens_generated, 10);
    EXPECT_EQ(response.inference_time_ms, 100);
    EXPECT_EQ(response.image_encoding_time_ms, 50);
}

TEST_F(VisionEncoderTest, MultipleImagesRequest) {
    // Test vision request with multiple images
    VisionRequest request;
    request.text_prompt = "Compare these images";
    request.image_paths = {"/path/to/image1.jpg", "/path/to/image2.jpg"};
    request.max_tokens = 512;
    
    EXPECT_EQ(request.image_paths.size(), 2);
    EXPECT_EQ(request.text_prompt, "Compare these images");
    EXPECT_EQ(request.max_tokens, 512);
}

// This test requires actual CLIP model file to run
// It will be skipped in CI unless model is available
TEST_F(VisionEncoderTest, DISABLED_EncodeImageWithRealModel) {
    // This test is disabled by default as it requires:
    // 1. THEMIS_ENABLE_LLM=ON
    // 2. Actual CLIP model file
    // 3. Test image file
    
    const std::string model_path = "/models/mmproj-model-f16.gguf";
    const std::string image_path = "/test/images/test.jpg";
    
    if (!std::filesystem::exists(model_path)) {
        GTEST_SKIP() << "CLIP model not found at: " << model_path;
    }
    
    if (!std::filesystem::exists(image_path)) {
        GTEST_SKIP() << "Test image not found at: " << image_path;
    }
    
#ifdef THEMIS_ENABLE_LLM
    VisionEncoder encoder(model_path);
    
    EXPECT_TRUE(encoder.isReady());
    EXPECT_GT(encoder.getEmbeddingDimension(), 0);
    EXPECT_GT(encoder.getNumPatches(), 0);
    
    auto embeddings = encoder.encodeImage(image_path);
    
    EXPECT_GT(embeddings.size(), 0);
    EXPECT_EQ(embeddings.size(), encoder.getTotalEmbeddingSize());
    
    // Check embeddings are normalized (L2 norm should be ~1.0)
    float sum = 0.0f;
    for (float v : embeddings) {
        sum += v * v;
    }
    float norm = std::sqrt(sum);
    EXPECT_NEAR(norm, 1.0f, 0.1f);
#else
    GTEST_SKIP() << "LLM support not enabled (THEMIS_ENABLE_LLM=OFF)";
#endif
}

TEST_F(VisionEncoderTest, ErrorHandling) {
    // Test error handling with non-existent model
    EXPECT_THROW({
        VisionEncoder encoder("/definitely/does/not/exist.gguf");
    }, std::runtime_error);
}

TEST_F(VisionEncoderTest, MoveSemantics) {
    // Test that VisionEncoder supports move semantics
    // This is important for efficient resource management
    
#ifdef THEMIS_ENABLE_LLM
    // Note: This will throw because model doesn't exist, but we're testing
    // that the move constructor/assignment are available
    try {
        VisionEncoder encoder1("/test/model.gguf");
        VisionEncoder encoder2(std::move(encoder1));
        
        // encoder1 should now be in moved-from state
        EXPECT_FALSE(encoder1.isReady());
        
    } catch (const std::runtime_error&) {
        // Expected - model file doesn't exist
        SUCCEED();
    }
#endif
}
