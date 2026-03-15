/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            orphan_detector.h                                  ║
  Version:         0.0.34                                             ║
  Last Modified:   2026-03-09 03:55:32                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     80                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <vector>
#include "cross_shard_transaction.h"

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
        // Timeout threshold - transactions older than this are considered orphans
        uint64_t timeout_seconds = 900;  // 15 minutes
        
        // States to check for orphans
        bool check_preparing = true;
        bool check_prepared = true;
        bool check_committing = true;
        bool check_aborting = true;
    };
    
    explicit OrphanDetector(const Config& config);
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
    Config config_;
};

} // namespace sharding
