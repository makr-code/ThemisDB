/**
 * @file continuous_learning_orchestrator.cpp
 * @brief Implementation of continuous learning orchestrator
 */

#include "rag/continuous_learning_orchestrator.h"

#include <algorithm>
#include <atomic>
#include <mutex>
#include <numeric>
#include <thread>

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
    // Stub implementation - would analyze prompt performance and generate variations
    // For now, just log that optimization was considered

    std::lock_guard<std::mutex> lock(impl_->mutex);

    // Check if we have enough data
    if (impl_->interactions.size() < impl_->config.min_feedback_samples) {
        return;
    }
    
    // TODO: Full implementation needed
    // 1. Analyze which prompts have low success rates
    // 2. Generate variations using LLM
    // 3. Test variations on historical failed queries
    // 4. Deploy best variation via A/B test
}

void ContinuousLearningOrchestrator::runRetrievalOptimization() {
    // Stub implementation - would use Bayesian optimization

    std::lock_guard<std::mutex> lock(impl_->mutex);

    if (impl_->interactions.size() < impl_->config.min_feedback_samples) {
        return;
    }
    
    // TODO: Full implementation needed
    // 1. Create a BayesianOptimizer for retrieval parameters
    // 2. Sample historical queries
    // 3. Test different parameter combinations
    // 4. Deploy best parameters via A/B test
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
    // Stub implementation - would persist to RocksDB or SQLite
    // For now, metrics are kept in memory only
}

void ContinuousLearningOrchestrator::loadMetrics() {
    // Stub implementation - would load from persistence
}

void ContinuousLearningOrchestrator::saveModelCheckpoint(const std::string &model_id) {
    // Stub implementation - would save model state
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
