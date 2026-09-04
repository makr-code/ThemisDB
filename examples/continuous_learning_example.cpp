/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            continuous_learning_example.cpp                    ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:53                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   90.0/100                                       ║
    • Total Lines:     205                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file continuous_learning_example.cpp
 * @brief Example demonstrating Continuous Learning Orchestrator usage
 *
 * This example shows how to:
 * 1. Configure and initialize the orchestrator
 * 2. Register RAG components
 * 3. Log interactions during production
 * 4. Monitor learning progress
 * 5. Handle automatic improvements
 */

#include <chrono>
#include <iostream>
#include <thread>

#include "rag/continuous_learning_orchestrator.h"

using namespace themis::rag::learning;

void printStats(const LearningStats &stats) {
    std::cout << "\n=== Learning Statistics ===\n";
    std::cout << "Total interactions: " << stats.total_interactions_logged << "\n";
    std::cout << "LoRA retraining count: " << stats.lora_retraining_count << "\n";
    std::cout << "Prompt optimizations: " << stats.prompt_optimizations << "\n";
    std::cout << "Retrieval optimizations: " << stats.retrieval_optimizations << "\n";
    std::cout << "Current accuracy: " << (stats.current_accuracy * 100) << "%\n";
    std::cout << "7-day average accuracy: " << (stats.accuracy_7d_avg * 100) << "%\n";
    std::cout << "Accuracy trend: " << (stats.accuracy_trend > 0 ? "↑ Improving" : "↓ Declining") << "\n";
    std::cout << "Active A/B tests: " << stats.active_ab_tests.size() << "\n";

    if (!stats.recent_improvements.empty()) {
        std::cout << "\nRecent improvements:\n";
        for (const auto &improvement : stats.recent_improvements) {
            std::cout << "  - " << improvement.component << ": " << improvement.improvement_type << " ("
                      << (improvement.metric_after - improvement.metric_before) * 100 << "% improvement)\n";
        }
    }
    std::cout << "==========================\n\n";
}

