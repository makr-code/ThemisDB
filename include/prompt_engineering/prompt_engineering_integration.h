/**
 * @file prompt_engineering_integration.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>
#include <thread>
#include <atomic>
#include <mutex>
#include <nlohmann/json.hpp>

#include "prompt_manager.h"
#include "prompt_optimizer.h"
#include "prompt_performance_tracker.h"
#include "self_improvement_orchestrator.h"
#include "feedback_collector.h"
#include "prompt_version_control.h"
#include "prompt_injection_detector.h"
#include "reflection_tuner.h"
#include "prompt_engineering_metrics.h"

namespace themis {
namespace prompt_engineering {

/**
 * @brief Configuration for the integration layer
 */
struct IntegrationConfig {
    // Auto-versioning
    bool enable_auto_versioning = true;
    
    // Auto-optimization
    bool enable_auto_optimization = true;
    std::chrono::seconds optimization_check_interval = std::chrono::hours(1);
    bool auto_commit_on_optimization = true;
    
    // Performance tracking
    bool enable_performance_tracking = true;
    
    // Feedback collection
    bool enable_feedback_collection = true;
    
    // Injection detection
    bool enable_injection_detection = true;

    // Reflection Tuning (optional; off by default — zero overhead when disabled)
    bool enable_reflection_tuning = false;
    size_t reflection_max_iterations = 3;

    // Optimization thresholds
    size_t min_executions_before_optimization = 100;
    double min_success_rate_for_optimization = 0.7;
    
    // Background worker
    bool background_worker_enabled = true;
    std::chrono::seconds background_worker_interval = std::chrono::hours(1);
    
    // Serialization
    nlohmann::json toJson() const;
    static IntegrationConfig fromJson(const nlohmann::json& j);
};

/**
 * @brief Execution context for tracking a single LLM call
 */
struct ExecutionContext {
    std::string execution_id;  // Unique ID for this execution
    std::string prompt_id;
    std::string original_prompt;
    std::string enhanced_prompt;
    nlohmann::json context;
    std::string version_id;  // Version used for this execution
    std::chrono::system_clock::time_point start_time;
    bool injection_detected = false;   // True when a prompt injection attempt was detected
    float injection_risk_score = 0.0f; // Risk score from PromptInjectionDetector [0,1]
    
    // Serialization
    nlohmann::json toJson() const;
    static ExecutionContext fromJson(const nlohmann::json& j);
};

/**
 * @brief Status of the integration system
 */
struct IntegrationStatus {
    bool running = false;
    bool background_worker_active = false;
    size_t total_executions = 0;
    size_t total_optimizations = 0;
    std::chrono::system_clock::time_point last_optimization;
    size_t active_prompts = 0;
    std::unordered_map<std::string, size_t> executions_by_prompt;
    
    // Serialization
    nlohmann::json toJson() const;
};

/**
 * @brief Worker status
 */
struct WorkerStatus {
    bool running = false;
    size_t cycles_completed = 0;
    size_t prompts_optimized = 0;
    std::chrono::system_clock::time_point last_run;
    std::chrono::system_clock::time_point next_scheduled_run;
    
    // Serialization
    nlohmann::json toJson() const;
};

/**
 * @brief Background optimization worker
 * 
 * Runs periodic optimization cycles on all prompts.
 */
class BackgroundOptimizationWorker {
public:
    BackgroundOptimizationWorker(
        std::chrono::seconds interval,
        std::shared_ptr<PromptPerformanceTracker> tracker,
        std::shared_ptr<SelfImprovementOrchestrator> orchestrator,
        std::shared_ptr<PromptVersionControl> version_control
    );
    
    ~BackgroundOptimizationWorker();
    
    // Lifecycle
    void start();
    void stop();
    bool isRunning() const;
    
    // Status
    WorkerStatus getStatus() const;
    
    // Manual trigger
    void runOptimizationCycle();
    
private:
    void workerLoop();
    
    std::chrono::seconds interval_;
    std::shared_ptr<PromptPerformanceTracker> tracker_;
    std::shared_ptr<SelfImprovementOrchestrator> orchestrator_;
    std::shared_ptr<PromptVersionControl> version_control_;
    
    std::atomic<bool> running_{false};
    std::atomic<bool> stop_requested_{false};
    std::unique_ptr<std::thread> worker_thread_;
    mutable std::mutex mutex_;
    
