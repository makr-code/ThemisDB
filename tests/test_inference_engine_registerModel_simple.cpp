#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

#include "llm/inference_engine_enhanced.h"
#include "threading/shared_worker_pool.h"

/**
 * Simplified focused tests for InferenceEngineEnhanced::registerModel fail-closed guard
 * 
 * Strategy: Test the guard via public API (getAvailableModels) without mocking
 * the complex ILLMPlugin interface. Verifies that empty model_id is rejected.
 */

using namespace themis::llm;
using namespace themis::threading;

class InferenceEngineEnhancedRegisterModelTest : public ::testing::Test {
protected:
    std::shared_ptr<InferenceEngineEnhanced> engine_;
    std::shared_ptr<SharedWorkerPool> pool_;

    void SetUp() override {
        // Minimal config for test
        InferenceEngineEnhanced::Config config;
        config.max_batch_size = 1;
        config.max_sequence_length = 512;
        config.enable_speculative_decoding = false;

        // Create shared worker pool
        pool_ = std::make_shared<SharedWorkerPool>(2);  // 2 worker threads

        // Create engine
        engine_ = std::make_shared<InferenceEngineEnhanced>(config, pool_);
    }

    void TearDown() override {
        engine_.reset();
        pool_.reset();
    }
};

/**
 * Test 1: Reject empty model_id
 */
TEST_F(InferenceEngineEnhancedRegisterModelTest, RejectsEmptyModelId) {
    auto models_before = engine_->getAvailableModels();
    size_t size_before = models_before.size();

    // Try to register with empty model_id and null plugin
    engine_->registerModel("", nullptr);

    auto models_after = engine_->getAvailableModels();
    size_t size_after = models_after.size();

    // Guard rejected empty model_id
    EXPECT_EQ(size_before, size_after);
}

/**
 * Test 2: Accept valid model_id (with null plugin for testing)
 * 
 * Note: In production, plugin must be non-null. For this fail-closed guard test,
 * we're only verifying that the guard rejects empty model_id, not that the plugin
 * is valid. Real production code will fail elsewhere if plugin is null.
 */
TEST_F(InferenceEngineEnhancedRegisterModelTest, AcceptsValidModelId) {
    auto models_before = engine_->getAvailableModels();
    size_t size_before = models_before.size();

    // Register with valid model_id (null plugin is fine for this guard test)
    engine_->registerModel("test-model-1", nullptr);

    auto models_after = engine_->getAvailableModels();
    size_t size_after = models_after.size();

    // Guard accepted (valid model_id)
    EXPECT_EQ(size_after, size_before + 1);
    
    // Verify model was registered
    bool found = false;
    for (const auto& m : models_after) {
        if (m == "test-model-1") {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}

/**
 * Test 3: Multiple models with valid IDs can be registered
 */
TEST_F(InferenceEngineEnhancedRegisterModelTest, MultipleModelsCanBeRegistered) {
    auto size_before = engine_->getAvailableModels().size();

    engine_->registerModel("model-1", nullptr);
    engine_->registerModel("model-2", nullptr);
    engine_->registerModel("model-3", nullptr);

    auto models_after = engine_->getAvailableModels();
    EXPECT_EQ(models_after.size(), size_before + 3);
}

/**
 * Test 4: Whitespace-only model_id is treated as valid (contains non-empty string)
 * 
 * Note: Real production code might want stricter validation (trim whitespace),
 * but the fail-closed guard in registerModel only checks .empty().
 */
TEST_F(InferenceEngineEnhancedRegisterModelTest, WhitespaceModelIdIsAccepted) {
    auto size_before = engine_->getAvailableModels().size();

    // Whitespace is technically non-empty
    engine_->registerModel("   ", nullptr);

    auto models_after = engine_->getAvailableModels();
    EXPECT_EQ(models_after.size(), size_before + 1);
}

/**
 * Test 5: Guard is independent across multiple calls
 */
TEST_F(InferenceEngineEnhancedRegisterModelTest, FailClosedGuardsAreIndependent) {
    // First invalid (empty)
    engine_->registerModel("", nullptr);
    
    // Valid
    engine_->registerModel("valid-model", nullptr);
    
    // Second invalid (empty)
    engine_->registerModel("", nullptr);

    auto models = engine_->getAvailableModels();
    
    // Only valid model should be registered
    EXPECT_EQ(models.size(), 1);
    
    bool found = false;
    for (const auto& m : models) {
        if (m == "valid-model") {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}
