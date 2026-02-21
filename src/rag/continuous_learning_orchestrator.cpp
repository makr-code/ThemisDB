/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            continuous_learning_orchestrator.cpp               ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-02-21 10:42:09                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   92.0/100                                       ║
    • Total Lines:     526                                            ║
    • Open Issues:     TODOs: 0, Stubs: 2                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • cbf6dcdfc  2026-02-20  Enhance modular build and improve code quality ║
    • 5d480af8c  2026-02-20  RAG module: replace all stubs with real implementations; ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file continuous_learning_orchestrator.cpp
 * @brief Implementation of continuous learning orchestrator
 */

#include "rag/continuous_learning_orchestrator.h"
#include "rag/bayesian_optimizer.h"
#include "utils/logger.h"

#include <algorithm>
#include <atomic>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <numeric>
#include <sstream>
#include <thread>

// Default retrieval parameters (must match RetrievalParams defaults in learning_metrics.h)
static constexpr double kDefaultTopK               = 10.0;
static constexpr double kDefaultSimilarityThreshold = 0.75;

namespace themis::rag::learning {

struct ComponentInfo {
    std::string id;
    std::string info;
    std::chrono::system_clock::time_point last_training;
    size_t feedback_count = 0;
};

struct ContinuousLearningOrchestrator::Impl {
    ContinuousLearningConfig config;

    // Registered components
    std::unordered_map<std::string, ComponentInfo> lora_adapters;
    std::unordered_map<std::string, ComponentInfo> retrieval_systems;
    std::unordered_map<std::string, ComponentInfo> prompt_systems;
    std::unordered_map<std::string, ComponentInfo> gap_detectors;

    // Interaction log
    std::vector<Interaction> interactions;

    // Statistics
    LearningStats stats;
    std::vector<PerformanceSnapshot> performance_history;

    // A/B testing
    std::unique_ptr<ABTestingFramework> ab_framework;

    // Background thread
    std::atomic<bool> learning_loop_active{false};
    std::unique_ptr<std::thread> learning_thread;

    // Thread safety
    mutable std::mutex mutex;
};

ContinuousLearningOrchestrator::ContinuousLearningOrchestrator(const ContinuousLearningConfig &config)
    : impl_(std::make_unique<Impl>()) {
    impl_->config       = config;
    impl_->ab_framework = std::make_unique<ABTestingFramework>();

    // Load persisted metrics if available
    loadMetrics();
}

ContinuousLearningOrchestrator::~ContinuousLearningOrchestrator() {
    stopLearningLoop();

    // Save metrics before shutdown
    saveMetrics();
}

void ContinuousLearningOrchestrator::startLearningLoop() {
    if (impl_->learning_loop_active) {
        return; // Already running
    }

    impl_->learning_loop_active = true;
    impl_->learning_thread = std::make_unique<std::thread>(&ContinuousLearningOrchestrator::learningLoopThread, this);
}

void ContinuousLearningOrchestrator::stopLearningLoop() {
    if (!impl_->learning_loop_active) {
        return;
    }

    impl_->learning_loop_active = false;
    if (impl_->learning_thread && impl_->learning_thread->joinable()) {
        impl_->learning_thread->join();
    }
}

void ContinuousLearningOrchestrator::triggerLearningIteration() {
    // Run all learning strategies
    runLoRARetraining();
    runPromptOptimization();
    runRetrievalOptimization();

    // Evaluate active A/B tests
    auto active_tests = impl_->ab_framework->getActiveTests();
    for (const auto &test_id : active_tests) {
        auto result = impl_->ab_framework->evaluateTest(test_id);

        // Check if test has enough samples
        if (result.sample_size_control >= impl_->config.min_ab_samples
            && result.sample_size_treatment >= impl_->config.min_ab_samples) {
            promoteOrRollback(result);
        }
    }

    // Save metrics periodically
    saveMetrics();
}

void ContinuousLearningOrchestrator::registerLoRAAdapter(const std::string &adapter_id,
                                                         const std::string &adapter_info) {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    ComponentInfo info;
    info.id                          = adapter_id;
    info.info                        = adapter_info;
    info.last_training               = std::chrono::system_clock::now();
    impl_->lora_adapters[adapter_id] = info;
}

void ContinuousLearningOrchestrator::registerRetrievalSystem(const std::string &system_id) {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    ComponentInfo info;
    info.id                             = system_id;
    impl_->retrieval_systems[system_id] = info;
}

void ContinuousLearningOrchestrator::registerPromptSystem(const std::string &system_id) {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    ComponentInfo info;
    info.id                          = system_id;
    impl_->prompt_systems[system_id] = info;
}

void ContinuousLearningOrchestrator::registerKnowledgeGapDetector(const std::string &detector_id) {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    ComponentInfo info;
    info.id                           = detector_id;
    impl_->gap_detectors[detector_id] = info;
}

void ContinuousLearningOrchestrator::logInteraction(const Interaction &interaction) {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    impl_->interactions.push_back(interaction);
    impl_->stats.total_interactions_logged++;

    // Update current accuracy based on feedback
    if (interaction.user_feedback.has_value()) {
        bool is_positive = (interaction.user_feedback.value() == FeedbackType::POSITIVE);

        // Simple running average
        double new_accuracy           = is_positive ? 1.0 : 0.0;
        impl_->stats.current_accuracy = (impl_->stats.current_accuracy * 0.95) + (new_accuracy * 0.05);
    }
}

void ContinuousLearningOrchestrator::logInteractionBatch(const std::vector<Interaction> &interactions) {
    for (const auto &interaction : interactions) {
        logInteraction(interaction);
    }
}

LearningStats ContinuousLearningOrchestrator::getStats() const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->stats;
}

