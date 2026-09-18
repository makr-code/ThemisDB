/**
 * @file cross_shard_ssi_manager.h
 * @brief Distributed Serializable Snapshot Isolation (SSI) manager for cross-shard transactions.
 *
 * Implements Optimistic SSI for distributed transactions:
 *  - Each shard participant registers its read predicate ranges and write keys during execution.
 *  - At prepare time, the coordinator calls validateAtPrepare() to detect cross-shard
 *    read-write (write-skew, phantom) and write-write (lost-update) conflicts.
 *  - If a conflict is found, the transaction is aborted so a caller can retry.
 *
 * Design trade-offs:
 *  - Optimistic: no blocking on reads; conflicts are detected at prepare time.
 *  - False-positive rate increases when predicate locks overflow max_predicate_locks_per_txn;
 *    callers should choose limits appropriate to their workload.
 *  - Global read/write-set synchronisation imposes memory overhead proportional to
 *    sum(read-set sizes) across all concurrent SERIALIZABLE transactions.
 *
 * Thread-safety: all public methods are thread-safe.
 *
 * @see CrossShardTransactionCoordinator
 */

// Copyright 2025 ThemisDB
// Licensed under MIT License

#pragma once

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <optional>

namespace themisdb::sharding {

// ============================================================================
// Data structures
// ============================================================================

/**
 * @brief A predicate (range) lock held by a cross-shard transaction on one shard.
 *
 * Represents the range [@p start_key, @p end_key] inclusive on both ends.
 * When @p end_key is empty the lock covers only the single key @p start_key.
 */
struct CrossShardPredicateLock {
    std::string shard_id;   ///< Shard that observed this read range.
    std::string start_key;  ///< Inclusive lower bound of the scanned range.
    std::string end_key;    ///< Inclusive upper bound; empty == single-key.
};

/**
 * @brief Describes a single cross-shard serialization conflict.
 *
 * A conflict record is produced when validateAtPrepare() detects that the
 * requesting transaction's read or write footprint overlaps with a concurrent
 * SERIALIZABLE transaction in a way that cannot be serialized.
 */
struct CrossShardSSIConflict {
    std::string transaction_id;             ///< Transaction being validated.
    std::string conflicting_transaction_id; ///< Concurrent transaction causing the conflict.
    std::string shard_id;                   ///< Shard where the overlap was detected.
    std::string key;                        ///< Key (or range start) that triggered the conflict.
    /// Conflict kind: "rw" (read-write; write-skew / phantom risk) or
    /// "ww" (write-write; lost-update risk).
    std::string conflict_type;
    std::string message;                    ///< Human-readable description.
};

// ============================================================================
// CrossShardSSIManager
// ============================================================================

/**
 * @brief Distributed SSI coordinator: tracks per-shard read/write sets for all
 *        active SERIALIZABLE cross-shard transactions and validates conflicts
 *        at prepare time.
 *
 * ### Usage
 * 1. Before (or during) executing a shard's portion of a SERIALIZABLE
 *    transaction, the shard reports its predicate ranges via registerReadSet()
 *    and its written keys via registerWriteSet().
 * 2. At prepare time CrossShardTransactionCoordinator calls validateAtPrepare()
 *    for the committing transaction.  A non-empty result means at least one
 *    serialization anomaly was detected; the coordinator must abort the
 *    transaction.
 * 3. After a transaction commits or aborts, call clearTransaction() to free
 *    its bookkeeping.
 *
 * ### Conflict detection semantics
 * - **RW conflict** (write-skew / phantom): A write key K in @e this
 *   transaction's write-set falls within a predicate range held by another
 *   active SERIALIZABLE transaction, *or* a write key K in another transaction
 *   falls within a predicate range held by @e this transaction.
 * - **WW conflict** (lost update): A write key K appears in both @e this
 *   transaction's write-set and another active SERIALIZABLE transaction's
 *   write-set.
 *
 * @note Only transactions registered via registerReadSet() / registerWriteSet()
 *       are visible to conflict detection.  Participants that do not call these
 *       methods are invisible (no cross-shard anomaly is detected for them).
 */
class CrossShardSSIManager {
public:
    /**
     * @brief Runtime configuration for distributed SSI.
     *
     * Tune these values to balance memory usage, false-positive abort rate,
     * and conflict-detection coverage.
     */
    struct Config {
        /// Enable predicate-lock tracking and cross-shard conflict detection.
        /// When false, validateAtPrepare() always returns an empty vector
        /// (no conflict detected), effectively degrading SERIALIZABLE to
        /// SNAPSHOT_ISOLATION.
        bool enable_predicate_locking = true;

        /// Maximum number of predicate lock ranges that may be registered per
        /// transaction.  When the per-transaction limit is reached additional
        /// registerReadSet() calls are silently ignored; the missing coverage
        /// may increase the false-negative rate (missed anomalies).
        size_t max_predicate_locks_per_txn = 1000;

