/**
 * @file prompt_engineering_integration.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=5, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "prompt_engineering/prompt_engineering_integration.h"
#include <random>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace themis {
namespace prompt_engineering {

// ============================================================================
// IntegrationConfig
// ============================================================================

nlohmann::json IntegrationConfig::toJson() const {
    return {
        {"enable_auto_versioning", enable_auto_versioning},
        {"enable_auto_optimization", enable_auto_optimization},
        {"optimization_check_interval_seconds", optimization_check_interval.count()},
        {"auto_commit_on_optimization", auto_commit_on_optimization},
        {"enable_performance_tracking", enable_performance_tracking},
        {"enable_feedback_collection", enable_feedback_collection},
        {"enable_injection_detection", enable_injection_detection},
        {"enable_reflection_tuning", enable_reflection_tuning},
        {"reflection_max_iterations", reflection_max_iterations},
        {"min_executions_before_optimization", min_executions_before_optimization},
        {"min_success_rate_for_optimization", min_success_rate_for_optimization},
        {"background_worker_enabled", background_worker_enabled},
        {"background_worker_interval_seconds", background_worker_interval.count()}
    };
}

IntegrationConfig IntegrationConfig::fromJson(const nlohmann::json& j) {
    IntegrationConfig config;
    config.enable_auto_versioning = j.value("enable_auto_versioning", true);
    config.enable_auto_optimization = j.value("enable_auto_optimization", true);
    config.optimization_check_interval = std::chrono::seconds(
        j.value("optimization_check_interval_seconds", 3600));
    config.auto_commit_on_optimization = j.value("auto_commit_on_optimization", true);
    config.enable_performance_tracking = j.value("enable_performance_tracking", true);
    config.enable_feedback_collection = j.value("enable_feedback_collection", true);
    config.enable_injection_detection = j.value("enable_injection_detection", true);
    config.enable_reflection_tuning = j.value("enable_reflection_tuning", false);
    config.reflection_max_iterations = j.value("reflection_max_iterations", size_t{3});
    config.min_executions_before_optimization = j.value("min_executions_before_optimization", 100);
    config.min_success_rate_for_optimization = j.value("min_success_rate_for_optimization", 0.7);
    config.background_worker_enabled = j.value("background_worker_enabled", true);
    config.background_worker_interval = std::chrono::seconds(
        j.value("background_worker_interval_seconds", 3600));
    return config;
}

// ============================================================================
// ExecutionContext
// ============================================================================

nlohmann::json ExecutionContext::toJson() const {
    auto timestamp = std::chrono::system_clock::to_time_t(start_time);
    return {
        {"execution_id", execution_id},
        {"prompt_id", prompt_id},
        {"original_prompt", original_prompt},
        {"enhanced_prompt", enhanced_prompt},
        {"context", context},
        {"version_id", version_id},
        {"start_time", timestamp},
        {"injection_detected", injection_detected},
        {"injection_risk_score", injection_risk_score}
    };
}

ExecutionContext ExecutionContext::fromJson(const nlohmann::json& j) {
    ExecutionContext ctx;
    ctx.execution_id = j.value("execution_id", "");
    ctx.prompt_id = j.value("prompt_id", "");
    ctx.original_prompt = j.value("original_prompt", "");
    ctx.enhanced_prompt = j.value("enhanced_prompt", "");
    ctx.context = j.value("context", nlohmann::json::object());
    ctx.version_id = j.value("version_id", "");
    ctx.injection_detected = j.value("injection_detected", false);
    ctx.injection_risk_score = j.value("injection_risk_score", 0.0f);
    
    if (j.contains("start_time")) {
        auto timestamp = j["start_time"].get<std::time_t>();
        ctx.start_time = std::chrono::system_clock::from_time_t(timestamp);
    }
    
    return ctx;
}

// ============================================================================
// IntegrationStatus
// ============================================================================

nlohmann::json IntegrationStatus::toJson() const {
    auto timestamp = std::chrono::system_clock::to_time_t(last_optimization);
    return {
        {"running", running},
        {"background_worker_active", background_worker_active},
        {"total_executions", total_executions},
        {"total_optimizations", total_optimizations},
        {"last_optimization", timestamp},
        {"active_prompts", active_prompts},
        {"executions_by_prompt", executions_by_prompt}
    };
}

// ============================================================================
// WorkerStatus
// ============================================================================

nlohmann::json WorkerStatus::toJson() const {
    auto last_timestamp = std::chrono::system_clock::to_time_t(last_run);
    auto next_timestamp = std::chrono::system_clock::to_time_t(next_scheduled_run);
    return {
        {"running", running},
        {"cycles_completed", cycles_completed},
        {"prompts_optimized", prompts_optimized},
        {"last_run", last_timestamp},
        {"next_scheduled_run", next_timestamp}
    };
}

// ============================================================================
// BackgroundOptimizationWorker
// ============================================================================

BackgroundOptimizationWorker::BackgroundOptimizationWorker(
    std::chrono::seconds interval,
    std::shared_ptr<PromptPerformanceTracker> tracker,
    std::shared_ptr<SelfImprovementOrchestrator> orchestrator,
    std::shared_ptr<PromptVersionControl> version_control
) : interval_(interval),
    tracker_(tracker),
    orchestrator_(orchestrator),
    version_control_(version_control) {
}

BackgroundOptimizationWorker::~BackgroundOptimizationWorker() {
    stop();
}

void BackgroundOptimizationWorker::start() {
    if (running_.load()) {
        return;  // Already running
    }
    
    stop_requested_.store(false);
    running_.store(true);
    
    worker_thread_ = std::make_unique<std::thread>(&BackgroundOptimizationWorker::workerLoop, this);
}

void BackgroundOptimizationWorker::stop() {
    if (!running_.load()) {
        return;  // Not running
    }
    
    stop_requested_.store(true);
    
    if (worker_thread_ && worker_thread_->joinable()) {
        worker_thread_->join();
    }
    
    running_.store(false);
}

bool BackgroundOptimizationWorker::isRunning() const {
    return running_.load();
}

WorkerStatus BackgroundOptimizationWorker::getStatus() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    WorkerStatus status;
    status.running = running_.load();
    status.cycles_completed = cycles_completed_;
    status.prompts_optimized = prompts_optimized_;
    status.last_run = last_run_;
    status.next_scheduled_run = last_run_ + interval_;
    
    return status;
}

void BackgroundOptimizationWorker::runOptimizationCycle() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    last_run_ = std::chrono::system_clock::now();
    
    // Run auto-optimization
    auto results = orchestrator_->runAutoOptimization();
    
    cycles_completed_++;
    prompts_optimized_ += results.size();
    
    // Log results
    for (const auto& result : results) {
        if (result.status == prompt_engineering::OptimizationStatus::COMPLETED) {
            // Version control automatically handles commits in orchestrator
        }
    }
}

void BackgroundOptimizationWorker::workerLoop() {
    while (!stop_requested_.load()) {
        // Run optimization cycle
        runOptimizationCycle();
        
        // Sleep for interval (with periodic wake-ups to check stop flag)
        auto sleep_end = std::chrono::steady_clock::now() + interval_;
        while (!stop_requested_.load() && std::chrono::steady_clock::now() < sleep_end) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
}

// ============================================================================
// PromptEngineeringIntegration
// ============================================================================

PromptEngineeringIntegration::PromptEngineeringIntegration(
    const IntegrationConfig& config,
    std::shared_ptr<PromptManager> manager,
    std::shared_ptr<PromptOptimizer> optimizer,
    std::shared_ptr<PromptPerformanceTracker> tracker,
    std::shared_ptr<SelfImprovementOrchestrator> orchestrator,
    std::shared_ptr<FeedbackCollector> feedback_collector,
    std::shared_ptr<PromptVersionControl> version_control,
    std::shared_ptr<PromptInjectionDetector> injection_detector
) : config_(config),
    manager_(manager),
    optimizer_(optimizer),
    tracker_(tracker),
    orchestrator_(orchestrator),
    feedback_collector_(feedback_collector),
    version_control_(version_control),
    injection_detector_(injection_detector) {
}

PromptEngineeringIntegration::~PromptEngineeringIntegration() {
    stop();
}

void PromptEngineeringIntegration::start() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (running_) {
        return;  // Already running
    }
    
    running_ = true;
    
    // Start background worker if enabled
    if (config_.background_worker_enabled) {
        startBackgroundOptimization();
    }
}

void PromptEngineeringIntegration::stop() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!running_) {
        return;  // Not running
    }
    
    // Stop background worker
    stopBackgroundOptimization();
    
    running_ = false;
}

IntegrationStatus PromptEngineeringIntegration::getStatus() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    IntegrationStatus status;
    status.running = running_;
    status.background_worker_active = background_worker_ && background_worker_->isRunning();
    status.total_executions = total_executions_;
    status.total_optimizations = total_optimizations_;
    status.last_optimization = last_optimization_;
    status.active_prompts = executions_by_prompt_.size();
    status.executions_by_prompt = executions_by_prompt_;
    
    return status;
}

ExecutionContext PromptEngineeringIntegration::beforeExecution(
    const std::string& prompt_id,
    const nlohmann::json& context
) {
    ExecutionContext ctx;
    ctx.execution_id = generateExecutionId();
    ctx.prompt_id = prompt_id;
    ctx.context = context;
    ctx.start_time = std::chrono::system_clock::now();
    
    // Get the prompt template
    auto template_opt = manager_->getTemplate(prompt_id);
    if (!template_opt.has_value()) {
        // Prompt doesn't exist, return empty context
        return ctx;
    }
    
    ctx.original_prompt = template_opt.value().content;
    
    // Enhance prompt with context
    ctx.enhanced_prompt = enhancePrompt(prompt_id, context);
    
    // Detect prompt injection in the prompt text
    if (config_.enable_injection_detection && injection_detector_) {
        auto detection = injection_detector_->detect(ctx.enhanced_prompt);
        if (detection.is_injection) {
            ctx.injection_detected = true;
            ctx.injection_risk_score = detection.risk_score;
            // Sanitize the prompt so downstream processing receives cleaned text
            ctx.enhanced_prompt = detection.sanitized_text;
            if (config_.enable_feedback_collection && feedback_collector_) {
                feedback_collector_->recordFeedback(
                    ctx.prompt_id,
                    ctx.original_prompt,
                    "",
                    FeedbackType::SECURITY_ISSUE,
                    "Prompt injection attempt detected (risk_score=" +
                        std::to_string(detection.risk_score) + ")",
                    detection.risk_score
                );
            }
        }
    }
    
    // Get current version
    if (config_.enable_auto_versioning) {
        auto latest = version_control_->getLatest(prompt_id);
        if (latest.has_value()) {
            ctx.version_id = latest->version_id;
        }
    }
    
    // Auto-version: commit pre-execution snapshot
    if (config_.enable_auto_versioning && !ctx.enhanced_prompt.empty()) {
        version_control_->commit(
            prompt_id,
            ctx.enhanced_prompt,
            "Pre-execution snapshot",
            "integration_system"
        );
    }
    
    // Track active execution
    {
        std::lock_guard<std::mutex> lock(mutex_);
        active_executions_[ctx.execution_id] = ctx;
    }
    
    return ctx;
}

void PromptEngineeringIntegration::afterExecution(
    const ExecutionContext& ctx,
    const std::string& response,
    bool success,
    double latency_ms,
    double user_feedback
) {
    // Remove from active executions
    {
        std::lock_guard<std::mutex> lock(mutex_);
        active_executions_.erase(ctx.execution_id);
        total_executions_++;
        executions_by_prompt_[ctx.prompt_id]++;
    }
    
    // Record performance metrics
    if (config_.enable_performance_tracking) {
        tracker_->recordExecution(ctx.prompt_id, success, latency_ms, user_feedback);
    }
    
    // Record feedback if execution failed
    if (config_.enable_feedback_collection && !success) {
        FeedbackType feedback_type = FeedbackType::PARSE_ERROR;
        std::string feedback_text = "Execution failed";
        
        // Try to determine failure reason
        if (response.empty()) {
            feedback_type = FeedbackType::TIMEOUT;
            feedback_text = "Empty response - possible timeout";
        }
        
        feedback_collector_->recordFeedback(
            ctx.prompt_id,
            ctx.enhanced_prompt,
            response,
            feedback_type,
            feedback_text,
            0.8  // High severity
        );
    }
    
    // Check if optimization is needed
    if (config_.enable_auto_optimization) {
        checkAndTriggerOptimization(ctx.prompt_id);
    }
    
    // Detect indirect injection in the model response
    if (config_.enable_injection_detection && injection_detector_) {
        auto detection = injection_detector_->detectInResponse(response);
        if (detection.is_injection) {
            if (config_.enable_feedback_collection && feedback_collector_) {
                feedback_collector_->recordFeedback(
                    ctx.prompt_id,
                    ctx.enhanced_prompt,
                    response,
                    FeedbackType::SECURITY_ISSUE,
                    "Indirect prompt injection detected in model response (risk_score=" +
                        std::to_string(detection.risk_score) + ")",
                    detection.risk_score
                );
            }
        }
    }

    // Snapshot reflection-tuning state under lock to prevent data races with
    // updateConfig() / setReflectionTuner() / setMetrics() on other threads.
    IntegrationConfig                        snap_config;
    std::shared_ptr<ReflectionTuner>         snap_tuner;
    std::shared_ptr<PromptEngineeringMetrics> snap_metrics;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        snap_config  = config_;
        snap_tuner   = reflection_tuner_;
        snap_metrics = metrics_;
    }

    // Optional reflection tuning pass — refine the response if a tuner is attached
    // and the execution succeeded (no point refining failed/empty responses).
    if (snap_config.enable_reflection_tuning && snap_tuner && success && !response.empty()) {
        // Apply reflection_max_iterations from IntegrationConfig to the tuner so
        // that callers who set config_.reflection_max_iterations actually see the
        // change take effect on the next tune() call.  We intentionally operate on
        // the snapshotted tuner pointer: if setReflectionTuner() is called concurrently,
        // the replacement takes effect on the *next* invocation, not mid-flight.
        auto tuner_cfg = snap_tuner->getConfig();
        tuner_cfg.max_iterations = snap_config.reflection_max_iterations;
        snap_tuner->setConfig(tuner_cfg);

        if (snap_metrics) {
            snap_metrics->recordReflectionCycleStart(ctx.prompt_id);
        }

        auto reflection_result = snap_tuner->tune(ctx.enhanced_prompt, response);

        if (snap_metrics) {
            const bool improved = reflection_result.quality_improvement > 0.0;
            snap_metrics->recordReflectionCycleComplete(
                ctx.prompt_id,
                reflection_result.total_iterations,
                improved);
            snap_metrics->recordReflectionQualityDelta(
                ctx.prompt_id,
                reflection_result.quality_improvement);
            if (reflection_result.halted_by_hallucination_guard) {
                snap_metrics->recordReflectionGuardFired(ctx.prompt_id);
            }
        }

        // Store reflection result as positive feedback when quality improved.
        if (snap_config.enable_feedback_collection && feedback_collector_ &&
            reflection_result.quality_improvement > 0.0) {
            feedback_collector_->recordFeedback(
                ctx.prompt_id,
                ctx.enhanced_prompt,
                reflection_result.final_response,
                FeedbackType::USER_POSITIVE,
                "Reflection tuning improved quality by " +
                    std::to_string(static_cast<int>(
                        reflection_result.quality_improvement * 100)) + "%",
                static_cast<float>(reflection_result.quality_improvement));
        }
    }
}

void PromptEngineeringIntegration::startBackgroundOptimization() {
    if (!background_worker_) {
        background_worker_ = std::make_unique<BackgroundOptimizationWorker>(
            config_.background_worker_interval,
            tracker_,
            orchestrator_,
            version_control_
        );
    }
    
    background_worker_->start();
}

void PromptEngineeringIntegration::stopBackgroundOptimization() {
    if (background_worker_) {
        background_worker_->stop();
    }
}

WorkerStatus PromptEngineeringIntegration::getBackgroundWorkerStatus() const {
    if (background_worker_) {
        return background_worker_->getStatus();
    }
    return WorkerStatus{};
}

nlohmann::json PromptEngineeringIntegration::getStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    nlohmann::json stats;
    stats["total_executions"] = total_executions_;
    stats["total_optimizations"] = total_optimizations_;
    stats["active_prompts"] = executions_by_prompt_.size();
    stats["executions_by_prompt"] = executions_by_prompt_;
    stats["running"] = running_;
    
    if (background_worker_) {
        stats["background_worker"] = background_worker_->getStatus().toJson();
    }
    
    return stats;
}

IntegrationConfig PromptEngineeringIntegration::getConfig() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return config_;
}

void PromptEngineeringIntegration::updateConfig(const IntegrationConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_ = config;
    
    // Restart background worker if interval changed
    if (background_worker_ && background_worker_->isRunning()) {
        background_worker_->stop();
        background_worker_ = std::make_unique<BackgroundOptimizationWorker>(
            config_.background_worker_interval,
            tracker_,
            orchestrator_,
            version_control_
        );
        background_worker_->start();
    }
}

void PromptEngineeringIntegration::setReflectionTuner(
    std::shared_ptr<ReflectionTuner> tuner) {
    std::lock_guard<std::mutex> lock(mutex_);
    reflection_tuner_ = std::move(tuner);
}

void PromptEngineeringIntegration::setMetrics(
    std::shared_ptr<PromptEngineeringMetrics> metrics) {
    std::lock_guard<std::mutex> lock(mutex_);
    metrics_ = std::move(metrics);
}

void PromptEngineeringIntegration::checkAndTriggerOptimization(const std::string& prompt_id) {
    // Check if optimization is needed
    auto metrics = tracker_->getMetrics(prompt_id);
    
    if (!metrics.has_value() || metrics->total_executions < config_.min_executions_before_optimization) {
        return;  // Not enough data yet
    }
    
    if (metrics->success_rate < config_.min_success_rate_for_optimization) {
        // Trigger optimization
        if (orchestrator_->shouldOptimize(prompt_id)) {
            // Create empty test cases for now
            std::vector<prompt_engineering::TestCase> empty_cases;
            auto result = orchestrator_->optimizePrompt(prompt_id, empty_cases);
            
            if (result.status == prompt_engineering::OptimizationStatus::COMPLETED) {
                std::lock_guard<std::mutex> lock(mutex_);
                total_optimizations_++;
                last_optimization_ = std::chrono::system_clock::now();
                
                // Auto-commit optimized version
                if (config_.auto_commit_on_optimization) {
                    version_control_->commit(
                        prompt_id,
                        result.optimized_version,
                        "Auto-optimization: " + std::to_string(result.improvement * 100) + "% improvement",
                        "integration_system"
                    );
                }
            }
        }
    }
}

std::string PromptEngineeringIntegration::enhancePrompt(
    const std::string& prompt_id,
    const nlohmann::json& context
) {
    // Convert JSON context to string map for context injection
    std::unordered_map<std::string, std::string> ctx_map = {};

    for (auto it = context.begin(); it != context.end(); ++it) {
        if (it.value().is_string()) {
            ctx_map[it.key()] = it.value().get<std::string>();
        } else if (!it.value().is_null()) {
            ctx_map[it.key()] = it.value().dump();
        }
    }

    // Get latest version if available
    if (config_.enable_auto_versioning) {
        auto latest = version_control_->getLatest(prompt_id);
        if (latest.has_value()) {
            auto tmpl = manager_->getTemplate(prompt_id);
            if (tmpl.has_value()) {
                return PromptManager::buildMultiModalPrompt(*tmpl, ctx_map);
            }
        }
    }

    // Fall back to template
    auto tmpl = manager_->getTemplate(prompt_id);
    if (tmpl.has_value()) {
        return PromptManager::buildMultiModalPrompt(*tmpl, ctx_map);
    }
    return "";
}

std::string PromptEngineeringIntegration::generateExecutionId() {
    // Generate UUID-like execution ID
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    
    const char* hex_chars = "0123456789abcdef";
    
    std::stringstream ss;
    for (int i = 0; i < 32; i++) {
        if (i == 8 || i == 12 || i == 16 || i == 20) {
            ss << '-';
        }
        ss << hex_chars[dis(gen)];
    }
    
    return ss.str();
}

} // namespace prompt_engineering
} // namespace themis