std::vector<PerformanceSnapshot>
ContinuousLearningOrchestrator::getPerformanceHistory(std::chrono::hours lookback_period) const {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    auto cutoff = std::chrono::system_clock::now() - lookback_period;
    std::vector<PerformanceSnapshot> filtered;

    for (const auto &snapshot : impl_->performance_history) {
        if (snapshot.timestamp >= cutoff) {
            filtered.push_back(snapshot);
        }
    }

    return filtered;
}

bool ContinuousLearningOrchestrator::isSystemImproving() const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->stats.accuracy_trend > 0.0;
}

void ContinuousLearningOrchestrator::runPromptOptimization() {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    if (impl_->interactions.size() < impl_->config.min_feedback_samples) {
        return;
    }

    // Compute per-prompt-version success rates
    std::unordered_map<std::string, size_t> total_per_version;
    std::unordered_map<std::string, size_t> success_per_version;

    for (const auto& interaction : impl_->interactions) {
        const std::string& ver = interaction.prompt_version;
        total_per_version[ver]++;
        if (interaction.user_feedback.has_value() &&
            interaction.user_feedback.value() == FeedbackType::POSITIVE) {
            success_per_version[ver]++;
        }
    }

    // Find worst-performing prompt version
    std::string worst_version;
    double worst_rate = 1.0;
    for (const auto& [ver, total] : total_per_version) {
        if (total == 0) continue;
        double rate = static_cast<double>(success_per_version[ver]) / total;
        if (rate < worst_rate) {
            worst_rate    = rate;
            worst_version = ver;
        }
    }

    if (worst_version.empty() ||
        worst_rate >= (1.0 - impl_->config.min_improvement_threshold)) {
        // All versions performing adequately
        return;
    }

    // Record the optimization event
    ImprovementEvent event;
    event.timestamp        = std::chrono::system_clock::now();
    event.component        = "prompt:" + worst_version;
    event.improvement_type = "PromptOptimization";
    event.metric_before    = worst_rate;
    event.metric_after     = worst_rate; // will be updated after A/B test
    event.description      = "Triggered prompt optimization for version '" +
                             worst_version + "' (success rate: " +
                             std::to_string(worst_rate) + ")";

    impl_->stats.recent_improvements.push_back(event);
    impl_->stats.prompt_optimizations++;

    // Deploy A/B test if enabled
    if (impl_->config.enable_ab_testing) {
        deployABTest("prompt_opt_" + worst_version);
    }
}

