/**
 * @file adaptive_retraining_controller.h
 * @brief Orchestrates automatic retraining triggers and rollback logic
 * 
 * This component implements the core logic for:
 * - Feedback threshold monitoring
 * - Time-interval based retraining
 * - Quality degradation detection
 * - Automatic rollback on failures
 * - Audit logging of train/deploy cycles
 */

#pragma once

#include "lora_feedback.h"
#include "telemetry_feedback_adapter.h"
#include "lora_audit_logger.h"
#include "lora_training_service.h"
#include <memory>
#include <functional>
#include <vector>
#include <chrono>
#include <atomic>
#include <nlohmann/json.hpp>

namespace themis {
namespace llm {
namespace lora {

using json = nlohmann::json;

/**
 * @brief Retraining decision rationale
 */
struct RetrainingDecision {
    virtual ~RetrainingDecision() = default;
    
    enum class Reason {
        FEEDBACK_THRESHOLD_MET,     // Enough feedback accumulated
        TIME_INTERVAL_ELAPSED,      // Scheduled retraining time
        QUALITY_DEGRADATION,        // Accuracy dropped below threshold
        MANUAL_TRIGGER,             // Manually triggered
        ERROR_RECOVERY              // Recovery from failures
    };
    
    bool should_retrain = false;
    Reason reason;
    std::string reason_description;
    float confidence = 0.0f;  // Confidence in this decision (0-1)
    json decision_metrics;
};

/**
 * @brief Result of a retraining attempt
 */
struct RetrainingResult {
    virtual ~RetrainingResult() = default;
    
    bool success = false;
    std::string old_version;
    std::string new_version;
    
    // Training metrics
    float training_time_seconds = 0.0f;
    float final_loss = 0.0f;
    float validation_accuracy = 0.0f;
    
    // Comparison with previous version
    float accuracy_improvement = 0.0f;   // delta from previous version
    float latency_improvement_ms = 0.0f; // delta from previous version
    
    // Deployment status
    bool is_deployed = false;
    bool requires_rollback = false;
    std::string error_message;
    
    json toJSON() const;
};

/**
 * @brief Retraining trigger configuration
 */
struct RetrainingTriggerConfig {
    virtual ~RetrainingTriggerConfig() = default;
    
    // Feedback threshold trigger
    bool feedback_trigger_enabled = true;
    size_t feedback_threshold = 50;         // Retrain after N feedback entries
    
    // Time interval trigger
    bool interval_trigger_enabled = true;
    std::chrono::hours retraining_interval{24};  // Retrain every 24 hours
    
    // Quality degradation trigger
    bool quality_trigger_enabled = true;
    float accuracy_drop_threshold = 0.05f;  // 5% accuracy drop
    float latency_increase_threshold = 1.2f; // 20% latency increase
    
    // Rollback configuration
    bool auto_rollback_enabled = true;
    float rollback_accuracy_threshold = 0.80f;
    
    // Rate limiting
    std::chrono::seconds min_time_between_retrains{3600};  // 1 hour
    
    json toJSON() const;
    static RetrainingTriggerConfig fromJSON(const json& j);
};

/**
 * @brief Orchestrates automatic LoRA/LLM retraining with feedback integration
 * 
 * Key responsibilities:
 * 1. Monitor feedback accumulation and trigger retraining
 * 2. Track adapter versions and enable rollback
 * 3. Integrate telemetry for quality degradation detection
 * 4. Maintain comprehensive audit logs
 * 5. Enforce safety guardrails (rollback on quality drops)
 * 
 * Thread-safe for concurrent operations.
 */
class AdaptiveRetrainingController {
public:
    struct Dependencies {
        std::shared_ptr<TelemetryFeedbackAdapter> telemetry_adapter;
        std::shared_ptr<LoRAAuditLogger> audit_logger;
        std::shared_ptr<LoRATrainingService> training_service;
    };
    
    /**
     * @brief Constructor
     * 
     * @param adapter_id The LoRA adapter to manage
     * @param deps External dependencies
     * @param config Retraining configuration
     */
    AdaptiveRetrainingController(
        const std::string& adapter_id,
        const Dependencies& deps,
        const RetrainingTriggerConfig& config = RetrainingTriggerConfig{}
    );
    
    ~AdaptiveRetrainingController() = default;
    
    /**
     * @brief Add feedback entry for evaluation
     * 
     * @param feedback The feedback to process
     */
    void addFeedback(const Feedback& feedback);
    
    /**
     * @brief Add telemetry metric
     * 
     * @param metric The telemetry metric
     */
    void addMetric(const TelemetryMetrics& metric);
    
    /**
     * @brief Evaluate if retraining should be triggered
     * 
     * @return Decision with reason and confidence
     */
    RetrainingDecision evaluateRetrainingNeed();
    
    /**
     * @brief Execute retraining if decision indicates it
     * 
     * @param decision The retraining decision
     * @param training_data Optional training data (if not provided, uses accumulated feedback)
     * @return Result of retraining attempt
     */
    RetrainingResult executeRetraining(
        const RetrainingDecision& decision,
        const std::optional<TrainingData>& training_data = std::nullopt
    );
    
    /**
     * @brief Check if quality has degraded and rollback if necessary
     * 
     * @return true if rollback was performed
     */
    bool checkAndRollbackIfNeeded();
    
    /**
     * @brief Manually trigger retraining
     * 
     * @param training_data Optional training data
     * @return Result of retraining attempt
     */
    RetrainingResult triggerRetrainingNow(
        const std::optional<TrainingData>& training_data = std::nullopt
    );
    
    /**
     * @brief Get version history for the adapter
     * 
     * @return List of version strings in chronological order
     */
    std::vector<std::string> getVersionHistory() const;
    
    /**
     * @brief Get current active version
     */
    std::string getCurrentVersion() const;
    
    /**
     * @brief Get metrics for a specific version
     */
    std::optional<AdapterVersionMetrics> getVersionMetrics(const std::string& version) const;
    
    /**
     * @brief Get accumulated feedback count
     */
    size_t getFeedbackCount() const;
    
    /**
     * @brief Get retraining history
     * 
     * @return Vector of retraining results
     */
    std::vector<RetrainingResult> getRetrainingHistory() const;
    
    /**
     * @brief Update configuration at runtime
     */
    void setConfig(const RetrainingTriggerConfig& config);
    
    /**
     * @brief Get current configuration
     */
    const RetrainingTriggerConfig& getConfig() const { return config_; }
    
    /**
     * @brief Clear feedback buffer (for testing)
     */
    void clearFeedbackBuffer();

private:
    std::string adapter_id_;
    Dependencies deps_;
    RetrainingTriggerConfig config_;
    
    mutable std::mutex mutex_;
    
    // State tracking
    std::vector<Feedback> feedback_buffer_;
    std::vector<RetrainingResult> retraining_history_;
    std::vector<std::string> version_history_;
    std::string current_version_;
    std::chrono::system_clock::time_point last_retraining_time_;
    
    // Metrics tracking
    std::unordered_map<std::string, AdapterVersionMetrics> version_metrics_;
    
    /**
     * @brief Check if enough time has passed since last retraining
     */
    bool isRateLimitExceeded() const;
    
    /**
     * @brief Convert feedback to training data
     */
    TrainingData feedbackToTrainingData(const std::vector<Feedback>& feedback) const;
    
    /**
     * @brief Update version history after successful training
     */
    void updateVersionHistory(const std::string& new_version);
};

}  // namespace lora
}  // namespace llm
}  // namespace themis
