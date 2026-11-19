/**
 * @file two_phase_commit_coordinator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.34
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2025 ThemisDB
// Licensed under MIT License
//
// Two-Phase Commit (2PC) Coordinator
//
// Implements the coordinator role in 2PC distributed transactions across shards.
// The coordinator drives both phases of the protocol:
//
//   Phase 1 (PREPARE): Send PREPARE to every participant shard; collect votes.
//   Phase 2 (COMMIT/ABORT): If all votes are COMMIT → broadcast COMMIT;
//                            otherwise → broadcast ABORT.
//
// Durability: A configured WAL directory is required for writes. The coordinator writes its
// own WAL log (BEGIN_TX, PREPARE_SENT, COMMIT_DECISION / ABORT_DECISION,
// COMPLETE) so that an in-doubt transaction can be re-driven after restart.
//
// Participants are exposed via the ShardRPCServer::RequestHandler interface,
// meaning both in-process (unit-test) and real gRPC-backed participants can
// be used interchangeably.

#pragma once

// Prevent Windows macro pollution for enum members named FAILURE.
#ifdef FAILURE
#undef FAILURE
#endif

#include "sharding/shard_rpc_server.h"
#include "sharding/shard_rpc_client.h"
#include "sharding/epoch_fencing.h"
#include "transaction/recoverable_two_phase_coordinator.h"
#include "transaction/two_phase_commit_wal_recovery.h"
#include "sharding/wal_manager.h"
#ifdef FAILURE
#undef FAILURE
#endif
#include <string>
#include <map>
#include <vector>
#include <mutex>
#include <memory>
#include <atomic>
#include <chrono>
#include <functional>
#include <optional>
#include <set>
#include <nlohmann/json.hpp>

namespace themis::sharding {

// ─────────────────────────────────────────────────────────────────────────────
// Result types
// ─────────────────────────────────────────────────────────────────────────────

/** @brief Outcome of a two-phase commit transaction. */
enum class CoordinatorTxnResult {
    COMMITTED,  ///< All participants committed
    ABORTED,    ///< At least one participant voted ABORT (or error)
    FAILURE       ///< Internal error; transaction state unknown
};

/** @brief Detailed outcome returned by TwoPhaseCommitCoordinator::commit(). */
struct CoordinatorTxnOutcome {
    CoordinatorTxnResult result        = CoordinatorTxnResult::FAILURE;
    std::string          transaction_id;
    std::string          reason;        ///< Populated on ABORTED / FAILURE

