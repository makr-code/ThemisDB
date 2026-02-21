/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            orphan_detector.h                                  ║
  Version:         0.0.11                                             ║
  Last Modified:   2026-02-21 14:07:34                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     87                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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

// Forward declaration
class CrossShardTransactionCoordinator;

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
        const std::shared_ptr<CrossShardTransactionCoordinator>& coordinator
    );
    
    /**
     * Check if a specific transaction is orphaned
     * @param transaction_id Transaction to check
     * @param coordinator The transaction coordinator
     * @return true if orphaned, false otherwise
     */
    bool isOrphaned(
        const std::string& transaction_id,
        const std::shared_ptr<CrossShardTransactionCoordinator>& coordinator
    );
    
private:
    Config config_;
};

} // namespace sharding
