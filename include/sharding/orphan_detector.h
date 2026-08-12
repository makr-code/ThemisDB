/**
 * @file orphan_detector.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <vector>
#include "cross_shard_transaction.h"

namespace themis::sharding {
class DistributedCoordinator;
} // namespace themis::sharding

namespace sharding {

/**
 * OrphanDetector identifies transactions that are stuck in inconsistent states
 * due to coordinator failures, network partitions, or participant crashes.
 */
class OrphanDetector {
public:
    /**
     * Configuration for orphan detection
     */
    struct Config {
        uint64_t timeout_seconds = 900;  ///< Age threshold for orphan detection in seconds.
        bool check_preparing = true;     ///< Include PREPARING transactions.
        bool check_prepared = true;      ///< Include PREPARED transactions.
        bool check_committing = true;    ///< Include COMMITTING transactions.
        bool check_aborting = true;      ///< Include ABORTING transactions.
    };

    /**
     * @brief Construct orphan detector with static configuration.
     * @param config Timeout and state-filter configuration.
     */
    explicit OrphanDetector(const Config& config);

    /**
     * Construct with an optional DistributedCoordinator for authoritative
     * in-flight transaction lookups.  When @p dist_coordinator is non-null,
     * detectOrphans() and isOrphaned() query it directly instead of (or in
     * addition to) the per-call CrossShardTransactionCoordinator.
     *
     * @param config           Orphan-detection configuration.
     * @param dist_coordinator Wired DistributedCoordinator; may be nullptr.
     */
    OrphanDetector(const Config& config,
                   themis::sharding::DistributedCoordinator* dist_coordinator);

    /** @brief Default destructor. */
    ~OrphanDetector() = default;
    
    /**
     * Detect orphaned transactions in the coordinator
     * @param coordinator The transaction coordinator to check
     * @return List of orphaned transaction IDs
     */
    std::vector<std::string> detectOrphans(
        const std::shared_ptr<themisdb::sharding::CrossShardTransactionCoordinator>& coordinator
    );
    
    /**
     * Check if a specific transaction is orphaned
     * @param transaction_id Transaction to check
     * @param coordinator The transaction coordinator
     * @return true if orphaned, false otherwise
     */
    bool isOrphaned(
        const std::string& transaction_id,
        const std::shared_ptr<themisdb::sharding::CrossShardTransactionCoordinator>& coordinator
    );

    /**
     * Reclaim stale Percolator locks left by failed coordinators.
     *
     * Scans all active transactions in @p coordinator, identifies those using
     * the PERCOLATOR protocol whose locks have been held longer than
     * Config::timeout_seconds, and aborts them via the coordinator's abort()
     * method.  The abort triggers lock release on every shard participant.
     *
     * @param coordinator  The cross-shard coordinator to scan and clean.
     * @return             Number of stale Percolator locks reclaimed.
     */
    size_t cleanPercolatorLocks(
        const std::shared_ptr<themisdb::sharding::CrossShardTransactionCoordinator>& coordinator
    );
    
private:
    Config config_;  ///< Runtime orphan-detection configuration.
    themis::sharding::DistributedCoordinator* distributed_coordinator_{nullptr}; ///< Optional authoritative coordinator backend.
};

} // namespace sharding
