/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            distributed_time_coordinator.cpp                   ║
  Version:         0.0.4                                              ║
  Last Modified:   2026-02-21 08:40:35                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     73                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c6716ede7  2026-02-16  Add ThemisDB Order Request Plugin with shortcodes, AJAX h... ║
    • 8400a4c76  2026-02-12  feat: Enhance ThemisDB with new components and improvements ║
    • 37da19d1c  2026-02-10  Refactor code structure for improved readability and main... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
