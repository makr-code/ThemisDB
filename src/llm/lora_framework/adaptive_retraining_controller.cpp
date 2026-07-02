/**
 * @file adaptive_retraining_controller.cpp
 * @brief Implementation of adaptive retraining orchestration
 */

#include "llm/lora_framework/adaptive_retraining_controller.h"
#include "utils/logger.h"
#include <spdlog/spdlog.h>
#include <uuid/uuid.h>
#include <algorithm>

namespace themis {
namespace llm {
namespace lora {

// ============================================================================
// RetrainingDecision Implementation
// ============================================================================

// ============================================================================
// RetrainingResult Implementation
// ============================================================================

json RetrainingResult::toJSON() const {
    return json{
        {"success", success},
        {"old_version", old_version},
        {"new_version", new_version},
        {"training_time_seconds", training_time_seconds},
        {"final_loss", final_loss},
        {"validation_accuracy", validation_accuracy},
        {"accuracy_improvement", accuracy_improvement},
        {"latency_improvement_ms", latency_improvement_ms},
        {"is_deployed", is_deployed},
        {"requires_rollback", requires_rollback},
        {"error_message", error_message}
    };
}

// ============================================================================
// RetrainingTriggerConfig Implementation
// ============================================================================

json RetrainingTriggerConfig::toJSON() const {
    return json{
        {"feedback_trigger_enabled", feedback_trigger_enabled},
        {"feedback_threshold", feedback_threshold},
        {"interval_trigger_enabled", interval_trigger_enabled},
        {"retraining_interval_hours", retraining_interval.count()},
        {"quality_trigger_enabled", quality_trigger_enabled},
        {"accuracy_drop_threshold", accuracy_drop_threshold},
        {"latency_increase_threshold", latency_increase_threshold},
        {"auto_rollback_enabled", auto_rollback_enabled},
        {"rollback_accuracy_threshold", rollback_accuracy_threshold},
        {"min_time_between_retrains_seconds", min_time_between_retrains.count()}
    };
}

RetrainingTriggerConfig RetrainingTriggerConfig::fromJSON(const json& j) {
    RetrainingTriggerConfig config;
    if (j.contains("feedback_trigger_enabled")) 
        config.feedback_trigger_enabled = j["feedback_trigger_enabled"];
    if (j.contains("feedback_threshold")) 
        config.feedback_threshold = j["feedback_threshold"];
    if (j.contains("interval_trigger_enabled")) 
        config.interval_trigger_enabled = j["interval_trigger_enabled"];
    if (j.contains("retraining_interval_hours")) 
        config.retraining_interval = std::chrono::hours(j["retraining_interval_hours"]);
    if (j.contains("quality_trigger_enabled")) 
        config.quality_trigger_enabled = j["quality_trigger_enabled"];
    if (j.contains("accuracy_drop_threshold")) 
        config.accuracy_drop_threshold = j["accuracy_drop_threshold"];
    if (j.contains("latency_increase_threshold")) 
        config.latency_increase_threshold = j["latency_increase_threshold"];
    if (j.contains("auto_rollback_enabled")) 
        config.auto_rollback_enabled = j["auto_rollback_enabled"];
    if (j.contains("rollback_accuracy_threshold")) 
        config.rollback_accuracy_threshold = j["rollback_accuracy_threshold"];
    if (j.contains("min_time_between_retrains_seconds")) 
        config.min_time_between_retrains = std::chrono::seconds(j["min_time_between_retrains_seconds"]);
    return config;
}

// ============================================================================
// AdaptiveRetrainingController Implementation
// ============================================================================

AdaptiveRetrainingController::AdaptiveRetrainingController(
    const std::string& adapter_id,
    const Dependencies& deps,
    const RetrainingTriggerConfig& config
)
    : adapter_id_(adapter_id)
    , deps_(deps)
    , config_(config)
    , current_version_("v1.0")
    , last_retraining_time_(std::chrono::system_clock::now())
{
    version_history_.push_back("v1.0");
    spdlog::info("AdaptiveRetrainingController initialized for adapter '{}'", adapter_id);
}

void AdaptiveRetrainingController::addFeedback(const Feedback& feedback) {
    std::lock_guard<std::mutex> lock(mutex_);
    feedback_buffer_.push_back(feedback);
    
    spdlog::debug("Added feedback for adapter '{}', buffer size: {}", 
                  adapter_id_, feedback_buffer_.size());
}

void AdaptiveRetrainingController::addMetric(const TelemetryMetrics& metric) {
    if (!deps_.telemetry_adapter) {
        spdlog::warn("Telemetry adapter not configured, metric not recorded");
        return;
    }
    
    auto feedback = deps_.telemetry_adapter->recordMetric(metric);
    if (feedback) {
        addFeedback(*feedback);
    }
}

RetrainingDecision AdaptiveRetrainingController::evaluateRetrainingNeed() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    RetrainingDecision decision;
    decision.should_retrain = false;
    
