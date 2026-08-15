/**
 * @file task_anomaly_detector.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "scheduler/task_audit_event.h"
#include <string>
#include <map>
#include <deque>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <memory>
#include <functional>

namespace themis {
namespace scheduler {

/**
 * @brief Configuration for anomaly detection
 */
struct AnomalyDetectorConfig {
    // Detection thresholds (0-1 scale)
    double frequency_threshold = 0.7;      // Trigger alert if frequency score > threshold
    double pattern_threshold = 0.7;        // Trigger alert if pattern score > threshold
    double resource_threshold = 0.8;       // Trigger alert if resource score > threshold
    double failure_rate_threshold = 0.6;   // Trigger alert if failure rate score > threshold
    double overall_threshold = 0.7;        // Trigger alert if overall score > threshold
    
    // Baseline learning
    size_t min_samples = 30;               // Minimum samples needed for baseline
    size_t max_history_size = 1000;        // Maximum history entries per task
    std::chrono::hours baseline_window{24}; // Time window for baseline calculation
    
    // Frequency detection
    double frequency_spike_factor = 3.0;    // Spike if frequency > mean * factor
    double frequency_drop_factor = 0.3;     // Drop if frequency < mean * factor
    
    // Resource detection
    double resource_spike_factor = 2.5;     // Spike if resource > mean * factor
    
    // Failure rate detection
    double failure_rate_spike = 0.3;        // Alert if failure rate > threshold
    
    // Pattern detection
    size_t pattern_window_size = 10;        // Window size for pattern analysis
    
    // Enabled features
    bool enable_frequency_detection = true;
    bool enable_pattern_detection = true;
    bool enable_resource_detection = true;
    bool enable_failure_rate_detection = true;
    
    // GAP 2 FIX: Callback for async anomaly notifications
    std::function<void(const std::string& task_id, const AnomalyMetrics&)> on_anomaly_detected;
};

/**
 * @brief Statistics for a single task
 */
struct TaskStatistics {
    // Execution frequency
    size_t total_executions = 0;
    double executions_per_hour = 0.0;
    double mean_execution_frequency = 0.0;
    double stddev_execution_frequency = 0.0;
    
    // Execution time
    double mean_execution_time_ms = 0.0;
    double stddev_execution_time_ms = 0.0;
    double min_execution_time_ms = 0.0;
    double max_execution_time_ms = 0.0;
    
    // Resource usage
    double mean_cpu_time_ms = 0.0;
    double stddev_cpu_time_ms = 0.0;
    double mean_memory_bytes = 0.0;
    double stddev_memory_bytes = 0.0;
    
    // Failure tracking
    size_t total_failures = 0;
    double failure_rate = 0.0;
    double recent_failure_rate = 0.0; // Last N executions
    
    // Time tracking
    std::chrono::system_clock::time_point first_execution;
    std::chrono::system_clock::time_point last_execution;
    
    // Pattern tracking
    std::deque<std::chrono::system_clock::time_point> execution_times;
    std::deque<double> execution_durations;
    std::deque<bool> execution_results; // true = success, false = failure
    std::deque<double> cpu_usage;
    std::deque<double> memory_usage;
};

/**
 * @brief Anomaly detector for task scheduler
 * 
 * Features:
 * - Statistical baseline learning from historical data
 * - Real-time anomaly scoring (0-1 scale)
 * - Multi-dimensional anomaly detection (frequency, pattern, resource, failure)
 * - Configurable sensitivity and thresholds
 * - Thread-safe operation
 * - Automatic baseline updates
 */
class TaskAnomalyDetector {
public:
    explicit TaskAnomalyDetector(const AnomalyDetectorConfig& config = AnomalyDetectorConfig());
    
    ~TaskAnomalyDetector();
    
    // Lifecycle (GAP 2 FIX: background thread management)
    void start();
    void stop();
    
    /**
     * @brief Record a task execution event
     * @param event Audit event to process
     * @return Anomaly metrics for this execution
     */
    AnomalyMetrics recordExecution(const TaskAuditEvent& event);
    
    /**
     * @brief Get statistics for a specific task
     * @param task_id Task identifier
     * @return Task statistics (or empty if task not found)
     */
    std::optional<TaskStatistics> getTaskStatistics(const std::string& task_id) const;
    
    /**
     * @brief Get all task statistics
     * @return Map of task_id -> statistics
     */
    std::map<std::string, TaskStatistics> getAllStatistics() const;
    
    /**
     * @brief Reset statistics for a specific task
     * @param task_id Task identifier
     */
    void resetTaskStatistics(const std::string& task_id);
    
    /**
     * @brief Reset all statistics
     */
    void resetAllStatistics();
    
    /**
     * @brief Check if task has sufficient baseline data
     * @param task_id Task identifier
     * @return true if baseline is established
     */
    bool hasBaseline(const std::string& task_id) const;
    
    /**
     * GAP 1 FIX: On-demand anomaly detection for external invocation
     * @brief Check anomaly status without recording an execution
     * @param task_id Task identifier
     * @return Anomaly metrics (advisory, non-blocking)
     */
    AnomalyMetrics checkAnomaly(const std::string& task_id) const;
    
    /**
     * GAP 3 FIX: Explicit baseline recalibration
     * @brief Recalibrate baseline for a task (reset time-window data)
     * @param task_id Task identifier
     * @note Preserves existing statistics but resets time-based calculations
     */
    void recalibrateBaseline(const std::string& task_id);
    
    /**
     * @brief Get current configuration
     */
    AnomalyDetectorConfig getConfig() const;
    
    /**
     * @brief Update configuration
     */
    void updateConfig(const AnomalyDetectorConfig& config);
    
    /**
     * @brief Export statistics to JSON (for persistence/analysis)
     */
    nlohmann::json exportStatistics() const;
    
    /**
     * @brief Import statistics from JSON (for restoration)
     */
    void importStatistics(const nlohmann::json& data);

private:
    AnomalyDetectorConfig config_;
    mutable std::mutex mutex_;
    
    // Per-task statistics
    std::map<std::string, TaskStatistics> task_stats_;
    
    // GAP 2 FIX: Background thread for async anomaly callback delivery
    std::thread callback_thread_;
    std::atomic<bool> running_{false};
    std::queue<std::pair<std::string, AnomalyMetrics>> anomaly_queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    
    // Background callback processor
    void anomalyCallbackWorker();
    
    // Anomaly detection methods
    double detectFrequencyAnomaly(const std::string& task_id, 
                                  const std::chrono::system_clock::time_point& now) const;
    
    double detectPatternAnomaly(const std::string& task_id,
                                const std::chrono::system_clock::time_point& now) const;
    
    double detectResourceAnomaly(const std::string& task_id,
                                 const TaskResourceUsage& resource_usage) const;
    
    double detectFailureRateAnomaly(const std::string& task_id,
                                    bool success) const;
    
    // Statistical helpers
    void updateStatistics(const std::string& task_id, const TaskAuditEvent& event);
    double calculateMean(const std::deque<double>& values) const;
    double calculateStdDev(const std::deque<double>& values, double mean) const;
    double calculatePercentile(const std::deque<double>& values, double percentile) const;
    
    // Cleanup old data
    void cleanupOldData(TaskStatistics& stats);
};

} // namespace scheduler
} // namespace themis
