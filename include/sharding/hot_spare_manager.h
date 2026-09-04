/**
 * @file hot_spare_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB Hot Spare Management System
 * 
 * Provides automatic hot spare activation and rebuild on shard failure
 * for zero-downtime recovery and high availability.
 * 
 * Features:
 * - Automatic spare pool management
 * - Fast failover (< 5 seconds)
 * - Background rebuild with throttling
 * - Progress tracking and ETA calculation
 * - Prometheus metrics integration
 * 
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "sharding/redundancy_strategy.h"
#include "sharding/shard_topology.h"
#include "sharding/shard_repair_engine.h"
#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>
#include <functional>
#include <chrono>
#include <optional>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <thread>
#include <queue>
#include <condition_variable>

namespace themis {
namespace sharding {

// Forward declarations
class ConsistentHashRing;

// ═══════════════════════════════════════════════════════════
// Hot Spare Configuration
// ═══════════════════════════════════════════════════════════

/**
 * Rebuild Priority
 * Determines bandwidth allocation for rebuild operations
 */
enum class RebuildPriority {
    LOW,        // Background rebuild, minimal impact
    MEDIUM,     // Balanced rebuild
    HIGH,       // Aggressive rebuild, faster recovery
    CRITICAL    // Maximum speed, may impact production
};

/**
 * Spare Shard State
 */
enum class SpareState {
    AVAILABLE,      // Ready for activation
    ACTIVATING,     // Being promoted to active
    ACTIVE,         // Currently serving traffic
    REBUILDING,     // Receiving data from replicas
    DEGRADED,       // Rebuild failed or incomplete
    OFFLINE         // Not available
};

/**
 * Hot Spare Configuration
 */
struct HotSpareConfig {
    bool enable = false;
    std::vector<std::string> spare_shards;  // List of spare shard IDs
    
    // Automatic rebuild settings
    bool auto_rebuild = true;
    RebuildPriority rebuild_priority = RebuildPriority::HIGH;
    uint32_t rebuild_throttle_mbps = 100;  // Bandwidth limit in MB/s
    
    // Health monitoring
    std::chrono::seconds health_check_interval{30};
    uint32_t failure_threshold = 3;  // Failures before marking shard down
    
    // Rebuild behavior
    uint32_t max_concurrent_rebuilds = 2;
    bool rebuild_in_chunks = true;
    uint32_t rebuild_chunk_size_mb = 64;
    
    // Alerting
    bool enable_alerts = true;
    std::function<void(const std::string&)> alert_callback;
    
    // Validate configuration
    bool validate() const;
};

/**
 * Spare Shard Info
 */
struct SpareShardInfo {
    std::string shard_id;
    SpareState state;
    std::chrono::system_clock::time_point state_changed;
    
    // If rebuilding
    std::optional<std::string> source_shard_id;
    uint64_t bytes_rebuilt = 0;
    uint64_t total_bytes = 0;
    std::chrono::system_clock::time_point rebuild_started;
    
    // Health
    uint32_t consecutive_failures = 0;
    std::chrono::system_clock::time_point last_health_check;
    
    // Progress calculation
    double getProgressPercentage() const {
        if (total_bytes == 0) {
          return 0.0;
        }
        return (bytes_rebuilt * 100.0) / total_bytes;
    }
    
    std::chrono::seconds getEstimatedTimeRemaining() const {
        if (bytes_rebuilt == 0 || total_bytes == 0) {
            return std::chrono::seconds(0);
        }
        
        auto elapsed = std::chrono::system_clock::now() - rebuild_started;
        double elapsed_seconds = std::chrono::duration<double>(elapsed).count();
        
        if (elapsed_seconds <= 0) {
          return std::chrono::seconds(0);
        }
        
        double bytes_per_second = bytes_rebuilt / elapsed_seconds;
        if (bytes_per_second <= 0) {
          return std::chrono::seconds(0);
        }
        
        uint64_t remaining_bytes = total_bytes - bytes_rebuilt;
        return std::chrono::seconds(static_cast<int64_t>(remaining_bytes / bytes_per_second));
    }
    
    double getRebuildThroughputMBps() const {
        if (state != SpareState::REBUILDING) {
          return 0.0;
        }
        
        auto elapsed = std::chrono::system_clock::now() - rebuild_started;
        double elapsed_seconds = std::chrono::duration<double>(elapsed).count();
        
        if (elapsed_seconds <= 0) {
          return 0.0;
        }
        
        return (bytes_rebuilt / (1024.0 * 1024.0)) / elapsed_seconds;
    }
};

/**
 * Rebuild Status
 */
struct RebuildStatus {
    bool is_rebuilding = false;
    uint32_t active_rebuilds = 0;
    std::vector<SpareShardInfo> rebuilding_spares;
    
    // Overall progress
    double overall_progress = 0.0;
    std::chrono::seconds estimated_time_remaining{0};
    double average_throughput_mbps = 0.0;
    
    // Statistics
    uint64_t total_rebuilds_completed = 0;
    uint64_t total_rebuilds_failed = 0;
    std::chrono::milliseconds average_rebuild_time{0};
};

/**
 * Failover Event
 */
