/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            themis_help_lora_example.cpp                       ║
  Version:         0.0.39                                             ║
  Last Modified:   2026-04-13 20:19:38                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     241                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file themis_help_lora_example.cpp
 * @brief Example usage of ThemisHelpLoRA - Documentation Q&A assistant
 * 
 * This example demonstrates:
 * - Initializing the ThemisHelpLoRA application
 * - Querying the documentation assistant
 * - Collecting user feedback (positive and negative)
 * - Training from feedback
 * - Training from documentation corpus
 * - Getting metrics and statistics
 * - Version management and rollback
 */

#include "llm/applications/themis_help_lora.h"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>

using namespace themis::llm::applications;

void printSeparator() {
    std::cout << "\n" << std::string(70, '=') << "\n\n";
}

int main() {
    std::cout << "ThemisHelpLoRA Example - Documentation Q&A Assistant\n";
    printSeparator();
    
    // ========================================================================
    // 1. Initialize ThemisHelpLoRA
    // ========================================================================
    std::cout << "1. Initializing ThemisHelpLoRA...\n";
    
    ThemisHelpLoRA::Config config;
    config.adapter_id = "themis_help_lora";
    config.base_model = "llama-2-7b";
    config.docs_database_path = "data/docs_database.json";
    config.feedback_batch_size = 100;  // Train after 100 feedback items
    config.min_accuracy_threshold = 0.80f;
    config.enable_ab_testing = true;
    config.enable_auto_rollback = true;
    
    ThemisHelpLoRA help(config);
    
    std::cout << "✅ ThemisHelpLoRA initialized successfully\n";
    std::cout << "   Adapter ID: " << config.adapter_id << "\n";
    std::cout << "   Base Model: " << config.base_model << "\n";
    std::cout << "   Current Version: " << help.getAdapterVersion() << "\n";
    
    // ========================================================================
    // 2. Query the Documentation Assistant
    // ========================================================================
    printSeparator();
    std::cout << "2. Querying the Documentation Assistant...\n\n";
    
    std::vector<std::string> questions = {
        "How do I enable sharding?",
        "What is the replication factor?",
        "How do I configure backups?",
        "What are the performance optimization options?"
    };
    
    for (const auto& question : questions) {
        std::cout << "Q: " << question << "\n";
        std::string answer = help.query(question);
        std::cout << "A: " << answer << "\n\n";
    }
    
    // ========================================================================
    // 3. Collect User Feedback
    // ========================================================================
    printSeparator();
    std::cout << "3. Collecting User Feedback...\n\n";
    
    // Positive feedback
    std::cout << "Adding positive feedback...\n";
    help.addPositiveFeedback(
        "How do I enable sharding?",
        "To enable sharding in ThemisDB: 1. Configure the shard key..."
    );
    help.addPositiveFeedback(
        "What is the replication factor?",
        "To configure replication in ThemisDB: 1. Set replicationFactor..."
    );
    
    // Negative feedback with correction
    std::cout << "Adding negative feedback with correction...\n";
    help.addNegativeFeedback(
        "How do I configure backups?",
        "Incorrect answer about backups",
        "The correct way is: Use themisdb-backup create --hot for hot backups..."
    );
    
    // Get feedback statistics
    auto stats = help.getFeedbackStats();
    std::cout << "\nFeedback Statistics:\n";
    std::cout << "  Total: " << stats["total_feedback"] << "\n";
    std::cout << "  Positive: " << stats["positive_feedback"] << "\n";
    std::cout << "  Negative: " << stats["negative_feedback"] << "\n";
    std::cout << "  Positive Ratio: " << stats["positive_ratio"] << "\n";
    
    // ========================================================================
    // 4. Train from Feedback
    // ========================================================================
    printSeparator();
    std::cout << "4. Training from Feedback...\n\n";
    
    // Collect more feedback to reach batch size
    std::cout << "Collecting additional feedback for training...\n";
    for (int i = 0; i < 10; i++) {
        help.addPositiveFeedback(
            "Question " + std::to_string(i),
            "Answer " + std::to_string(i)
        );
    }
    
    std::cout << "Starting training from feedback...\n";
    auto training_result = help.trainFromFeedback();
    
    if (training_result.success) {
        std::cout << "✅ Training completed successfully!\n";
        std::cout << "   New Version: " << training_result.version << "\n";
        std::cout << "   Final Loss: " << training_result.final_loss << "\n";
        std::cout << "   Validation Accuracy: " << training_result.validation_accuracy << "\n";
        std::cout << "   Epochs: " << training_result.epochs_completed << "\n";
        std::cout << "   Training Time: " << training_result.training_time.count() << " seconds\n";
    } else {
        std::cout << "❌ Training failed: " << training_result.error_message << "\n";
    }
    
    // ========================================================================
    // 5. Train from Documentation Corpus
    // ========================================================================
    printSeparator();
    std::cout << "5. Training from Documentation Corpus...\n\n";
    
    std::cout << "Starting training from documentation corpus...\n";
    auto doc_training_result = help.trainFromDocumentation();
    
    if (doc_training_result.success) {
        std::cout << "✅ Documentation training completed successfully!\n";
        std::cout << "   Final Loss: " << doc_training_result.final_loss << "\n";
        std::cout << "   Validation Accuracy: " << doc_training_result.validation_accuracy << "\n";
        std::cout << "   Epochs: " << doc_training_result.epochs_completed << "\n";
        std::cout << "   Training Time: " << doc_training_result.training_time.count() << " seconds\n";
    } else {
        std::cout << "❌ Documentation training failed: " << doc_training_result.error_message << "\n";
    }
    
    // ========================================================================
    // 6. Get Performance Metrics
    // ========================================================================
    printSeparator();
    std::cout << "6. Performance Metrics...\n\n";
    
    auto metrics = help.getMetrics();
    std::cout << "Performance Metrics:\n";
    std::cout << "  Total Queries: " << metrics["total_queries"] << "\n";
    std::cout << "  Successful Queries: " << metrics["successful_queries"] << "\n";
    std::cout << "  Failed Queries: " << metrics["failed_queries"] << "\n";
    std::cout << "  Success Rate: " << (metrics["success_rate"].get<double>() * 100.0) << "%\n";
    std::cout << "  Average Latency: " << metrics["average_latency_ms"] << " ms\n";
    std::cout << "  Cache Hit Rate: " << (metrics["cache_hit_rate"].get<double>() * 100.0) << "%\n";
    
    // ========================================================================
    // 7. Version Management
    // ========================================================================
    printSeparator();
    std::cout << "7. Version Management...\n\n";
    
    std::cout << "Current Version: " << help.getAdapterVersion() << "\n";
    
    // Check if adapter is loaded
    if (help.isAdapterLoaded()) {
        std::cout << "Adapter Status: ✅ Loaded\n";
    } else {
        std::cout << "Adapter Status: ⚠️ Not Loaded\n";
    }
    
    // Reload adapter
    std::cout << "\nReloading adapter...\n";
    if (help.reloadAdapter()) {
        std::cout << "✅ Adapter reloaded successfully\n";
    } else {
        std::cout << "❌ Failed to reload adapter\n";
    }
    
    // Rollback to previous version
    std::cout << "\nRolling back to previous version...\n";
    if (help.rollbackToPreviousVersion()) {
        std::cout << "✅ Rolled back successfully\n";
        std::cout << "   New Version: " << help.getAdapterVersion() << "\n";
    } else {
        std::cout << "❌ Failed to rollback\n";
    }
    
    // ========================================================================
    // 8. Final Status
    // ========================================================================
    printSeparator();
    std::cout << "8. Final Status...\n\n";
    
    auto final_stats = help.getFeedbackStats();
    auto final_metrics = help.getMetrics();
    
    std::cout << "ThemisHelpLoRA Final Status:\n";
    std::cout << "  Adapter Version: " << help.getAdapterVersion() << "\n";
    std::cout << "  Total Queries Processed: " << final_metrics["total_queries"] << "\n";
    std::cout << "  Feedback Collected: " << final_stats["total_feedback"] << "\n";
    std::cout << "  Success Rate: " << (final_metrics["success_rate"].get<double>() * 100.0) << "%\n";
    
    printSeparator();
    std::cout << "Example completed successfully!\n\n";
    
    return 0;
}
