/*
 * ThemisDB | File: partition_detector.h | Version: 0.0.47
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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
    HEALTHY,        // All nodes reachable
    DEGRADED,       // Some nodes slow/unreachable
    PARTITIONED     // Network partition detected
};

/**
 * @brief Node connectivity status
 */
struct NodeConnectivity {
    std::string node_id;
    bool reachable;
    std::chrono::milliseconds last_rtt;  // Round-trip time
    std::chrono::steady_clock::time_point last_successful_contact;
    uint32_t consecutive_failures;
    double packet_loss_rate;
};

/**
 * @brief Partition event
 */
struct PartitionEvent {
    std::chrono::steady_clock::time_point detected_at;
    std::chrono::steady_clock::time_point healed_at;
    std::vector<std::string> partition_a;
    std::vector<std::string> partition_b;
    bool is_healed;
    std::string description;
};

/**
 * @brief Configuration for partition detection
 */
struct PartitionDetectorConfig {
    std::chrono::milliseconds health_check_interval{1000};
    std::chrono::milliseconds heartbeat_timeout{500};
    uint32_t max_consecutive_failures{3};
    double packet_loss_threshold{0.5};  // 50% packet loss
    bool enable_split_brain_detection{true};
    bool enable_auto_healing{true};
};

/**
 * @brief Detects network partitions in distributed cluster
 * 
 * Monitors node connectivity and detects when the cluster
 * splits into separate partitions that cannot communicate.
 */
class PartitionDetector {
public:
    using HealthCheckCallback = std::function<bool(const std::string& node_id)>;
    using PartitionCallback = std::function<void(const PartitionEvent& event)>;
    
    explicit PartitionDetector(const PartitionDetectorConfig& config);
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
     */
    void addNode(const std::string& node_id);
    
    /**
     * @brief Remove node from monitoring
     */
    void removeNode(const std::string& node_id);
    
    /**
     * @brief Record heartbeat from node
     */
    void recordHeartbeat(const std::string& node_id, 
                        std::chrono::milliseconds rtt);
    
    /**
     * @brief Record failed contact with node
     */
    void recordFailure(const std::string& node_id);
    
    /**
     * @brief Get current network health
     */
    NetworkHealth getNetworkHealth() const;
    
    /**
     * @brief Get node connectivity status
     */
    std::vector<NodeConnectivity> getNodeConnectivity() const;
    
    /**
     * @brief Check if split-brain detected
     */
    bool isSplitBrainDetected() const;
    
    /**
     * @brief Get partition history
     */
    std::vector<PartitionEvent> getPartitionHistory() const;
    
    /**
     * @brief Set health check callback
     */
    void setHealthCheckCallback(HealthCheckCallback callback);
    
    /**
     * @brief Set partition event callback
     */
    void setPartitionCallback(PartitionCallback callback);
    
    /**
     * @brief Get statistics
     */
    struct Statistics {
        std::atomic<uint64_t> total_health_checks{0};
        std::atomic<uint64_t> failed_health_checks{0};
        std::atomic<uint64_t> partitions_detected{0};
        std::atomic<uint64_t> partitions_healed{0};
        std::chrono::steady_clock::time_point last_partition_time;
    };
    
    const Statistics& getStatistics() const { return stats_; }

private:
    PartitionDetectorConfig config_;
    
    mutable std::mutex nodes_mutex_;
    std::map<std::string, NodeConnectivity> nodes_;
    
    mutable std::mutex events_mutex_;
    std::vector<PartitionEvent> partition_history_;
    
    std::atomic<bool> running_{false};
    std::atomic<NetworkHealth> current_health_{NetworkHealth::HEALTHY};
    std::atomic<bool> split_brain_detected_{false};
    
    Statistics stats_;
    
    HealthCheckCallback health_check_callback_;
    PartitionCallback partition_callback_;
    
    /**
     * @brief Health check loop
     */
    void healthCheckLoop();
    std::thread health_check_thread_;
    
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
