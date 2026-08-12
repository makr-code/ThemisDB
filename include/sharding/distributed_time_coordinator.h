/**
 * @file distributed_time_coordinator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=5; TODO=1, Stub=2, Unimpl=0, Mock=2, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2025 ThemisDB
// Licensed under MIT License

#pragma once

#include "sharding/consensus_module.h"
#include <memory>
#include <chrono>
#include <cstdint>

namespace themisdb::sharding {

/**
 * @brief Distributed Time Coordinator using Consensus Log Index
 * 
 * Provides logically consistent timestamps for MVCC by using the consensus
 * log index as the source of truth for causal ordering.
 * 
 * This design replaces Google Spanner-style TrueTime (NTP-based) with a
 * consensus-based approach that is:
 * - Fully deterministic (no external dependencies)
 * - Production-ready (no NTP stub implementation)
 * - Easily testable (mock consensus module)
 * - Causally consistent (log index guarantees ordering)
 */
class DistributedTimeCoordinator {
public:
    /**
     * @brief Time interval with logical timestamp and uncertainty
     */
    struct TimeInterval {
        int64_t logical_timestamp;  ///< Consensus-log-based logical timestamp.
        int64_t uncertainty_ns;     ///< Uncertainty bound in nanoseconds.
        int64_t system_time_ns;     ///< Local wall-clock time for diagnostics/reference.
    };
    
    /**
     * @brief Configuration for DistributedTimeCoordinator
     */
    struct Config {
        int64_t base_uncertainty_ns = 1000000;  ///< Added uncertainty (default 1 ms).
        bool use_log_index_only = true;         ///< Use consensus log index as sole ordering source.
    };
    
    /**
     * @brief Construct DistributedTimeCoordinator
     * @param consensus Shared pointer to consensus module
     * @param config Configuration settings
     */
    explicit DistributedTimeCoordinator(
        std::shared_ptr<ConsensusModule> consensus,
        const Config& config
    );

    /**
     * @brief Construct coordinator with default configuration.
     * @param consensus Shared consensus module instance.
     */
    explicit DistributedTimeCoordinator(
        std::shared_ptr<ConsensusModule> consensus
    );
    
    /**
     * @brief Get current time with logical ordering guarantee
     * @return TimeInterval with logical timestamp and uncertainty
     */
    TimeInterval now() const;
    
    /**
     * @brief Get timestamp for transaction snapshot
     * 
     * Uses the current commit index as basis for causal ordering.
     * This ensures all reads see consistent data as of this point.
     * 
     * @return Snapshot timestamp based on commit index
     */
    int64_t getSnapshotTimestamp() const;
    
    /**
     * @brief Get timestamp for transaction commit
     * 
     * Uses next available log index to ensure causality.
     * This ensures commit timestamp > snapshot timestamp (external consistency).
     * 
     * @return Commit timestamp based on next log index
     */
    int64_t getCommitTimestamp() const;
    
    /**
     * @brief Check if timestamp T1 definitely happened before T2
     * @param ts1 First timestamp
     * @param ts2 Second timestamp
     * @return True if ts1 + uncertainty < ts2
     */
    bool definitelyBefore(int64_t ts1, int64_t ts2) const {
        return ts1 + config_.base_uncertainty_ns < ts2;
    }
    
    /**
     * @brief Check whether this node is the current Raft leader.
     *
     * Linearizable reads must only be served by the leader.
     * Returns true if the underlying ConsensusModule reports leadership.
     */
    bool isLeader() const {
        return consensus_->isLeader();
    }

    /**
     * @brief Get current log index (basis for timestamps)
     * @return Current log index from consensus module
     */
    uint64_t getCurrentLogIndex() const;

private:
    std::shared_ptr<ConsensusModule> consensus_;  ///< Consensus backend used for log-index timestamps.
    Config config_;                               ///< Runtime time-coordination configuration.
};

} // namespace themisdb::sharding
