/**
 * @file global_transaction_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2025 ThemisDB
// Licensed under MIT License
//
// Global Transaction Manager — multi-region ACID guarantees
//
// Provides a single coordinator that drives ACID transactions spanning
// multiple geographic regions.  Each region exposes an
// IGlobalRegionParticipant interface; the manager runs a two-phase commit
// across all regions that participate in a given transaction and uses
// TrueTime to assign globally-monotone commit timestamps.
//
// Protocol Flow:
//   1. beginTransaction()  – register regions, obtain a global TXN-ID
//   2. addOperation()      – accumulate per-region operations
//   3. commit()            – 2PC across all regions with TrueTime wait
//      a. Phase 1 PREPARE  – each region locks its rows and votes
//      b. TrueTime stamp   – coordinator waits until timestamp is past
//      c. Phase 2 COMMIT   – each region applies changes
//   4. abort()             – broadcast ABORT to all registered regions
//
// Recovery: The manager writes a WAL (BEGIN_TX / COMMIT_TX / ABORT_TX)
// so that an in-doubt transaction can be re-driven after a coordinator
// restart by calling recoverInDoubtTransactions().
//
// Thread-safety:
//   GlobalTransactionManager itself is thread-safe for concurrent callers.
//   A single GlobalTransaction handle must only be used from one thread
//   at a time.

#pragma once

#include "sharding/truetime.h"
#include "sharding/wal_manager.h"
#include "transaction/recoverable_two_phase_coordinator.h"
#include <atomic>
#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace themis::transaction {

// ─────────────────────────────────────────────────────────────────────────────
// Region participant interface
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Interface that every regional transaction coordinator must implement.
 *
 * The GlobalTransactionManager calls these methods in lock-step with
 * the two-phase commit protocol.  Concrete implementations wrap a local
 * DistributedTransactionCoordinator (for in-process tests) or a gRPC
 * stub (for real deployments).
 */
class IGlobalRegionParticipant {
public:
    virtual ~IGlobalRegionParticipant() = default;

    /**
     * @brief Phase 1: Prepare a transaction.
     *
     * The region must validate and durably lock all rows named in @p ops,
     * write a PREPARE log entry, and return its vote.
     *
     * @param txn_id     Globally unique transaction identifier
     * @param ops        JSON array of operations for this region
     * @return           true → vote COMMIT; false → vote ABORT
     */
    [[nodiscard]] virtual bool prepare(
        const std::string&    txn_id,
        const nlohmann::json& ops
    ) = 0;

    /**
     * @brief Phase 2 (commit path): Apply the prepared operations.
     *
     * Called only when the coordinator has received COMMIT votes from
     * every participant.
     *
     * @param txn_id           Transaction to commit
     * @param commit_timestamp TrueTime commit timestamp (nanoseconds since epoch)
     */
    virtual void commit(
        const std::string& txn_id,
        int64_t            commit_timestamp
    ) = 0;

    /**
     * @brief Phase 2 (abort path): Discard the prepared operations.
     *
     * @param txn_id  Transaction to abort
     */
    virtual void abort(const std::string& txn_id) = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// Result / state types
// ─────────────────────────────────────────────────────────────────────────────

/** @brief Possible outcomes of a global commit call. */
enum class GlobalTxnResult {
    COMMITTED,  ///< All regions committed
    ABORTED,    ///< At least one region voted ABORT (or an error occurred)
    ERROR       ///< Internal error; transaction state uncertain
};

/** @brief Detailed outcome returned by GlobalTransactionManager::commit(). */
struct GlobalTxnOutcome {
    GlobalTxnResult result        = GlobalTxnResult::ERROR;
    std::string     transaction_id;
    std::string     reason;        ///< Populated on ABORTED / ERROR
    int64_t         commit_timestamp_ns = 0; ///< TrueTime commit timestamp (0 if not committed)

