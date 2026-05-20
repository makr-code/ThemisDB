/*
 * ThemisDB | File: distributed_time_coordinator.cpp | Version: 0.0.47 | Last Modified: 2026-05-20 17:13:04
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 62
 * Open Issues: TODOs=1, Stubs=1, Gaps=3, Unimpl=0, Mock=1, Sim=0, Debt=0
 * Gap Correlation: internal=3 | external_v3=12 | delta=9 | status=divergent
 * External Severity (v3): C=0, H=11, M=1
 * PR: #1033 Replace TrueTime stub with consensus-based DistributedTimeCoordinator (2026-03-11T17:55:17Z)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "sharding/distributed_time_coordinator.h"
#include <spdlog/spdlog.h>
#include <chrono>

namespace themisdb::sharding {

DistributedTimeCoordinator::DistributedTimeCoordinator(
    std::shared_ptr<ConsensusModule> consensus,
    const Config& config
)
    : consensus_(consensus), config_(config) {
    spdlog::info("DistributedTimeCoordinator initialized (log_index_only={}, base_uncertainty_ns={})",
                config_.use_log_index_only, config_.base_uncertainty_ns);
}

DistributedTimeCoordinator::DistributedTimeCoordinator(
    std::shared_ptr<ConsensusModule> consensus
) : DistributedTimeCoordinator(consensus, Config{})
{
}

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

int64_t DistributedTimeCoordinator::getSnapshotTimestamp() const {
    // For snapshots, use current commit index
    // Ensures all reads see consistent data as of this point
    return static_cast<int64_t>(consensus_->getCommitIndex());
}

int64_t DistributedTimeCoordinator::getCommitTimestamp() const {
    // For commits, use next log index
    // Ensures commit timestamp > snapshot timestamp (external consistency)
    return static_cast<int64_t>(consensus_->getLastLogIndex()) + 1;
}

uint64_t DistributedTimeCoordinator::getCurrentLogIndex() const {
    return consensus_->getLastLogIndex();
}

} // namespace themis::sharding
