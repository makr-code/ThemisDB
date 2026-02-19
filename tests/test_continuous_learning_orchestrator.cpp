/**
 * @file test_continuous_learning_orchestrator.cpp
 * @brief Unit tests for Continuous Learning Orchestrator
 */

#include <gtest/gtest.h>
#include "rag/continuous_learning_orchestrator.h"
#include <thread>
#include <chrono>

using namespace themis::rag::learning;

class ContinuousLearningOrchestratorTest : public ::testing::Test {
protected:
    void SetUp() override {
        config_.min_feedback_samples = 10;
        config_.min_accuracy_drop = 0.05;
        config_.retraining_interval = std::chrono::hours(1);
        config_.enable_ab_testing = true;
        config_.ab_test_traffic_split = 0.1;
        config_.min_ab_samples = 30;
        config_.min_improvement_threshold = 0.02;
        config_.learning_loop_interval = std::chrono::seconds(1);
        
        orchestrator_ = std::make_unique<ContinuousLearningOrchestrator>(config_);
    }
    
    void TearDown() override {
        if (orchestrator_) {
            orchestrator_->stopLearningLoop();
        }
    }
    
    ContinuousLearningConfig config_;
    std::unique_ptr<ContinuousLearningOrchestrator> orchestrator_;
};

TEST_F(ContinuousLearningOrchestratorTest, Construction) {
    EXPECT_NE(orchestrator_, nullptr);
    
    auto stats = orchestrator_->getStats();
    EXPECT_EQ(stats.total_interactions_logged, 0);
    EXPECT_EQ(stats.lora_retraining_count, 0);
}

TEST_F(ContinuousLearningOrchestratorTest, ComponentRegistration) {
    orchestrator_->registerLoRAAdapter("adapter1", "Test adapter");
    orchestrator_->registerRetrievalSystem("retrieval1");
    orchestrator_->registerPromptSystem("prompt1");
    orchestrator_->registerKnowledgeGapDetector("detector1");
    
    // No direct way to verify, but should not crash
    SUCCEED();
}

TEST_F(ContinuousLearningOrchestratorTest, LogInteraction) {
    Interaction interaction;
    interaction.interaction_id = "int_001";
    interaction.timestamp = std::chrono::system_clock::now();
    interaction.query = "What is machine learning?";
    interaction.generated_answer = "Machine learning is...";
    interaction.confidence_score = 0.85;
    interaction.user_feedback = FeedbackType::POSITIVE;
    
    orchestrator_->logInteraction(interaction);
    
    auto stats = orchestrator_->getStats();
    EXPECT_EQ(stats.total_interactions_logged, 1);
}

TEST_F(ContinuousLearningOrchestratorTest, LogInteractionBatch) {
    std::vector<Interaction> interactions;
    
    for (int i = 0; i < 5; i++) {
        Interaction interaction;
        interaction.interaction_id = "int_" + std::to_string(i);
        interaction.timestamp = std::chrono::system_clock::now();
        interaction.query = "Query " + std::to_string(i);
        interaction.generated_answer = "Answer " + std::to_string(i);
        interaction.confidence_score = 0.8 + i * 0.02;
        interaction.user_feedback = (i % 2 == 0) ? FeedbackType::POSITIVE : FeedbackType::NEGATIVE;
        
        interactions.push_back(interaction);
    }
    
    orchestrator_->logInteractionBatch(interactions);
    
    auto stats = orchestrator_->getStats();
    EXPECT_EQ(stats.total_interactions_logged, 5);
}

TEST_F(ContinuousLearningOrchestratorTest, AccuracyTracking) {
    // Log interactions with feedback
    for (int i = 0; i < 10; i++) {
        Interaction interaction;
        interaction.interaction_id = "int_" + std::to_string(i);
        interaction.timestamp = std::chrono::system_clock::now();
        interaction.query = "Test query";
        interaction.generated_answer = "Test answer";
        
        // 80% positive feedback
        if (i < 8) {
            interaction.user_feedback = FeedbackType::POSITIVE;
        } else {
            interaction.user_feedback = FeedbackType::NEGATIVE;
        }
        
        orchestrator_->logInteraction(interaction);
    }
    
    auto stats = orchestrator_->getStats();
    EXPECT_GT(stats.current_accuracy, 0.5);  // Should reflect positive feedback
}

