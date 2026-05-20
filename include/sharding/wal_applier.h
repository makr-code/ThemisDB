/*
 * ThemisDB | File: wal_applier.h | Version: 0.0.47
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#include "sharding/wal_manager.h"
#include "utils/retry_policy.h"
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
    /// Maximum number of times `applyEntry()` attempts the apply handler.
    size_t max_apply_retries = 3;
    /// Initial delay before the first retry in milliseconds.
    /// Each subsequent delay doubles (exponential backoff).
    uint32_t retry_initial_delay_ms = 100;
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