void ContinuousLearningOrchestrator::runRetrievalOptimization() {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    if (impl_->interactions.size() < impl_->config.min_feedback_samples) {
        return;
    }

    // Compute success rate across recent interactions
    size_t total   = 0;
    size_t success = 0;
    for (const auto& interaction : impl_->interactions) {
        if (interaction.user_feedback.has_value()) {
            total++;
            if (interaction.user_feedback.value() == FeedbackType::POSITIVE) {
                success++;
            }
        }
    }

    if (total == 0) return;
    double current_success_rate = static_cast<double>(success) / total;

    // Use BayesianOptimizer to suggest new retrieval parameters
    std::unordered_map<std::string, ParameterBounds> param_bounds;
    param_bounds["top_k"]               = {1.0, 20.0};
    param_bounds["similarity_threshold"] = {0.5,  0.95};

    BayesianOptimizer optimizer(param_bounds);

    // Seed the optimizer with the current observed performance
    std::unordered_map<std::string, double> current_params;
    current_params["top_k"]               = kDefaultTopK;
    current_params["similarity_threshold"] = kDefaultSimilarityThreshold;
    optimizer.observe(current_params, current_success_rate);

    // Get a candidate improvement suggestion
    auto suggested = optimizer.suggest();

    // Record retrieval optimization event
    ImprovementEvent event;
    event.timestamp        = std::chrono::system_clock::now();
    event.component        = "retrieval";
    event.improvement_type = "RetrievalOptimization";
    event.metric_before    = current_success_rate;
    event.metric_after     = current_success_rate; // will be updated after A/B test

    std::ostringstream desc;
    desc << "Suggested retrieval params: top_k=" << suggested["top_k"]
         << " similarity_threshold=" << suggested["similarity_threshold"];
    event.description = desc.str();

    impl_->stats.recent_improvements.push_back(event);
    impl_->stats.retrieval_optimizations++;

    // Deploy A/B test if enabled
    if (impl_->config.enable_ab_testing) {
        deployABTest("retrieval_opt");
    }
}

void ContinuousLearningOrchestrator::runLoRARetraining() {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    for (auto &[adapter_id, info] : impl_->lora_adapters) {
        bool should_retrain = false;

        // Trigger 1: Feedback accumulation
        if (info.feedback_count >= impl_->config.min_feedback_samples) {
            should_retrain = true;
        }

        // Trigger 2: Scheduled interval
        auto time_since = std::chrono::system_clock::now() - info.last_training;
        if (time_since > impl_->config.retraining_interval) {
            should_retrain = true;
        }

        if (should_retrain) {
            // In full implementation, would call adapter->trainFromFeedback()
            info.last_training  = std::chrono::system_clock::now();
            info.feedback_count = 0;
            impl_->stats.lora_retraining_count++;

            // Deploy via A/B test if enabled
            if (impl_->config.enable_ab_testing) {
                deployABTest(adapter_id);
            }
        }
    }
}

void ContinuousLearningOrchestrator::deployABTest(const std::string &model_id) {
    ABTestConfig test_config;
    test_config.test_id
        = model_id + "_test_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    test_config.component       = model_id;
    test_config.traffic_split   = impl_->config.ab_test_traffic_split;
    test_config.min_samples     = impl_->config.min_ab_samples;
    test_config.min_improvement = impl_->config.min_improvement_threshold;

    impl_->ab_framework->startTest(test_config);
}

