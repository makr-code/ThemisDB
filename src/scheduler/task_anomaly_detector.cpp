/**
 * @file task_anomaly_detector.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=5, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "scheduler/task_anomaly_detector.h"
#include "utils/logger.h"
#include <algorithm>
#include <numeric>
#include <cmath>

namespace themis {
namespace scheduler {

TaskAnomalyDetector::TaskAnomalyDetector(const AnomalyDetectorConfig& config)
    : config_(config) {
    // GAP 2 FIX: Only start the background callback thread when a callback is
    // registered; creating an always-on thread with no functional benefit wastes
    // resources and complicates shutdown coordination.
    if (config_.on_anomaly_detected) {
        start();
    }
}

TaskAnomalyDetector::~TaskAnomalyDetector() {
    stop();
}

// GAP 2 FIX: Lifecycle management for background callback thread
void TaskAnomalyDetector::start() {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    if (running_.load()) {
        return;
    }
    
    running_.store(true);
    callback_thread_ = std::thread(&TaskAnomalyDetector::anomalyCallbackWorker, this);
    THEMIS_INFO("TaskAnomalyDetector: background callback thread started");
}

void TaskAnomalyDetector::stop() {
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        if (!running_.load()) {
            return;
        }
        running_.store(false);
    }
    
    queue_cv_.notify_all();
    
    if (callback_thread_.joinable()) {
        callback_thread_.join();
    }
    
    THEMIS_INFO("TaskAnomalyDetector: background callback thread stopped");
}

// Background worker thread for async anomaly callbacks
void TaskAnomalyDetector::anomalyCallbackWorker() {
    THEMIS_DEBUG("TaskAnomalyDetector callback worker started");
    
    while (running_.load()) {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        
        // Wait for anomalies to be queued (with timeout to check running_ flag)
        queue_cv_.wait_for(lock, std::chrono::milliseconds(500),
                          [this] { return !anomaly_queue_.empty() || !running_.load(); });
        
        while (!anomaly_queue_.empty()) {
            auto [task_id, metrics] = std::move(anomaly_queue_.front());
            anomaly_queue_.pop();
            
            // Release lock before invoking callback (to prevent deadlock)
            lock.unlock();
            
            // Invoke callback outside of lock
            if (config_.on_anomaly_detected) {
                try {
                    config_.on_anomaly_detected(task_id, metrics);
                } catch (const std::exception& e) {
                    THEMIS_ERROR("TaskAnomalyDetector: anomaly callback failed: {}", e.what());
                }
            }
            
            lock.lock();
        }
    }
    
    THEMIS_DEBUG("TaskAnomalyDetector callback worker stopped");
}

AnomalyMetrics TaskAnomalyDetector::recordExecution(const TaskAuditEvent& event) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    const std::string& task_id = event.task_id;
    
    // Update statistics first
    updateStatistics(task_id, event);
    
    // Initialize anomaly metrics
    AnomalyMetrics metrics;
    
    // Check if we have enough baseline data
    if (!hasBaseline(task_id)) {
        // Not enough data for anomaly detection yet
        metrics.description = "Insufficient baseline data";
        return metrics;
    }
    
    // Detect anomalies across different dimensions
    if (config_.enable_frequency_detection) {
        metrics.frequency_score = detectFrequencyAnomaly(task_id, event.timestamp);
    }
    
    if (config_.enable_pattern_detection) {
        metrics.pattern_score = detectPatternAnomaly(task_id, event.timestamp);
    }
    
    if (config_.enable_resource_detection) {
        metrics.resource_score = detectResourceAnomaly(task_id, event.resource_usage);
    }
    
    if (config_.enable_failure_rate_detection) {
        metrics.failure_rate_score = detectFailureRateAnomaly(task_id, event.success);
    }
    
    // Calculate overall anomaly score (weighted average)
    metrics.overall_score = (
        metrics.frequency_score * 0.25 +
        metrics.pattern_score * 0.25 +
        metrics.resource_score * 0.25 +
        metrics.failure_rate_score * 0.25
    );
    
    // Determine if this is anomalous
    metrics.is_anomalous = metrics.overall_score > config_.overall_threshold;
    
    // Build description
    if (metrics.is_anomalous) {
        std::vector<std::string> anomalies;
        
        if (metrics.frequency_score > config_.frequency_threshold) {
            anomalies.push_back("frequency deviation");
        }
        if (metrics.pattern_score > config_.pattern_threshold) {
            anomalies.push_back("pattern deviation");
        }
        if (metrics.resource_score > config_.resource_threshold) {
            anomalies.push_back("resource usage spike");
        }
        if (metrics.failure_rate_score > config_.failure_rate_threshold) {
            anomalies.push_back("elevated failure rate");
        }
        
        if (!anomalies.empty()) {
            metrics.description = "Detected: ";
            for (size_t i = 0; i < anomalies.size(); i++) {
                if (i > 0) {
                  metrics.description += ", ";
                }
                metrics.description += anomalies[i];
            }
        }
    }
    
    // GAP 2 FIX: Queue anomaly for async callback delivery (non-blocking)
    if (metrics.is_anomalous && config_.on_anomaly_detected) {
        std::lock_guard<std::mutex> queue_lock(queue_mutex_);
        anomaly_queue_.push({task_id, metrics});
        queue_cv_.notify_one();
    }
    
    return metrics;
}

void TaskAnomalyDetector::updateStatistics(const std::string& task_id, 
                                           const TaskAuditEvent& event) {
    auto& stats = task_stats_[task_id];
    
    // Update execution counts
    stats.total_executions++;
    if (!event.success) {
        stats.total_failures++;
    }
    
    // Update time tracking
    if (stats.total_executions == 1) {
        stats.first_execution = event.timestamp;
    }
    stats.last_execution = event.timestamp;
    
    // Record execution time
    stats.execution_times.push_back(event.timestamp);
    stats.execution_durations.push_back(event.duration_ms);
    stats.execution_results.push_back(event.success);
    
    // Record resource usage
    stats.cpu_usage.push_back(event.resource_usage.cpu_time_ms);
    stats.memory_usage.push_back(static_cast<double>(event.resource_usage.memory_bytes));
    
    // Limit history size
    if (static_cast<int>(stats.execution_times.size()) > config_.max_history_size) {
        stats.execution_times.pop_front();
        stats.execution_durations.pop_front();
        stats.execution_results.pop_front();
        stats.cpu_usage.pop_front();
        stats.memory_usage.pop_front();
    }
    
    // Update calculated statistics
    if (!stats.execution_durations.empty()) {
        stats.mean_execution_time_ms = calculateMean(stats.execution_durations);
        stats.stddev_execution_time_ms = calculateStdDev(stats.execution_durations, 
                                                          stats.mean_execution_time_ms);
        
        stats.min_execution_time_ms = *std::min_element(stats.execution_durations.begin(),
                                                         stats.execution_durations.end());
        stats.max_execution_time_ms = *std::max_element(stats.execution_durations.begin(),
                                                         stats.execution_durations.end());
    }
    
    if (!stats.cpu_usage.empty()) {
        stats.mean_cpu_time_ms = calculateMean(stats.cpu_usage);
        stats.stddev_cpu_time_ms = calculateStdDev(stats.cpu_usage, stats.mean_cpu_time_ms);
    }
    
    if (!stats.memory_usage.empty()) {
        stats.mean_memory_bytes = calculateMean(stats.memory_usage);
        stats.stddev_memory_bytes = calculateStdDev(stats.memory_usage, stats.mean_memory_bytes);
    }
    
    // Calculate failure rates
    if (stats.total_executions > 0) {
        stats.failure_rate = static_cast<double>(stats.total_failures) / stats.total_executions;
        
        // Recent failure rate (last 20 executions)
        size_t recent_window = std::min(size_t(20), static_cast<size_t>(stats.execution_results.size()));
        size_t recent_failures = 0;
        for (size_t i = static_cast<int>(stats.execution_results.size()) - recent_window; 
             i <static_cast<int>(stats.execution_results.size()); i++) {
            if (!stats.execution_results[i]) {
                recent_failures++;
            }
        }
        stats.recent_failure_rate = static_cast<double>(recent_failures) / recent_window;
    }
    
    // Calculate execution frequency
    if (static_cast<int>(stats.execution_times.size()) >= 2) {
        auto duration = stats.last_execution - stats.first_execution;
        auto hours = std::chrono::duration_cast<std::chrono::hours>(duration).count();
        if (hours > 0) {
            stats.executions_per_hour = static_cast<double>(stats.total_executions) / hours;
        }
    }
    
    // Cleanup old data outside baseline window
    cleanupOldData(stats);
}

double TaskAnomalyDetector::detectFrequencyAnomaly(const std::string& task_id,
                                                   const std::chrono::system_clock::time_point& now) const {
    auto it = task_stats_.find(task_id);
    if (it == task_stats_.end()) {
        return 0.0;
    }
    const auto& stats = it->second;
    
    if (static_cast<int>(stats.execution_times.size()) < config_.min_samples) {
        return 0.0;
    }
    
    // Calculate recent execution frequency (last hour)
    auto one_hour_ago = now - std::chrono::hours(1);
    size_t recent_count = std::count_if(stats.execution_times.begin(),
                                       stats.execution_times.end(),
                                       [one_hour_ago](const auto& t) { return t >= one_hour_ago; });
    
    double recent_frequency = static_cast<double>(recent_count);
    
    // Calculate baseline frequency (mean executions per hour)
    double baseline_frequency = stats.executions_per_hour;
    
    if (baseline_frequency == 0) {
        return 0.0;
    }
    
    // Detect spike (frequency much higher than baseline)
    if (recent_frequency > baseline_frequency * config_.frequency_spike_factor) {
        double spike_ratio = recent_frequency / (baseline_frequency * config_.frequency_spike_factor);
        return std::min(1.0, spike_ratio);
    }
    
    // Detect drop (frequency much lower than baseline)
    // Only if we expect at least one execution
    if (baseline_frequency >= 1.0 && 
        recent_frequency < baseline_frequency * config_.frequency_drop_factor) {
        double drop_ratio = 1.0 - (recent_frequency / (baseline_frequency * config_.frequency_drop_factor));
        return std::min(1.0, drop_ratio);
    }
    
    return 0.0;
}

double TaskAnomalyDetector::detectPatternAnomaly(const std::string& task_id,
                                                 const std::chrono::system_clock::time_point& now) const {
    auto it = task_stats_.find(task_id);
    if (it == task_stats_.end()) {
        return 0.0;
    }
    const auto& stats = it->second;
    
    if (static_cast<int>(stats.execution_times.size()) < config_.pattern_window_size) {
        return 0.0;
    }
    
    // Analyze inter-execution intervals
    std::vector<double> intervals = {};

    for (size_t i = 1; i <static_cast<int>(stats.execution_times.size()); i++) {
        auto interval = stats.execution_times[i] - stats.execution_times[static_cast<int>(i - 1)];
        intervals.push_back(std::chrono::duration<double>(interval).count());
    }
    
    if (intervals.empty()) {
        return 0.0;
    }
    
    // Calculate baseline pattern (mean and stddev of intervals)
    double mean_interval = calculateMean(
        std::deque<double>(intervals.begin(), intervals.end()));
    double stddev_interval = calculateStdDev(
        std::deque<double>(intervals.begin(), intervals.end()), mean_interval);
    
    // Get recent interval
    if (static_cast<int>(intervals.size()) >= 2) {
        double recent_interval = intervals.back();
        
        // Check if recent interval is significantly different from baseline
        if (stddev_interval > 0) {
            double z_score = std::abs(recent_interval - mean_interval) / stddev_interval;
            // Normalize to 0-1 scale (z-score > 3 is anomalous)
            return std::min(1.0, z_score / 3.0);
        }
    }
    
    return 0.0;
}

double TaskAnomalyDetector::detectResourceAnomaly(const std::string& task_id,
                                                  const TaskResourceUsage& resource_usage) const {
    auto it = task_stats_.find(task_id);
    if (it == task_stats_.end()) {
        return 0.0;
    }
    const auto& stats = it->second;
    
    if (static_cast<int>(stats.cpu_usage.size()) < config_.min_samples) {
        return 0.0;
    }
    
    double max_score = 0.0;
    
    // Check CPU usage
    if (stats.mean_cpu_time_ms > 0) {
        double cpu_ratio = resource_usage.cpu_time_ms / stats.mean_cpu_time_ms;
        if (cpu_ratio > config_.resource_spike_factor) {
            double cpu_score = (cpu_ratio - config_.resource_spike_factor) / config_.resource_spike_factor;
            max_score = std::max(max_score, std::min(1.0, cpu_score));
        }
    }
    
    // Check memory usage
    if (stats.mean_memory_bytes > 0) {
        double mem_ratio = static_cast<double>(resource_usage.memory_bytes) / stats.mean_memory_bytes;
        if (mem_ratio > config_.resource_spike_factor) {
            double mem_score = (mem_ratio - config_.resource_spike_factor) / config_.resource_spike_factor;
            max_score = std::max(max_score, std::min(1.0, mem_score));
        }
    }
    
    return max_score;
}

double TaskAnomalyDetector::detectFailureRateAnomaly(const std::string& task_id,
                                                     bool success) const {
    auto it = task_stats_.find(task_id);
    if (it == task_stats_.end()) {
        return 0.0;
    }
    const auto& stats = it->second;
    
    if (static_cast<int>(stats.execution_results.size()) < config_.min_samples) {
        return 0.0;
    }
    
    // Check recent failure rate
    if (stats.recent_failure_rate > config_.failure_rate_spike) {
        // Normalize to 0-1 scale
        return std::min(1.0, stats.recent_failure_rate / config_.failure_rate_spike);
    }
    
    return 0.0;
}

bool TaskAnomalyDetector::hasBaseline(const std::string& task_id) const {
    auto it = task_stats_.find(task_id);
    if (it == task_stats_.end()) {
        return false;
    }
    
    return it->second.total_executions >= config_.min_samples;
}

// GAP 1 FIX: On-demand anomaly detection
AnomalyMetrics TaskAnomalyDetector::checkAnomaly(const std::string& task_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    AnomalyMetrics metrics;
    
    // Check if we have enough baseline data
    if (!hasBaseline(task_id)) {
        metrics.description = "Insufficient baseline data";
        return metrics;
    }
    
    auto it = task_stats_.find(task_id);
    if (it == task_stats_.end()) {
        metrics.description = "Task not found";
        return metrics;
    }
    
    // Get current timestamp for anomaly checks
    auto now = std::chrono::system_clock::now();
    
    // Detect anomalies across different dimensions
    if (config_.enable_frequency_detection) {
        metrics.frequency_score = detectFrequencyAnomaly(task_id, now);
    }
    
    if (config_.enable_pattern_detection) {
        metrics.pattern_score = detectPatternAnomaly(task_id, now);
    }
    
    if (config_.enable_resource_detection) {
        // Use current resource baseline as a reference
        TaskResourceUsage ref_usage;
        ref_usage.cpu_time_ms = it->second.mean_cpu_time_ms;
        ref_usage.memory_bytes = static_cast<uint64_t>(it->second.mean_memory_bytes);
        metrics.resource_score = detectResourceAnomaly(task_id, ref_usage);
    }
    
    if (config_.enable_failure_rate_detection) {
        // Use current failure status as reference
        bool current_success = it->second.failure_rate < 0.5;  // Heuristic threshold
        metrics.failure_rate_score = detectFailureRateAnomaly(task_id, current_success);
    }
    
    // Calculate overall anomaly score (weighted average)
    metrics.overall_score = (
        metrics.frequency_score * 0.25 +
        metrics.pattern_score * 0.25 +
        metrics.resource_score * 0.25 +
        metrics.failure_rate_score * 0.25
    );
    
    // Determine if this is anomalous
    metrics.is_anomalous = metrics.overall_score > config_.overall_threshold;
    
    // Build description
    if (metrics.is_anomalous) {
        std::vector<std::string> anomalies;
        
        if (metrics.frequency_score > config_.frequency_threshold) {
            anomalies.push_back("frequency deviation");
        }
        if (metrics.pattern_score > config_.pattern_threshold) {
            anomalies.push_back("pattern deviation");
        }
        if (metrics.resource_score > config_.resource_threshold) {
            anomalies.push_back("resource usage spike");
        }
        if (metrics.failure_rate_score > config_.failure_rate_threshold) {
            anomalies.push_back("elevated failure rate");
        }
        
        if (!anomalies.empty()) {
            metrics.description = "Detected: ";
            for (size_t i = 0; i < anomalies.size(); i++) {
                if (i > 0) {
                  metrics.description += ", ";
                }
                metrics.description += anomalies[i];
            }
        }
    }
    
    return metrics;
}

// GAP 3 FIX: Explicit baseline recalibration
void TaskAnomalyDetector::recalibrateBaseline(const std::string& task_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = task_stats_.find(task_id);
    if (it == task_stats_.end()) {
        THEMIS_WARN("TaskAnomalyDetector: recalibration requested for unknown task '{}'", task_id);
        return;
    }
    
    auto& stats = it->second;
    
    // Reset time-based baseline calculations while preserving raw history
    stats.first_execution = stats.last_execution;
    
    // Clear old data outside the current baseline window
    cleanupOldData(stats);
    
    // Recalculate baseline statistics from remaining data
    if (!stats.execution_durations.empty()) {
        stats.mean_execution_time_ms = calculateMean(stats.execution_durations);
        stats.stddev_execution_time_ms = calculateStdDev(stats.execution_durations,
                                                        stats.mean_execution_time_ms);
        stats.min_execution_time_ms = *std::min_element(stats.execution_durations.begin(),
                                                       stats.execution_durations.end());
        stats.max_execution_time_ms = *std::max_element(stats.execution_durations.begin(),
                                                       stats.execution_durations.end());
    }
    
    if (!stats.cpu_usage.empty()) {
        stats.mean_cpu_time_ms = calculateMean(stats.cpu_usage);
        stats.stddev_cpu_time_ms = calculateStdDev(stats.cpu_usage, stats.mean_cpu_time_ms);
    }
    
    if (!stats.memory_usage.empty()) {
        stats.mean_memory_bytes = calculateMean(stats.memory_usage);
        stats.stddev_memory_bytes = calculateStdDev(stats.memory_usage, stats.mean_memory_bytes);
    }
    
    // Recalculate execution frequency
    if (static_cast<int>(stats.execution_times.size()) >= 2) {
        auto duration = stats.last_execution - stats.first_execution;
        auto hours = std::chrono::duration_cast<std::chrono::hours>(duration).count();
        if (hours > 0) {
            stats.executions_per_hour = static_cast<double>(stats.total_executions) / hours;
        }
    }
    
    THEMIS_INFO("TaskAnomalyDetector: baseline recalibrated for task '{}' (executions={})",
               task_id, stats.total_executions);
}

std::optional<TaskStatistics> TaskAnomalyDetector::getTaskStatistics(const std::string& task_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = task_stats_.find(task_id);
    if (it != task_stats_.end()) {
        return it->second;
    }
    
    return std::nullopt;
}

std::map<std::string, TaskStatistics> TaskAnomalyDetector::getAllStatistics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return task_stats_;
}

void TaskAnomalyDetector::resetTaskStatistics(const std::string& task_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    task_stats_.erase(task_id);
}

void TaskAnomalyDetector::resetAllStatistics() {
    std::lock_guard<std::mutex> lock(mutex_);
    task_stats_.clear();
}

AnomalyDetectorConfig TaskAnomalyDetector::getConfig() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return config_;
}

void TaskAnomalyDetector::updateConfig(const AnomalyDetectorConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_ = config;
}

nlohmann::json TaskAnomalyDetector::exportStatistics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    nlohmann::json j;
    j["config"] = {
        {"frequency_threshold", config_.frequency_threshold},
        {"pattern_threshold", config_.pattern_threshold},
        {"resource_threshold", config_.resource_threshold},
        {"failure_rate_threshold", config_.failure_rate_threshold},
        {"overall_threshold", config_.overall_threshold},
        {"min_samples", config_.min_samples}
    };
    
    j["tasks"] = nlohmann::json::object();
    for (const auto& [task_id, stats] : task_stats_) {
        nlohmann::json task_json;
        task_json["total_executions"] = stats.total_executions;
        task_json["total_failures"] = stats.total_failures;
        task_json["failure_rate"] = stats.failure_rate;
        task_json["recent_failure_rate"] = stats.recent_failure_rate;
        task_json["mean_execution_time_ms"] = stats.mean_execution_time_ms;
        task_json["stddev_execution_time_ms"] = stats.stddev_execution_time_ms;
        task_json["min_execution_time_ms"] = stats.min_execution_time_ms;
        task_json["max_execution_time_ms"] = stats.max_execution_time_ms;
        task_json["executions_per_hour"] = stats.executions_per_hour;
        task_json["mean_cpu_time_ms"] = stats.mean_cpu_time_ms;
        task_json["mean_memory_bytes"] = stats.mean_memory_bytes;

        // Persist raw deque data so baseline survives restarts
        auto dequeToJson = [](const std::deque<double>& d) {
            nlohmann::json arr = nlohmann::json::array();
            for (double v : d) {
              arr.push_back(v);
            }
            return arr;
        };
        task_json["execution_durations"] = dequeToJson(stats.execution_durations);
        task_json["cpu_usage"]           = dequeToJson(stats.cpu_usage);
        task_json["memory_usage"]        = dequeToJson(stats.memory_usage);

        // Persist execution timestamps as milliseconds since epoch
        nlohmann::json ts_arr = nlohmann::json::array();
        for (const auto& tp : stats.execution_times) {
            ts_arr.push_back(std::chrono::duration_cast<std::chrono::milliseconds>(
                tp.time_since_epoch()).count());
        }
        task_json["execution_times_ms"] = ts_arr;

        // Persist execution results (success/failure booleans)
        nlohmann::json res_arr = nlohmann::json::array();
        for (bool b : stats.execution_results) {
          res_arr.push_back(b);
        }
        task_json["execution_results"] = res_arr;

        j["tasks"][task_id] = task_json;
    }
    
    return j;
}

void TaskAnomalyDetector::importStatistics(const nlohmann::json& data) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Import configuration if present
    if (data.contains("config")) {
        const auto& cfg = data["config"];
        if (cfg.contains("frequency_threshold"))
            config_.frequency_threshold = cfg["frequency_threshold"];
        if (cfg.contains("pattern_threshold"))
            config_.pattern_threshold = cfg["pattern_threshold"];
        if (cfg.contains("resource_threshold"))
            config_.resource_threshold = cfg["resource_threshold"];
        if (cfg.contains("failure_rate_threshold"))
            config_.failure_rate_threshold = cfg["failure_rate_threshold"];
        if (cfg.contains("overall_threshold"))
            config_.overall_threshold = cfg["overall_threshold"];
        if (cfg.contains("min_samples"))
            config_.min_samples = cfg["min_samples"];
    }

    // Restore per-task statistics including the raw deque data for baseline detection
    if (!data.contains("tasks")) {
      return;
    }

    for (const auto& [task_id, task_json] : data["tasks"].items()) {
        TaskStatistics& stats = task_stats_[task_id];

        if (task_json.contains("total_executions"))
            stats.total_executions = task_json["total_executions"];
        if (task_json.contains("total_failures"))
            stats.total_failures = task_json["total_failures"];
        if (task_json.contains("failure_rate"))
            stats.failure_rate = task_json["failure_rate"];
        if (task_json.contains("recent_failure_rate"))
            stats.recent_failure_rate = task_json["recent_failure_rate"];
        if (task_json.contains("mean_execution_time_ms"))
            stats.mean_execution_time_ms = task_json["mean_execution_time_ms"];
        if (task_json.contains("stddev_execution_time_ms"))
            stats.stddev_execution_time_ms = task_json["stddev_execution_time_ms"];
        if (task_json.contains("min_execution_time_ms"))
            stats.min_execution_time_ms = task_json["min_execution_time_ms"];
        if (task_json.contains("max_execution_time_ms"))
            stats.max_execution_time_ms = task_json["max_execution_time_ms"];
        if (task_json.contains("executions_per_hour"))
            stats.executions_per_hour = task_json["executions_per_hour"];
        if (task_json.contains("mean_cpu_time_ms"))
            stats.mean_cpu_time_ms = task_json["mean_cpu_time_ms"];
        if (task_json.contains("mean_memory_bytes"))
            stats.mean_memory_bytes = task_json["mean_memory_bytes"];

        // Restore raw deques
        auto jsonToDequeDouble = [](const nlohmann::json& arr) {
            std::deque<double> d = {};

            for (const auto& v : arr) {
              d.push_back(v.get<double>());
            }
            return d;
        };

        if (task_json.contains("execution_durations"))
            stats.execution_durations = jsonToDequeDouble(task_json["execution_durations"]);
        if (task_json.contains("cpu_usage"))
            stats.cpu_usage = jsonToDequeDouble(task_json["cpu_usage"]);
        if (task_json.contains("memory_usage"))
            stats.memory_usage = jsonToDequeDouble(task_json["memory_usage"]);

        if (task_json.contains("execution_times_ms")) {
            stats.execution_times.clear();
            for (const auto& ms : task_json["execution_times_ms"]) {
                stats.execution_times.push_back(
                    std::chrono::system_clock::time_point(
                        std::chrono::milliseconds(ms.get<int64_t>())));
            }
        }

        if (task_json.contains("execution_results")) {
            stats.execution_results.clear();
            for (const auto& b : task_json["execution_results"])
                stats.execution_results.push_back(b.get<bool>());
        }
    }
}

// Helper methods
double TaskAnomalyDetector::calculateMean(const std::deque<double>& values) const {
    if (values.empty()) {
        return 0.0;
    }
    
    double sum = std::accumulate(values.begin(), values.end(), 0.0);
    return sum / values.size();
}

double TaskAnomalyDetector::calculateStdDev(const std::deque<double>& values, double mean) const {
    if (static_cast<int>(values.size()) < 2) {
        return 0.0;
    }
    
    double sum_sq_diff = 0.0;
    for (const auto& val : values) {
        double diff = val - mean;
        sum_sq_diff += diff * diff;
    }
    
    return std::sqrt(sum_sq_diff / (static_cast<int>(values.size()) - 1));
}

double TaskAnomalyDetector::calculatePercentile(const std::deque<double>& values, 
                                                double percentile) const {
    if (values.empty()) {
        return 0.0;
    }
    
    std::vector<double> sorted(values.begin(), values.end());
    std::sort(sorted.begin(), sorted.end());
    
    size_t index = static_cast<size_t>(percentile * (static_cast<int>(sorted.size()) - 1));
    return sorted[index];
}

void TaskAnomalyDetector::cleanupOldData(TaskStatistics& stats) {
    if (stats.execution_times.empty()) {
        return;
    }
    
    auto cutoff = stats.last_execution - config_.baseline_window;
    
    // Remove old entries
    while (!stats.execution_times.empty() && stats.execution_times.front() < cutoff) {
        stats.execution_times.pop_front();
        if (!stats.execution_durations.empty()) {
            stats.execution_durations.pop_front();
        }
        if (!stats.execution_results.empty()) {
            stats.execution_results.pop_front();
        }
        if (!stats.cpu_usage.empty()) {
            stats.cpu_usage.pop_front();
        }
        if (!stats.memory_usage.empty()) {
            stats.memory_usage.pop_front();
        }
    }
}

} // namespace scheduler
} // namespace themis
