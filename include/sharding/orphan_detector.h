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
