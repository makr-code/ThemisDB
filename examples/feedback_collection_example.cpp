/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            feedback_collection_example.cpp                    ║
  Version:         0.0.43                                             ║
  Last Modified:   2026-04-15 04:08:36                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   93.0/100                                       ║
    • Total Lines:     338                                            ║
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
 * @file feedback_collection_example.cpp
 * @brief Example demonstrating feedback collection and analysis
 * 
 * Shows how to:
 * - Record different types of feedback
 * - Analyze failure patterns
 * - Identify problematic prompts
 * - Integrate with optimization workflow
 */

#include <iostream>
#include <memory>
#include "prompt_engineering/feedback_collector.h"
#include "prompt_engineering/prompt_performance_tracker.h"
#include "prompt_engineering/self_improvement_orchestrator.h"

using namespace themis::prompt_engineering;

int main() {
    std::cout << "=== Feedback Collection System Example ===\n\n";
    
    // ========================================================================
    // Step 1: Initialize Feedback Collector
    // ========================================================================
    std::cout << "Step 1: Initializing feedback collector...\n";
    
    auto feedback_collector = std::make_shared<FeedbackCollector>();
    
    std::cout << "✓ Feedback collector initialized\n\n";
    
    // ========================================================================
    // Step 2: Simulate User Feedback
    // ========================================================================
    std::cout << "Step 2: Recording user feedback...\n";
    
    std::string prompt_id = "summarization_v1";
    
    // Positive feedback
    feedback_collector->recordFeedback(
        prompt_id,
        "Summarize this article about AI",
        "AI is transforming healthcare through...",
        FeedbackType::USER_POSITIVE,
        "Very clear and concise!",
        0.9  // High satisfaction
    );
    
    // Negative feedback
    feedback_collector->recordFeedback(
        prompt_id,
        "Summarize this technical paper",
        "The paper discusses...",
        FeedbackType::USER_NEGATIVE,
        "Too simplified, missing key details",
        0.3  // Low satisfaction
    );
    
    std::cout << "✓ User feedback recorded\n\n";
    
    // ========================================================================
    // Step 3: Simulate System-Detected Issues
    // ========================================================================
    std::cout << "Step 3: Recording system-detected issues...\n";
    
    // Hallucination detected
    feedback_collector->recordFeedback(
        prompt_id,
        "What is the capital of Atlantis?",
        "The capital of Atlantis is Poseidon City...",
        FeedbackType::HALLUCINATION_DETECTED,
        "Response contains fabricated information",
        0.8  // High severity
    );
    
    // Parse error
    feedback_collector->recordFeedback(
        prompt_id,
        "List three benefits",
        "Here are the benefits: 1. First 2. Second",
        FeedbackType::PARSE_ERROR,
        "Expected 3 items, got 2",
        0.5
    );
    
    // Timeout
    feedback_collector->recordFeedback(
        prompt_id,
        "Analyze this 10000 word document",
        "",
        FeedbackType::TIMEOUT,
        "Query exceeded 30s timeout",
        0.6
    );
    
    std::cout << "✓ System issues recorded\n\n";
    
    // ========================================================================
    // Step 4: Get Feedback Statistics
    // ========================================================================
    std::cout << "Step 4: Analyzing feedback statistics...\n";
    
    auto stats = feedback_collector->getStats(prompt_id);
    
    std::cout << "  Feedback Statistics:\n";
    std::cout << "    - Total feedback: " << stats.total_feedback << "\n";
    std::cout << "    - Positive ratio: " << (stats.positive_ratio * 100) << "%\n";
    std::cout << "    - Negative ratio: " << (stats.negative_ratio * 100) << "%\n";
    std::cout << "    - Hallucinations: " << stats.hallucination_count << "\n";
    std::cout << "    - Errors: " << stats.error_count << "\n";
    
    if (!stats.common_issues.empty()) {
        std::cout << "    - Common issues:\n";
        for (const auto& issue : stats.common_issues) {
            std::cout << "      * " << issue << "\n";
        }
    }
    
    std::cout << "\n✓ Statistics calculated\n\n";
    
    // ========================================================================
    // Step 5: Analyze Failed Queries
    // ========================================================================
    std::cout << "Step 5: Analyzing failed queries...\n";
    
    // Simulate more failures for pattern analysis
    feedback_collector->recordFeedback(
        prompt_id,
        "How do I install Python",
        "...",
        FeedbackType::CONTEXT_MISSING,
        "Missing OS context"
    );
    feedback_collector->recordFeedback(
        prompt_id,
        "How do I setup Docker",
        "...",
        FeedbackType::CONTEXT_MISSING,
        "Missing environment details"
    );
    feedback_collector->recordFeedback(
        prompt_id,
        "How do I configure MySQL",
        "...",
        FeedbackType::CONTEXT_MISSING,
        "Missing version info"
    );
    
    auto failed_queries = feedback_collector->getFailedQueries(prompt_id, 10);
    
    std::cout << "  Failed Queries (" << failed_queries.size() << " total):\n";
    for (size_t i = 0; i < std::min(size_t(3), failed_queries.size()); ++i) {
        const auto& [query, response, type] = failed_queries[i];
        std::cout << "    " << (i+1) << ". \"" << query << "\"\n";
        std::cout << "       Type: " << feedbackTypeToString(type) << "\n";
    }
    
    std::cout << "\n✓ Failed queries retrieved\n\n";
    
    // ========================================================================
    // Step 6: Pattern Analysis
    // ========================================================================
    std::cout << "Step 6: Identifying failure patterns...\n";
    
    auto patterns = feedback_collector->analyzeFailurePatterns(prompt_id, 2);
    
    std::cout << "  Identified Patterns (" << patterns.size() << "):\n";
    for (const auto& pattern : patterns) {
        std::cout << "    Pattern: \"" << pattern.pattern << "\"\n";
        std::cout << "      - Occurrences: " << pattern.occurrences << "\n";
        std::cout << "      - Primary type: " << feedbackTypeToString(pattern.primary_type) << "\n";
        std::cout << "      - Avg severity: " << pattern.avg_severity << "\n";
        std::cout << "      - Examples:\n";
        for (size_t i = 0; i < std::min(size_t(2), pattern.examples.size()); ++i) {
            std::cout << "        * " << pattern.examples[i] << "\n";
        }
        std::cout << "\n";
    }
    
    std::cout << "✓ Patterns identified\n\n";
    
    // ========================================================================
    // Step 7: Identify Problematic Prompts
    // ========================================================================
    std::cout << "Step 7: Identifying problematic prompts...\n";
    
    // Create another prompt with more negative feedback
    std::string bad_prompt = "query_handler_v1";
    for (int i = 0; i < 7; ++i) {
        feedback_collector->recordFeedback(
            bad_prompt,
            "query " + std::to_string(i),
            "response",
            FeedbackType::USER_NEGATIVE
        );
    }
    for (int i = 0; i < 3; ++i) {
        feedback_collector->recordFeedback(
            bad_prompt,
            "query",
            "response",
            FeedbackType::USER_POSITIVE
        );
    }
    
    auto problematic = feedback_collector->getPromptsWithNegativeFeedback(0.5, 5);
    
    std::cout << "  Prompts with >= 50% negative feedback:\n";
    for (const auto& pid : problematic) {
        auto pstats = feedback_collector->getStats(pid);
        std::cout << "    - " << pid << ": "
                  << (pstats.negative_ratio * 100) << "% negative ("
                  << pstats.total_feedback << " total)\n";
    }
    
    std::cout << "\n✓ Problematic prompts identified\n\n";
    
    // ========================================================================
    // Step 8: Integration with Optimization
    // ========================================================================
    std::cout << "Step 8: Demonstrating optimization integration...\n";
    
    std::cout << "  Optimization Workflow:\n";
    std::cout << "    1. FeedbackCollector identifies prompts with issues\n";
    std::cout << "    2. Retrieve failed queries for test case generation\n";
    std::cout << "    3. Analyze patterns to understand root causes\n";
    std::cout << "    4. Pass insights to SelfImprovementOrchestrator\n";
    std::cout << "    5. Optimize prompt with failure context\n";
    
    std::cout << "\n  Example code:\n";
    std::cout << "  ```cpp\n";
    std::cout << "  auto problematic = feedback_collector->getPromptsWithNegativeFeedback();\n";
    std::cout << "  for (const auto& prompt_id : problematic) {\n";
    std::cout << "      auto failed = feedback_collector->getFailedQueries(prompt_id);\n";
    std::cout << "      auto patterns = feedback_collector->analyzeFailurePatterns(prompt_id);\n";
    std::cout << "      \n";
    std::cout << "      // Generate test cases from failures\n";
    std::cout << "      std::vector<TestCase> test_cases;\n";
    std::cout << "      for (const auto& [query, expected, type] : failed) {\n";
    std::cout << "          test_cases.push_back({query, expected, {}});\n";
    std::cout << "      }\n";
    std::cout << "      \n";
    std::cout << "      // Optimize with context\n";
    std::cout << "      orchestrator->optimizePrompt(prompt_id, test_cases);\n";
    std::cout << "  }\n";
    std::cout << "  ```\n";
    
    std::cout << "\n✓ Integration demonstrated\n\n";
    
    // ========================================================================
    // Step 9: Summary Across All Prompts
    // ========================================================================
    std::cout << "Step 9: Getting system-wide summary...\n";
    
    auto summary = feedback_collector->getSummary();
    
    std::cout << "  System-Wide Summary:\n";
    std::cout << "    - Total prompts tracked: " << summary["total_prompts_tracked"] << "\n";
    std::cout << "    - Total feedback: " << summary["total_feedback"] << "\n";
    std::cout << "    - Positive feedback: " << summary["positive_feedback"] << "\n";
    std::cout << "    - Negative feedback: " << summary["negative_feedback"] << "\n";
    std::cout << "    - Hallucinations: " << summary["hallucinations"] << "\n";
    std::cout << "    - Errors: " << summary["errors"] << "\n";
    
    if (summary.contains("positive_ratio")) {
        std::cout << "    - Overall positive ratio: "
                  << (summary["positive_ratio"].get<double>() * 100) << "%\n";
    }
    
    std::cout << "\n✓ Summary generated\n\n";
    
    // ========================================================================
    // Step 10: Maintenance Operations
    // ========================================================================
    std::cout << "Step 10: Demonstrating maintenance operations...\n";
    
    std::cout << "  Available operations:\n";
    std::cout << "    - pruneOldFeedback(): Remove feedback older than X days\n";
    std::cout << "    - clearFeedback(): Clear all feedback for a prompt\n";
    std::cout << "    - getFeedbackInTimeRange(): Time-based queries\n";
    
    // Example: Get recent feedback (last hour)
    auto now = std::chrono::system_clock::now();
    auto one_hour_ago = now - std::chrono::hours(1);
    auto recent = feedback_collector->getFeedbackInTimeRange(prompt_id, one_hour_ago, now);
    
    std::cout << "\n  Recent feedback (last hour): " << recent.size() << " entries\n";
    
    std::cout << "\n✓ Maintenance demonstrated\n\n";
    
    // ========================================================================
    // Complete!
    // ========================================================================
    std::cout << "=== Feedback Collection Example Complete! ===\n\n";
    
    std::cout << "Key Capabilities Demonstrated:\n";
    std::cout << "  ✓ Recording user feedback (positive/negative)\n";
    std::cout << "  ✓ Recording system-detected issues\n";
    std::cout << "  ✓ Statistical analysis\n";
    std::cout << "  ✓ Failed query retrieval\n";
    std::cout << "  ✓ Pattern analysis\n";
    std::cout << "  ✓ Problem identification\n";
    std::cout << "  ✓ Integration with optimization\n";
    std::cout << "  ✓ System-wide summaries\n";
    std::cout << "  ✓ Maintenance operations\n\n";
    
    std::cout << "Production Integration:\n";
    std::cout << "  1. Record feedback after each LLM execution\n";
    std::cout << "  2. Run periodic analysis to identify issues\n";
    std::cout << "  3. Feed insights to optimization workflow\n";
    std::cout << "  4. Monitor trends over time\n";
    std::cout << "  5. Prune old data regularly\n\n";
    
    return 0;
}
