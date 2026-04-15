/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            wal_applier.h                                      ║
  Version:         0.0.43                                             ║
  Last Modified:   2026-04-15 04:13:48                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     157                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "sharding/wal_manager.h"
#include <string>
#include <memory>
#include <functional>
#include <mutex>
#include <atomic>

namespace themis::sharding {

/**
 * WAL Applier
 * 
 * Applies WAL entries received from primary on replica shard.
 * Maintains replica's LSN and ensures consistency.
 * 
 * Features:
 * - Apply WAL entries to local storage
 * - LSN tracking and validation
 * - Conflict detection
 * - Catchup mechanism
 * - Transaction support
 */

/**
 * Apply Handler
 * Called for each WAL entry to be applied
 * Returns true if successfully applied, false otherwise
 */
using ApplyHandler = std::function<bool(const WALEntry&)>;

/**
 * WAL Applier Configuration
 */
struct WALApplierConfig {
    std::string replica_id;
    bool strict_mode = true;  // Fail on LSN mismatch
    bool enable_conflict_detection = true;
    size_t max_apply_retries = 3;
};

/**
 * WAL Applier Statistics
 */
struct WALApplierStats {
    uint64_t total_entries_applied = 0;
    uint64_t total_bytes_applied = 0;
    uint64_t conflicts_detected = 0;
    uint64_t apply_failures = 0;
    uint64_t lsn_mismatches = 0;
    LSN current_replica_lsn;
};

/**
 * Apply Result
 */
struct ApplyResult {
    bool success = false;
    size_t entries_applied = 0;
    std::vector<std::string> errors;
    LSN last_applied_lsn;
};

/**
 * WAL Applier
 * 
 * Applies WAL entries on replica
 */
class WALApplier {
public:
    explicit WALApplier(const WALApplierConfig& config);
    ~WALApplier();
    
    /**
     * Set apply handler
     * This function is called for each entry to apply to storage
     */
    void setApplyHandler(ApplyHandler handler);
    
    /**
     * Apply batch of entries from primary
     * @param entries Entries to apply
     * @return Apply result
     */
    ApplyResult applyBatch(const std::vector<WALEntry>& entries);
    
    /**
     * Get current replica LSN
     */
    LSN getCurrentLSN() const;
    
    /**
     * Set current replica LSN (for initialization)
     */
    void setCurrentLSN(const LSN& lsn);
    
    /**
     * Get statistics
     */
    WALApplierStats getStatistics() const;
    
    /**
     * Reset statistics
     */
    void resetStatistics();

private:
    WALApplierConfig config_;
    
    mutable std::mutex mutex_;
    LSN current_lsn_;
    
    ApplyHandler apply_handler_;
    
    mutable std::mutex stats_mutex_;
    WALApplierStats stats_;
    
    /**
     * Apply single entry
     */
    bool applyEntry(const WALEntry& entry);
    
    /**
     * Validate LSN sequence
     */
    bool validateLSN(const LSN& expected, const LSN& actual);
    
    /**
     * Handle conflict
     */
    bool handleConflict(const WALEntry& entry);
};

} // namespace themis::sharding
