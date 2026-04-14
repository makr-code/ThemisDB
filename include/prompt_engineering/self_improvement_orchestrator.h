/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            self_improvement_orchestrator.h                    ║
  Version:         0.0.39                                             ║
  Last Modified:   2026-04-13 20:24:47                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     346                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file self_improvement_orchestrator.h
 * @brief Orchestrates autonomous prompt optimization and A/B testing
 * 
 * Coordinates the self-improvement workflow:
 * 1. Monitors prompt performance via PromptPerformanceTracker
 * 2. Triggers optimization when thresholds are met
 * 3. Manages A/B testing of optimized prompts
 * 4. Performs automatic rollback on performance degradation
 * 
 * This is the central coordinator for autonomous prompt engineering.
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <chrono>
#include <mutex>
#include <nlohmann/json.hpp>

namespace themis {
namespace prompt_engineering {

// Forward declarations
class PromptPerformanceTracker;
class PromptOptimizer;
class PromptManager;
class PromptEvaluator;
class FeedbackCollector;
struct TestCase;

/**
 * @brief Configuration for self-improvement orchestration
 */
struct ImprovementConfig {
    // Trigger conditions
    double min_success_rate = 0.8;           ///< Trigger optimization if below this
    size_t min_executions = 100;             ///< Minimum executions before optimization
    std::chrono::hours reoptimize_interval{24}; ///< Time between re-optimizations
    
    // Optimization strategy
    size_t max_iterations = 5;               ///< Maximum optimization iterations
    double target_improvement = 0.1;         ///< Target improvement (10%)
    
    // A/B testing
    bool enable_ab_testing = true;           ///< Enable A/B testing before full deployment
    size_t ab_test_sample_size = 1000;       ///< Number of samples for A/B test
    double ab_test_confidence = 0.95;        ///< Confidence level for A/B test
    
    // Safety
    bool enable_auto_rollback = true;        ///< Enable automatic rollback
    double rollback_threshold = 0.9;         ///< Rollback if performance drops below this factor
    size_t rollback_grace_period_samples = 50; ///< Samples to observe before rollback
    
    // Scheduling
    bool enable_scheduled_optimization = false; ///< Enable time-based optimization
    std::chrono::hours schedule_interval{168};  ///< Weekly optimization by default
};

/**
 * @brief Status of an optimization operation
 */
enum class OptimizationStatus {
    NOT_STARTED,      ///< Optimization not yet started
    IN_PROGRESS,      ///< Optimization currently running
    COMPLETED,        ///< Optimization completed successfully
    AB_TESTING,       ///< A/B testing in progress
    DEPLOYED,         ///< New version deployed to production
    ROLLED_BACK,      ///< Rolled back due to poor performance
    FAILED            ///< Optimization failed
};

/**
 * @brief Result of an optimization operation
 */
struct OptimizationResult {
    std::string prompt_id;                   ///< Prompt being optimized
    std::string original_version;            ///< Original prompt version
    std::string optimized_version;           ///< Optimized prompt version
    OptimizationStatus status;               ///< Current status
    double baseline_score = 0.0;             ///< Original performance score
    double optimized_score = 0.0;            ///< Optimized performance score
    double improvement = 0.0;                ///< Improvement percentage
    size_t iterations = 0;                   ///< Number of optimization iterations
    std::chrono::system_clock::time_point started_at;   ///< Optimization start time
    std::chrono::system_clock::time_point completed_at; ///< Optimization completion time
    nlohmann::json metadata;                 ///< Additional metadata
    
    /**
     * @brief Convert result to JSON
     */
    nlohmann::json toJson() const;
};

/**
 * @brief A/B test configuration and results
 */
struct ABTest {
    std::string test_id;                     ///< Unique test identifier
    std::string prompt_id;                   ///< Prompt being tested
    std::string version_a;                   ///< Control version (original)
    std::string version_b;                   ///< Treatment version (optimized)
    size_t samples_a = 0;                    ///< Samples for version A
    size_t samples_b = 0;                    ///< Samples for version B
    double score_a = 0.0;                    ///< Average score for version A
    double score_b = 0.0;                    ///< Average score for version B
    bool is_significant = false;             ///< Whether results are statistically significant
    double p_value = 1.0;                    ///< Statistical p-value
    std::chrono::system_clock::time_point started_at;   ///< Test start time
    std::chrono::system_clock::time_point completed_at; ///< Test completion time
    
    /**
     * @brief Convert test to JSON
     */
    nlohmann::json toJson() const;
};

/**
 * @brief Self-improvement orchestrator
 * 
 * Coordinates autonomous prompt optimization:
 * - Monitors performance metrics
 * - Triggers optimization automatically
 * - Manages A/B testing
 * - Handles rollback on degradation
 * 
 * Thread-safe for concurrent operation.
 */
class SelfImprovementOrchestrator {
public:
    /**
     * @brief Constructor
     * @param config Improvement configuration
     * @param tracker Performance tracker
     * @param optimizer Prompt optimizer
     * @param manager Prompt manager
     * @param evaluator Prompt evaluator
     */
    SelfImprovementOrchestrator(
        const ImprovementConfig& config,
        std::shared_ptr<PromptPerformanceTracker> tracker,
        std::shared_ptr<PromptOptimizer> optimizer,
        std::shared_ptr<PromptManager> manager,
        std::shared_ptr<PromptEvaluator> evaluator
    );
    
