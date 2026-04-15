/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            complete_integration_example.cpp                   ║
  Version:         0.0.46                                             ║
  Last Modified:   2026-04-15 18:01:28                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   92.0/100                                       ║
    • Total Lines:     292                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * Complete Integration Example
 * 
 * Demonstrates the full prompt engineering system working end-to-end
 * with all 6 phases integrated seamlessly.
 */

#include <iostream>
#include <memory>
#include <thread>
#include <chrono>

#include "prompt_engineering/prompt_manager.h"
#include "prompt_engineering/prompt_optimizer.h"
#include "prompt_engineering/prompt_performance_tracker.h"
#include "prompt_engineering/self_improvement_orchestrator.h"
#include "prompt_engineering/feedback_collector.h"
#include "prompt_engineering/prompt_version_control.h"
#include "prompt_engineering/prompt_engineering_integration.h"

using namespace themis::prompt_engineering;

int main() {
    std::cout << "=== Complete Prompt Engineering System Demo ===" << std::endl;
    std::cout << std::endl;
    
    // ========================================================================
    // Phase 1: Initialize all components
    // ========================================================================
    std::cout << "Phase 1: Initializing components..." << std::endl;
    
    auto manager = std::make_shared<PromptManager>(nullptr, nullptr);
    auto optimizer = std::make_shared<PromptOptimizer>(nullptr, nullptr);
    auto tracker = std::make_shared<PromptPerformanceTracker>(nullptr, nullptr);
    auto feedback_collector = std::make_shared<FeedbackCollector>(nullptr, nullptr);
    auto version_control = std::make_shared<PromptVersionControl>(nullptr, nullptr);
    
    SelfImprovementOrchestrator::ImprovementConfig orchestrator_config;
    orchestrator_config.min_executions = 5;  // Low for demo
    orchestrator_config.min_success_rate = 0.8;
    orchestrator_config.enable_ab_testing = true;
    auto orchestrator = std::make_shared<SelfImprovementOrchestrator>(
        orchestrator_config, tracker, optimizer, manager, nullptr
    );
    
    // ========================================================================
    // Phase 2: Configure Integration Layer
    // ========================================================================
    std::cout << "Phase 2: Configuring integration layer..." << std::endl;
    
    IntegrationConfig config;
    config.enable_auto_versioning = true;
    config.enable_auto_optimization = true;
    config.enable_performance_tracking = true;
    config.enable_feedback_collection = true;
    config.min_executions_before_optimization = 5;
    config.background_worker_enabled = true;
    config.background_worker_interval = std::chrono::seconds(10);  // 10s for demo
    
    auto integration = std::make_shared<PromptEngineeringIntegration>(
        config, manager, optimizer, tracker, orchestrator,
        feedback_collector, version_control
    );
    
    // ========================================================================
    // Phase 3: Register Prompts
    // ========================================================================
    std::cout << "Phase 3: Registering prompts..." << std::endl;
    
    manager->registerTemplate(
        "sql_generator",
        "Generate a SQL query for: {task}\nContext: {context}\nQuery:"
    );
    
    manager->registerTemplate(
        "code_reviewer",
        "Review this code:\n{code}\n\nProvide feedback on:\n- Quality\n- Security\n- Performance"
    );
    
    // ========================================================================
    // Phase 4: Start System
    // ========================================================================
    std::cout << "Phase 4: Starting system..." << std::endl;
    integration->start();
    std::cout << "System started successfully!" << std::endl;
    std::cout << std::endl;
    
    // ========================================================================
    // Phase 5: Simulate Production Usage
    // ========================================================================
    std::cout << "Phase 5: Simulating production usage..." << std::endl;
    
    // Simulate SQL generation requests
    for (int i = 0; i < 10; i++) {
        // Before execution hook
        nlohmann::json context = {
            {"task", "Find all users created in the last 30 days"},
            {"context", "Users table with id, name, email, created_at columns"}
        };
        
        auto exec_ctx = integration->beforeExecution("sql_generator", context);
        
        std::cout << "Execution " << (i + 1) << ":" << std::endl;
        std::cout << "  Execution ID: " << exec_ctx.execution_id << std::endl;
        std::cout << "  Enhanced prompt: " << exec_ctx.enhanced_prompt.substr(0, 50) << "..." << std::endl;
        
        // Simulate LLM execution
        // In real usage, you would call: auto response = llm->generate(exec_ctx.enhanced_prompt);
        std::string response = "SELECT * FROM users WHERE created_at >= DATE_SUB(NOW(), INTERVAL 30 DAY)";
        bool success = (i % 4 != 3);  // 75% success rate
        double latency_ms = 100.0 + (i * 10);
        double user_feedback = success ? 0.8 : 0.2;
        
        // After execution hook
        integration->afterExecution(exec_ctx, response, success, latency_ms, user_feedback);
        
        std::cout << "  Success: " << (success ? "✓" : "✗") << std::endl;
        std::cout << "  Latency: " << latency_ms << "ms" << std::endl;
        std::cout << std::endl;
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // ========================================================================
    // Phase 6: Check System Status
    // ========================================================================
    std::cout << "Phase 6: Checking system status..." << std::endl;
    
    auto status = integration->getStatus();
    std::cout << "System Status:" << std::endl;
    std::cout << "  Running: " << (status.running ? "Yes" : "No") << std::endl;
    std::cout << "  Background Worker: " << (status.background_worker_active ? "Active" : "Inactive") << std::endl;
    std::cout << "  Total Executions: " << status.total_executions << std::endl;
    std::cout << "  Total Optimizations: " << status.total_optimizations << std::endl;
    std::cout << "  Active Prompts: " << status.active_prompts << std::endl;
    std::cout << std::endl;
    
    // ========================================================================
    // Phase 7: View Performance Metrics
    // ========================================================================
    std::cout << "Phase 7: Viewing performance metrics..." << std::endl;
    
    auto metrics = tracker->getMetrics("sql_generator");
    std::cout << "Performance Metrics (sql_generator):" << std::endl;
    std::cout << "  Total Executions: " << metrics.total_executions << std::endl;
    std::cout << "  Successful: " << metrics.successful_executions << std::endl;
    std::cout << "  Failed: " << metrics.failed_executions << std::endl;
    std::cout << "  Success Rate: " << (metrics.success_rate * 100) << "%" << std::endl;
    std::cout << "  Avg Latency: " << metrics.avg_latency_ms << "ms" << std::endl;
    std::cout << "  Avg User Satisfaction: " << metrics.avg_user_satisfaction << std::endl;
    std::cout << std::endl;
    
    // ========================================================================
    // Phase 8: View Feedback
    // ========================================================================
    std::cout << "Phase 8: Viewing feedback..." << std::endl;
    
    auto feedback_stats = feedback_collector->getStats("sql_generator");
    std::cout << "Feedback Stats:" << std::endl;
    std::cout << "  Total Feedback: " << feedback_stats.total_feedback << std::endl;
    std::cout << "  Positive Ratio: " << (feedback_stats.positive_ratio * 100) << "%" << std::endl;
    std::cout << "  Negative Ratio: " << (feedback_stats.negative_ratio * 100) << "%" << std::endl;
    std::cout << "  Hallucinations: " << feedback_stats.hallucination_count << std::endl;
    std::cout << "  Errors: " << feedback_stats.error_count << std::endl;
    std::cout << std::endl;
    
    // ========================================================================
    // Phase 9: View Version History
    // ========================================================================
    std::cout << "Phase 9: Viewing version history..." << std::endl;
    
    auto history = version_control->getHistory("sql_generator", "main", 5);
    std::cout << "Version History (sql_generator, last 5):" << std::endl;
    for (size_t i = 0; i < history.size(); i++) {
        const auto& version = history[i];
        std::cout << "  [" << (i + 1) << "] " << version.version_id.substr(0, 8) << std::endl;
        std::cout << "      Message: " << version.commit_message << std::endl;
        std::cout << "      Author: " << version.author << std::endl;
        std::cout << "      Branch: " << version.branch << std::endl;
        auto timestamp = std::chrono::system_clock::to_time_t(version.timestamp);
        std::cout << "      Time: " << std::ctime(&timestamp);
    }
    std::cout << std::endl;
    
    // ========================================================================
    // Phase 10: Background Worker Status
    // ========================================================================
    std::cout << "Phase 10: Checking background worker..." << std::endl;
    
    auto worker_status = integration->getBackgroundWorkerStatus();
    std::cout << "Background Worker Status:" << std::endl;
    std::cout << "  Running: " << (worker_status.running ? "Yes" : "No") << std::endl;
    std::cout << "  Cycles Completed: " << worker_status.cycles_completed << std::endl;
    std::cout << "  Prompts Optimized: " << worker_status.prompts_optimized << std::endl;
    
    auto last_run = std::chrono::system_clock::to_time_t(worker_status.last_run);
    auto next_run = std::chrono::system_clock::to_time_t(worker_status.next_scheduled_run);
    std::cout << "  Last Run: " << std::ctime(&last_run);
    std::cout << "  Next Run: " << std::ctime(&next_run);
    std::cout << std::endl;
    
    // ========================================================================
    // Phase 11: Complete Statistics
    // ========================================================================
    std::cout << "Phase 11: Complete system statistics..." << std::endl;
    
    auto stats = integration->getStats();
    std::cout << "System Statistics:" << std::endl;
    std::cout << stats.dump(2) << std::endl;
    std::cout << std::endl;
    
    // ========================================================================
    // Phase 12: Manual Optimization Trigger
    // ========================================================================
    std::cout << "Phase 12: Manually triggering optimization..." << std::endl;
    
    if (orchestrator->shouldOptimize("sql_generator")) {
        std::cout << "Optimization needed! Triggering..." << std::endl;
        
        // This would normally use real test cases
        auto result = orchestrator->optimizePrompt("sql_generator");
        
        std::cout << "Optimization Result:" << std::endl;
        std::cout << "  Success: " << (result.success ? "✓" : "✗") << std::endl;
        std::cout << "  Iterations: " << result.iterations_performed << std::endl;
        std::cout << "  Improvement: " << (result.improvement * 100) << "%" << std::endl;
        std::cout << "  Message: " << result.message << std::endl;
        
        if (result.success) {
            // Commit the optimized version
            version_control->commit(
                "sql_generator",
                result.optimized_prompt,
                "Manual optimization: +" + std::to_string(result.improvement * 100) + "%",
                "user"
            );
        }
    } else {
        std::cout << "No optimization needed at this time." << std::endl;
    }
    std::cout << std::endl;
    
    // ========================================================================
    // Phase 13: Shutdown
    // ========================================================================
    std::cout << "Phase 13: Graceful shutdown..." << std::endl;
    
    integration->stop();
    
    std::cout << "System stopped successfully!" << std::endl;
    std::cout << std::endl;
    
    // ========================================================================
    // Summary
    // ========================================================================
    std::cout << "=== Demo Complete ===" << std::endl;
    std::cout << std::endl;
    std::cout << "This demo showcased:" << std::endl;
    std::cout << "  ✓ Seamless LLM integration hooks" << std::endl;
    std::cout << "  ✓ Automatic performance tracking" << std::endl;
    std::cout << "  ✓ Automatic feedback collection" << std::endl;
    std::cout << "  ✓ Automatic versioning" << std::endl;
    std::cout << "  ✓ Background optimization worker" << std::endl;
    std::cout << "  ✓ Manual optimization triggers" << std::endl;
    std::cout << "  ✓ Complete statistics and monitoring" << std::endl;
    std::cout << std::endl;
    std::cout << "All 6 phases working together seamlessly! 🎉" << std::endl;
    
    return 0;
}