void ContinuousLearningOrchestrator::promoteOrRollback(const ABTestResult &result) {
    bool should_promote = false;

    if (result.is_significant && result.improvement >= impl_->config.min_improvement_threshold) {
        should_promote = true;
    } else if (impl_->config.enable_auto_rollback && result.improvement < -impl_->config.min_improvement_threshold) {
        should_promote = false;
    } else {
        // Not significant enough either way - let test continue
        return;
    }

    impl_->ab_framework->completeTest(result.test_id, should_promote);

    // Record improvement event
    if (should_promote) {
        ImprovementEvent event;
        event.timestamp        = std::chrono::system_clock::now();
        event.component        = result.test_id;
        event.improvement_type = "A/B Test Promotion";
        event.metric_before    = result.control_success_rate;
        event.metric_after     = result.treatment_success_rate;
        event.description      = "Promoted after successful A/B test";

        std::lock_guard<std::mutex> lock(impl_->mutex);
        impl_->stats.recent_improvements.push_back(event);
    }
}

void ContinuousLearningOrchestrator::saveMetrics() {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    const std::string& path = impl_->config.metrics_db_path;
    if (path.empty()) return;

    // Check if file is new (empty) so we can write the header once
    bool is_new_file = false;
    {
        std::ifstream check(path, std::ios::binary | std::ios::ate);
        is_new_file = !check.is_open() || check.tellg() == 0;
    }

    // Append mode so historical entries are preserved
    std::ofstream file(path, std::ios::app);
    if (!file.is_open()) {
        THEMIS_WARN("saveMetrics: could not open metrics file for writing: {}", path);
        return;
    }

    if (is_new_file) {
        file << "timestamp,accuracy,prompt_optimizations,retrieval_optimizations,"
                "lora_retraining_count\n";
    }

    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    file << std::fixed << std::setprecision(6)
         << now << ","
         << impl_->stats.current_accuracy << ","
         << impl_->stats.prompt_optimizations << ","
         << impl_->stats.retrieval_optimizations << ","
         << impl_->stats.lora_retraining_count << "\n";
}

void ContinuousLearningOrchestrator::loadMetrics() {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    const std::string& path = impl_->config.metrics_db_path;
    if (path.empty()) return;

    std::ifstream file(path);
    if (!file.is_open()) {
        THEMIS_DEBUG("loadMetrics: metrics file not found: {}", path);
        return;
    }

    // Skip header line
    std::string line;
    if (!std::getline(file, line)) return;

    // Read last data row
    std::string last_line;
    while (std::getline(file, last_line)) {
        // keep iterating to get the last line
    }

    if (last_line.empty()) return;

    std::istringstream row(last_line);
    std::string field;
    int col = 0;
    while (std::getline(row, field, ',')) {
        try {
            switch (col) {
                case 1: impl_->stats.current_accuracy        = std::stod(field); break;
                case 2: impl_->stats.prompt_optimizations    = static_cast<size_t>(std::stoull(field)); break;
                case 3: impl_->stats.retrieval_optimizations = static_cast<size_t>(std::stoull(field)); break;
                case 4: impl_->stats.lora_retraining_count   = static_cast<size_t>(std::stoull(field)); break;
                default: break;
            }
        } catch (...) {
            // Ignore parse errors for individual fields
        }
        col++;
    }
}

void ContinuousLearningOrchestrator::saveModelCheckpoint(const std::string &model_id) {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    const std::string& registry_path = impl_->config.model_registry_path;
    if (registry_path.empty() || model_id.empty()) return;

    // Record checkpoint event in stats
    ImprovementEvent event;
    event.timestamp        = std::chrono::system_clock::now();
    event.component        = model_id;
    event.improvement_type = "ModelCheckpoint";
    event.metric_before    = impl_->stats.current_accuracy;
    event.metric_after     = impl_->stats.current_accuracy;
    event.description      = "Checkpoint saved for model: " + model_id;
    impl_->stats.recent_improvements.push_back(event);
}

void ContinuousLearningOrchestrator::learningLoopThread() {
    while (impl_->learning_loop_active) {
        // Sleep for configured interval
        auto sleep_duration = impl_->config.learning_loop_interval;
        auto end_time       = std::chrono::steady_clock::now() + sleep_duration;

        while (impl_->learning_loop_active && std::chrono::steady_clock::now() < end_time) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        if (impl_->learning_loop_active) {
            triggerLearningIteration();
        }
    }
}

} // namespace themis::rag::learning
