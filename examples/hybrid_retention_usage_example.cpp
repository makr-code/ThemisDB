/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            hybrid_retention_usage_example.cpp                 ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:54                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     330                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file hybrid_retention_usage_example.cpp
 * @brief Complete usage example for HybridRetentionManager
 * 
 * Demonstrates how to use the production-ready hybrid retention system
 * that combines Gorilla compression, adaptive retention, and time-based retention.
 */

#include "scheduler/hybrid_retention_manager.h"
#include "scheduler/task_scheduler.h"
#include "query/query_engine.h"
#include "timeseries/tsstore.h"
#include <iostream>
#include <thread>

namespace themis {
namespace examples {

/**
 * Example 1: Basic Hybrid Retention Setup
 */
void example_basic_hybrid_setup(QueryEngine* query_engine, TSStore* tsstore) {
    std::cout << "=== Example 1: Basic Hybrid Retention Setup ===" << std::endl;
    
    // Create task scheduler
    TaskScheduler::Config scheduler_config;
    scheduler_config.max_concurrent_tasks = 4;
    scheduler_config.check_interval = std::chrono::seconds(30);
    
    TaskScheduler scheduler(query_engine, scheduler_config);
    scheduler.start();
    
    // Create hybrid retention manager with default config
    HybridRetentionConfig hybrid_config;
    
    HybridRetentionManager retention_manager(
        query_engine,
        tsstore,
        &scheduler,
        hybrid_config
    );
    
    // Start the hybrid retention system
    retention_manager.start();
    
    std::cout << "Hybrid retention system started with default configuration:" << std::endl;
    std::cout << "  Stage 1 (Gorilla): 0-7 days, lossless compression" << std::endl;
    std::cout << "  Stage 2 (Adaptive): 7-365 days, variance-based" << std::endl;
    std::cout << "  Stage 3 (Time-Based): >365 days, daily aggregates" << std::endl;
    
    // The system now runs automatically in the background
    // In production, this would run indefinitely
    
    // For demo, wait a bit
    std::this_thread::sleep_for(std::chrono::seconds(5));
    
    // Get status report
    auto status = retention_manager.getStatusReport();
    std::cout << "\nStatus Report:\n" << status.dump(2) << std::endl;
    
    // Stop
    retention_manager.stop();
    scheduler.stop();
}

/**
 * Example 2: Customized Hybrid Configuration
 */
void example_customized_hybrid_config(QueryEngine* query_engine, TSStore* tsstore) {
    std::cout << "\n=== Example 2: Customized Hybrid Configuration ===" << std::endl;
    
    TaskScheduler scheduler(query_engine);
    scheduler.start();
    
    // Customize configuration for different requirements
    HybridRetentionConfig config;
    
    // Stage 1: Keep hot data for 14 days instead of 7
    config.stage1.duration = std::chrono::hours(24 * 14);
    config.stage1.check_interval = std::chrono::hours(12);
    
    // Stage 2: More aggressive adaptive thresholds
    config.stage2.low_cv_threshold = 3.0;      // More aggressive (was 5.0)
    config.stage2.medium_cv_threshold = 15.0;   // More aggressive (was 20.0)
    config.stage2.low_cv_resolution = "2h";     // Less granular (was 1h)
    config.stage2.medium_cv_resolution = "30m"; // Less granular (was 15m)
    config.stage2.high_cv_resolution = "5m";    // Less granular (was 1m)
    config.stage2.check_interval = std::chrono::hours(6);
    
    // Stage 3: Disabled (keep adaptive retention forever)
    config.stage3.enabled = false;
    
    // Enable automatic cleanup with verification
    config.auto_cleanup = true;
    config.verify_aggregates = true;
    
    HybridRetentionManager retention_manager(
        query_engine,
        tsstore,
        &scheduler,
        config
    );
    
    retention_manager.start();
    
    std::cout << "Hybrid retention started with custom configuration:" << std::endl;
    std::cout << "  - Hot data kept for 14 days (Gorilla)" << std::endl;
    std::cout << "  - More aggressive adaptive thresholds" << std::endl;
    std::cout << "  - Stage 3 (daily aggregates) disabled" << std::endl;
    std::cout << "  - Automatic cleanup enabled" << std::endl;
    
    retention_manager.stop();
    scheduler.stop();
}

/**
 * Example 3: Manual Execution and Monitoring
 */
void example_manual_execution_monitoring(QueryEngine* query_engine, TSStore* tsstore) {
    std::cout << "\n=== Example 3: Manual Execution and Monitoring ===" << std::endl;
    
    TaskScheduler scheduler(query_engine);
    scheduler.start();
    
    HybridRetentionManager retention_manager(
        query_engine,
        tsstore,
        &scheduler
    );
    
    retention_manager.start();
    
    // Manual execution of individual stages
    std::cout << "\nManually executing Stage 1 (Gorilla compression)..." << std::endl;
    retention_manager.executeStage1();
    
    std::cout << "Manually executing Stage 2 (Adaptive retention)..." << std::endl;
    retention_manager.executeStage2();
    
    std::cout << "Manually executing Stage 3 (Time-based retention)..." << std::endl;
    retention_manager.executeStage3();
    
    // Get detailed statistics
    auto stats = retention_manager.getStats();
    
    std::cout << "\n=== Retention Statistics ===" << std::endl;
    std::cout << "Stage 1 (Gorilla):" << std::endl;
    std::cout << "  Total compressions: " << stats.stage1.compressions_total << std::endl;
    std::cout << "  Failed: " << stats.stage1.compressions_failed << std::endl;
    std::cout << "  Avg compression ratio: " << stats.stage1.avg_compression_ratio << ":1" << std::endl;
    
    std::cout << "\nStage 2 (Adaptive):" << std::endl;
    std::cout << "  Total aggregations: " << stats.stage2.aggregations_total << std::endl;
    std::cout << "  Failed: " << stats.stage2.aggregations_failed << std::endl;
    std::cout << "  Anomalies preserved: " << stats.stage2.anomalies_preserved << std::endl;
    
    std::cout << "\nStage 3 (Time-Based):" << std::endl;
    std::cout << "  Total aggregations: " << stats.stage3.aggregations_total << std::endl;
    std::cout << "  Failed: " << stats.stage3.aggregations_failed << std::endl;
    
    std::cout << "\nOverall:" << std::endl;
    std::cout << "  Storage saved: " << stats.total_storage_bytes_saved / 1024 / 1024 << " MB" << std::endl;
    std::cout << "  Reduction: " << stats.overall_storage_reduction_percent << "%" << std::endl;
    
    retention_manager.stop();
    scheduler.stop();
}

/**
 * Example 4: Per-Metric Configuration
 * 
 * Different metrics can have different retention strategies.
 */
void example_per_metric_configuration(QueryEngine* query_engine, TSStore* tsstore) {
    std::cout << "\n=== Example 4: Per-Metric Configuration ===" << std::endl;
    
    TaskScheduler scheduler(query_engine);
    scheduler.start();
    
    // Temperature sensors: Very stable, aggressive downsampling
    HybridRetentionConfig temp_config;
    temp_config.stage1.metric_pattern = "temperature_*";
    temp_config.stage2.low_cv_threshold = 2.0;  // Very aggressive
    temp_config.stage2.medium_cv_threshold = 8.0;
    temp_config.stage2.low_cv_resolution = "2h";
    
    HybridRetentionManager temp_retention(
        query_engine,
        tsstore,
        &scheduler,
        temp_config
    );
    temp_retention.start();
    
    // Vibration sensors: Highly variable, preserve detail
    HybridRetentionConfig vibration_config;
    vibration_config.stage1.metric_pattern = "vibration_*";
    vibration_config.stage1.duration = std::chrono::hours(24 * 30);  // Keep 30 days
    vibration_config.stage2.low_cv_threshold = 15.0;  // Less aggressive
    vibration_config.stage2.medium_cv_threshold = 40.0;
    vibration_config.stage2.high_cv_resolution = "1s";  // Keep full resolution!
    
    HybridRetentionManager vibration_retention(
        query_engine,
        tsstore,
        &scheduler,
        vibration_config
    );
    vibration_retention.start();
    
    std::cout << "Multiple retention managers running:" << std::endl;
    std::cout << "  - Temperature: Aggressive downsampling" << std::endl;
    std::cout << "  - Vibration: Preserve high detail" << std::endl;
    
    // Both run independently
    std::this_thread::sleep_for(std::chrono::seconds(5));
    
    temp_retention.stop();
    vibration_retention.stop();
    scheduler.stop();
}

/**
 * Example 5: Integration with Monitoring
 */
void example_monitoring_integration(QueryEngine* query_engine, TSStore* tsstore) {
    std::cout << "\n=== Example 5: Monitoring Integration ===" << std::endl;
    
    TaskScheduler scheduler(query_engine);
    scheduler.start();
    
    HybridRetentionManager retention_manager(
        query_engine,
        tsstore,
        &scheduler
    );
    
    retention_manager.start();
    
    // Periodic status reporting
    auto report_status = [&retention_manager]() {
        auto report = retention_manager.getStatusReport();
        
        std::cout << "\n=== Retention Status Report ===" << std::endl;
        std::cout << "Running: " << (report["running"].get<bool>() ? "Yes" : "No") << std::endl;
        
        if (report.contains("stats")) {
            auto stats = report["stats"];
            
            // Calculate overall efficiency
            auto stage1 = stats["stage1"];
            auto stage2 = stats["stage2"];
            auto stage3 = stats["stage3"];
            
            size_t total_operations = 
                stage1["compressions_total"].get<size_t>() +
                stage2["aggregations_total"].get<size_t>() +
                stage3["aggregations_total"].get<size_t>();
            
            size_t total_failures =
                stage1["compressions_failed"].get<size_t>() +
                stage2["aggregations_failed"].get<size_t>() +
                stage3["aggregations_failed"].get<size_t>();
            
            double success_rate = total_operations > 0 
                ? (double)(total_operations - total_failures) / total_operations * 100
                : 0.0;
            
            std::cout << "Total Operations: " << total_operations << std::endl;
            std::cout << "Success Rate: " << success_rate << "%" << std::endl;
            std::cout << "Anomalies Preserved: " 
                      << stage2["anomalies_preserved"].get<size_t>() << std::endl;
        }
        
        std::cout << "==============================\n" << std::endl;
    };
    
    // Report status every 10 seconds
    for (int i = 0; i < 3; i++) {
        std::this_thread::sleep_for(std::chrono::seconds(10));
        report_status();
    }
    
    retention_manager.stop();
    scheduler.stop();
}

/**
 * Main runner for all examples
 */
void run_hybrid_retention_examples(QueryEngine* query_engine, TSStore* tsstore) {
    std::cout << "====================================================" << std::endl;
    std::cout << "  Hybrid Retention Manager - Complete Examples" << std::endl;
    std::cout << "====================================================" << std::endl;
    
    example_basic_hybrid_setup(query_engine, tsstore);
    example_customized_hybrid_config(query_engine, tsstore);
    example_manual_execution_monitoring(query_engine, tsstore);
    example_per_metric_configuration(query_engine, tsstore);
    example_monitoring_integration(query_engine, tsstore);
    
    std::cout << "\n====================================================" << std::endl;
    std::cout << "  All examples completed successfully!" << std::endl;
    std::cout << "====================================================" << std::endl;
}

} // namespace examples
} // namespace themis
