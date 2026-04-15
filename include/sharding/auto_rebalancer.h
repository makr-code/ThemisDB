/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            auto_rebalancer.h                                  ║
  Version:         0.0.45                                             ║
  Last Modified:   2026-04-15 07:09:27                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     369                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • e963d4e9ba  2026-04-14  fix(concurrency): eliminate deadlocks, blocking I/O under... ║
    • 71d99c4f28  2026-04-14  fix(concurrency): eliminate deadlocks, blocking I/O under... ║
    • e80b3d54b7  2026-03-14  feat(sharding): integrate PredictiveFailureDetector into ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

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

// Forward declare AuditLogger so sharding headers don't drag in heavy auth headers
namespace themis { namespace utils { class AuditLogger; } }

// Forward-declare PredictiveFailureDetector from its own namespace so
// auto_rebalancer.h doesn't drag in the heavy predictive_detector.h
// (which transitively includes redundancy_strategy.h).
namespace themisdb { namespace sharding { class PredictiveFailureDetector; } }

namespace themis {
namespace sharding {

// Forward declarations
class ShardTopology;
class PrometheusMetrics;
class DataMigrator;

// ─────────────────────────────────────────────────────────────────────────────
// HotShardSplitPolicy
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Policy for detecting hot (overloaded) shards and proposing splits.
 *
 * Evaluates current and predicted load from ShardLoadDetector and produces
 * SplitProposal objects when a shard exceeds configured thresholds.
 *
 * Three detection modes:
 *  1. Reactive   – current CPU or storage already exceeds the threshold.
 *  2. Statistical – `ShardLoadDetector::forecastLoad()` (linear regression)
 *                   projects load > `predictive_load_threshold` within the
 *                   configured horizon (default 5 min).
 *  3. ML-based   – `PredictiveFailureDetector::predictShard()` (ONNX-backed ML)
 *                   reports failure probability ≥ `failure_probability_threshold`;
 *                   high failure probability is treated as a pre-emptive split
 *                   trigger because it indicates the shard is under excessive stress.
 *
 * Mode 3 requires calling `setPredictiveDetector()` before `evaluate()`.
 *
 * Example:
 *   HotShardSplitPolicy::Config cfg;
 *   cfg.cpu_split_threshold = 0.80;
 *   HotShardSplitPolicy policy(load_detector, cfg);
 *   policy.setPredictiveDetector(failure_detector);
 *   auto proposals = policy.evaluate();
 */
class HotShardSplitPolicy {
public:
    struct Config {
        /// CPU usage fraction that triggers a reactive split (default 80 %).
        double cpu_split_threshold = 0.80;

        /// Storage usage fraction that triggers a reactive split (default 80 %).
        double storage_split_threshold = 0.80;

        /// Composite load score (0–100) that triggers a statistical predictive split.
        double predictive_load_threshold = 80.0;

        /// How far ahead to forecast load (statistical mode).
        std::chrono::minutes forecast_horizon{5};

        /// Enable statistical (pre-emptive) splitting based on forecasted load.
        bool enable_predictive_splitting = true;

        /// Failure probability [0, 1] from PredictiveFailureDetector above which
        /// a pre-emptive split is triggered (ML-based mode).
        /// Only consulted when a PredictiveFailureDetector is attached.
        float failure_probability_threshold = 0.70f;

        /// Enable ML-based predictive splitting via PredictiveFailureDetector.
        bool enable_ml_predictive_splitting = true;
    };

    /**
     * Proposal to split a specific hot shard.
     */
    struct SplitProposal {
        /// Shard that should be split.
        std::string hot_shard_id;

        /// Human-readable reason (e.g. "CPU 85%", "predicted composite 82/100",
        /// "ML failure probability 0.72").
        std::string reason;

        /// Current composite load score (0–100).
        double current_load_percent = 0.0;

        /// Predicted composite load score (0–100); equals current_load_percent
        /// when the proposal is reactive rather than predictive.
        double predicted_load_percent = 0.0;

        /// True when the proposal is based on forecasted (not current) load.
        bool is_predictive = false;
    };

    explicit HotShardSplitPolicy(std::shared_ptr<ShardLoadDetector> detector);

    HotShardSplitPolicy(
        std::shared_ptr<ShardLoadDetector> detector,
        const Config& config
    );

    /**
     * Attach a PredictiveFailureDetector for ML-based predictive splitting.
     *
     * When set, `evaluate()` will call `detector->predictShard()` for every
     * tracked shard and emit a split proposal when the failure probability
     * meets or exceeds `Config::failure_probability_threshold`.
     *
     * The detector is held as a raw pointer (non-owning) because
     * PredictiveFailureDetector is constructed with non-ownable references
     * (RedundancyStrategy&, ShardTopology&) and typically has a longer lifetime
     * than HotShardSplitPolicy.
     *
     * @param pd Non-owning pointer; pass nullptr to disable ML-based path.
     */
    void setPredictiveDetector(themisdb::sharding::PredictiveFailureDetector* pd);

    /**
     * Evaluate current and forecasted shard load and return all split proposals.
     * @return Zero or more proposals; empty when no shards require splitting.
     */
    std::vector<SplitProposal> evaluate() const;

    const Config& getConfig() const { return config_; }

private:
    std::shared_ptr<ShardLoadDetector> detector_;
    Config config_;
    // Non-owning pointer; may be nullptr when ML integration is not configured.
    themisdb::sharding::PredictiveFailureDetector* predictive_detector_ = nullptr;
};

// ─────────────────────────────────────────────────────────────────────────────

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

    /**
     * Attach a HotShardSplitPolicy so the monitor loop also evaluates hot-shard splits.
     * @param policy Policy instance (may be nullptr to disable)
     */
    void setSplitPolicy(std::shared_ptr<HotShardSplitPolicy> policy);

    /**
     * Attach an audit logger for emitting SHARD_SPLIT / SHARD_MERGE compliance events.
     * @param audit_logger Logger instance (may be nullptr to disable audit)
     */
    void setAuditLogger(std::shared_ptr<themis::utils::AuditLogger> audit_logger);

private:
    std::shared_ptr<ShardTopology> topology_;
    std::shared_ptr<ShardLoadDetector> load_detector_;
    std::shared_ptr<PrometheusMetrics> metrics_;
    std::shared_ptr<DataMigrator> migrator_;
    Config config_;

    // Optional hot-shard split policy
    std::shared_ptr<HotShardSplitPolicy> split_policy_;

    // Optional audit logger for compliance events
    std::shared_ptr<themis::utils::AuditLogger> audit_logger_;
    
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
    std::atomic<uint64_t> split_proposals_total_{0};
    std::chrono::system_clock::time_point last_check_time_;
    
    // Monitoring loop
    void monitorLoop();

    // Hot-shard split handling
    void evaluateAndExecuteSplits();
    bool executeSplitProposal(const HotShardSplitPolicy::SplitProposal& proposal);
    
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