    [[nodiscard]] bool committed() const {
        return result == GlobalTxnResult::COMMITTED;
    }
};

/** @brief Lifecycle state of a global transaction. */
enum class GlobalTxnState {
    ACTIVE,          ///< Coordinator created; Phase 1 not yet sent
    PREPARING,       ///< Phase 1 in progress
    COMMIT_DECIDED,  ///< All voted COMMIT; Phase 2 in progress
    ABORT_DECIDED,   ///< At least one voted ABORT; Phase 2 in progress
    COMPLETED,       ///< Phase 2 complete; transaction finished
    FAILED           ///< Unrecoverable error
};

/** @brief Per-region tracking kept by the coordinator. */
struct RegionTxnRecord {
    std::string region_id;
    bool        voted    = false;  ///< true = COMMIT vote
    bool        phase2_acked = false;
};

/** @brief Internal coordinator record for one global transaction. */
struct GlobalTxnRecord {
    std::string                  transaction_id;
    GlobalTxnState               state  = GlobalTxnState::ACTIVE;
    std::chrono::steady_clock::time_point started_at;

    /// Per-region pending operations (JSON)
    std::map<std::string, nlohmann::json> region_ops;

    /// Per-region vote tracking
    std::map<std::string, RegionTxnRecord> region_records;

    /// TrueTime commit timestamp (nanoseconds since epoch); 0 until assigned
    int64_t commit_timestamp_ns = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// GlobalTransactionManager
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Global coordinator for multi-region ACID transactions.
 *
 * Usage:
 * @code
 *   GlobalTransactionManager gtm("coord-global", truetime);
 *   gtm.registerRegion("us-east-1", &usEastParticipant);
 *   gtm.registerRegion("eu-west-1", &euWestParticipant);
 *
 *   auto txn_id = gtm.beginTransaction({"us-east-1", "eu-west-1"});
 *   gtm.addOperation(txn_id, "us-east-1", {{"type","PUT"},{"key","users:1"}});
 *   gtm.addOperation(txn_id, "eu-west-1", {{"type","PUT"},{"key","users:1"}});
 *   auto outcome = gtm.commit(txn_id);
 *   assert(outcome.committed());
 * @endcode
 */
class GlobalTransactionManager : public IRecoverableTwoPhaseCoordinator {
public:
    /** @brief Configuration for the global coordinator. */
    struct Config {
        /// WAL directory for durable logging; empty = WAL disabled
        std::string wal_directory;

        /// Flush WAL synchronously on every write
        bool sync_wal_writes = true;
    };

    /**
     * @brief Construct a global transaction coordinator.
     *
     * @param coordinator_id  Unique name for this coordinator instance
     * @param truetime        TrueTime clock for commit-timestamp assignment
     */
    explicit GlobalTransactionManager(
        const std::string&                          coordinator_id,
        std::shared_ptr<themis::sharding::TrueTime> truetime
    );

    /**
     * @brief Construct a global transaction coordinator.
     *
     * @param coordinator_id  Unique name for this coordinator instance
     * @param truetime        TrueTime clock for commit-timestamp assignment
     * @param config          Optional configuration (WAL, timeouts, …)
     */
    explicit GlobalTransactionManager(
        const std::string&                          coordinator_id,
        std::shared_ptr<themis::sharding::TrueTime> truetime,
        const Config&                               config
    );

    ~GlobalTransactionManager() = default;

    // Disable copy; allow move
    GlobalTransactionManager(const GlobalTransactionManager&)            = delete;
    GlobalTransactionManager& operator=(const GlobalTransactionManager&) = delete;

    // ── Region management ─────────────────────────────────────────────────────

    /**
     * @brief Register a region participant.
     *
     * The participant must remain valid for the lifetime of the coordinator
     * (or until explicitly unregistered).
     *
     * @param region_id   Geographic region identifier (e.g. "us-east-1")
     * @param participant Pointer to the region's transaction coordinator proxy
     */
    void registerRegion(
        const std::string&        region_id,
        IGlobalRegionParticipant* participant
    );

    /**
     * @brief Unregister a previously registered region.
     * @return true if the region was found and removed
     */
    bool unregisterRegion(const std::string& region_id);

    /** @brief Return the number of registered regions. */
    size_t regionCount() const;

    // ── Transaction lifecycle ─────────────────────────────────────────────────

    /**
     * @brief Begin a new global transaction.
     *
     * @param region_ids  Regions that will participate in this transaction.
     *                    All must have been registered via registerRegion().
     * @return            Globally unique transaction ID
     * @throws std::invalid_argument if region_ids is empty or a region is unknown
     */
    std::string beginTransaction(const std::vector<std::string>& region_ids);

