#include <gtest/gtest.h>
#include "llm/llamacpp_inference_engine.h"
#include <stdexcept>

using namespace themis::llm;

/**
 * @file test_inference_error_handling.cpp
 * @brief Tests for LLM inference engine error handling
 * 
 * Test Coverage:
 * - No stub responses in production code
 * - Proper error handling for null handles
 * - Fail-fast behavior
 * - Clear error messages
 */

class InferenceErrorHandlingTest : public ::testing::Test {
protected:
    void SetUp() override {
        config_.n_ctx = 2048;
        config_.n_batch = 512;
        config_.n_threads = 4;
        config_.n_gpu_layers = 0;
        config_.block_size = 16;
        config_.num_blocks = 256;
    }
    
    LlamaCppInferenceEngine::Config config_;
};

// ===== Null Handle Error Tests =====

TEST_F(InferenceErrorHandlingTest, Infer_ThrowsWhenNoModelLoaded) {
    LlamaCppInferenceEngine engine(config_);
    
    InferenceRequest request;
    request.prompt = "Test prompt";
    request.max_tokens = 100;
    
    // Should throw because no model is loaded
    EXPECT_THROW({
        engine.infer(request);
    }, std::runtime_error) << "Should throw when no model is loaded";
}

TEST_F(InferenceErrorHandlingTest, Infer_ErrorMessageContainsDetails) {
    LlamaCppInferenceEngine engine(config_);
    
    InferenceRequest request;
    request.prompt = "Test prompt";
    request.max_tokens = 100;
    
    // Capture the error message
    try {
        engine.infer(request);
        FAIL() << "Expected exception was not thrown";
    } catch (const std::runtime_error& e) {
        std::string error_msg = e.what();
        
        // Error message should be clear and informative
        EXPECT_TRUE(
            error_msg.find("model") != std::string::npos ||
            error_msg.find("loaded") != std::string::npos
        ) << "Error message should mention model/loaded state: " << error_msg;
    }
}

TEST_F(InferenceErrorHandlingTest, Infer_NoStubResponses) {
    LlamaCppInferenceEngine engine(config_);
    
    InferenceRequest request;
    request.prompt = "Test prompt";
    request.max_tokens = 100;
    
    // Should throw, not return a stub response
    bool threw_exception = false;
    try {
        auto response = engine.infer(request);
        
        // If we get here, check that response is not a stub
        // (though we should throw, so this shouldn't execute)
        EXPECT_TRUE(response.text.empty() || response.text[0] != '[')
            << "Response should not be a placeholder like '[Generated response...'";
        
    } catch (const std::runtime_error& e) {
        threw_exception = true;
    }
    
    EXPECT_TRUE(threw_exception) << "Should throw exception, not return stub response";
}

// ===== Model Loading State Tests =====

TEST_F(InferenceErrorHandlingTest, ModelInfo_EmptyWhenNotLoaded) {
    LlamaCppInferenceEngine engine(config_);
    
    std::string info = engine.getModelInfo();
    
    // Should indicate no model is loaded
    EXPECT_TRUE(
        info.find("No model") != std::string::npos ||
        info.find("not loaded") != std::string::npos ||
        info.empty()
    ) << "Model info should indicate no model loaded: " << info;
}

TEST_F(InferenceErrorHandlingTest, UnloadModel_SafeWhenNotLoaded) {
    LlamaCppInferenceEngine engine(config_);
    
    // Should not crash when unloading non-loaded model
    EXPECT_NO_THROW({
        engine.unloadModel();
    }) << "Unload should be safe when no model is loaded";
}

// ===== Integration Tests =====

TEST_F(InferenceErrorHandlingTest, LoadModelFromThemisDB_FailsGracefully) {
    LlamaCppInferenceEngine engine(config_);
    
    // Try to load from ThemisDB without proper setup
    // Should fail gracefully, not crash
    EXPECT_FALSE(engine.loadModelFromThemisDB("nonexistent://model"))
        << "Should return false for invalid model URN";
}

TEST_F(InferenceErrorHandlingTest, LoadModel_FailsGracefullyForNonexistentFile) {
    LlamaCppInferenceEngine engine(config_);
    
    // Try to load nonexistent file
    // Should fail gracefully, not crash
    bool loaded = engine.loadModel("/nonexistent/model.gguf", "test-model");
    
    EXPECT_FALSE(loaded) << "Should return false for nonexistent file";
}

// ===== Statistics Tests =====

TEST_F(InferenceErrorHandlingTest, Stats_InitializedToZero) {
    LlamaCppInferenceEngine engine(config_);
    
    auto stats = engine.getStats();
    
    EXPECT_EQ(stats.total_tokens_processed, 0);
    EXPECT_EQ(stats.total_requests, 0);
    EXPECT_EQ(stats.total_failures, 0);
}

TEST_F(InferenceErrorHandlingTest, Stats_NotUpdatedOnFailedInference) {
    LlamaCppInferenceEngine engine(config_);
    
    InferenceRequest request;
    request.prompt = "Test";
    request.max_tokens = 10;
    
    // Try inference (will fail)
    try {
        engine.infer(request);
    } catch (...) {
        // Expected
    }
    
    auto stats = engine.getStats();
    
    // Stats should not reflect successful inference
    EXPECT_EQ(stats.total_tokens_processed, 0)
        << "Should not count tokens for failed inference";
}

// ===== Fail-Fast Behavior Tests =====

TEST_F(InferenceErrorHandlingTest, FailFast_ImmediateErrorOnNullHandle) {
    LlamaCppInferenceEngine engine(config_);
    
    InferenceRequest request;
    request.prompt = "Test";
    
    // Should fail immediately, not try to generate stub response
    auto start = std::chrono::steady_clock::now();
    
    try {
        engine.infer(request);
        FAIL() << "Should have thrown exception";
    } catch (const std::runtime_error&) {
        // Expected
    }
    
    auto end = std::chrono::steady_clock::now();
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    // Should fail very quickly (< 100ms), not wait to generate fake response
    EXPECT_LT(duration_ms, 100) << "Should fail fast without generating stub";
}

// ===== Error Message Quality Tests =====

TEST_F(InferenceErrorHandlingTest, ErrorMessages_AreDescriptive) {
    LlamaCppInferenceEngine engine(config_);
    
    InferenceRequest request;
    request.prompt = "Test";
    
    try {
        engine.infer(request);
        FAIL() << "Should have thrown";
    } catch (const std::runtime_error& e) {
        std::string msg = e.what();
        
        // Should not be a generic error message
        EXPECT_GT(msg.length(), 10) << "Error message too short: " << msg;
        
        // Should mention the problem
        EXPECT_FALSE(msg.empty()) << "Error message should not be empty";
    }
}

TEST_F(InferenceErrorHandlingTest, NoModelHandle_SpecificError) {
    LlamaCppInferenceEngine engine(config_);
    
    InferenceRequest request;
    request.prompt = "Test";
    
    try {
        engine.infer(request);
        FAIL() << "Should throw";
    } catch (const std::runtime_error& e) {
        std::string msg = e.what();
        
        // Should mention model or handle issue
        bool mentions_model_issue = 
            msg.find("model") != std::string::npos ||
            msg.find("handle") != std::string::npos ||
            msg.find("loaded") != std::string::npos ||
            msg.find("initialized") != std::string::npos;
        
        EXPECT_TRUE(mentions_model_issue) 
            << "Error should mention model/handle issue: " << msg;
    }
}
