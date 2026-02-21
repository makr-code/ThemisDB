/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            auto_rebalancer.h                                  ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-02-21 13:48:23                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     224                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#ifndef THEMIS_AUTO_REBALANCER_H
#define THEMIS_AUTO_REBALANCER_H

#include "sharding/shard_load_detector.h"
#include "sharding/rebalance_operation.h"
#include <string>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <vector>
#include <chrono>
#include <map>
#include <nlohmann/json.hpp>

namespace themis {
namespace sharding {

// Forward declarations
class ShardTopology;
class PrometheusMetrics;
class DataMigrator;

/**
 * Automatic Rebalancing Coordinator
 * 
 * Monitors cluster load and automatically triggers rebalancing operations
 * when imbalances are detected.
 * 
 * Features:
 * - Periodic load monitoring
 * - Automatic imbalance detection
 * - Rebalance operation scheduling
 * - Concurrent operation management
 * - Health checks and rollback
 * - Progress tracking
 * 
 * Example:
 *   auto rebalancer = std::make_unique<AutoRebalancer>(
 *       topology, load_detector, metrics, migrator
 *   );
 *   rebalancer->start();
 *   
 *   // Automatic rebalancing runs in background
 *   
 *   rebalancer->stop();
 */
class AutoRebalancer {
public:
    struct Config {
        // Monitoring interval
        std::chrono::milliseconds check_interval{std::chrono::minutes(5)};
        
        // Maximum concurrent rebalance operations
        size_t max_concurrent_operations = 2;
        
        // Operator credentials for signing operations
        std::string operator_cert_path;
        std::string operator_key_path;
        std::string ca_cert_path;
        
        // Automatic triggering
        bool auto_trigger_enabled = true;
        bool require_manual_approval = false;
        
        // Safety limits
        size_t max_operations_per_day = 10;
        double max_data_movement_percent = 20.0;  // Max 20% of cluster data per operation
        
        // Rebalance operation config
        uint32_t batch_size = 1000;
        bool verify_data = true;
        bool enable_rollback = true;
    };
    
    struct OperationStatus {
        std::string operation_id;
        RebalanceState state;
        RebalanceProgress progress;
        std::chrono::system_clock::time_point start_time;
        std::chrono::system_clock::time_point end_time;
        std::string error_message;
    };
    
    explicit AutoRebalancer(
        std::shared_ptr<ShardTopology> topology,
        std::shared_ptr<ShardLoadDetector> load_detector,
        std::shared_ptr<PrometheusMetrics> metrics,
        std::shared_ptr<DataMigrator> migrator
    );
    
    AutoRebalancer(
        std::shared_ptr<ShardTopology> topology,
        std::shared_ptr<ShardLoadDetector> load_detector,
        std::shared_ptr<PrometheusMetrics> metrics,
        std::shared_ptr<DataMigrator> migrator,
        const Config& config
    );
    
    ~AutoRebalancer();
    
    /**
     * Start automatic rebalancing monitoring
     */
    void start();
    
    /**
     * Stop automatic rebalancing
     */
    void stop();
    
    /**
     * Check if rebalancer is running
     */
    bool isRunning() const { return running_.load(); }
    
    /**
     * Manually trigger rebalance check
     * @return true if rebalance was triggered
     */
    bool triggerCheck();
    
    /**
     * Approve pending rebalance operation
     * (Only needed if require_manual_approval=true)
     * @param operation_id Operation to approve
     * @return true if approved successfully
     */
    bool approveOperation(const std::string& operation_id);
    
    /**
     * Cancel active rebalance operation
     * @param operation_id Operation to cancel
     * @return true if cancelled successfully
     */
    bool cancelOperation(const std::string& operation_id);
    
    /**
     * Get status of all operations
     * @return Vector of operation statuses
     */
    std::vector<OperationStatus> getOperationStatuses() const;
    
    /**
     * Get statistics
     * @return JSON statistics
     */
    nlohmann::json getStatistics() const;
    
private:
    std::shared_ptr<ShardTopology> topology_;
    std::shared_ptr<ShardLoadDetector> load_detector_;
    std::shared_ptr<PrometheusMetrics> metrics_;
    std::shared_ptr<DataMigrator> migrator_;
    Config config_;
    
    // Threading
    std::atomic<bool> running_{false};
    std::thread monitor_thread_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    
    // Active operations
    std::map<std::string, std::unique_ptr<RebalanceOperation>> active_operations_;
    std::vector<OperationStatus> operation_history_;
    
    // Pending approvals
    std::map<std::string, LoadImbalanceResult::RebalanceRecommendation> pending_approvals_;
    
    // Statistics
    std::atomic<uint64_t> total_checks_{0};
    std::atomic<uint64_t> triggered_operations_{0};
    std::atomic<uint64_t> completed_operations_{0};
    std::atomic<uint64_t> failed_operations_{0};
    std::chrono::system_clock::time_point last_check_time_;
    
    // Monitoring loop
    void monitorLoop();
    
    // Rebalance execution
    bool executeRebalance(const LoadImbalanceResult::RebalanceRecommendation& recommendation);
    std::string generateOperationId() const;
    std::string signOperation(const std::string& operation_id) const;
    
    // Safety checks
    bool canTriggerRebalance() const;
    bool isWithinSafetyLimits(const LoadImbalanceResult& imbalance) const;
    
    // Operation management
    void cleanupCompletedOperations();
    void updateOperationStatus(const std::string& operation_id, RebalanceState state);
};

} // namespace sharding
} // namespace themis

#endif // THEMIS_AUTO_REBALANCER_H
