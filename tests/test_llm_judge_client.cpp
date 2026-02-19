/**
 * @file test_llm_judge_client.cpp
 * @brief Unit tests for LLM Judge Client
 */

#include "rag/llm_judge_client.h"
#include "llm/inference_engine_enhanced.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace themis::rag::judge;
using namespace themis::llm;

// ============================================================================
// Mock LLM Plugin for Testing
// ============================================================================

class MockLLMPlugin : public ILLMPlugin {
public:
    explicit MockLLMPlugin(const std::string& model_id, size_t latency_ms = 10)
        : model_id_(model_id), latency_ms_(latency_ms) {}
    
    std::string getModelId() const override {
        return model_id_;
    }
    
    InferenceResponse generate(const InferenceRequest& request) override {
        // Simulate latency
        std::this_thread::sleep_for(std::chrono::milliseconds(latency_ms_));
        
        InferenceResponse response;
        response.text = "{\"score\": 4, \"reasoning\": \"Good answer\"}";
        response.tokens_generated = 20;
        response.total_tokens = 50;
        return response;
    }
    
    bool isLoaded() const override { return true; }
    
private:
    std::string model_id_;
    size_t latency_ms_;
};

// ============================================================================
// LLM Judge Client Tests
// ============================================================================

class LLMJudgeClientTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create inference engine
        InferenceEngineEnhanced::Config engine_cfg;
        engine_cfg.enable_context_caching = true;
        engine_cfg.max_cache_entries = 100;
        engine = std::make_shared<InferenceEngineEnhanced>(engine_cfg);
        
        // Register mock model
        auto mock_plugin = std::make_shared<MockLLMPlugin>("test_model", 10);
        engine->registerModel("test_model", mock_plugin);
        engine->start();
        
        // Create client with engine
        client = std::make_unique<LLMJudgeClient>(engine);
    }
    
    void TearDown() override {
        if (engine) {
            engine->shutdown();
        }
    }
    
    std::shared_ptr<InferenceEngineEnhanced> engine;
    std::unique_ptr<LLMJudgeClient> client;
};

TEST_F(LLMJudgeClientTest, BasicConstruction) {
    EXPECT_TRUE(client->isReady());
    EXPECT_EQ(client->getEngine(), engine);
}

TEST_F(LLMJudgeClientTest, GenerateText) {
    std::string prompt = "Evaluate the quality of this answer.";
    
    auto response = client->generate(prompt);
    
    EXPECT_FALSE(response.text.empty());
    EXPECT_GT(response.generation_time_ms, 0.0);
}

TEST_F(LLMJudgeClientTest, GenerateMultipleSamples) {
    std::string prompt = "Evaluate the quality of this answer.";
    size_t num_samples = 3;
    
    auto responses = client->generateMultiple(prompt, num_samples);
    
    EXPECT_EQ(responses.size(), num_samples);
    for (const auto& response : responses) {
        EXPECT_FALSE(response.text.empty());
    }
}

TEST_F(LLMJudgeClientTest, ExtractTokenProbabilities) {
    std::string prompt = "Rate this answer on a scale of 1-5.";
    std::vector<std::string> score_tokens = {"1", "2", "3", "4", "5"};
    
    auto probs = client->extractTokenProbabilities(prompt, score_tokens);
    
    EXPECT_EQ(probs.size(), score_tokens.size());
    
    // Probabilities should sum to approximately 1.0
    double sum = 0.0;
    for (double p : probs) {
        EXPECT_GE(p, 0.0);
        EXPECT_LE(p, 1.0);
        sum += p;
    }
    EXPECT_NEAR(sum, 1.0, 0.01);
}

TEST_F(LLMJudgeClientTest, ConfigurationUpdate) {
    LLMJudgeClient::Config new_config;
    new_config.temperature = 0.5;
    new_config.max_tokens = 2048;
    
    client->setConfig(new_config);
    
    auto retrieved_config = client->getConfig();
    EXPECT_DOUBLE_EQ(retrieved_config.temperature, 0.5);
    EXPECT_EQ(retrieved_config.max_tokens, 2048);
}

TEST_F(LLMJudgeClientTest, CacheFunctionality) {
    // Generate with same prompt multiple times
    std::string prompt = "Test caching with this prompt.";
    
    auto response1 = client->generate(prompt);
    auto time1 = response1.generation_time_ms;
    
    auto response2 = client->generate(prompt);
    auto time2 = response2.generation_time_ms;
    
    // Second call might be faster due to caching (if implemented)
    // For now, just verify both succeed
    EXPECT_FALSE(response1.text.empty());
    EXPECT_FALSE(response2.text.empty());
}

TEST_F(LLMJudgeClientTest, ClientWithoutEngine) {
    // Create client without engine
    LLMJudgeClient standalone_client;
    
    EXPECT_FALSE(standalone_client.isReady());
    
    // Should return empty/default responses
    auto response = standalone_client.generate("test prompt");
    EXPECT_FALSE(response.text.empty());  // Should return "{}" as fallback
}

TEST_F(LLMJudgeClientTest, EngineInjection) {
    // Create client without engine
    LLMJudgeClient standalone_client;
    EXPECT_FALSE(standalone_client.isReady());
    
    // Inject engine
    standalone_client.setEngine(engine);
    EXPECT_TRUE(standalone_client.isReady());
    
    // Now generate should work
    auto response = standalone_client.generate("test prompt");
    EXPECT_FALSE(response.text.empty());
}

TEST_F(LLMJudgeClientTest, PrewarmCache) {
    std::vector<std::string> common_prompts = {
        "Evaluate faithfulness",
        "Evaluate relevance",
        "Evaluate coherence"
    };
    
    // Should not throw
    EXPECT_NO_THROW(client->prewarmCache(common_prompts));
}

TEST_F(LLMJudgeClientTest, ClearCache) {
    // Generate some responses to populate cache
    client->generate("test prompt 1");
    client->generate("test prompt 2");
    
    // Clear cache - should not throw
    EXPECT_NO_THROW(client->clearCache());
}

// ============================================================================
// Performance Tests
// ============================================================================

TEST_F(LLMJudgeClientTest, PerformanceUnder500ms) {
    std::string prompt = "Quick evaluation test.";
    
    auto start = std::chrono::steady_clock::now();
    auto response = client->generate(prompt);
    auto end = std::chrono::steady_clock::now();
    
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        end - start
    ).count();
    
    // Should be fast (mock has 10ms latency + overhead)
    EXPECT_LT(duration_ms, 500);  // Well under target
    EXPECT_LT(response.generation_time_ms, 500.0);
}

TEST_F(LLMJudgeClientTest, BatchPerformance) {
    std::string prompt = "Batch evaluation test.";
    size_t num_samples = 5;
    
    auto start = std::chrono::steady_clock::now();
    auto responses = client->generateMultiple(prompt, num_samples);
    auto end = std::chrono::steady_clock::now();
    
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        end - start
    ).count();
    
    EXPECT_EQ(responses.size(), num_samples);
    // With 10ms per sample, 5 samples should be ~50ms + overhead
    EXPECT_LT(duration_ms, 200);
}
