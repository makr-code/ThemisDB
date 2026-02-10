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
        {"start_time", timestamp}
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
    std::shared_ptr<PromptVersionControl> version_control
) : config_(config),
    manager_(manager),
    optimizer_(optimizer),
    tracker_(tracker),
    orchestrator_(orchestrator),
    feedback_collector_(feedback_collector),
    version_control_(version_control) {
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
    // Get latest version if available
    if (config_.enable_auto_versioning) {
        auto latest = version_control_->getLatest(prompt_id);
        if (latest.has_value()) {
            // Use versioned content
            auto tmpl = manager_->getTemplate(prompt_id);
            if (tmpl.has_value()) {
                return tmpl->content;
            }
        }
    }
    
    // Fall back to template
    auto tmpl = manager_->getTemplate(prompt_id);
    if (tmpl.has_value()) {
        return tmpl->content;
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
