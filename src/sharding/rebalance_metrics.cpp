#include "sharding/rebalance_metrics.h"
#include "utils/logger.h"

namespace themis {
namespace sharding {

nlohmann::json RebalanceStats::toJson() const {
    return nlohmann::json{
        {"total_operations", total_operations},
        {"in_progress", in_progress},
        {"completed", completed},
        {"failed", failed},
        {"rolled_back", rolled_back},
        {"total_bytes_moved", total_bytes_moved},
        {"total_duration_ms", total_duration.count()},
        {"avg_bytes_per_operation", avg_bytes_per_operation},
        {"avg_duration_ms", avg_duration_ms},
        {"success_rate", success_rate}
    };
}

nlohmann::json OperationMetrics::toJson() const {
    return nlohmann::json{
        {"operation_id", operation_id},
        {"start_time", std::chrono::duration_cast<std::chrono::milliseconds>(
            start_time.time_since_epoch()).count()},
        {"end_time", std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time.time_since_epoch()).count()},
        {"bytes_moved", bytes_moved},
        {"records_moved", records_moved},
        {"progress_percent", progress_percent},
        {"state", state},
        {"error_message", error_message}
    };
}

void RebalanceMetrics::recordOperationStart(const std::string& operation_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    OperationMetrics metrics;
    metrics.operation_id = operation_id;
    metrics.start_time = std::chrono::system_clock::now();
    metrics.state = "IN_PROGRESS";
    
    operation_metrics_[operation_id] = metrics;
    
    total_operations_++;
    in_progress_++;
    
    THEMIS_INFO("Rebalance metrics: Operation {} started", operation_id);
}

void RebalanceMetrics::recordOperationProgress(
    const std::string& operation_id,
    double progress,
    uint64_t bytes_moved,
    uint64_t records_moved) {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = operation_metrics_.find(operation_id);
    if (it == operation_metrics_.end()) {
        THEMIS_WARN("Operation metrics not found: {}", operation_id);
        return;
    }
    
    it->second.progress_percent = progress;
    it->second.bytes_moved = bytes_moved;
    it->second.records_moved = records_moved;
    
    THEMIS_DEBUG("Rebalance metrics: Operation {} progress: {:.1f}%, {} bytes, {} records",
                 operation_id, progress, bytes_moved, records_moved);
}

void RebalanceMetrics::recordOperationComplete(
    const std::string& operation_id,
    uint64_t total_bytes,
    uint64_t total_records) {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = operation_metrics_.find(operation_id);
    if (it == operation_metrics_.end()) {
        THEMIS_WARN("Operation metrics not found: {}", operation_id);
        return;
    }
    
    it->second.end_time = std::chrono::system_clock::now();
    it->second.bytes_moved = total_bytes;
    it->second.records_moved = total_records;
    it->second.progress_percent = 100.0;
    it->second.state = "COMPLETED";
    
    in_progress_--;
    completed_++;
    total_bytes_moved_ += total_bytes;
    
    THEMIS_INFO("Rebalance metrics: Operation {} completed - {} bytes, {} records",
                operation_id, total_bytes, total_records);
}

void RebalanceMetrics::recordOperationFailed(
    const std::string& operation_id,
    const std::string& reason) {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = operation_metrics_.find(operation_id);
    if (it == operation_metrics_.end()) {
        THEMIS_WARN("Operation metrics not found: {}", operation_id);
        return;
    }
    
    it->second.end_time = std::chrono::system_clock::now();
    it->second.state = "FAILED";
    it->second.error_message = reason;
    
    in_progress_--;
    failed_++;
    
    THEMIS_ERROR("Rebalance metrics: Operation {} failed - {}", operation_id, reason);
}

void RebalanceMetrics::recordOperationRolledBack(const std::string& operation_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = operation_metrics_.find(operation_id);
    if (it == operation_metrics_.end()) {
        THEMIS_WARN("Operation metrics not found: {}", operation_id);
        return;
    }
    
    it->second.state = "ROLLED_BACK";
    
    // Adjust counters
    if (in_progress_ > 0) {
        in_progress_--;
    }
    rolled_back_++;
    
    THEMIS_INFO("Rebalance metrics: Operation {} rolled back", operation_id);
}

RebalanceStats RebalanceMetrics::getStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return calculateStats();
}

std::optional<OperationMetrics> RebalanceMetrics::getOperationMetrics(
    const std::string& operation_id) const {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = operation_metrics_.find(operation_id);
    if (it != operation_metrics_.end()) {
        return it->second;
    }
    
    return std::nullopt;
}

std::vector<OperationMetrics> RebalanceMetrics::getAllOperationMetrics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<OperationMetrics> result;
    result.reserve(operation_metrics_.size());
    
    for (const auto& [id, metrics] : operation_metrics_) {
        result.push_back(metrics);
    }
    
    return result;
}

nlohmann::json RebalanceMetrics::getStatsJson() const {
    return getStats().toJson();
}

void RebalanceMetrics::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    operation_metrics_.clear();
    total_operations_ = 0;
    in_progress_ = 0;
    completed_ = 0;
    failed_ = 0;
    rolled_back_ = 0;
    total_bytes_moved_ = 0;
    
    THEMIS_INFO("Rebalance metrics reset");
}

RebalanceStats RebalanceMetrics::calculateStats() const {
    RebalanceStats stats;
    
    stats.total_operations = total_operations_.load();
    stats.in_progress = in_progress_.load();
    stats.completed = completed_.load();
    stats.failed = failed_.load();
    stats.rolled_back = rolled_back_.load();
    stats.total_bytes_moved = total_bytes_moved_.load();
    
    // Calculate total duration
    std::chrono::milliseconds total_duration_ms{0};
    
    for (const auto& [id, metrics] : operation_metrics_) {
        if (metrics.state == "COMPLETED" || metrics.state == "FAILED") {
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                metrics.end_time - metrics.start_time
            );
            total_duration_ms += duration;
        }
    }
    
    stats.total_duration = total_duration_ms;
    
    // Calculate averages
    size_t completed_count = stats.completed + stats.failed;
    if (completed_count > 0) {
        stats.avg_bytes_per_operation = static_cast<double>(stats.total_bytes_moved) / completed_count;
        stats.avg_duration_ms = static_cast<double>(stats.total_duration.count()) / completed_count;
    }
    
    // Calculate success rate
    if (stats.total_operations > 0) {
        stats.success_rate = static_cast<double>(stats.completed) / stats.total_operations * 100.0;
    }
    
    return stats;
}

} // namespace sharding
} // namespace themis