    // Check rate limiting first
    if (isRateLimitExceeded()) {
        decision.reason_description = "Rate limit exceeded, waiting before next retrain";
        return decision;
    }
    
    // 1. Check feedback threshold trigger
    if (config_.feedback_trigger_enabled && feedback_buffer_.size() >= config_.feedback_threshold) {
        decision.should_retrain = true;
        decision.reason = RetrainingDecision::Reason::FEEDBACK_THRESHOLD_MET;
        decision.reason_description = "Feedback threshold reached: " + 
                                      std::to_string(feedback_buffer_.size()) + " / " +
                                      std::to_string(config_.feedback_threshold);
        decision.confidence = 0.95f;
        return decision;
    }
    
    // 2. Check time interval trigger
    if (config_.interval_trigger_enabled) {
        auto time_since_last = std::chrono::system_clock::now() - last_retraining_time_;
        if (time_since_last > config_.retraining_interval) {
            decision.should_retrain = true;
            decision.reason = RetrainingDecision::Reason::TIME_INTERVAL_ELAPSED;
            decision.reason_description = "Scheduled retraining interval elapsed";
            decision.confidence = 0.85f;
            
            // But only if we have at least some feedback
            if (feedback_buffer_.size() > 10) {
                return decision;
            }
        }
    }
    
    // 3. Check quality degradation trigger
    if (config_.quality_trigger_enabled && deps_.telemetry_adapter) {
        auto current_metrics = deps_.telemetry_adapter->computeVersionMetrics(adapter_id_);
        
        // Compare with baseline if available
        if (version_history_.size() > 1) {
            auto it = version_metrics_.find(version_history_[version_history_.size() - 2]);
            if (it != version_metrics_.end()) {
                if (deps_.telemetry_adapter->isQualityDegraded(adapter_id_, it->second)) {
                    decision.should_retrain = true;
                    decision.reason = RetrainingDecision::Reason::QUALITY_DEGRADATION;
                    decision.reason_description = "Quality degradation detected";
                    decision.confidence = 0.90f;
                    return decision;
                }
            }
        }
    }
    
    return decision;
}

RetrainingResult AdaptiveRetrainingController::executeRetraining(
    const RetrainingDecision& decision,
    const std::optional<TrainingData>& training_data
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    RetrainingResult result;
    result.old_version = current_version_;
    
    if (!decision.should_retrain) {
        spdlog::info("Retraining not triggered for adapter '{}'", adapter_id_);
        return result;
    }
    
    // Prepare training data
    TrainingData data;
    if (training_data) {
        data = *training_data;
    } else {
        data = feedbackToTrainingData(feedback_buffer_);
    }
    
    if (data.samples.empty()) {
        result.error_message = "No training data available";
        spdlog::warn("Cannot retrain adapter '{}': no training data", adapter_id_);
        return result;
    }
    
    // Log training start
    if (deps_.audit_logger) {
        deps_.audit_logger->logTraining(
            LoRAAuditEventType::TRAINING_STARTED,
            adapter_id_,
            data.samples.size(),
            0.0f,
            0.0f,
            {
                {"reason", decision.reason_description},
                {"confidence", decision.confidence}
            }
        );
    }
    
    try {
        // Execute training
        auto training_result = deps_.training_service->trainOnTheFly(adapter_id_, data);
        
        if (!training_result.success) {
            result.error_message = training_result.error_message;
            
            if (deps_.audit_logger) {
                deps_.audit_logger->logTraining(
                    LoRAAuditEventType::TRAINING_FAILED,
                    adapter_id_,
                    data.samples.size(),
                    0.0f,
                    0.0f,
                    {{"error", training_result.error_message}}
                );
            }
            
            spdlog::error("Training failed for adapter '{}': {}", 
                         adapter_id_, training_result.error_message);
            return result;
        }
        
        // Update version
        int version_num = std::stoi(current_version_.substr(1));
        version_num++;
        std::string new_version = "v" + std::to_string(version_num);
        
        // Update state
        current_version_ = new_version;
        version_history_.push_back(new_version);
        last_retraining_time_ = std::chrono::system_clock::now();
        
        // Clear feedback buffer
        feedback_buffer_.clear();
        
        // Prepare result
        result.success = true;
        result.new_version = new_version;
        result.final_loss = training_result.final_loss;
        result.validation_accuracy = training_result.validation_accuracy;
        result.training_time_seconds = training_result.training_time.count();
        result.is_deployed = true;
        
        // Store metrics
        AdapterVersionMetrics metrics;
        metrics.version = new_version;
        metrics.validation_accuracy = training_result.validation_accuracy;
        version_metrics_[new_version] = metrics;
        
        // Log training completion
        if (deps_.audit_logger) {
            deps_.audit_logger->logTraining(
                LoRAAuditEventType::TRAINING_COMPLETED,
                adapter_id_,
                data.samples.size(),
                training_result.final_loss,
                training_result.validation_accuracy,
                {
                    {"new_version", new_version},
                    {"duration_seconds", result.training_time_seconds}
                }
            );
        }
        
        spdlog::info("Training completed successfully for adapter '{}': {} -> {}", 
                    adapter_id_, result.old_version, new_version);
        
    } catch (const std::exception& e) {
        result.error_message = std::string(e.what());
        
        if (deps_.audit_logger) {
            deps_.audit_logger->logTraining(
                LoRAAuditEventType::TRAINING_FAILED,
                adapter_id_,
                data.samples.size(),
                0.0f,
                0.0f,
                {{"exception", e.what()}}
            );
        }
        
        spdlog::error("Training exception for adapter '{}': {}", adapter_id_, e.what());
    }
    
    retraining_history_.push_back(result);
    return result;
}

bool AdaptiveRetrainingController::checkAndRollbackIfNeeded() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!config_.auto_rollback_enabled) {
        return false;
    }
    
