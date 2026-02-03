#pragma once

#include "sharding/rebalance_operation.h"
#include <string>
#include <memory>
#include <map>
#include <mutex>
#include <optional>
#include <vector>
#include <chrono>

namespace themis {
namespace sharding {

// Forward declarations
class DataMovementCoordinator;
class ShardTopology;
class MetadataShard;

/**
 * RebalanceExecutor
 * 
 * Manages the execution of rebalance operations with support for:
 * - Operation validation and safety checks
 * - Lock acquisition on affected shards
 * - Data movement coordination
 * - Topology updates
 * - Pause/resume capability
 * - Rollback on failure
 */
class RebalanceExecutor {
public:
    struct Config {
        std::chrono::milliseconds operation_timeout{600000};  // 10 minutes
        size_t max_retries{3};
        bool require_approval{true};
    };
    
    explicit RebalanceExecutor(const Config& config);
    
    /**
     * Execute rebalance operation
     * @param op Operation to execute
     * @return true on success, false on failure
     */
    bool execute(RebalanceOperation& op);
    
    /**
     * Pause ongoing rebalance
     * @param operation_id Operation identifier
     * @return true if paused successfully
     */
    bool pause(const std::string& operation_id);
    
    /**
     * Resume paused rebalance
     * @param operation_id Operation identifier
     * @return true if resumed successfully
     */
    bool resume(const std::string& operation_id);
    
    /**
     * Rollback failed rebalance
     * @param operation_id Operation identifier
     * @return true if rolled back successfully
     */
    bool rollback(const std::string& operation_id);
    
    /**
     * Get operation by ID
     * @param operation_id Operation identifier
     * @return Operation if found
     */
    std::optional<RebalanceOperation*> getOperation(const std::string& operation_id);
    
    /**
     * List all operations with optional state filter
     * @param filter_state Optional state to filter by
     * @return Vector of operation pointers
     */
    std::vector<RebalanceOperation*> listOperations(
        std::optional<RebalanceState> filter_state = std::nullopt
    );
    
    /**
     * Set coordinator for data movement
     */
    void setDataMovementCoordinator(std::shared_ptr<DataMovementCoordinator> coordinator);
    
    /**
     * Set shard topology manager
     */
    void setShardTopology(std::shared_ptr<ShardTopology> topology);
    
    /**
     * Set metadata shard
     */
    void setMetadataShard(std::shared_ptr<MetadataShard> metadata);
    
private:
    Config config_;
    std::map<std::string, RebalanceOperation*> operations_;
    std::shared_ptr<DataMovementCoordinator> movement_coordinator_;
    std::shared_ptr<ShardTopology> topology_;
    std::shared_ptr<MetadataShard> metadata_;
    mutable std::mutex mutex_;
    
    // Paused operations tracking
    std::map<std::string, bool> paused_operations_;
    
    // Internal execution phases
    bool validateOperation(const RebalanceOperation& op);
    bool acquireLocks(const RebalanceOperation& op);
    bool executeMovement(RebalanceOperation& op);
    bool updateTopology(const RebalanceOperation& op);
    bool releaseLocks(const RebalanceOperation& op);
};

} // namespace sharding
} // namespace themis
