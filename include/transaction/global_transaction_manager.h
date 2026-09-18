/**
 * @file global_transaction_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class IGlobalRegionParticipant {
public:
    /**
     * @brief IGlobal Region Participant.
     * @return Return value.
     */
    virtual ~IGlobalRegionParticipant() = default;

    [[nodiscard]] virtual bool prepare(
        const std::string&    txn_id,
        const nlohmann::json& ops
    ) = 0;

    /**
     * @brief Commit.
     * @param[in] txn_id Identifier of the txn.
     * @param[in] commit_timestamp Input parameter.
     */
    virtual void commit(
        const std::string& txn_id,
        int64_t            commit_timestamp
    ) = 0;

    /**
     * @brief Abort.
     * @param[in] txn_id Identifier of the txn.
     */
    virtual void abort(const std::string& txn_id) = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// Result / state types
// ─────────────────────────────────────────────────────────────────────────────

enum class GlobalTxnResult {
    COMMITTED,  ///< All regions committed
    ABORTED,    ///< At least one region voted ABORT (or an error occurred)
    ERROR       ///< Internal error; transaction state uncertain
};

struct GlobalTxnOutcome {
    GlobalTxnResult result        = GlobalTxnResult::ERROR;
    std::string     transaction_id;
    std::string     reason;        ///< Populated on ABORTED / ERROR
    int64_t         commit_timestamp_ns = 0; ///< TrueTime commit timestamp (0 if not committed)

    [[nodiscard]] bool committed() const {
        return result == GlobalTxnResult::COMMITTED;
    }
};

enum class GlobalTxnState {
    ACTIVE,          ///< Coordinator created; Phase 1 not yet sent
    PREPARING,       ///< Phase 1 in progress
    COMMIT_DECIDED,  ///< All voted COMMIT; Phase 2 in progress
    ABORT_DECIDED,   ///< At least one voted ABORT; Phase 2 in progress
    COMPLETED,       ///< Phase 2 complete; transaction finished
    FAILED           ///< Unrecoverable error
};

struct RegionTxnRecord {
    std::string region_id;
    bool        voted    = false;  ///< true = COMMIT vote
    bool        phase2_acked = false;
};

struct GlobalTxnRecord {
    std::string                  transaction_id;
    GlobalTxnState               state  = GlobalTxnState::ACTIVE;
    std::chrono::steady_clock::time_point started_at;

    std::map<std::string, nlohmann::json> region_ops;

    std::map<std::string, RegionTxnRecord> region_records;

    int64_t commit_timestamp_ns = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// GlobalTransactionManager
// ─────────────────────────────────────────────────────────────────────────────

class GlobalTransactionManager : public IRecoverableTwoPhaseCoordinator {
public:
    struct Config {
        std::string wal_directory;

        bool sync_wal_writes = true;
    };

    /**
     * @brief Global Transaction Manager.
     * @param[in] coordinator_id Identifier of the coordinator.
     * @param[in] truetime Input parameter.
     * @return Return value.
     */
    explicit GlobalTransactionManager(
        const std::string&                          coordinator_id,
        std::shared_ptr<themis::sharding::TrueTime> truetime
    );

    /**
     * @brief Global Transaction Manager.
     * @param[in] coordinator_id Identifier of the coordinator.
     * @param[in] truetime Input parameter.
     * @param[in] config Input parameter.
     * @return Return value.
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


    /**
     * @brief Register Region.
     * @param[in] region_id Identifier of the region.
     * @param[in,out] participant Input/output parameter.
     */
    void registerRegion(
        const std::string&        region_id,
        IGlobalRegionParticipant* participant
    );

    /**
     * @brief Unregister Region.
     * @param[in] region_id Identifier of the region.
     * @return True when the operation succeeds.
     */
    bool unregisterRegion(const std::string& region_id);

    /**
     * @brief Region Count.
     * @return Return value.
     */
    size_t regionCount() const;


    /**
     * @brief Begin Transaction.
     * @param[in] region_ids Input parameter.
     * @return Return value.
     */
    std::string beginTransaction(const std::vector<std::string>& region_ids);

    /**
     * @brief Add Operation.
     * @param[in] txn_id Identifier of the txn.
     * @param[in] region_id Identifier of the region.
     * @param[in] op Input parameter.
     * @return True when the operation succeeds.
     */
    bool addOperation(
        const std::string&    txn_id,
        const std::string&    region_id,
        const nlohmann::json& op
    );

    /**
     * @brief Commit.
     * @param[in] txn_id Identifier of the txn.
     * @return Return value.
     */
    GlobalTxnOutcome commit(const std::string& txn_id);

    /**
     * @brief Abort.
     * @param[in] txn_id Identifier of the txn.
     * @return True when the operation succeeds.
     */
    bool abort(const std::string& txn_id);

    // ── Recovery ─────────────────────────────────────────────────────────────

    size_t recoverInDoubtTransactions() override;

    [[nodiscard]] std::string recoveryCoordinatorName() const override;

    [[nodiscard]] std::string recoveryBackendName() const override;

    [[nodiscard]] std::vector<RecoverableTwoPhaseTransaction>
    getRecoverableTransactions() const override;


    /**
     * @brief Get Transaction State.
     * @param[in] txn_id Identifier of the txn.
     * @return Return value.
     */
    std::optional<GlobalTxnState> getTransactionState(
        const std::string& txn_id
    ) const;

    /**
     * @brief Return access control statistics.
     * @return Access control statistics.
     */
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


    /**
     * @brief Run Phase1.
     * @param[in,out] rec Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool runPhase1(GlobalTxnRecord& rec);

    /**
     * @brief Run Phase2.
     * @param[in,out] rec Input/output parameter.
     * @param[in] do_commit Input parameter.
     */
    void runPhase2(GlobalTxnRecord& rec, bool do_commit);

    /**
     * @brief Log To WAL.
     * @param[in] type Input parameter.
     * @param[in] txn_id Identifier of the txn.
     * @param[in] data Input parameter.
     */
    void logToWAL(
        themis::sharding::WALEntryType type,
        const std::string&             txn_id,
        const nlohmann::json&          data
    );

    /**
     * @brief Generate Transaction Id.
     * @return Return value.
     */
    std::string generateTransactionId();
};

} // namespace themis::transaction