    if (!deps_.telemetry_adapter) {
        return false;
    }
    
    // Get current metrics
    auto current_metrics = deps_.telemetry_adapter->computeVersionMetrics(adapter_id_);
    
    // Check if accuracy has dropped below rollback threshold
    if (current_metrics.avg_accuracy < config_.rollback_accuracy_threshold) {
        spdlog::warn("Quality degradation detected for adapter '{}': accuracy {}",
                     adapter_id_, current_metrics.avg_accuracy);
        
        // Find previous stable version
        if (version_history_.size() > 1) {
            auto previous_version = version_history_[version_history_.size() - 2];
            
            // Log rollback
            if (deps_.audit_logger) {
                deps_.audit_logger->logEvent(
                    LoRAAuditEventType::ROLLBACK_TRIGGERED,
                    adapter_id_,
                    {
                        {"from_version", current_version_},
                        {"to_version", previous_version},
                        {"reason", "Accuracy below threshold"},
                        {"current_accuracy", current_metrics.avg_accuracy}
                    }
                );
            }
            
            current_version_ = previous_version;
            spdlog::info("Automatic rollback: {} -> {}", 
                        version_history_.back(), previous_version);
            return true;
        }
    }
    
    return false;
}

RetrainingResult AdaptiveRetrainingController::triggerRetrainingNow(
    const std::optional<TrainingData>& training_data
) {
    RetrainingDecision decision;
    decision.should_retrain = true;
    decision.reason = RetrainingDecision::Reason::MANUAL_TRIGGER;
    decision.reason_description = "Manually triggered";
    decision.confidence = 1.0f;
    
    return executeRetraining(decision, training_data);
}

std::vector<std::string> AdaptiveRetrainingController::getVersionHistory() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return version_history_;
}

std::string AdaptiveRetrainingController::getCurrentVersion() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return current_version_;
}

std::optional<AdapterVersionMetrics> AdaptiveRetrainingController::getVersionMetrics(
    const std::string& version
) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = version_metrics_.find(version);
    if (it != version_metrics_.end()) {
        return it->second;
    }
    return std::nullopt;
}

size_t AdaptiveRetrainingController::getFeedbackCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return feedback_buffer_.size();
}

std::vector<RetrainingResult> AdaptiveRetrainingController::getRetrainingHistory() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return retraining_history_;
}

void AdaptiveRetrainingController::setConfig(const RetrainingTriggerConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_ = config;
}

void AdaptiveRetrainingController::clearFeedbackBuffer() {
    std::lock_guard<std::mutex> lock(mutex_);
    feedback_buffer_.clear();
}

// ============================================================================
// Private Methods
// ============================================================================

bool AdaptiveRetrainingController::isRateLimitExceeded() const {
    auto time_since = std::chrono::system_clock::now() - last_retraining_time_;
    return time_since < config_.min_time_between_retrains;
}

TrainingData AdaptiveRetrainingController::feedbackToTrainingData(
    const std::vector<Feedback>& feedback
) const {
    TrainingData data;
    data.dataset_name = "feedback_" + adapter_id_ + "_" + current_version_;
    
    for (const auto& fb : feedback) {
        if (fb.flagged_for_training) {
            TrainingDataSample sample;
            sample.input = fb.prompt;
            sample.output = fb.response;
            sample.metadata = {
                {"rating", fb.rating},
                {"category", fb.training_category},
                {"weight", fb.training_weight}
            };
            data.samples.push_back(sample);
        }
    }
    
    return data;
}

void AdaptiveRetrainingController::updateVersionHistory(const std::string& new_version) {
    if (std::find(version_history_.begin(), version_history_.end(), new_version) 
        == version_history_.end()) {
        version_history_.push_back(new_version);
    }
}

}  // namespace lora
}  // namespace llm
}  // namespace themis