struct HotSpareFailoverEvent {
    std::chrono::system_clock::time_point timestamp;
    std::string failed_shard_id;
    std::string spare_shard_id;
    std::chrono::milliseconds failover_duration;
    bool success;
    std::string error_message;
};

// ═══════════════════════════════════════════════════════════
// Hot Spare Manager
// ═══════════════════════════════════════════════════════════

/**
 * Hot Spare Manager
 * 
 * Manages hot spare pool and automatic failover/rebuild
 */
class HotSpareManager {
public:
    using WriteHandler = std::function<bool(const std::string& shard_id, 
                                            const std::string& doc_id,
                                            const std::vector<uint8_t>& data)>;
    
    using ReadHandler = std::function<std::optional<std::vector<uint8_t>>(
                                            const std::string& shard_id,
                                            const std::string& doc_id)>;
    
    using DocumentIterator = std::function<std::vector<std::string>(const std::string& shard_id)>;
    
    explicit HotSpareManager(
        const HotSpareConfig& config,
        RedundancyStrategy& strategy,
        ShardTopology& topology
    );
    
    ~HotSpareManager();
    
    // Lifecycle
    void start();
    void stop();
    bool isRunning() const;
    
    // Spare pool management
    void addSpare(const std::string& shard_id);
    void removeSpare(const std::string& shard_id);
    std::vector<std::string> getAvailableSpares() const;
    std::vector<SpareShardInfo> getAllSpares() const;
    
    // Failover operations
    bool activateSpare(
        const std::string& failed_shard_id,
        ConsistentHashRing& ring,
        ReadHandler read_handler,
        WriteHandler write_handler,
        DocumentIterator doc_iterator
    );
    
    // Manual operations
    void triggerRebuild(const std::string& spare_shard_id);
    void pauseRebuild(const std::string& spare_shard_id);
    void resumeRebuild(const std::string& spare_shard_id);
    void cancelRebuild(const std::string& spare_shard_id);
    
    // Status
    RebuildStatus getRebuildStatus() const;
    std::optional<SpareShardInfo> getSpareInfo(const std::string& shard_id) const;
    std::vector<HotSpareFailoverEvent> getFailoverHistory(size_t max_count = 100) const;
    
    // Configuration
    const HotSpareConfig& getConfig() const { return config_; }
    void updateConfig(const HotSpareConfig& config);
    
    // Statistics
    struct Stats {
        uint64_t total_failovers = 0;
        uint64_t successful_failovers = 0;
        uint64_t failed_failovers = 0;
        uint64_t total_rebuilds = 0;
        uint64_t successful_rebuilds = 0;
        uint64_t failed_rebuilds = 0;
        uint64_t spares_available = 0;
        uint64_t spares_active = 0;
        uint64_t spares_rebuilding = 0;
        std::chrono::milliseconds avg_failover_time{0};
        std::chrono::milliseconds avg_rebuild_time{0};
    };
    
    Stats getStats() const;
    
    // Prometheus metrics
    std::string exportPrometheusMetrics() const;

    /**
     * Attach a ShardRepairEngine so that after a successful failover,
     * the spare shard is automatically scheduled for full data rebuild
     * via ShardRepairEngine::triggerRepair(spare_shard_id).
     */
    void setRepairEngine(std::shared_ptr<themis::sharding::ShardRepairEngine> engine);
    
private:
    // Background threads
    void healthCheckLoop();
    void rebuildLoop();
    
    // Health monitoring
    void checkSpareHealth(SpareShardInfo& spare);
    void handleShardFailure(const std::string& shard_id);
    
    // Rebuild operations
    struct RebuildTask {
        std::string spare_shard_id;
        std::string source_shard_id;
        std::vector<std::string> documents;
        uint64_t total_bytes;
        bool paused = false;
        
        // Handlers for data transfer
        ConsistentHashRing* ring;
        ReadHandler read_handler;
        WriteHandler write_handler;
    };
    
    bool rebuildShard(RebuildTask& task);
    
    // Spare selection
    std::optional<std::string> selectBestSpare() const;
    
    // Alerting
    void sendAlert(const std::string& message);
    
    // Configuration and state
    HotSpareConfig config_;
    RedundancyStrategy& strategy_;
    ShardTopology& topology_;
    
    // Threading
    std::atomic<bool> running_{false};
    std::thread health_check_thread_;
    std::thread rebuild_thread_;
    
    // Spare tracking
    mutable std::shared_mutex spares_mutex_;
    std::map<std::string, SpareShardInfo> spares_;
    
    // Rebuild queue
    std::mutex rebuild_mutex_;
    std::condition_variable rebuild_cv_;
    std::queue<RebuildTask> rebuild_queue_;
    std::map<std::string, bool> rebuild_paused_;
    
    // Failover history
    mutable std::mutex history_mutex_;
    std::vector<HotSpareFailoverEvent> failover_history_;
    static constexpr size_t MAX_HISTORY_SIZE = 1000;
    
    // Statistics
    mutable std::mutex stats_mutex_;
    Stats stats_;

    // Optional repair engine: triggers rebuild via ShardRepairEngine after failover
    std::shared_ptr<themis::sharding::ShardRepairEngine> repair_engine_;
};

} // namespace sharding
} // namespace themis

// Backward compatibility shim: expose under themisdb::sharding
namespace themisdb {
namespace sharding {
using namespace themis::sharding;
}
}