    // Statistics
    size_t cycles_completed_ = 0;
    size_t prompts_optimized_ = 0;
    std::chrono::system_clock::time_point last_run_;
};

/**
 * @brief Main integration point for prompt engineering system
 * 
 * This class provides seamless hooks into the LLM execution flow,
 * automatically tracking performance, collecting feedback, and
 * triggering optimization when needed.
 * 
 * Usage:
 * @code
 * auto integration = std::make_shared<PromptEngineeringIntegration>(...);
 * integration->start();
 * 
 * // Before LLM execution
 * auto ctx = integration->beforeExecution(prompt_id, context);
 * 
 * // Execute with enhanced prompt
 * auto response = llm->generate(ctx.enhanced_prompt);
 * 
 * // After LLM execution
 * integration->afterExecution(ctx, response, success, latency);
 * @endcode
 */
class PromptEngineeringIntegration {
public:
    PromptEngineeringIntegration(
        const IntegrationConfig& config,
        std::shared_ptr<PromptManager> manager,
        std::shared_ptr<PromptOptimizer> optimizer,
        std::shared_ptr<PromptPerformanceTracker> tracker,
        std::shared_ptr<SelfImprovementOrchestrator> orchestrator,
        std::shared_ptr<FeedbackCollector> feedback_collector,
        std::shared_ptr<PromptVersionControl> version_control,
        std::shared_ptr<PromptInjectionDetector> injection_detector = nullptr
    );
    
    ~PromptEngineeringIntegration();
    
    // Lifecycle management
    void start();
    void stop();
    IntegrationStatus getStatus() const;
    
    // Execution hooks
    ExecutionContext beforeExecution(
        const std::string& prompt_id,
        const nlohmann::json& context = {}
    );
    
    void afterExecution(
        const ExecutionContext& ctx,
        const std::string& response,
        bool success,
        double latency_ms,
        double user_feedback = 0.0
    );
    
    // Background optimization
    void startBackgroundOptimization();
    void stopBackgroundOptimization();
    WorkerStatus getBackgroundWorkerStatus() const;
    
    // Statistics
    nlohmann::json getStats() const;
    
    // Configuration
    IntegrationConfig getConfig() const;
    void updateConfig(const IntegrationConfig& config);

    // Reflection Tuning integration
    /**
     * @brief Attach a `ReflectionTuner` for optional post-generation response
     *        refinement.
     *
     * When set and `IntegrationConfig::enable_reflection_tuning` is `true`,
     * `afterExecution()` runs a reflection cycle on each successful response
     * and records the results via `PromptEngineeringMetrics`.
     */
    void setReflectionTuner(std::shared_ptr<ReflectionTuner> tuner);

    /**
     * @brief Attach a `PromptEngineeringMetrics` instance for reflection
     *        observability.  When not set, reflection metrics are silently
     *        discarded.
     */
    void setMetrics(std::shared_ptr<PromptEngineeringMetrics> metrics);
    
private:
    void checkAndTriggerOptimization(const std::string& prompt_id);
    std::string enhancePrompt(
        const std::string& prompt_id,
        const nlohmann::json& context
    );
    std::string generateExecutionId();
    
    IntegrationConfig config_;
    std::shared_ptr<PromptManager> manager_;
    std::shared_ptr<PromptOptimizer> optimizer_;
    std::shared_ptr<PromptPerformanceTracker> tracker_;
    std::shared_ptr<SelfImprovementOrchestrator> orchestrator_;
    std::shared_ptr<FeedbackCollector> feedback_collector_;
    std::shared_ptr<PromptVersionControl> version_control_;
    std::shared_ptr<PromptInjectionDetector> injection_detector_;
    std::shared_ptr<ReflectionTuner>         reflection_tuner_;
    std::shared_ptr<PromptEngineeringMetrics> metrics_;
    
    std::unique_ptr<BackgroundOptimizationWorker> background_worker_;
    
    mutable std::mutex mutex_;
    bool running_ = false;
    
    // Statistics
    size_t total_executions_ = 0;
    size_t total_optimizations_ = 0;
    std::chrono::system_clock::time_point last_optimization_;
    std::unordered_map<std::string, size_t> executions_by_prompt_;
    
    // Active executions (for cleanup)
    std::unordered_map<std::string, ExecutionContext> active_executions_;
};

} // namespace prompt_engineering
} // namespace themis