    /**
     * @brief Append an operation for a specific region.
     *
     * Operations are buffered locally until commit() is called.
     *
     * @param txn_id    Transaction ID returned by beginTransaction()
     * @param region_id Target region
     * @param op        JSON object describing the operation
     * @return          false if the transaction or region is not found
     */
    bool addOperation(
        const std::string&    txn_id,
        const std::string&    region_id,
        const nlohmann::json& op
    );

    /**
     * @brief Commit a global transaction via two-phase commit.
     *
     * Drives both phases synchronously:
     * 1. Sends PREPARE to every participating region.
     * 2. Waits for all votes.
     * 3. Assigns a TrueTime commit timestamp and waits until it is past.
     * 4. Sends COMMIT (or ABORT) to every participant.
     *
     * @param txn_id  Transaction to commit
     * @return        Outcome with COMMITTED, ABORTED, or ERROR
     */
    GlobalTxnOutcome commit(const std::string& txn_id);

    /**
     * @brief Abort a global transaction.
     *
     * If Phase 1 has already been sent, ABORT is broadcast to all
     * participants.  Otherwise the transaction is simply discarded.
     *
     * @param txn_id  Transaction to abort
     * @return        false if the transaction was not found
     */
    bool abort(const std::string& txn_id);

    // ── Recovery ─────────────────────────────────────────────────────────────

    /**
     * @brief Recover and re-drive in-doubt transactions from the WAL.
     *
     * Must be called once after a restart, before accepting new
     * transactions.  Re-sends the Phase 2 decision to participants
     * that have not yet acknowledged.
     *
     * @return Number of in-doubt transactions resolved
     */
    size_t recoverInDoubtTransactions() override;

    /**
     * @brief Return the canonical coordinator name for global recovery reports.
     * @return "GlobalTransactionManager".
     */
    [[nodiscard]] std::string recoveryCoordinatorName() const override;

    /**
     * @brief Return the durable backend used by this coordinator.
     * @return "WAL" when enabled, otherwise "disabled".
     */
    [[nodiscard]] std::string recoveryBackendName() const override;

    /**
     * @brief Snapshot current in-doubt transactions using the shared state model.
     * @return Normalized non-final transaction list for global recovery orchestration.
     */
    [[nodiscard]] std::vector<RecoverableTwoPhaseTransaction>
    getRecoverableTransactions() const override;

    // ── Introspection ─────────────────────────────────────────────────────────

    /**
     * @brief Return the current state of a tracked transaction.
     */
    std::optional<GlobalTxnState> getTransactionState(
        const std::string& txn_id
    ) const;

    /** @brief Return coordinator statistics as a JSON object. */
    nlohmann::json getStatistics() const;

private:
    const std::string                         coordinator_id_;
    std::shared_ptr<themis::sharding::TrueTime> truetime_;
    Config                                    config_;

    mutable std::mutex mutex_;
    std::map<std::string, IGlobalRegionParticipant*> regions_;
    std::map<std::string, GlobalTxnRecord>           transactions_;

    std::unique_ptr<themis::sharding::WALManager> wal_;

    // Statistics
    std::atomic<uint64_t> total_transactions_{0};
    std::atomic<uint64_t> total_commits_{0};
    std::atomic<uint64_t> total_aborts_{0};
    std::atomic<uint64_t> total_errors_{0};

    const std::chrono::steady_clock::time_point start_time_{
        std::chrono::steady_clock::now()
    };

    std::atomic<uint64_t> txn_counter_{0};

    // ── Internal helpers ──────────────────────────────────────────────────────

    /// Run Phase 1: send PREPARE to all participating regions
    bool runPhase1(GlobalTxnRecord& rec);

    /// Run Phase 2: send COMMIT or ABORT to all participating regions
    void runPhase2(GlobalTxnRecord& rec, bool do_commit);

    /// Persist a WAL entry
    void logToWAL(
        themis::sharding::WALEntryType type,
        const std::string&             txn_id,
        const nlohmann::json&          data
    );

    /// Generate a globally-unique transaction identifier
    std::string generateTransactionId();
};

} // namespace themis::transaction
