/**
 * @file two_phase_commit_participant.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2025 ThemisDB
// Licensed under MIT License
//
// Two-Phase Commit (2PC) Participant
//
// Implements the shard-side participant role in 2PC distributed transactions.
// Each Raft-group leader runs a TwoPhaseCommitParticipant that handles:
//
//   PREPARE  → validate ops, acquire row locks, write PREPARE_TX to WAL, vote
//   COMMIT   → apply operations, release locks, write COMMIT_TX to WAL
//   ABORT    → release locks, write ABORT_TX to WAL
//
// Idempotency: duplicate PREPARE/COMMIT/ABORT messages for the same transaction
// are handled safely (idempotent log + state machine).
//
// This class implements ShardRPCServer::RequestHandler so it can be attached
// directly to a ShardRPCServer for real network deployments.

#pragma once

#include "sharding/shard_rpc_server.h"
#include "sharding/epoch_fencing.h"
#include "sharding/wal_manager.h"
#include <string>
#include <map>
#include <mutex>
#include <memory>
#include <atomic>
#include <chrono>
#include <functional>
#include <optional>
#include <nlohmann/json.hpp>

namespace themis::sharding {

/**
 * @brief State of a prepared transaction on the participant side
 */
enum class ParticipantTxnState {
    PREPARED,   ///< Vote COMMIT abgegeben, wartet auf Coordinator-Entscheidung.
    COMMITTED,  ///< COMMIT empfangen und Operationen erfolgreich angewendet.
    ABORTED     ///< ABORT empfangen oder PREPARE abgelehnt; kein lokaler Timeout-Abbruch.
};

/**
 * @brief Metadata tracked per prepared transaction at the participant
 */
struct ParticipantTransaction {
    std::string             transaction_id; ///< Globale Transaktions-ID.
    std::string             coordinator_shard_id; ///< Authenticated coordinator bound at PREPARE.
    ParticipantTxnState     state = ParticipantTxnState::PREPARED; ///< Aktueller Teilnehmerzustand.
    nlohmann::json          operations; ///< Gepufferte Operationen fuer spaeteres COMMIT.
    EpochToken              fencing_token; ///< Epoch credential checked at PREPARE and COMMIT.
    std::chrono::steady_clock::time_point prepared_at;
    int64_t                 prepare_timestamp = 0; ///< Optionaler PREPARE-Zeitstempel vom Coordinator.
    int64_t                 commit_timestamp  = 0; ///< MVCC-COMMIT-Zeitstempel (auf COMMIT gesetzt).
    bool                    vote_committed    = true; ///< Gespeicherte PREPARE-Stimme (idempotente Antworten).
    bool                    resolution_pending = false; ///< Recovered, expired, or failed delivery awaits the coordinator.
    bool                    timeout_reported = false; ///< Counts an expired PREPARE once; never changes the decision.
};

/**
 * @brief Callback for applying committed operations to the storage engine
 *
 * @param txn_id    Transaction ID
 * @param ops       JSON array of operations to apply
 * @param commit_ts MVCC commit timestamp
 * @return true only after atomically applying and durably persisting the writes.
 * @note Must deduplicate by transaction ID across crashes between applying writes
 *       and recording participant COMMIT. The borrowed operations are valid only
 *       during this call; a false return must remain safe to retry.
 */
using ApplyOperationsCallback =
    std::function<bool(const std::string& txn_id,
                       const nlohmann::json& ops,
                       int64_t commit_ts)>;

/**
 * @brief Callback for validating and locking operations in PREPARE phase
 *
 * @param txn_id Transaction ID
 * @param ops    JSON array of operations to validate/lock
 * @return true if all operations are valid and locks acquired
 */
using ValidateAndLockCallback =
    std::function<bool(const std::string& txn_id,
                       const nlohmann::json& ops)>;

/**
 * @brief Callback to release locks held by a transaction
 *
 * @param txn_id Transaction ID
 */
using ReleaseLockCallback = std::function<void(const std::string& txn_id)>;

/**
 * @brief Shard-side participant in a Two-Phase Commit protocol
 *
 * Attach to a ShardRPCServer via setRequestHandler() to handle
 * incoming PREPARE/COMMIT/ABORT messages from the coordinator.
 *
 * All state transitions are written to a WAL before the response is
 * sent, ensuring that a crash between phases can be recovered on restart.
 * The apply callback must be idempotent by transaction ID because a crash
 * after applying operations but before the durable COMMIT record can cause
 * recovery to redeliver the coordinator's durable COMMIT decision.
 */
class TwoPhaseCommitParticipant : public ShardRPCServer::RequestHandler {
public:
    /**
     * @brief Configuration for the participant
     */
    struct Config {
        std::string wal_directory; ///< Required for votes/terminal acknowledgements; empty permits introspection only.
        std::chrono::milliseconds prepare_timeout{60000}; ///< PREPARED age before reporting unresolved recovery, never auto-ABORT.
        bool sync_wal_writes = true; ///< Erzwingt synchrones WAL-Flush pro Eintrag.
        std::shared_ptr<EpochFencingManager> fencing_manager; ///< Required for write authorization.
    };