int main() {
    std::cout << "Continuous Learning Orchestrator Example\n";
    std::cout << "========================================\n\n";

    // Step 1: Configure the orchestrator
    ContinuousLearningConfig config;
    config.min_feedback_samples      = 100;
    config.min_accuracy_drop         = 0.05;
    config.retraining_interval       = std::chrono::hours(24);
    config.enable_ab_testing         = true;
    config.ab_test_traffic_split     = 0.1; // 10% traffic for new models
    config.min_ab_samples            = 1000;
    config.min_improvement_threshold = 0.02; // 2% minimum improvement
    config.enable_auto_rollback      = true;
    config.learning_loop_interval    = std::chrono::seconds(3600); // Check every hour

    std::cout << "Configuration:\n";
    std::cout << "  Min feedback samples: " << config.min_feedback_samples << "\n";
    std::cout << "  Retraining interval: 24 hours\n";
    std::cout << "  A/B test traffic split: 10%\n";
    std::cout << "  Min improvement threshold: 2%\n\n";

    // Step 2: Initialize orchestrator
    auto orchestrator = std::make_unique<ContinuousLearningOrchestrator>(config);
    std::cout << "✓ Orchestrator initialized\n\n";

    // Step 3: Register RAG components
    std::cout << "Registering components:\n";
    orchestrator->registerLoRAAdapter("themis_help_lora", "Documentation Q&A adapter");
    std::cout << "  ✓ LoRA adapter registered\n";

    orchestrator->registerRetrievalSystem("vector_index_main");
    std::cout << "  ✓ Retrieval system registered\n";

    orchestrator->registerPromptSystem("prompt_library");
    std::cout << "  ✓ Prompt system registered\n";

    orchestrator->registerKnowledgeGapDetector("gap_detector_v1");
    std::cout << "  ✓ Knowledge gap detector registered\n\n";

    // Step 4: Start background learning loop
    std::cout << "Starting continuous learning loop...\n";
    orchestrator->startLearningLoop();
    std::cout << "✓ Learning loop active\n\n";

    // Step 5: Simulate production usage
    std::cout << "Simulating production interactions...\n";

    for (int i = 0; i < 50; i++) {
        Interaction interaction;
        interaction.interaction_id   = "int_" + std::to_string(i);
        interaction.timestamp        = std::chrono::system_clock::now();
        interaction.query            = "Example query " + std::to_string(i);
        interaction.generated_answer = "Example answer " + std::to_string(i);
        interaction.confidence_score = 0.75 + (i % 20) * 0.01;
        interaction.perplexity       = 15.0 + (i % 10);

        // Simulate feedback (80% positive)
        if (i % 5 != 0) {
            interaction.user_feedback = FeedbackType::POSITIVE;
        } else {
            interaction.user_feedback = FeedbackType::NEGATIVE;
        }

        // Simulate gap detection
        interaction.gap_detection_result.gap_detected     = (i % 10 == 0);
        interaction.gap_detection_result.confidence_score = 0.85;

        interaction.model_version            = "themis_help_lora_v1";
        interaction.retrieval_config_version = "retrieval_v1";
        interaction.prompt_version           = "prompt_v1";

        orchestrator->logInteraction(interaction);

        // Show progress
        if ((i + 1) % 10 == 0) {
            std::cout << "  Logged " << (i + 1) << " interactions...\n";
        }
    }

    std::cout << "✓ 50 interactions logged\n\n";

    // Step 6: Check statistics
    auto stats = orchestrator->getStats();
    printStats(stats);

    // Step 7: Manually trigger a learning iteration
    std::cout << "Triggering manual learning iteration...\n";
    orchestrator->triggerLearningIteration();
    std::cout << "✓ Learning iteration completed\n\n";

    // Step 8: Check updated statistics
    stats = orchestrator->getStats();
    printStats(stats);

    // Step 9: Check performance history
    std::cout << "Checking performance history (last 24 hours)...\n";
    auto history = orchestrator->getPerformanceHistory(std::chrono::hours(24));
    std::cout << "Performance snapshots: " << history.size() << "\n\n";

    // Step 10: Check if system is improving
    bool improving = orchestrator->isSystemImproving();
    std::cout << "System improvement status: " << (improving ? "✓ Improving" : "⚠ Not improving") << "\n\n";

    // Step 11: Simulate batch logging
    std::cout << "Logging batch of interactions...\n";
    const int BATCH_SIZE = 20;
    std::vector<Interaction> batch = {};

    for (int i = 0; i < BATCH_SIZE; i++) {
        Interaction interaction;
        interaction.interaction_id   = "batch_" + std::to_string(i);
        interaction.timestamp        = std::chrono::system_clock::now();
        interaction.query            = "Batch query " + std::to_string(i);
        interaction.generated_answer = "Batch answer " + std::to_string(i);
        interaction.confidence_score = 0.8;
        interaction.user_feedback    = FeedbackType::POSITIVE;

        batch.push_back(interaction);
    }

    orchestrator->logInteractionBatch(batch);
    std::cout << "✓ Batch of " << batch.size() << " interactions logged\n\n";

    // Step 12: Final statistics
    stats = orchestrator->getStats();
    printStats(stats);

    // Step 13: Cleanup
    std::cout << "Stopping learning loop...\n";
    orchestrator->stopLearningLoop();
    std::cout << "✓ Learning loop stopped\n\n";

    std::cout << "Example completed successfully!\n";
    std::cout << "\nKey Takeaways:\n";
    std::cout << "1. Configure orchestrator with appropriate thresholds\n";
    std::cout << "2. Register all RAG components for automatic optimization\n";
    std::cout << "3. Log every interaction with feedback when available\n";
    std::cout << "4. Monitor statistics to track improvements\n";
    std::cout << "5. Trust the system to optimize automatically\n";

    return 0;
}