        /// Maximum number of write keys tracked per transaction.  Excess keys
        /// are silently dropped, which may increase the false-negative rate.
        size_t max_write_keys_per_txn = 10000;
    };

    /**
     * @brief Default-construct the manager with default configuration.
     */
    CrossShardSSIManager();

    /**
     * @brief Construct the manager with the supplied configuration.
     * @param config Initial SSI tuning parameters.
     */
    explicit CrossShardSSIManager(const Config& config);

    ~CrossShardSSIManager() = default;

    // Non-copyable; moveable.
    CrossShardSSIManager(const CrossShardSSIManager&) = delete;
    CrossShardSSIManager& operator=(const CrossShardSSIManager&) = delete;
    CrossShardSSIManager(CrossShardSSIManager&&) noexcept = default;
    CrossShardSSIManager& operator=(CrossShardSSIManager&&) noexcept = default;

    // ── Registration API ─────────────────────────────────────────────────────

    /**
     * @brief Register the predicate (range) locks observed on @p shard_id by
     *        transaction @p txn_id during its read phase.
     *
     * May be called multiple times for the same (txn_id, shard_id) pair;
     * new predicates are appended up to the per-transaction cap.  Calls on
     * finished or unknown transactions are silently ignored.
     *
     * Thread-safe.
     *
     * @param txn_id      Global transaction identifier.
     * @param shard_id    Shard that performed the reads.
     * @param predicates  Predicate ranges observed on @p shard_id.
     */
    void registerReadSet(const std::string& txn_id,
                         const std::string& shard_id,
                         const std::vector<CrossShardPredicateLock>& predicates);

    /**
     * @brief Register the keys written on @p shard_id by transaction @p txn_id.
     *
     * May be called multiple times for the same (txn_id, shard_id) pair;
     * new keys are appended up to the per-transaction cap.
     *
     * Thread-safe.
     *
     * @param txn_id    Global transaction identifier.
     * @param shard_id  Shard that performed the writes.
     * @param keys      Written keys on @p shard_id.
     */
    void registerWriteSet(const std::string& txn_id,
                          const std::string& shard_id,
                          const std::vector<std::string>& keys);

    // ── Validation ───────────────────────────────────────────────────────────

    /**
     * @brief Validate cross-shard SSI conflicts for @p txn_id at prepare time.
     *
     * Compares @p txn_id's combined read-set and write-set against those of
     * every other tracked transaction that has overlapping shard participation.
     *
     * Returns an empty vector when:
     *  - predicate locking is disabled,
     *  - @p txn_id has no registered read or write sets,
     *  - no conflicting transactions are tracked.
     *
     * Thread-safe; acquires a shared lock internally.
     *
     * @param txn_id  Transaction to validate.
     * @return        All detected serialization conflicts; empty == no conflict.
     */
    [[nodiscard]] std::vector<CrossShardSSIConflict>
    validateAtPrepare(const std::string& txn_id) const;

    // ── Lifecycle ────────────────────────────────────────────────────────────

    /**
     * @brief Remove all bookkeeping for @p txn_id.
     *
     * Must be called when a transaction commits or aborts to prevent unbounded
     * memory growth.  Safe to call for unknown transaction IDs (no-op).
     *
     * Thread-safe.
     *
     * @param txn_id  Transaction whose data should be released.
     */
    void clearTransaction(const std::string& txn_id);

    // ── Configuration ────────────────────────────────────────────────────────

    /**
     * @brief Replace the runtime configuration.
     *
     * New values take effect immediately for subsequent registrations and
     * validations; in-flight transactions are unaffected.
     *
     * Thread-safe.
     *
     * @param config  New SSI tuning parameters.
     */
    void setConfig(const Config& config);

    /**
     * @brief Return the currently active configuration.
     * Thread-safe.
     */
    [[nodiscard]] Config getConfig() const;

    // ── Diagnostics ──────────────────────────────────────────────────────────

    /// Returns the number of transactions currently tracked.
    [[nodiscard]] size_t trackedTransactionCount() const;

private:
    // ── Internal helpers ─────────────────────────────────────────────────────

    /// Returns true when @p key falls within the predicate range [start, end].
    /// An empty @p end is treated as a single-key predicate (key == start).
    [[nodiscard]] static bool keyInRange(const std::string& key,
                                         const std::string& start,
                                         const std::string& end) noexcept;

    // ── State ─────────────────────────────────────────────────────────────────

    mutable std::mutex mutex_; ///< Protects all mutable state below.
    Config config_;            ///< Runtime tuning parameters.

    /// Predicate (range) locks per transaction, grouped by shard.
    /// Outer key: txn_id → inner key: shard_id → vector of predicate ranges.
    std::map<std::string,
             std::map<std::string, std::vector<CrossShardPredicateLock>>> read_sets_;

    /// Write keys per transaction, grouped by shard.
    /// Outer key: txn_id → inner key: shard_id → vector of written keys.
    std::map<std::string,
             std::map<std::string, std::vector<std::string>>> write_sets_;
};

} // namespace themisdb::sharding