    /** @brief Return true when outcome is final COMMITTED. */
    [[nodiscard]] bool committed() const {
        return result == CoordinatorTxnResult::COMMITTED;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Coordinator-side transaction state (persisted in WAL)
// ─────────────────────────────────────────────────────────────────────────────

/** @brief Coordinator-local lifecycle states persisted in WAL and memory. */
enum class CoordinatorTxnState {
    ACTIVE,              ///< Coordinator has started, Phase 1 not yet sent
    PREPARING,           ///< Phase 1 in progress (PREPARE sent, collecting votes)
    COMMIT_DECIDED,      ///< All voted COMMIT; Phase 2 COMMIT in progress
    ABORT_DECIDED,       ///< At least one voted ABORT; Phase 2 ABORT in progress
    COMPLETED,           ///< All participants acknowledged Phase 2; done
    FAILED,              ///< Unrecoverable error
    PREPARED             ///< All participants voted COMMIT; no decision recorded yet
};

/** @brief Per-transaction tracking record kept by the coordinator. */
struct CoordinatorTxnRecord {
    std::string         transaction_id;
    CoordinatorTxnState state   = CoordinatorTxnState::ACTIVE;
    bool                decision_commit = false; ///< Selected terminal decision for idempotent replies.
    std::chrono::steady_clock::time_point started_at;

    /// Ordered list of participant shard IDs used for Phase 2 replay after recovery.
    std::vector<std::string> participant_shards;

    /// Map: shard_id → serialised per-shard operations (JSON)
    std::map<std::string, std::string> shard_payloads;

    /// Map: shard_id → PREPARE vote (true=COMMIT, false=ABORT, absent=not-yet)
    std::map<std::string, bool>        votes;

    /// Shards that have acknowledged Phase 2
    std::vector<std::string>           phase2_acked;

    /// Existing normalized WAL timestamp, shared by every COMMIT delivery/replay.
    std::optional<int64_t> commit_timestamp_ns;
};

// ─────────────────────────────────────────────────────────────────────────────
// TwoPhaseCommitCoordinator
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Coordinator for cross-shard Two-Phase Commit transactions.
 *
 * Usage:
 * @code
 *   // durable_config supplies a WAL directory and current fencing credentials.
 *   TwoPhaseCommitCoordinator coord("coord-1", durable_config);
 *   coord.registerParticipant("shard-A", &participantA);
 *   coord.registerParticipant("shard-B", &participantB);
 *
 *   auto outcome = coord.commit("txn-42", {
 *       {"shard-A", nlohmann::json::array(…)},
 *       {"shard-B", nlohmann::json::array(…)},
 *   });
 *   assert(outcome.committed());
 * @endcode
 */
class TwoPhaseCommitCoordinator : public themis::transaction::IRecoverableTwoPhaseCoordinator {
public:
    /**
     * @brief Configuration for the coordinator.
     */
    struct Config {
        /// Durable WAL directory. Empty permits introspection only; writes fail closed.
        std::string wal_directory;

        /// How long to wait for a participant's PREPARE vote before aborting
        std::chrono::milliseconds prepare_timeout{30000};

        /// Sync non-mandatory WAL writes; BEGIN and decision records are always synced.
        bool sync_wal_writes = true;

        /// Supplies a trusted current-epoch token scoped to each target shard.
        /// Missing or invalid providers fail write transactions closed.
        std::function<std::optional<EpochToken>(const std::string&)> fencing_token_provider;

        /// Resolve trusted transport/TLS configuration for recovered shard IDs.
        /// Missing entries leave recovery blocked; no transport is invented.
        std::function<std::optional<ShardRPCClient::Config>(const std::string&)>
            participant_endpoint_resolver;

        /// Explicit test-endpoint handler; never used for production transports.
        ShardRPCClient::InProcessResponseHandler in_process_rpc_response_handler;

        /// Optional shared MVCC timestamp allocator, called once before COMMIT.
        /// Positive timestamps use the existing commit_timestamp_ns WAL field;
        /// endpoint replay reuses it and never allocates a replacement.
        std::function<std::optional<int64_t>(const std::string&)>
            commit_timestamp_provider;
    };

    /**
     * @brief Construct a coordinator.
     *
     * @param coordinator_id  Unique identifier for this coordinator instance
     * @note With no WAL configuration this instance supports introspection only;
     *       use the Config overload to enable durable transaction writes.
     */
    explicit TwoPhaseCommitCoordinator(
        const std::string& coordinator_id
    );

    /**
     * @brief Construct a coordinator.
     *
     * @param coordinator_id  Unique identifier for this coordinator instance
     * @param config          Configuration (WAL, timeouts, …)
     */
    explicit TwoPhaseCommitCoordinator(
        const std::string& coordinator_id,
        const Config&      config
    );

    ~TwoPhaseCommitCoordinator() = default;

    // Disable copy; allow move
    TwoPhaseCommitCoordinator(const TwoPhaseCommitCoordinator&)            = delete;
    TwoPhaseCommitCoordinator& operator=(const TwoPhaseCommitCoordinator&) = delete;

    // ── Participant management ────────────────────────────────────────────────

    /**
     * @brief Register a remote participant shard by network endpoint.
     *
     * Creates a ShardRPCClientAdapter internally that translates
     * RequestHandler calls into ShardRPCClient network calls.
     * The adapter's lifetime is tied to this coordinator instance.
     *
     * @param shard_id   Unique identifier of the shard
     * @param rpc_config ShardRPCClient configuration (endpoint, TLS, timeouts)
     * @throws std::invalid_argument for empty IDs, malformed endpoints or test
     *         endpoints without an explicit handler.
     * @throws std::logic_error if a protocol RPC or recovery is currently active.
     */
    void registerParticipantByEndpoint(
        const std::string&           shard_id,
        const ShardRPCClient::Config& rpc_config
    );

    /**
     * @brief Register a participant shard.
     *
     * Participants must remain valid for the lifetime of the coordinator
     * (or until explicitly unregistered).
     *
     * @param shard_id    Unique identifier of the shard
     * @param participant Pointer to the participant (in-process or gRPC proxy)
     * @throws std::invalid_argument for an empty ID or null participant.
     * @throws std::logic_error if a protocol RPC or recovery is currently active.
     */
    void registerParticipant(
        const std::string&               shard_id,
        ShardRPCServer::RequestHandler*  participant
    );

    /**
     * @brief Unregister a previously registered participant.
     * @return true if found and removed; false when absent or protocol RPCs/
     *         recovery still borrow the registered participant.
     */
    bool unregisterParticipant(const std::string& shard_id);

    /**
     * @brief Return the number of registered participants.
     */
    size_t participantCount() const;

    // ── Core 2PC protocol ────────────────────────────────────────────────────

    /**
     * @brief Execute a two-phase commit for a cross-shard transaction.
     *
     * This method drives both phases synchronously:
     * 1. Sends PREPARE to each shard listed in @p ops_per_shard.
     * 2. If all vote COMMIT → sends COMMIT to all; otherwise ABORT to all.
     * BEGIN, the final decision, and completion are
     * synchronously persisted before any participant RPC in the corresponding
     * phase. An empty WAL directory rejects writes before participant RPCs.
     *
     * Only shards present in @p ops_per_shard participate.  Shards not
     * listed are not contacted.
     *
     * @param transaction_id  Globally unique transaction identifier
     * @param ops_per_shard   Map from shard_id → JSON array of operations
     * @return COMMITTED only after every participant acknowledges COMMIT;
     *         ABORTED when prepare selects ABORT and phase two completes;
     *         FAILURE when validation, decision persistence, or a phase-two
     *         acknowledgement fails. After a durable decision, FAILURE leaves
     *         that decision authoritative for recovery; COMMIT is never
     *         changed to ABORT.
     */
    CoordinatorTxnOutcome commit(
        const std::string&                             transaction_id,
        const std::map<std::string, nlohmann::json>&   ops_per_shard
    );

    /**
     * @brief Durably begin and prepare without selecting a final decision.
     * @param transaction_id Non-empty globally unique ID.
     * @param ops_per_shard Non-empty map of registered shards to operation arrays.
     * @return True only when all shards voted COMMIT; false on validation,
     *         durability or RPC failure. A failed vote drives durable ABORT.
     * @note Call commit() or abort() to finish; restart recovery aborts a
     *       prepared transaction with no recorded decision. Concurrent drivers
     *       for the same ID fail closed.
     */
    [[nodiscard]] bool prepare(
        const std::string& transaction_id,
        const std::map<std::string, nlohmann::json>& ops_per_shard);

    /**
     * @brief Durably select ABORT and obtain every participant acknowledgement.
     * @param transaction_id Non-empty globally unique ID.
     * @param participant_shards Non-empty unique registered participant IDs;
     *        must match any previously prepared transaction.
     * @return ABORTED only after durable completion; FAILURE on missing WAL,
     *         invalid identities, unavailable participants or incomplete delivery.
     *         An existing COMMIT decision is never overwritten.
     */
    [[nodiscard]] CoordinatorTxnOutcome abort(
        const std::string& transaction_id,
        const std::vector<std::string>& participant_shards);

    /**
     * @brief Check whether durable coordination may accept new transactions.
     * @return False without WAL, during failed/blocked recovery, or after live
     *         decision delivery/completion failure. Prepared live transactions
     *         alone do not block readiness.
     */
    [[nodiscard]] bool isReady() const;

    // ── Recovery ─────────────────────────────────────────────────────────────

    /**
     * @brief Re-drive in-doubt transactions from the WAL.
     *
     * Call after a crash/restart, before accepting new transactions. Reads the
     * coordinator WAL and re-sends a recorded Phase 2 decision to participants.
     * If no decision was recorded, recovery conservatively selects ABORT.
     * Repeated delivery is expected to be idempotent.
     *
     * @return Number of transactions whose phase-two acknowledgements all
     *         succeeded and whose completion record was written successfully.
     * @throws std::runtime_error if WAL durability/replay fails or a recovered
     *         transaction lacks valid participant identities. Callers must
     *         keep service unavailable; a zero resolved count is not success.
     * @note An ambiguous live WAL append/flush fault requires restart and a
     *       validated rescan before recovery; it cannot select a new decision.
     */
    [[nodiscard]] size_t recoverInDoubtTransactions() override;

    /**
     * @brief Return the canonical coordinator name for global recovery reports.
     * @return Stable coordinator type name.
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
    [[nodiscard]] std::vector<themis::transaction::RecoverableTwoPhaseTransaction>
    getRecoverableTransactions() const override;

    // ── Introspection ─────────────────────────────────────────────────────────

    /**
     * @brief Return the current state of a transaction, if tracked.
     */
    std::optional<CoordinatorTxnState> getTransactionState(
        const std::string& transaction_id
    ) const;

    /**
     * @brief Inspect the authoritative decision without driving participant RPCs.
     * @param transaction_id Transaction ID to inspect.
     * @return True for COMMIT, false for ABORT, or nullopt for unknown IDs or
     *         transactions without a successfully persisted decision.
     */
    [[nodiscard]] std::optional<bool> getTransactionDecision(
        const std::string& transaction_id) const;

    /**
     * @brief Snapshot authoritative votes, acknowledgements and lifecycle metadata.
     * @param transaction_id ID of the transaction to inspect.
     * @return An owned record copy, or nullopt when unknown. No RPC is performed;
     *         callers must not infer completion from an individual acknowledgement.
     */
    [[nodiscard]] std::optional<CoordinatorTxnRecord> getTransactionRecord(
        const std::string& transaction_id) const;

    /**
     * @brief Return coordinator statistics as a JSON object.
     * @return Counters plus `recovery_status` (`not_run`, `running`, `complete`,
     *         `blocked`, or `failed`) and `recovery_failed`; failed replay remains
     *         visible even when it could not reconstruct a transaction.
     *         `wal_recovery_status` reports the canonical WAL scan result,
     *         including `truncated_tail` when the valid prefix was recovered.
     */
    nlohmann::json getStatistics() const;

private:
    const std::string coordinator_id_; ///< Eindeutige Coordinator-Identitaet fuer WAL/Metriken.
    Config            config_;         ///< Laufzeitkonfiguration (Timeouts, WAL-Optionen).

    mutable std::timed_mutex mutex_; ///< Schuetzt Teilnehmer- und Transaktionsregister.
    std::map<std::string, ShardRPCServer::RequestHandler*> participants_; ///< Registrierte Teilnehmer nach Shard-ID.
    std::map<std::string, CoordinatorTxnRecord>            transactions_; ///< Laufende und abgeschlossene Coordinator-Records.
    std::set<std::string> active_drivers_; ///< Reject concurrent protocol drivers for one ID.
    themis::transaction::TwoPhaseCommitWALRecovery::RecoveredTransactions recovered_wal_transactions_;
    mutable std::mutex recovery_history_mutex_; ///< Serializes WAL append and replay-state updates.
    bool wal_write_failed_ = false; ///< Ambiguous durability faults require restart/rescan; guarded by recovery_history_mutex_.
    bool recovery_failed_ = false;
    std::string recovery_status_ = "not_run";

    // Adapters created by registerParticipantByEndpoint() – owned by the coordinator
    std::map<std::string, std::unique_ptr<ShardRPCServer::RequestHandler>> owned_adapters_; ///< Eigentum an endpoint-basierten RPC-Adaptern.

    // WAL for coordinator durability
    std::unique_ptr<WALManager> wal_; ///< Optionales Coordinator-WAL fuer Crash-Recovery.

    // Statistics
    std::atomic<uint64_t> total_transactions_{0}; ///< Anzahl gestarteter Transaktionen.
    std::atomic<uint64_t> total_commits_{0};      ///< Anzahl finaler COMMIT-Ergebnisse.
    std::atomic<uint64_t> total_aborts_{0};       ///< Anzahl finaler ABORT-Ergebnisse.
    std::atomic<uint64_t> total_errors_{0};       ///< Anzahl interner Fehlerfaelle.

    // Startup time
    const std::chrono::steady_clock::time_point start_time_{
        std::chrono::steady_clock::now()
    };

    // ── Internal helpers ──────────────────────────────────────────────────────

    CoordinatorTxnOutcome execute(
        const std::string& transaction_id,
        const std::map<std::string, nlohmann::json>& ops_per_shard,
        bool prepare_only, bool abort_only);

    /// Build production/test endpoint transport, preserving recorded MVCC timestamps.
    std::unique_ptr<ShardRPCServer::RequestHandler> makeEndpointParticipant(
        const ShardRPCClient::Config& rpc_config);

    /// Run Phase 1: send PREPARE to all participants; return true if all agreed.
    /// @param lock  A held unique_lock on mutex_. It is briefly released around
    ///              each blocking RPC call and re-acquired before returning
    ///              (2PC-1 fix: avoid holding mutex_ during network I/O).
    bool runPhase1(CoordinatorTxnRecord& rec, std::unique_lock<std::timed_mutex>& lock);

    /// Run Phase 2: broadcast COMMIT or ABORT to all participants.
    /// @param lock  Same as runPhase1 — released around each RPC, re-acquired.
    /// @return True only when every registered participant acknowledges the decision.
    bool runPhase2(CoordinatorTxnRecord& rec, bool commit, std::unique_lock<std::timed_mutex>& lock);

    /// Build the serialised payload for a single shard.
    /// @param ops JSON-Operationen fuer einen Teilnehmer.
    /// @param token Current fencing credential scoped to that participant.
    /// @return Transportpayload fuer onPrepare().
    static std::string buildPayload(const nlohmann::json& ops, const EpochToken& token);

    /// Persist a coordinator WAL entry.
    /// @param type WAL-Eintragstyp.
    /// @param txn_id Betroffene Transaktions-ID.
    /// @param data Zusaetzliche WAL-Nutzdaten.
    /// @param force_sync Require a successful flush regardless of configuration.
    /// @return True only when the requested append and required flush succeed.
    [[nodiscard]] bool logToWAL(WALEntryType type,
                                const std::string& txn_id,
                                const nlohmann::json& data,
                                bool force_sync = false);
};

} // namespace themis::sharding
