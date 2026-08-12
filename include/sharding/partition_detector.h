/**
 * @file partition_detector.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2025 ThemisDB
// Licensed under MIT License

#pragma once

#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <functional>
#include <atomic>
#include <mutex>
#include <memory>
#include <thread>

namespace themisdb {
namespace sharding {

/**
 * @brief Network health status
 */
enum class NetworkHealth {
    HEALTHY,        ///< All monitored nodes are reachable and stable.
    DEGRADED,       ///< Some nodes are slow or intermittently failing.
    PARTITIONED     ///< Network partition condition detected.
};

/**
 * @brief Node connectivity status
 */
struct NodeConnectivity {
    std::string node_id;                                  ///< Node identifier.
    bool reachable;                                       ///< Reachability flag based on recent checks.
    std::chrono::milliseconds last_rtt;                  ///< Last observed round-trip time.
    std::chrono::steady_clock::time_point last_successful_contact; ///< Timestamp of last successful contact.
    uint32_t consecutive_failures;                       ///< Consecutive failed health checks.
    double packet_loss_rate;                             ///< Smoothed packet-loss estimate in range [0,1].
};

/**
 * @brief Partition event
 */
struct PartitionEvent {
    std::chrono::steady_clock::time_point detected_at;   ///< Time partition was first detected.
    std::chrono::steady_clock::time_point healed_at;     ///< Time partition was marked healed.
    std::vector<std::string> partition_a;                ///< First node set in partition split.
    std::vector<std::string> partition_b;                ///< Second node set in partition split.
    bool is_healed;                                      ///< True once healing has been observed.
    std::string description;                             ///< Human-readable event description.
};

/**
 * @brief Configuration for partition detection
 */
struct PartitionDetectorConfig {
    std::chrono::milliseconds health_check_interval{1000}; ///< Interval between active health-check rounds.
    std::chrono::milliseconds heartbeat_timeout{500};      ///< Timeout threshold for stale contacts.
    uint32_t max_consecutive_failures{3};                  ///< Failure count before node becomes unreachable.
    double packet_loss_threshold{0.5};                     ///< Packet-loss threshold for degraded/partition logic.
    bool enable_split_brain_detection{true};               ///< Enables split-brain ratio detection heuristic.
    bool enable_auto_healing{true};                        ///< Enables automatic healing detection.
};

/**
 * @brief Detects network partitions in distributed cluster
 * 
 * Monitors node connectivity and detects when the cluster
 * splits into separate partitions that cannot communicate.
 */
class PartitionDetector {
public:
    /** @brief Callback signature used for active node health checks. */
    using HealthCheckCallback = std::function<bool(const std::string& node_id)>;
    /** @brief Callback signature for partition detect/heal events. */
    using PartitionCallback = std::function<void(const PartitionEvent& event)>;

    /**
     * @brief Construct partition detector with runtime settings.
     * @param config Monitoring cadence and threshold configuration.
     */
    explicit PartitionDetector(const PartitionDetectorConfig& config);

    /** @brief Stop monitoring thread during destruction. */
    ~PartitionDetector();
    
    /**
     * @brief Start monitoring
     */
    void start();
    
    /**
     * @brief Stop monitoring
     */
    void stop();
    
    /**
     * @brief Add node to monitor
        * @param node_id Node identifier to begin tracking.
     */
    void addNode(const std::string& node_id);
    
    /**
     * @brief Remove node from monitoring
        * @param node_id Node identifier to remove.
     */
    void removeNode(const std::string& node_id);
    
    /**
     * @brief Record heartbeat from node
        * @param node_id Node identifier.
        * @param rtt Observed round-trip time for successful contact.
     */
    void recordHeartbeat(const std::string& node_id, 
                        std::chrono::milliseconds rtt);
    
    /**
     * @brief Record failed contact with node
        * @param node_id Node identifier.
     */
    void recordFailure(const std::string& node_id);
    
    /**
     * @brief Get current network health
        * @return Current aggregated network health state.
     */
    NetworkHealth getNetworkHealth() const;
    
    /**
     * @brief Get node connectivity status
        * @return Snapshot of per-node connectivity state.
     */
    std::vector<NodeConnectivity> getNodeConnectivity() const;
    
    /**
     * @brief Check if split-brain detected
        * @return True when split-brain condition is currently flagged.
     */
    bool isSplitBrainDetected() const;
    
    /**
     * @brief Get partition history
        * @return Historical partition detect/heal events.
     */
    std::vector<PartitionEvent> getPartitionHistory() const;
    
    /**
     * @brief Set health check callback
        * @param callback Callback performing node liveness checks.
     */
    void setHealthCheckCallback(HealthCheckCallback callback);
    
    /**
     * @brief Set partition event callback
        * @param callback Callback invoked on detect/heal partition events.
     */
    void setPartitionCallback(PartitionCallback callback);
    
    /**
     * @brief Get statistics
     */
    struct Statistics {
        std::atomic<uint64_t> total_health_checks{0};   ///< Total monitoring loop iterations.
        std::atomic<uint64_t> failed_health_checks{0};  ///< Count of failed node checks.
        std::atomic<uint64_t> partitions_detected{0};   ///< Number of detected partition events.
        std::atomic<uint64_t> partitions_healed{0};     ///< Number of healed partition events.
        std::chrono::steady_clock::time_point last_partition_time; ///< Timestamp of latest partition detection.
    };

    /** @brief Return live detector statistics counters. */
    const Statistics& getStatistics() const { return stats_; }

private:
    PartitionDetectorConfig config_;  ///< Monitoring configuration.
    
    mutable std::mutex nodes_mutex_;
    std::map<std::string, NodeConnectivity> nodes_;  ///< Connectivity state per monitored node.
    
    mutable std::mutex events_mutex_;
    std::vector<PartitionEvent> partition_history_;  ///< Partition event history.
    
    std::atomic<bool> running_{false};                                 ///< Worker thread lifecycle flag.
    std::atomic<NetworkHealth> current_health_{NetworkHealth::HEALTHY}; ///< Aggregated network health.
    std::atomic<bool> split_brain_detected_{false};                     ///< Split-brain condition flag.
    
    Statistics stats_;  ///< Runtime counters and timestamps.
    
    HealthCheckCallback health_check_callback_;  ///< Active health-check callback.
    PartitionCallback partition_callback_;       ///< Partition detect/heal callback.
    
    /**
     * @brief Health check loop
     */
    void healthCheckLoop();
    std::thread health_check_thread_;  ///< Background monitoring worker.
    
    /**
     * @brief Detect partition from node connectivity
     */
    void detectPartition();
    
    /**
     * @brief Check for partition healing
     */
    void checkPartitionHealing();
    
    /**
     * @brief Update network health status
     */
    void updateNetworkHealth();
};

}  // namespace sharding
}  // namespace themisdb
