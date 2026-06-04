/**
 * @file wal_applier.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

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
    /**
     * @brief Enforce fail-closed LSN ordering.
     *
     * When enabled, each applied entry must be either:
     * - the immediate successor of the current replica LSN, or
     * - a bootstrap replay of `0/0` when the replica is still at its initial position.
     *
     * Duplicate, stale, and out-of-order entries are rejected.
     */
    bool strict_mode = true;
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
    /**
     * @brief Construct WAL applier with replica-side apply policy.
     * @param config WAL apply configuration.
     */
    explicit WALApplier(const WALApplierConfig& config);

    /** @brief Destructor for WAL applier instance. */
    ~WALApplier();
    
    /**
     * @brief Install entry apply handler used for storage mutation.
     * @param handler Callback invoked per WAL entry.
     */
    void setApplyHandler(ApplyHandler handler);
    
    /**
        * @brief Apply a batch of entries from a primary shard.
        * @param entries Entries to apply.
        * @return Apply result including failure diagnostics.
        * @note In strict mode, the method fails closed on stale/duplicate/out-of-order LSNs.
     */
    ApplyResult applyBatch(const std::vector<WALEntry>& entries);
    
    /** @brief Return current replica LSN cursor. */
    LSN getCurrentLSN() const;
    
    /** @brief Set current replica LSN (e.g. bootstrap/recovery initialization). */
    void setCurrentLSN(const LSN& lsn);
    
    /** @brief Return WAL applier statistics snapshot. */
    WALApplierStats getStatistics() const;
    
    /** @brief Reset statistics counters while preserving current replica LSN. */
    void resetStatistics();

private:
    WALApplierConfig config_;
    
    mutable std::mutex mutex_;
    LSN current_lsn_;
    
    ApplyHandler apply_handler_;
    
    mutable std::mutex stats_mutex_;
    WALApplierStats stats_;
    
    /** @brief Apply one WAL entry with retry policy. */
    bool applyEntry(const WALEntry& entry);
    
    /** @brief Validate expected-to-actual LSN sequence continuity. */
    bool validateLSN(const LSN& expected, const LSN& actual);
    
    /** @brief Handle conflict-detection accounting/decision for one entry. */
    bool handleConflict(const WALEntry& entry);
};

} // namespace themis::sharding