    /**
     * @brief Construct a participant with optional storage callbacks
     *
     * This default-config overload has no WAL and accepts introspection only;
     * use the Config overload with all three callbacks for writes.
     *
     * @param shard_id   Identifier of this shard
     * @param validate   Callback to validate & lock ops in PREPARE phase
     * @param apply      Callback to apply ops in COMMIT phase
     * @param release    Callback to release locks in ABORT/after COMMIT
     */
    explicit TwoPhaseCommitParticipant(
        const std::string&          shard_id,
        ValidateAndLockCallback     validate = nullptr,
        ApplyOperationsCallback     apply    = nullptr,
        ReleaseLockCallback         release  = nullptr
    );

    /**
     * @brief Construct a participant with optional storage callbacks
     *
     * PREPARE requires all three callbacks, a configured WAL, fencing manager
     * and valid per-shard token. Missing callbacks block readiness. Recovered
     * PREPARE records require validation and release callbacks to restore locks.
     *
     * @param shard_id   Identifier of this shard
     * @param config     Participant configuration
     * @param validate   Callback to validate & lock ops in PREPARE phase
     * @param apply      Callback to apply ops in COMMIT phase
     * @param release    Callback to release locks in ABORT/after COMMIT
     * @throws std::runtime_error if prepared WAL state cannot be restored safely.
     */
    explicit TwoPhaseCommitParticipant(
        const std::string&          shard_id,
        const Config&               config,
        ValidateAndLockCallback     validate = nullptr,
        ApplyOperationsCallback     apply    = nullptr,
        ReleaseLockCallback         release  = nullptr
    );

    ~TwoPhaseCommitParticipant() override = default;

    // ── ShardRPCServer::RequestHandler ──────────────────────────────────────

    /**
     * @brief Handle PREPARE request (2PC Phase 1)
     *
     * Validates operations, acquires locks, writes PREPARE_TX to WAL,
     * and returns VOTE_COMMIT (true) or VOTE_ABORT (false). When WAL is
     * configured, a failed durable PREPARE write releases acquired locks and
     * returns VOTE_ABORT. Missing WAL, any storage/lock callback, or empty
     * identifiers reject the request before validation/locking; no volatile
     * COMMIT vote is permitted.
     * Idempotent: a duplicate PREPARE returns the original vote without
     * re-locking only when operations, coordinator, target shard and fencing
     * epoch match the recorded request; mismatches return false.
    * @param transaction_id Non-empty globally unique transaction ID.
    * @param coordinator_shard_id Non-empty coordinator identity matching the token issuer.
    * @param transaction_data Serialisiertes JSON mit Teilnehmeroperationen.
    * @return true fuer VOTE_COMMIT, false fuer VOTE_ABORT.
     */
    bool onPrepare(
        const std::string& transaction_id,
        const std::string& coordinator_shard_id,
        const std::string& transaction_data
    ) override;

    /**
     * @brief Handle COMMIT request (2PC Phase 2)
     *
     * Applies buffered operations using the MVCC commit timestamp, durably
     * writes COMMIT_TX before acknowledging the request, and then releases locks.
     * If the WAL write fails, returns false and retains the prepared state.
     * Idempotent: duplicate COMMIT messages return true immediately.
     * Missing WAL, empty IDs and contradictory terminal decisions return false.
     * A missing apply callback leaves a recovered PREPARE unresolved and returns
     * false without writing COMMIT; an already durable COMMIT remains idempotent.
    * @param transaction_id Non-empty globally unique transaction ID.
    * @return true bei erfolgreichem (oder idempotent bereits erfolgtem) COMMIT.
     */
    bool onCommit(const std::string& transaction_id) override;
    bool onCommitFromCoordinator(
        const std::string& transaction_id,
        const std::string& coordinator_shard_id
    ) override;

    /**
     * @brief Handle ABORT request
     *
     * Durably writes ABORT_TX before releasing locks and acknowledging.
     * Idempotent: duplicate ABORT messages return true immediately.
     * Missing WAL, empty IDs and already committed transactions return false.
    * @param transaction_id Non-empty globally unique transaction ID.
    * @return true bei erfolgreichem (oder idempotent bereits erfolgtem) ABORT.
     */
    bool onAbort(const std::string& transaction_id) override;
    bool onAbortFromCoordinator(
        const std::string& transaction_id,
        const std::string& coordinator_shard_id
    ) override;

    /**
     * @brief Handle health-check request.
     * @return Unhealthy while WAL/lock recovery failed or a recovered, expired,
     *         or failed-apply PREPARE awaits a coordinator decision. Ordinary
     *         in-flight PREPAREs do not block readiness.
     * @note Decision RPCs remain available while reconciliation blocks readiness.
     *       Missing WAL or storage/lock callbacks block readiness and new PREPARE.
     */
    ShardRPCServer::HealthInfo onHealthCheck() override;

    // ── Participant-specific API ─────────────────────────────────────────────

