#pragma once

#include <string>
#include <map>
#include <mutex>
#include <atomic>
#include <chrono>
#include <nlohmann/json.hpp>

namespace themis {
namespace sharding {

// Forward declaration
class RebalanceOperation;

/**
 * Statistics for rebalance operations
 */
struct RebalanceStats {
    size_t total_operations{0};
    size_t in_progress{0};
    size_t completed{0};
    size_t failed{0};
    size_t rolled_back{0};
    uint64_t total_bytes_moved{0};
    std::chrono::milliseconds total_duration{0};
    
    // Average metrics
    double avg_bytes_per_operation{0.0};
    double avg_duration_ms{0.0};
    double success_rate{0.0};
    
    nlohmann::json toJson() const;
};

/**
 * Per-operation metrics
 */
struct OperationMetrics {
    std::string operation_id;
    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point end_time;
    uint64_t bytes_moved{0};
    uint64_t records_moved{0};
    double progress_percent{0.0};
    std::string state;
    std::string error_message;
    
    nlohmann::json toJson() const;
};

/**
 * RebalanceMetrics
 * 
 * Tracks and reports metrics for rebalance operations.
 * Provides:
 * - Operation lifecycle tracking
 * - Progress monitoring
 * - Success/failure statistics
 * - Performance metrics
 */
class RebalanceMetrics {
public:
    RebalanceMetrics() = default;
    
    /**
     * Record operation start
     */
    void recordOperationStart(const std::string& operation_id);
    
    /**
     * Record operation progress
     */
    void recordOperationProgress(
        const std::string& operation_id,
        double progress,
        uint64_t bytes_moved,
        uint64_t records_moved
    );
    
    /**
     * Record operation completion
     */
    void recordOperationComplete(
        const std::string& operation_id,
        uint64_t total_bytes,
        uint64_t total_records
    );
    
    /**
     * Record operation failure
     */
    void recordOperationFailed(
        const std::string& operation_id,
        const std::string& reason
    );
    
    /**
     * Record operation rollback
     */
    void recordOperationRolledBack(const std::string& operation_id);
    
    /**
     * Get overall statistics
     */
    RebalanceStats getStats() const;
    
    /**
     * Get metrics for specific operation
     */
    std::optional<OperationMetrics> getOperationMetrics(const std::string& operation_id) const;
    
    /**
     * Get all operation metrics
     */
    std::vector<OperationMetrics> getAllOperationMetrics() const;
    
    /**
     * Get statistics as JSON
     */
    nlohmann::json getStatsJson() const;
    
    /**
     * Reset all metrics (for testing)
     */
    void reset();
    
private:
    mutable std::mutex mutex_;
    std::map<std::string, OperationMetrics> operation_metrics_;
    
    // Aggregate counters
    std::atomic<size_t> total_operations_{0};
    std::atomic<size_t> in_progress_{0};
    std::atomic<size_t> completed_{0};
    std::atomic<size_t> failed_{0};
    std::atomic<size_t> rolled_back_{0};
    std::atomic<uint64_t> total_bytes_moved_{0};
    
    // Helper to calculate derived stats
    RebalanceStats calculateStats() const;
};

} // namespace sharding
} // namespace themis
