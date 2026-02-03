#include "sharding/rebalance_executor.h"
#include "sharding/data_movement_coordinator.h"
#include "sharding/shard_topology.h"
#include "sharding/metadata_shard.h"
#include "utils/logger.h"
#include "utils/tracing.h"
#include <algorithm>
#include <thread>

namespace themis {
namespace sharding {

RebalanceExecutor::RebalanceExecutor(const Config& config)
    : config_(config) {
    THEMIS_INFO("RebalanceExecutor initialized with timeout={}ms, max_retries={}",
                config_.operation_timeout.count(), config_.max_retries);
}

bool RebalanceExecutor::execute(RebalanceOperation& op) {
    auto span = Tracer::startSpan("RebalanceExecutor.execute");
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if already paused
    if (paused_operations_.count("temp_id") && paused_operations_["temp_id"]) {
        THEMIS_WARN("Cannot execute operation - system is paused");
        return false;
    }
    
    // Phase 1: Validate operation
    if (!validateOperation(op)) {
        THEMIS_ERROR("Operation validation failed");
        span.recordError("Validation failed");
        return false;
    }
    
    // Phase 2: Acquire locks
    if (!acquireLocks(op)) {
        THEMIS_ERROR("Failed to acquire locks for operation");
        span.recordError("Lock acquisition failed");
        op.fail("Failed to acquire shard locks");
        return false;
    }
    
    // Phase 3: Execute data movement
    bool movement_success = false;
    try {
        movement_success = executeMovement(op);
    } catch (const std::exception& e) {
        THEMIS_ERROR("Exception during data movement: {}", e.what());
        span.recordError(e.what());
        op.fail(std::string("Data movement exception: ") + e.what());
        releaseLocks(op);
        return false;
    }
    
    if (!movement_success) {
        THEMIS_ERROR("Data movement failed");
        span.recordError("Data movement failed");
        releaseLocks(op);
        return false;
    }
    
    // Phase 4: Update topology
    if (!updateTopology(op)) {
        THEMIS_ERROR("Failed to update topology");
        span.recordError("Topology update failed");
        op.fail("Failed to update shard topology");
        releaseLocks(op);
        return false;
    }
    
    // Phase 5: Release locks
    if (!releaseLocks(op)) {
        THEMIS_WARN("Failed to cleanly release locks");
        // Continue anyway - operation succeeded
    }
    
    // Mark operation as complete
    op.complete();
    
    THEMIS_INFO("Rebalance operation executed successfully");
    return true;
}

bool RebalanceExecutor::pause(const std::string& operation_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = operations_.find(operation_id);
    if (it == operations_.end()) {
        THEMIS_WARN("Operation not found: {}", operation_id);
        return false;
    }
    
    paused_operations_[operation_id] = true;
    THEMIS_INFO("Operation paused: {}", operation_id);
    return true;
}

bool RebalanceExecutor::resume(const std::string& operation_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = paused_operations_.find(operation_id);
    if (it == paused_operations_.end() || !it->second) {
        THEMIS_WARN("Operation not paused: {}", operation_id);
        return false;
    }
    
    paused_operations_[operation_id] = false;
    THEMIS_INFO("Operation resumed: {}", operation_id);
    return true;
}

bool RebalanceExecutor::rollback(const std::string& operation_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = operations_.find(operation_id);
    if (it == operations_.end()) {
        THEMIS_WARN("Operation not found for rollback: {}", operation_id);
        return false;
    }
    
    RebalanceOperation* op = it->second;
    
    // Trigger rollback on the operation
    bool rolled_back = op->rollback();
    
    if (rolled_back) {
        THEMIS_INFO("Operation rolled back: {}", operation_id);
        releaseLocks(*op);
    } else {
        THEMIS_ERROR("Failed to rollback operation: {}", operation_id);
    }
    
    return rolled_back;
}

std::optional<RebalanceOperation*> RebalanceExecutor::getOperation(const std::string& operation_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = operations_.find(operation_id);
    if (it != operations_.end()) {
        return it->second;
    }
    
    return std::nullopt;
}

std::vector<RebalanceOperation*> RebalanceExecutor::listOperations(
    std::optional<RebalanceState> filter_state) {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<RebalanceOperation*> result;
    
    for (const auto& [id, op] : operations_) {
        if (!filter_state.has_value() || op->getState() == filter_state.value()) {
            result.push_back(op);
        }
    }
    
    return result;
}

void RebalanceExecutor::setDataMovementCoordinator(std::shared_ptr<DataMovementCoordinator> coordinator) {
    movement_coordinator_ = coordinator;
}

void RebalanceExecutor::setShardTopology(std::shared_ptr<ShardTopology> topology) {
    topology_ = topology;
}

void RebalanceExecutor::setMetadataShard(std::shared_ptr<MetadataShard> metadata) {
    metadata_ = metadata;
}

// Private helper methods

bool RebalanceExecutor::validateOperation(const RebalanceOperation& op) {
    // Check that operation is in correct state
    if (op.getState() != RebalanceState::IN_PROGRESS) {
        THEMIS_ERROR("Operation not in IN_PROGRESS state");
        return false;
    }
    
    // Validate that shards exist in topology
    if (topology_) {
        // Source shard validation would go here
        // For now, assume valid
    }
    
    return true;
}

bool RebalanceExecutor::acquireLocks(const RebalanceOperation& op) {
    // In a real implementation, this would:
    // 1. Contact source shard and acquire write lock
    // 2. Contact target shard and acquire write lock
    // 3. Use distributed lock manager (e.g., etcd)
    
    THEMIS_DEBUG("Acquiring locks for rebalance operation");
    
    // Placeholder: assume locks acquired
    return true;
}

bool RebalanceExecutor::executeMovement(RebalanceOperation& op) {
    if (!movement_coordinator_) {
        THEMIS_ERROR("No data movement coordinator configured");
        return false;
    }
    
    THEMIS_INFO("Starting data movement");
    
    // Get operation config
    auto progress = op.getProgress();
    
    // Start streaming via coordinator
    // In real implementation, we'd extract token ranges from operation config
    std::vector<uint64_t> token_ranges = {0, 1000000};  // Placeholder
    
    std::string stream_id = movement_coordinator_->startStreaming(
        "source_shard",  // Would come from op config
        "target_shard",  // Would come from op config
        token_ranges,
        [&op](const StreamState& state) {
            // Progress callback
            op.updateProgress(
                state.batches_acknowledged * 1000,  // Estimate records
                state.bytes_transferred
            );
        }
    );
    
    if (stream_id.empty()) {
        THEMIS_ERROR("Failed to start data streaming");
        return false;
    }
    
    // Wait for streaming to complete
    // In real implementation, this would be async with timeout
    auto timeout = std::chrono::steady_clock::now() + config_.operation_timeout;
    
    while (std::chrono::steady_clock::now() < timeout) {
        auto stream_state = movement_coordinator_->getStreamState(stream_id);
        
        if (!stream_state.has_value()) {
            THEMIS_ERROR("Stream state lost for {}", stream_id);
            return false;
        }
        
        if (stream_state->is_complete) {
            THEMIS_INFO("Data streaming completed successfully");
            break;
        }
        
        if (stream_state->has_error) {
            THEMIS_ERROR("Data streaming failed: {}", stream_state->error_message);
            return false;
        }
        
        // Sleep briefly before checking again
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Verify data integrity
    bool verified = movement_coordinator_->verifyDataIntegrity(
        "source_shard",
        "target_shard",
        token_ranges
    );
    
    if (!verified) {
        THEMIS_ERROR("Data integrity verification failed");
        return false;
    }
    
    THEMIS_INFO("Data movement completed and verified");
    return true;
}

bool RebalanceExecutor::updateTopology(const RebalanceOperation& op) {
    if (!topology_) {
        THEMIS_WARN("No topology manager configured");
        return true;  // Non-fatal
    }
    
    THEMIS_INFO("Updating shard topology");
    
    // In real implementation, this would:
    // 1. Update token range assignments
    // 2. Mark target shard as owning the migrated range
    // 3. Update routing tables
    // 4. Notify all nodes of topology change
    
    // Placeholder: assume success
    return true;
}

bool RebalanceExecutor::releaseLocks(const RebalanceOperation& op) {
    THEMIS_DEBUG("Releasing locks for rebalance operation");
    
    // In real implementation, this would:
    // 1. Release write lock on source shard
    // 2. Release write lock on target shard
    // 3. Update distributed lock manager
    
    // Placeholder: assume success
    return true;
}

} // namespace sharding
} // namespace themis
