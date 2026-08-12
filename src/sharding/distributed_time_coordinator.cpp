/**
 * @file distributed_time_coordinator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "sharding/distributed_time_coordinator.h"
#include <spdlog/spdlog.h>
#include <chrono>

namespace themisdb::sharding {

/** @brief Construct distributed time coordinator with explicit config. */
DistributedTimeCoordinator::DistributedTimeCoordinator(
    std::shared_ptr<ConsensusModule> consensus,
    const Config& config
)
    : consensus_(consensus), config_(config) {
    spdlog::info("DistributedTimeCoordinator initialized (log_index_only={}, base_uncertainty_ns={})",
                config_.use_log_index_only, config_.base_uncertainty_ns);
}

/** @brief Construct distributed time coordinator with default config. */
DistributedTimeCoordinator::DistributedTimeCoordinator(
    std::shared_ptr<ConsensusModule> consensus
) : DistributedTimeCoordinator(consensus, Config{})
{
}

/** @brief Return current logical time interval derived from consensus log index. */
DistributedTimeCoordinator::TimeInterval DistributedTimeCoordinator::now() const {
    uint64_t log_index = getCurrentLogIndex();
    
    TimeInterval interval;
    interval.logical_timestamp = static_cast<int64_t>(log_index);
    interval.uncertainty_ns = config_.base_uncertainty_ns;
    interval.system_time_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    
    return interval;
}

/** @brief Return snapshot timestamp mapped to current committed log index. */
int64_t DistributedTimeCoordinator::getSnapshotTimestamp() const {
    // For snapshots, use current commit index
    // Ensures all reads see consistent data as of this point
    return static_cast<int64_t>(consensus_->getCommitIndex());
}

/** @brief Return commit timestamp mapped to next prospective log index. */
int64_t DistributedTimeCoordinator::getCommitTimestamp() const {
    // For commits, use next log index
    // Ensures commit timestamp > snapshot timestamp (external consistency)
    return static_cast<int64_t>(consensus_->getLastLogIndex()) + 1;
}

/** @brief Return current last log index from consensus backend. */
uint64_t DistributedTimeCoordinator::getCurrentLogIndex() const {
    return consensus_->getLastLogIndex();
}

} // namespace themis::sharding
