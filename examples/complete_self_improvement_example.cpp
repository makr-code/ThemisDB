/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            complete_self_improvement_example.cpp              ║
  Version:         0.0.40                                             ║
  Last Modified:   2026-04-14 06:49:24                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   93.0/100                                       ║
    • Total Lines:     281                                            ║
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
 * @file complete_self_improvement_example.cpp
 * @brief Complete example of autonomous prompt self-improvement
 * 
 * Demonstrates the full workflow:
 * 1. Performance tracking
 * 2. Automatic optimization triggering
 * 3. A/B testing
 * 4. Deployment and rollback
 */

#include <iostream>
#include <memory>
#include "prompt_engineering/prompt_manager.h"
#include "prompt_engineering/prompt_performance_tracker.h"
#include "prompt_engineering/prompt_optimizer.h"
#include "prompt_engineering/prompt_evaluator.h"
#include "prompt_engineering/self_improvement_orchestrator.h"

using namespace themis::prompt_engineering;

int main() {
    std::cout << "=== Complete Self-Improvement Workflow Example ===\n\n";
    
    // ========================================================================
    // Step 1: Initialize Components
    // ========================================================================
    std::cout << "Step 1: Initializing components...\n";
    
    auto manager = std::make_shared<PromptManager>();
    auto tracker = std::make_shared<PromptPerformanceTracker>();
    
    OptimizationConfig opt_config;
    opt_config.max_iterations = 5;
    auto optimizer = std::make_shared<PromptOptimizer>(opt_config);
    
    EvaluatorConfig eval_config;
    auto evaluator = std::make_shared<PromptEvaluator>(eval_config);
    
    // Configure self-improvement
    ImprovementConfig improvement_config;
    improvement_config.min_success_rate = 0.7;       // Trigger if below 70%
    improvement_config.min_executions = 10;          // Need at least 10 samples
    improvement_config.enable_ab_testing = true;     // Enable A/B testing
    improvement_config.ab_test_sample_size = 20;     // 20 samples for A/B test
    improvement_config.enable_auto_rollback = true;  // Enable rollback
    
    auto orchestrator = std::make_shared<SelfImprovementOrchestrator>(
        improvement_config,
        tracker,
        optimizer,
        manager,
        evaluator
    );
    
    std::cout << "✓ Components initialized\n\n";
    
    // ========================================================================
    // Step 2: Create Initial Prompt Templates
    // ========================================================================
    std::cout << "Step 2: Creating prompt templates...\n";
    
    PromptManager::PromptTemplate summarize_prompt;
    summarize_prompt.name = "Document Summarizer";
    summarize_prompt.version = "v1.0";
    summarize_prompt.content = "Summarize this document: {document}";
    summarize_prompt.description = "Basic document summarization";
    
    auto created = manager->createTemplate(summarize_prompt);
    std::string prompt_id = created.id;
    
    std::cout << "✓ Created prompt: " << prompt_id << "\n\n";
    
    // ========================================================================
    // Step 3: Simulate Production Usage (with poor performance)
    // ========================================================================
    std::cout << "Step 3: Simulating production usage...\n";
    
    // Simulate 15 executions with 40% success rate (below threshold)
    for (int i = 0; i < 15; ++i) {
        bool success = (i < 6);  // 6/15 = 0.4 success rate
        double latency_ms = 100.0 + (rand() % 50);
        
        tracker->recordExecution(prompt_id, success, latency_ms);
        
        if (i % 5 == 0) {
            auto metrics = tracker->getMetrics(prompt_id);
            if (metrics) {
                std::cout << "  After " << metrics->total_executions 
                         << " executions: success_rate=" << metrics->success_rate
                         << ", avg_latency=" << metrics->avg_latency_ms << "ms\n";
            }
        }
    }
    
    auto final_metrics = tracker->getMetrics(prompt_id);
    std::cout << "\n✓ Final metrics: success_rate=" << final_metrics->success_rate
              << " (below threshold of 0.7)\n\n";
    
    // ========================================================================
    // Step 4: Check if Optimization Should Be Triggered
    // ========================================================================
    std::cout << "Step 4: Checking optimization criteria...\n";
    
    bool should_opt = orchestrator->shouldOptimize(prompt_id);
    std::cout << "  Should optimize: " << (should_opt ? "YES" : "NO") << "\n";
    
    if (should_opt) {
        std::cout << "  ✓ Criteria met: low success rate + sufficient samples\n\n";
    }
    
    // ========================================================================
    // Step 5: Manual Optimization (with test cases)
    // ========================================================================
    std::cout << "Step 5: Triggering optimization...\n";
    
    std::vector<TestCase> test_cases = {
        {"Document about AI", "AI is transforming...", {}},
        {"Document about climate", "Climate change affects...", {}},
        {"Document about economics", "The economy is...", {}}
    };
    
    auto opt_result = orchestrator->optimizePrompt(prompt_id, test_cases);
    
    std::cout << "  Optimization result:\n";
    std::cout << "    - Status: " << static_cast<int>(opt_result.status) << "\n";
    std::cout << "    - Baseline score: " << opt_result.baseline_score << "\n";
    std::cout << "    - Optimized score: " << opt_result.optimized_score << "\n";
    std::cout << "    - Improvement: " << (opt_result.improvement * 100) << "%\n";
    std::cout << "    - Iterations: " << opt_result.iterations << "\n";
    
    if (opt_result.metadata.contains("ab_test_id")) {
        std::cout << "    - A/B test ID: " << opt_result.metadata["ab_test_id"] << "\n";
    }
    
    std::cout << "\n✓ Optimization completed\n\n";
    
    // ========================================================================
    // Step 6: A/B Testing (if enabled)
    // ========================================================================
    if (improvement_config.enable_ab_testing && 
        opt_result.metadata.contains("ab_test_id")) {
        
        std::cout << "Step 6: Running A/B test...\n";
        
        std::string test_id = opt_result.metadata["ab_test_id"];
        
        // Simulate A/B test observations
        // Version A (original): 40% success
        // Version B (optimized): 75% success
        for (int i = 0; i < 10; ++i) {
            orchestrator->recordABTestObservation(
                test_id, "a", i < 4, 100.0 + (rand() % 20)
            );
        }
        
        for (int i = 0; i < 10; ++i) {
            orchestrator->recordABTestObservation(
                test_id, "b", i < 8, 95.0 + (rand() % 20)
            );
        }
        
        // Check test results
        auto ab_test = orchestrator->getABTestResults(test_id);
        if (ab_test) {
            std::cout << "  A/B Test Results:\n";
            std::cout << "    - Version A samples: " << ab_test->samples_a << "\n";
            std::cout << "    - Version A score: " << ab_test->score_a << "\n";
            std::cout << "    - Version B samples: " << ab_test->samples_b << "\n";
            std::cout << "    - Version B score: " << ab_test->score_b << "\n";
            std::cout << "    - Statistically significant: " 
                     << (ab_test->is_significant ? "YES" : "NO") << "\n";
            std::cout << "    - P-value: " << ab_test->p_value << "\n";
        }
        
        std::cout << "\n✓ A/B test completed\n\n";
    }
    
    // ========================================================================
    // Step 7: View Optimization History
    // ========================================================================
    std::cout << "Step 7: Viewing optimization history...\n";
    
    auto history = orchestrator->getOptimizationHistory(prompt_id);
    std::cout << "  Total optimizations: " << history.size() << "\n";
    
    for (size_t i = 0; i < history.size(); ++i) {
        const auto& h = history[i];
        std::cout << "  Optimization " << (i + 1) << ":\n";
        std::cout << "    - Improvement: " << (h.improvement * 100) << "%\n";
        std::cout << "    - Status: " << static_cast<int>(h.status) << "\n";
        std::cout << "    - Iterations: " << h.iterations << "\n";
    }
    
    std::cout << "\n✓ History retrieved\n\n";
    
    // ========================================================================
    // Step 8: Automatic Optimization Scan
    // ========================================================================
    std::cout << "Step 8: Running automatic optimization scan...\n";
    
    auto auto_results = orchestrator->runAutoOptimization();
    std::cout << "  Prompts optimized: " << auto_results.size() << "\n";
    
    std::cout << "\n✓ Auto-optimization scan completed\n\n";
    
    // ========================================================================
    // Step 9: Performance Summary
    // ========================================================================
    std::cout << "Step 9: Performance summary...\n";
    
    auto summary = tracker->getSummaryStatistics();
    std::cout << "  Total prompts tracked: " << summary["total_prompts"] << "\n";
    std::cout << "  Total executions: " << summary["total_executions"] << "\n";
    std::cout << "  Average success rate: " << summary["avg_success_rate"] << "\n";
    std::cout << "  Average latency: " << summary["avg_latency_ms"] << "ms\n";
    
    std::cout << "\n✓ Summary generated\n\n";
    
    // ========================================================================
    // Step 10: Rollback Example (optional)
    // ========================================================================
    std::cout << "Step 10: Demonstrating rollback capability...\n";
    
    // In production, rollback would be triggered if:
    // - New version performs worse than expected
    // - Critical issues detected
    // - Manual intervention required
    
    std::cout << "  Rollback available for prompt: " << prompt_id << "\n";
    std::cout << "  (Execute orchestrator->rollbackPrompt(prompt_id) to rollback)\n";
    
    std::cout << "\n✓ Rollback mechanism ready\n\n";
    
    // ========================================================================
    // Complete!
    // ========================================================================
    std::cout << "=== Self-Improvement Workflow Complete! ===\n\n";
    
    std::cout << "Key Capabilities Demonstrated:\n";
    std::cout << "  ✓ Performance tracking with metrics\n";
    std::cout << "  ✓ Automatic optimization triggering\n";
    std::cout << "  ✓ Manual optimization with test cases\n";
    std::cout << "  ✓ A/B testing framework\n";
    std::cout << "  ✓ Statistical significance testing\n";
    std::cout << "  ✓ Optimization history tracking\n";
    std::cout << "  ✓ Automatic rollback capability\n";
    std::cout << "  ✓ Summary statistics\n\n";
    
    std::cout << "Next Steps:\n";
    std::cout << "  1. Integrate with production LLM calls\n";
    std::cout << "  2. Enable scheduled optimization checks\n";
    std::cout << "  3. Configure A/B test parameters\n";
    std::cout << "  4. Set up monitoring and alerts\n";
    std::cout << "  5. Implement feedback collection\n\n";
    
    return 0;
}