    /**
     * @brief Run automatic optimization check
     * 
     * Scans all tracked prompts and triggers optimization for those
     * that meet the trigger conditions.
     * 
     * @return Vector of optimization results
     */
    std::vector<OptimizationResult> runAutoOptimization();
    
    /**
     * @brief Manually trigger optimization for a specific prompt
     * @param prompt_id Prompt template ID
     * @param test_cases Test cases for evaluation
     * @return Optimization result
     */
    OptimizationResult optimizePrompt(
        const std::string& prompt_id,
        const std::vector<TestCase>& test_cases
    );
    
    /**
     * @brief Start an A/B test between two prompt versions
     * @param prompt_id Prompt template ID
     * @param version_a Control version
     * @param version_b Treatment version
     * @param sample_size Number of samples for the test
     * @return Test ID
     */
    std::string startABTest(
        const std::string& prompt_id,
        const std::string& version_a,
        const std::string& version_b,
        size_t sample_size = 0  // 0 = use config default
    );
    
    /**
     * @brief Record an A/B test observation
     * @param test_id Test identifier
     * @param version_used Version that was used (a or b)
     * @param success Whether the execution succeeded
     * @param latency_ms Execution latency
     */
    void recordABTestObservation(
        const std::string& test_id,
        const std::string& version_used,
        bool success,
        double latency_ms
    );
    
    /**
     * @brief Check if an A/B test is complete and analyze results
     * @param test_id Test identifier
     * @return true if test is complete, false otherwise
     */
    bool checkABTestCompletion(const std::string& test_id);
    
    /**
     * @brief Get A/B test results
     * @param test_id Test identifier
     * @return Test results, or nullopt if test not found
     */
    std::optional<ABTest> getABTestResults(const std::string& test_id) const;
    
    /**
     * @brief Rollback a prompt to its previous version
     * @param prompt_id Prompt template ID
     * @return true if rollback successful
     */
    bool rollbackPrompt(const std::string& prompt_id);
    
    /**
     * @brief Get optimization history for a prompt
     * @param prompt_id Prompt template ID
     * @return Vector of optimization results
     */
    std::vector<OptimizationResult> getOptimizationHistory(
        const std::string& prompt_id
    ) const;
    
    /**
     * @brief Get all active A/B tests
     * @return Vector of active tests
     */
    std::vector<ABTest> getActiveABTests() const;
    
    /**
     * @brief Get current configuration
     */
    const ImprovementConfig& getConfig() const { return config_; }
    
    /**
     * @brief Update configuration
     */
    void setConfig(const ImprovementConfig& config);

    /**
     * @brief Attach a FeedbackCollector for synthetic test-case generation
     *
     * When set, @c runAutoOptimization() will build test cases from historical
     * positive-feedback entries (query → response) instead of skipping prompts
     * that have no externally supplied test cases.
     *
     * @param collector Shared pointer to the FeedbackCollector instance
     */
    void setFeedbackCollector(std::shared_ptr<FeedbackCollector> collector) {
        std::lock_guard<std::mutex> lock(mutex_);
        feedback_collector_ = std::move(collector);
    }
    
    /**
     * @brief Check if a prompt should be optimized
     * @param prompt_id Prompt template ID
     * @return true if optimization should be triggered
     */
    bool shouldOptimize(const std::string& prompt_id) const;

private:
    ImprovementConfig config_;
    std::shared_ptr<PromptPerformanceTracker> tracker_;
    std::shared_ptr<PromptOptimizer> optimizer_;
    std::shared_ptr<PromptManager> manager_;
    std::shared_ptr<PromptEvaluator> evaluator_;
    std::shared_ptr<FeedbackCollector> feedback_collector_;  ///< Optional, for auto test-case synthesis
    
    mutable std::mutex mutex_;
    
    // Optimization history: prompt_id -> results
    std::unordered_map<std::string, std::vector<OptimizationResult>> optimization_history_;
    
    // Active A/B tests: test_id -> test
    std::unordered_map<std::string, ABTest> active_ab_tests_;
    
    // Last optimization time: prompt_id -> timestamp
    std::unordered_map<std::string, std::chrono::system_clock::time_point> last_optimization_;
    
    /**
     * @brief Generate unique test ID
     */
    std::string generateTestId() const;
    
    /**
     * @brief Check if enough time has passed since last optimization
     */
    bool canReoptimize(const std::string& prompt_id) const;
    
    /**
     * @brief Perform statistical analysis on A/B test results
     */
    void analyzeABTest(ABTest& test);
    
    /**
     * @brief Deploy optimized prompt version
     */
    void deployOptimizedVersion(const std::string& prompt_id, const std::string& version);

    /**
     * @brief Build synthetic test cases from positive historical feedback
     *
     * Uses query→response pairs from @c USER_POSITIVE feedback as training
     * examples so that @c runAutoOptimization() does not need external test
     * data.
     *
     * @param prompt_id Prompt template ID
     * @param max_cases Maximum number of test cases to return (0 = all)
     * @return Vector of synthetic TestCase objects; empty if no feedback available
     */
    std::vector<TestCase> buildTestCasesFromFeedback(
        const std::string& prompt_id,
        size_t max_cases = 50
    ) const;
};

} // namespace prompt_engineering
} // namespace themis