    /**
     * @brief Return the current state of a prepared transaction, or nullopt
        * @param transaction_id Abzufragende Transaktions-ID.
        * @return Optionaler Teilnehmerzustand; nullopt falls unbekannt.
     */
    std::optional<ParticipantTxnState> getTransactionState(
        const std::string& transaction_id
    ) const;

    /**
     * @brief Report prepared transactions that have exceeded prepare_timeout.
     *
     * Should be called periodically (for example by maintenance/housekeeping).
     * An expired PREPARE blocks readiness and increments total_timeouts once.
     * It never records ABORT, releases locks, or overrides the coordinator:
     * after voting COMMIT only the authoritative coordinator may decide.
     * @return Always zero: no transaction is aborted by a timeout. The existing
     *         method name is retained for callers; inspect active_in_doubt and
     *         total_timeouts in getStatistics() for unresolved work.
     */
    size_t abortTimedOutTransactions();

    /**
     * @brief Recover state from WAL after a crash
     *
     * The constructor runs this recovery synchronously before the participant
     * can handle requests; callers may invoke it again to inspect the unresolved
     * count without restoring locks twice. The WAL manager streams every durable
     * record before trimming its bounded serving cache, retains terminal decisions
     * for idempotent redelivery, and recovery restores
     * locks for replayed PREPARED transactions before reporting completion.
     * In-doubt transactions (PREPARED with no COMMIT/ABORT decision) are
     * left in PREPARED state; the coordinator should re-resolve them.
     *
     * @return Number of in-doubt transactions found.
     * @throws std::exception when WAL reading, replay, or prepared lock
     *         restoration fails, including absent lock callbacks; callers must
     *         keep the participant unavailable until recovery succeeds.
     */
    size_t recoverFromWAL();

    /**
     * @brief Statistics about this participant
     * @return Counters, state counts, uptime, ready, recovery_status, and
     *         active_in_doubt (recovered/expired/failed-apply prepared transactions).
     *         recovery_complete means WAL replay and lock restoration completed,
     *         not that all coordinator decisions have been resolved.
     */
    nlohmann::json getStatistics() const;

private:
    bool onCommitInternal(
        const std::string& transaction_id,
        const std::optional<std::string>& coordinator_shard_id
    );
    bool onAbortInternal(
        const std::string& transaction_id,
        const std::optional<std::string>& coordinator_shard_id
    );

    const std::string shard_id_; ///< Lokale Shard-ID dieses Participants.
    Config            config_;   ///< Laufzeitkonfiguration (WAL/Timeout).

    // Missing storage callbacks disable PREPARE and readiness.
    ValidateAndLockCallback validate_and_lock_; ///< PREPARE-Validierung und Sperrverwaltung.
    ApplyOperationsCallback apply_operations_;  ///< COMMIT-Anwendung der gepufferten Operationen.
    ReleaseLockCallback     release_locks_;     ///< Freigabe gehaltener Sperren.

    // In-memory transaction state
    mutable std::mutex                         mutex_; ///< Schutz fuer Teilnehmerzustandsmap.
    std::map<std::string, ParticipantTransaction> transactions_; ///< In-Memory-Transaktionszustand.
    bool durability_failed_ = false;
    bool recovery_complete_ = true;
    [[nodiscard]] bool storageCallbacksConfigured() const noexcept {
        return validate_and_lock_ && apply_operations_ && release_locks_;
    }
    [[nodiscard]] size_t recoveryBlockersLocked() const; ///< Requires mutex_; counts unresolved recovery, not live prepares.

    // WAL for durable logging
    std::unique_ptr<WALManager> wal_; ///< Optionales WAL-Backend fuer Crash-Recovery.

    // Statistics
    std::atomic<uint64_t> total_prepares_{0}; ///< Anzahl verarbeiteter PREPARE-Aufrufe.
    std::atomic<uint64_t> total_commits_{0};  ///< Anzahl erfolgreicher COMMIT-Entscheidungen.
    std::atomic<uint64_t> total_aborts_{0};   ///< Anzahl vom Coordinator bestaetigter ABORT-Entscheidungen.
    std::atomic<uint64_t> total_timeouts_{0}; ///< Anzahl einmalig gemeldeter PREPARE-Timeouts, ohne Auto-Abort.

    // Startup time for uptime reporting
    const std::chrono::steady_clock::time_point start_time_{std::chrono::steady_clock::now()}; ///< Startzeit fuer Uptime-Berechnung.

    // ── Internal helpers ────────────────────────────────────────────────────

    /**
     * @brief Persist participant phase/state event to WAL if enabled.
     * @return True when the append and requested flush succeed.
     */
    bool logToWAL(WALEntryType type,
                  const std::string& txn_id,
                  const nlohmann::json& data,
                  bool force_sync = false);

    /**
     * @brief Apply a durable record during recovery.
     * @param entry Replayed record in WAL sequence order.
     * @throws std::runtime_error if terminal COMMIT/ABORT records contradict each other.
     */
    void applyRecoveredEntry(const WALEntry& entry);
};

} // namespace themis::sharding
