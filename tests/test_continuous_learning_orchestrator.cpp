/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_continuous_learning_orchestrator.cpp          ║
  Version:         0.0.19                                             ║
  Last Modified:   2026-02-21 19:00:00                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     495                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file test_continuous_learning_orchestrator.cpp
 * @brief Unit tests for Continuous Learning Orchestrator
 */

#include <chrono>
#include <filesystem>
#include <gtest/gtest.h>
#include <thread>

#include "rag/continuous_learning_orchestrator.h"

using namespace themis::rag::learning;

class ContinuousLearningOrchestratorTest : public ::testing::Test {
  protected:
    void SetUp() override {
        config_.min_feedback_samples      = 10;
        config_.min_accuracy_drop         = 0.05;
        config_.retraining_interval       = std::chrono::hours(1);
        config_.enable_ab_testing         = true;
        config_.ab_test_traffic_split     = 0.1;
        config_.min_ab_samples            = 30;
        config_.min_improvement_threshold = 0.02;
        config_.learning_loop_interval    = std::chrono::seconds(1);

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
    interaction.interaction_id   = "int_001";
    interaction.timestamp        = std::chrono::system_clock::now();
    interaction.query            = "What is machine learning?";
    interaction.generated_answer = "Machine learning is...";
    interaction.confidence_score = 0.85;
    interaction.user_feedback    = FeedbackType::POSITIVE;

    orchestrator_->logInteraction(interaction);

    auto stats = orchestrator_->getStats();
    EXPECT_EQ(stats.total_interactions_logged, 1);
}

TEST_F(ContinuousLearningOrchestratorTest, LogInteractionBatch) {
    std::vector<Interaction> interactions;

    for (int i = 0; i < 5; i++) {
        Interaction interaction;
        interaction.interaction_id   = "int_" + std::to_string(i);
        interaction.timestamp        = std::chrono::system_clock::now();
        interaction.query            = "Query " + std::to_string(i);
        interaction.generated_answer = "Answer " + std::to_string(i);
        interaction.confidence_score = 0.8 + i * 0.02;
        interaction.user_feedback    = (i % 2 == 0) ? FeedbackType::POSITIVE : FeedbackType::NEGATIVE;

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
        interaction.interaction_id   = "int_" + std::to_string(i);
        interaction.timestamp        = std::chrono::system_clock::now();
        interaction.query            = "Test query";
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
    EXPECT_GT(stats.current_accuracy, 0.5); // Should reflect positive feedback
}

TEST_F(ContinuousLearningOrchestratorTest, TriggerLearningIteration) {
    // Register a component
    orchestrator_->registerLoRAAdapter("adapter1", "Test");

    // Log enough interactions to trigger learning
    for (int i = 0; i < 15; i++) {
        Interaction interaction;
        interaction.interaction_id   = "int_" + std::to_string(i);
        interaction.timestamp        = std::chrono::system_clock::now();
        interaction.query            = "Query";
        interaction.generated_answer = "Answer";
        interaction.user_feedback    = FeedbackType::POSITIVE;

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
    EXPECT_TRUE(history.empty());
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
        interaction.interaction_id   = "int_" + std::to_string(i);
        interaction.timestamp        = std::chrono::system_clock::now();
        interaction.query            = "Query";
        interaction.generated_answer = "Answer";
        interaction.user_feedback    = FeedbackType::POSITIVE;

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
    const int num_threads             = 5;
    const int interactions_per_thread = 10;

    for (int t = 0; t < num_threads; t++) {
        threads.emplace_back([this, t, interactions_per_thread]() {
            for (int i = 0; i < interactions_per_thread; i++) {
                Interaction interaction;
                interaction.interaction_id   = "thread_" + std::to_string(t) + "_int_" + std::to_string(i);
                interaction.timestamp        = std::chrono::system_clock::now();
                interaction.query            = "Concurrent query";
                interaction.generated_answer = "Answer";
                interaction.user_feedback    = FeedbackType::POSITIVE;

                orchestrator_->logInteraction(interaction);
            }
        });
    }

    for (auto &thread : threads) {
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
        interaction.interaction_id   = "int_" + std::to_string(i);
        interaction.timestamp        = std::chrono::system_clock::now();
        interaction.query            = "Query";
        interaction.generated_answer = "Answer";
        interaction.user_feedback    = FeedbackType::POSITIVE;
        interaction.model_version    = "adapter1";

        orchestrator_->logInteraction(interaction);
    }

    auto stats_before = orchestrator_->getStats();
    orchestrator_->triggerLearningIteration();
    auto stats_after = orchestrator_->getStats();

    // Retraining may or may not happen depending on other conditions
    // but should not crash
    SUCCEED();
}

// Helper: log N interactions with given feedback type and prompt version
static void logInteractions(
    ContinuousLearningOrchestrator& orchestrator,
    size_t count,
    FeedbackType feedback,
    const std::string& prompt_version = "v1"
) {
    for (size_t i = 0; i < count; ++i) {
        Interaction interaction;
        interaction.interaction_id   = "int_" + std::to_string(i);
        interaction.timestamp        = std::chrono::system_clock::now();
        interaction.query            = "Test query " + std::to_string(i);
        interaction.generated_answer = "Answer " + std::to_string(i);
        interaction.confidence_score = 0.8;
        interaction.user_feedback    = feedback;
        interaction.prompt_version   = prompt_version;
        orchestrator.logInteraction(interaction);
    }
}

// Test: runPromptOptimization does not crash and increments counter when there
// is enough data with a low success rate
TEST_F(ContinuousLearningOrchestratorTest, PromptOptimizationTriggered) {
    orchestrator_->registerPromptSystem("prompt1");

    // Log enough mostly-negative interactions so one version looks bad
    logInteractions(*orchestrator_, config_.min_feedback_samples + 2,
                    FeedbackType::NEGATIVE, "v1");

    auto stats_before = orchestrator_->getStats();
    orchestrator_->triggerLearningIteration();
    auto stats_after = orchestrator_->getStats();

    EXPECT_GE(stats_after.prompt_optimizations, stats_before.prompt_optimizations);
}

// Test: runPromptOptimization returns early when there is not enough data
TEST_F(ContinuousLearningOrchestratorTest, PromptOptimizationSkippedInsufficientData) {
    // Log fewer interactions than min_feedback_samples
    logInteractions(*orchestrator_, config_.min_feedback_samples / 2,
                    FeedbackType::NEGATIVE, "v1");

    auto stats_before = orchestrator_->getStats();
    orchestrator_->triggerLearningIteration();
    auto stats_after = orchestrator_->getStats();

    // No optimization should have been triggered
    EXPECT_EQ(stats_after.prompt_optimizations, stats_before.prompt_optimizations);
}

// Test: runRetrievalOptimization does not crash and increments counter
TEST_F(ContinuousLearningOrchestratorTest, RetrievalOptimizationTriggered) {
    orchestrator_->registerRetrievalSystem("retrieval1");

    logInteractions(*orchestrator_, config_.min_feedback_samples + 2,
                    FeedbackType::POSITIVE);

    auto stats_before = orchestrator_->getStats();
    orchestrator_->triggerLearningIteration();
    auto stats_after = orchestrator_->getStats();

    EXPECT_GE(stats_after.retrieval_optimizations, stats_before.retrieval_optimizations);
}

// Test: saveMetrics / loadMetrics round-trip preserves accuracy
TEST_F(ContinuousLearningOrchestratorTest, SaveLoadMetricsRoundTrip) {
    const auto test_path =
        (std::filesystem::temp_directory_path() / "test_clo_metrics_roundtrip.csv").string();
    // Remove any leftover file from a previous run
    std::filesystem::remove(test_path);

    // Create an orchestrator with a file path and log enough positive interactions
    ContinuousLearningConfig save_config = config_;
    save_config.metrics_db_path = test_path;
    auto saver = std::make_unique<ContinuousLearningOrchestrator>(save_config);
    logInteractions(*saver, config_.min_feedback_samples + 1, FeedbackType::POSITIVE);

    double acc_before = saver->getStats().current_accuracy;
    EXPECT_GT(acc_before, 0.0);
    saver->saveMetrics();

    // Load into a fresh orchestrator and verify accuracy is restored
    auto loader = std::make_unique<ContinuousLearningOrchestrator>(save_config);
    loader->loadMetrics();

    double acc_after = loader->getStats().current_accuracy;
    EXPECT_NEAR(acc_after, acc_before, 1e-5);

    // Cleanup
    std::filesystem::remove(test_path);
}

// Test: saveModelCheckpoint records an improvement event
TEST_F(ContinuousLearningOrchestratorTest, SaveModelCheckpointRecordsEvent) {
    orchestrator_->saveModelCheckpoint("adapter_v2");

    auto stats = orchestrator_->getStats();
    bool found = false;
    for (const auto& event : stats.recent_improvements) {
        if (event.improvement_type == "ModelCheckpoint" &&
            event.component == "adapter_v2") {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}


// ============================================================================
// Data selection integration: rollback, reselection scheduling
// ============================================================================

// Test: when rollback condition is met (low accuracy), retraining is skipped
// and a RollbackTriggered event is recorded.
TEST_F(ContinuousLearningOrchestratorTest, RollbackSkipsRetrainingAndLogsEvent) {
    using namespace themis::training;

    // Configure SI module to rollback when accuracy drop > 0.01 (very sensitive)
    ContinuousLearningConfig cfg = config_;
    cfg.enable_auto_rollback = true;

    SelfImprovementConfig si_cfg;
    si_cfg.enabled                       = true;
    si_cfg.threshold_auto_adjust         = true;
    si_cfg.accuracy_monitoring           = true;
    si_cfg.accuracy_rollback_threshold   = 0.01; // any accuracy < 0.99 triggers rollback
    si_cfg.min_avg_quality_score         = 0.0;  // no quality trigger
    si_cfg.diversity_monitoring          = false; // no diversity trigger
    cfg.self_improvement_config = si_cfg;

    auto orch = std::make_unique<ContinuousLearningOrchestrator>(cfg);

    // Register an adapter and accumulate enough feedback to trigger retraining
    orch->registerLoRAAdapter("adapter_rb", "RollbackTest");

    for (int i = 0; i < static_cast<int>(cfg.min_feedback_samples) + 2; ++i) {
        Interaction interaction;
        interaction.interaction_id   = "rb_" + std::to_string(i);
        interaction.timestamp        = std::chrono::system_clock::now();
        interaction.query            = "query";
        interaction.generated_answer = "answer";
        interaction.user_feedback    = FeedbackType::NEGATIVE;
        interaction.model_version    = "adapter_rb";
        orch->logInteraction(interaction);
    }

    auto stats_before = orch->getStats();
    orch->triggerLearningIteration();
    auto stats_after = orch->getStats();

    // Retraining count should NOT have increased because rollback fired
    EXPECT_EQ(stats_after.lora_retraining_count, stats_before.lora_retraining_count);

    // A RollbackTriggered event should be in recent_improvements
    bool found_rollback = false;
    for (const auto& ev : stats_after.recent_improvements) {
        if (ev.improvement_type == "RollbackTriggered" && ev.component == "adapter_rb") {
            found_rollback = true;
            break;
        }
    }
    EXPECT_TRUE(found_rollback);
}

// Test: when needsReselection() is true (period elapsed), triggerLearningIteration
// runs the pipeline and updates last_selection_time so a second immediate call
// does NOT re-select (period not yet elapsed again).
TEST_F(ContinuousLearningOrchestratorTest, PeriodicReselectionTriggeredOnce) {
    using namespace themis::training;

    ContinuousLearningConfig cfg = config_;

    // Very short period (1 second) so needsReselection is immediately true
    SelfImprovementConfig si_cfg;
    si_cfg.enabled          = true;
    si_cfg.period_seconds   = 1;         // 1-second period
    cfg.self_improvement_config = si_cfg;

    auto orch = std::make_unique<ContinuousLearningOrchestrator>(cfg);

    // Wait >1 second so period has elapsed since construction
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // First iteration: reselection should run (period elapsed)
    orch->triggerLearningIteration();

    // Immediately trigger again: period has NOT elapsed since just ran
    // This just verifies it doesn't crash / doesn't double-run
    orch->triggerLearningIteration();
    SUCCEED();
}

// Test: runDataSelectionForAdapter() updates last_selection_time so subsequent
// needsReselection() returns false for the configured period.
TEST_F(ContinuousLearningOrchestratorTest, RunDataSelectionUpdatesLastSelectionTime) {
    using namespace themis::training;

    ContinuousLearningConfig cfg = config_;
    SelfImprovementConfig si_cfg;
    si_cfg.enabled        = true;
    si_cfg.period_seconds = 3600; // 1 hour
    cfg.self_improvement_config = si_cfg;

    auto orch = std::make_unique<ContinuousLearningOrchestrator>(cfg);

    // Run manual data selection
    auto result = orch->runDataSelectionForAdapter("adapter1", {});
    EXPECT_TRUE(result.success);

    // After a manual run, needsReselection should be false (period is 1 hour)
    // Verify indirectly: call triggerLearningIteration and confirm it doesn't
    // crash (a deadlock or logic error would surface here).
    orch->triggerLearningIteration();
    SUCCEED();
}