TEST_F(ContinuousLearningOrchestratorTest, TriggerLearningIteration) {
    // Register a component
    orchestrator_->registerLoRAAdapter("adapter1", "Test");
    
    // Log enough interactions to trigger learning
    for (int i = 0; i < 15; i++) {
        Interaction interaction;
        interaction.interaction_id = "int_" + std::to_string(i);
        interaction.timestamp = std::chrono::system_clock::now();
        interaction.query = "Query";
        interaction.generated_answer = "Answer";
        interaction.user_feedback = FeedbackType::POSITIVE;
        
        orchestrator_->logInteraction(interaction);
    }
    
    auto stats_before = orchestrator_->getStats();
    
    // Trigger learning
    orchestrator_->triggerLearningIteration();
    
    auto stats_after = orchestrator_->getStats();
    
    // Stats should be updated (though specific changes depend on thresholds)
    EXPECT_GE(stats_after.total_interactions_logged, stats_before.total_interactions_logged);
}

TEST_F(ContinuousLearningOrchestratorTest, LearningLoop) {
    // Start learning loop
    orchestrator_->startLearningLoop();
    
    // Let it run briefly
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Stop learning loop
    orchestrator_->stopLearningLoop();
    
    // Should complete without hanging
    SUCCEED();
}

TEST_F(ContinuousLearningOrchestratorTest, PerformanceHistory) {
    auto history = orchestrator_->getPerformanceHistory(std::chrono::hours(24));
    
    // Initially empty
    EXPECT_TRUE(history.empty() || history.size() == 0);
}

TEST_F(ContinuousLearningOrchestratorTest, SystemImprovementTracking) {
    bool improving = orchestrator_->isSystemImproving();
    
    // Initially should be false (no trend)
    EXPECT_FALSE(improving);
}

TEST_F(ContinuousLearningOrchestratorTest, MultipleComponentsRegistration) {
    // Register multiple components of each type
    orchestrator_->registerLoRAAdapter("lora1", "LoRA 1");
    orchestrator_->registerLoRAAdapter("lora2", "LoRA 2");
    orchestrator_->registerRetrievalSystem("retrieval1");
    orchestrator_->registerRetrievalSystem("retrieval2");
    orchestrator_->registerPromptSystem("prompt1");
    
    // Log interactions
    for (int i = 0; i < 20; i++) {
        Interaction interaction;
        interaction.interaction_id = "int_" + std::to_string(i);
        interaction.timestamp = std::chrono::system_clock::now();
        interaction.query = "Query";
        interaction.generated_answer = "Answer";
        interaction.user_feedback = FeedbackType::POSITIVE;
        
        orchestrator_->logInteraction(interaction);
    }
    
    // Trigger learning
    orchestrator_->triggerLearningIteration();
    
    // Should handle multiple components gracefully
    SUCCEED();
}

TEST_F(ContinuousLearningOrchestratorTest, ConcurrentInteractionLogging) {
    // Test thread safety by logging from multiple threads
    std::vector<std::thread> threads;
    const int num_threads = 5;
    const int interactions_per_thread = 10;
    
    for (int t = 0; t < num_threads; t++) {
        threads.emplace_back([this, t, interactions_per_thread]() {
            for (int i = 0; i < interactions_per_thread; i++) {
                Interaction interaction;
                interaction.interaction_id = "thread_" + std::to_string(t) + "_int_" + std::to_string(i);
                interaction.timestamp = std::chrono::system_clock::now();
                interaction.query = "Concurrent query";
                interaction.generated_answer = "Answer";
                interaction.user_feedback = FeedbackType::POSITIVE;
                
                orchestrator_->logInteraction(interaction);
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    auto stats = orchestrator_->getStats();
    EXPECT_EQ(stats.total_interactions_logged, num_threads * interactions_per_thread);
}

TEST_F(ContinuousLearningOrchestratorTest, RetrainingTriggers) {
    // Register adapter with feedback
    orchestrator_->registerLoRAAdapter("adapter1", "Test");
    
    // Log enough interactions to trigger retraining
    for (int i = 0; i < config_.min_feedback_samples + 5; i++) {
        Interaction interaction;
        interaction.interaction_id = "int_" + std::to_string(i);
        interaction.timestamp = std::chrono::system_clock::now();
        interaction.query = "Query";
        interaction.generated_answer = "Answer";
        interaction.user_feedback = FeedbackType::POSITIVE;
        interaction.model_version = "adapter1";
        
        orchestrator_->logInteraction(interaction);
    }
    
    auto stats_before = orchestrator_->getStats();
    orchestrator_->triggerLearningIteration();
    auto stats_after = orchestrator_->getStats();
    
    // Retraining may or may not happen depending on other conditions
    // but should not crash
    SUCCEED();
}
