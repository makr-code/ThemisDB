/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            raft_mvcc_bridge.cpp                               ║
  Version:         0.0.44                                             ║
  Last Modified:   2026-04-15 05:44:45                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     143                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "storage/raft_mvcc_bridge.h"
#include <spdlog/spdlog.h>

namespace themis {

using DTC = themisdb::sharding::DistributedTimeCoordinator;

// ─────────────────────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────────────────────

RaftMvccBridge::RaftMvccBridge(
    std::shared_ptr<MVCCStore> mvcc_store,
    std::shared_ptr<DTC>       coordinator
)
    : mvcc_store_(std::move(mvcc_store))
    , coordinator_(std::move(coordinator))
{
    if (!mvcc_store_) {
        throw std::invalid_argument("RaftMvccBridge: mvcc_store cannot be null");
    }
    if (!coordinator_) {
        throw std::invalid_argument("RaftMvccBridge: coordinator cannot be null");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Timestamp conversion
// ─────────────────────────────────────────────────────────────────────────────

/* static */
HLCTimestamp RaftMvccBridge::toHlcTimestamp(const DTC::TimeInterval& interval) {
    // Convert wall-clock nanoseconds → milliseconds for HLC physical component.
    const uint64_t physical_ms =
        (interval.system_time_ns > 0)
            ? static_cast<uint64_t>(interval.system_time_ns) / 1'000'000ULL
            : 0ULL;

    // Embed the lower 20 bits of the Raft log index as the HLC logical counter.
    // This preserves intra-millisecond ordering up to 1,048,575 ops/ms per shard.
    const uint32_t logical =
        static_cast<uint32_t>(
            static_cast<uint64_t>(interval.logical_timestamp) & 0x000FFFFFULL
        );

    return HLCTimestamp::from(physical_ms, logical);
}

HLCTimestamp RaftMvccBridge::snapshotTimestamp() {
    // Ask the coordinator for the current view of time (Raft log index +
    // wall-clock nanoseconds).
    DTC::TimeInterval interval = coordinator_->now();
    HLCTimestamp synthetic = toHlcTimestamp(interval);

    // Advance the MVCCStore's local clock so the returned timestamp is
    // guaranteed to be ≥ both the current local HLC and the synthetic Raft
    // timestamp.  This ensures monotonicity across repeated calls.
    HLCTimestamp snapshot_ts = mvcc_store_->updateClock(synthetic);
    spdlog::debug("RaftMvccBridge::snapshotTimestamp raft_log_idx={} hlc={}",
                  interval.logical_timestamp, snapshot_ts.value);
    return snapshot_ts;
}

// ─────────────────────────────────────────────────────────────────────────────
// Reads
// ─────────────────────────────────────────────────────────────────────────────

RaftMvccBridge::LinearizableResult
RaftMvccBridge::linearizableRead(std::string_view key) {
    LinearizableResult result;

    if (!coordinator_->isLeader()) {
        // Non-leader cannot serve linearizable reads.
        // Caller must redirect to the current leader.
        result.is_leader = false;
        spdlog::debug("RaftMvccBridge::linearizableRead – not leader, redirect required");
        return result;
    }

    result.is_leader = true;
    result.value     = mvcc_store_->getLatest(key);
    return result;
}

std::optional<std::vector<uint8_t>>
RaftMvccBridge::snapshotRead(std::string_view key, HLCTimestamp ts) {
    return mvcc_store_->getAtTimestamp(key, ts);
}

// ─────────────────────────────────────────────────────────────────────────────
// Writes
// ─────────────────────────────────────────────────────────────────────────────

HLCTimestamp RaftMvccBridge::raftAwareWrite(
    std::string_view            key,
    const std::vector<uint8_t>& value
) {
    // Derive a Raft-consistent HLC timestamp and advance the local clock.
    HLCTimestamp commit_ts = snapshotTimestamp();

    // Write to the MVCC store with the explicit timestamp so the new version
    // is visible to any subsequent snapshotTimestamp() call.
    mvcc_store_->putWithTimestamp(key, value, commit_ts);

    spdlog::debug("RaftMvccBridge::raftAwareWrite key='{}' ts={}",
                  std::string(key), commit_ts.value);
    return commit_ts;
}

// ─────────────────────────────────────────────────────────────────────────────
// Accessors
// ─────────────────────────────────────────────────────────────────────────────

bool RaftMvccBridge::isLeader() const {
    return coordinator_->isLeader();
}

} // namespace themis
