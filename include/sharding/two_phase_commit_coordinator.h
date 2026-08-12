/**
 * @file two_phase_commit_coordinator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.34
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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
// Durability: Coordinator writes its own WAL log (BEGIN_TX, PREPARE_SENT,
// COMMIT_DECISION / ABORT_DECISION, COMPLETE) so that an in-doubt transaction
// can be re-driven to completion after a coordinator restart.
//
// Participants are exposed via the ShardRPCServer::RequestHandler interface,
// meaning both in-process (unit-test) and real gRPC-backed participants can
// be used interchangeably.

#pragma once

// Prevent Windows macro pollution for enum members named ERROR.
#ifdef ERROR
#undef ERROR
#endif

#include "sharding/shard_rpc_server.h"
#include "sharding/shard_rpc_client.h"
#include "transaction/recoverable_two_phase_coordinator.h"
#include "sharding/wal_manager.h"
#ifdef ERROR
#undef ERROR
#endif
#include <string>
#include <map>
#include <vector>
#include <mutex>
#include <memory>
#include <atomic>
#include <chrono>
#include <optional>
#include <nlohmann/json.hpp>

namespace themis::sharding {

// ─────────────────────────────────────────────────────────────────────────────
// Result types
// ─────────────────────────────────────────────────────────────────────────────

/** @brief Outcome of a two-phase commit transaction. */
enum class CoordinatorTxnResult {
    COMMITTED,  ///< All participants committed
    ABORTED,    ///< At least one participant voted ABORT (or error)
    ERROR       ///< Internal error; transaction state unknown
};

/** @brief Detailed outcome returned by TwoPhaseCommitCoordinator::commit(). */
struct CoordinatorTxnOutcome {
    CoordinatorTxnResult result        = CoordinatorTxnResult::ERROR;
    std::string          transaction_id;
    std::string          reason;        ///< Populated on ABORTED / ERROR

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
    FAILED               ///< Unrecoverable error
};

/** @brief Per-transaction tracking record kept by the coordinator. */
struct CoordinatorTxnRecord {
    std::string         transaction_id;
    CoordinatorTxnState state   = CoordinatorTxnState::ACTIVE;
    std::chrono::steady_clock::time_point started_at;

    /// Ordered list of participant shard IDs used for Phase 2 replay after recovery.
    std::vector<std::string> participant_shards;

    /// Map: shard_id → serialised per-shard operations (JSON)
    std::map<std::string, std::string> shard_payloads;

    /// Map: shard_id → PREPARE vote (true=COMMIT, false=ABORT, absent=not-yet)
    std::map<std::string, bool>        votes;

    /// Shards that have acknowledged Phase 2
    std::vector<std::string>           phase2_acked;
};

// ─────────────────────────────────────────────────────────────────────────────
// TwoPhaseCommitCoordinator
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Coordinator for cross-shard Two-Phase Commit transactions.
 *
 * Usage:
 * @code
 *   TwoPhaseCommitCoordinator coord("coord-1");
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
        /// WAL directory for durable logging; leave empty to disable WAL
        std::string wal_directory;

        /// How long to wait for a participant's PREPARE vote before aborting
        std::chrono::milliseconds prepare_timeout{30000};

        /// Flush WAL synchronously on every write
        bool sync_wal_writes = true;
    };

    /**
     * @brief Construct a coordinator.
     *
     * @param coordinator_id  Unique identifier for this coordinator instance
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
     */
    void registerParticipant(
        const std::string&               shard_id,
        ShardRPCServer::RequestHandler*  participant
    );

    /**
     * @brief Unregister a previously registered participant.
     * @return true if the shard was found and removed
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
     *
     * Only shards present in @p ops_per_shard participate.  Shards not
     * listed are not contacted.
     *
     * @param transaction_id  Globally unique transaction identifier
     * @param ops_per_shard   Map from shard_id → JSON array of operations
     * @return                Outcome with COMMITTED, ABORTED, or ERROR
     */
    CoordinatorTxnOutcome commit(
        const std::string&                             transaction_id,
        const std::map<std::string, nlohmann::json>&   ops_per_shard
    );

    // ── Recovery ─────────────────────────────────────────────────────────────

    /**
     * @brief Recover and re-drive in-doubt transactions from the WAL.
     *
     * Must be called once after a crash/restart, before accepting new
     * transactions.  Reads the coordinator WAL and re-sends the Phase 2
     * decision (COMMIT or ABORT) to all participants that have not yet
     * acknowledged.
     *
     * @return Number of in-doubt transactions resolved
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
     * @brief Return coordinator statistics as a JSON object.
     */
    nlohmann::json getStatistics() const;

private:
    const std::string coordinator_id_; ///< Eindeutige Coordinator-Identitaet fuer WAL/Metriken.
    Config            config_;         ///< Laufzeitkonfiguration (Timeouts, WAL-Optionen).

    mutable std::timed_mutex mutex_; ///< Schuetzt Teilnehmer- und Transaktionsregister.
    std::map<std::string, ShardRPCServer::RequestHandler*> participants_; ///< Registrierte Teilnehmer nach Shard-ID.
    std::map<std::string, CoordinatorTxnRecord>            transactions_; ///< Laufende und abgeschlossene Coordinator-Records.

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

    /// Run Phase 1: send PREPARE to all participants; return true if all agreed.
    /// @param lock  A held unique_lock on mutex_. It is briefly released around
    ///              each blocking RPC call and re-acquired before returning
    ///              (2PC-1 fix: avoid holding mutex_ during network I/O).
    bool runPhase1(CoordinatorTxnRecord& rec, std::unique_lock<std::timed_mutex>& lock);

    /// Run Phase 2: broadcast COMMIT or ABORT to all participants.
    /// @param lock  Same as runPhase1 — released around each RPC, re-acquired.
    void runPhase2(CoordinatorTxnRecord& rec, bool commit, std::unique_lock<std::timed_mutex>& lock);

    /// Build the serialised payload for a single shard.
    /// @param ops JSON-Operationen fuer einen Teilnehmer.
    /// @return Transportpayload fuer onPrepare().
    static std::string buildPayload(const nlohmann::json& ops);

    /// Persist a coordinator WAL entry.
    /// @param type WAL-Eintragstyp.
    /// @param txn_id Betroffene Transaktions-ID.
    /// @param data Zusaetzliche WAL-Nutzdaten.
    void logToWAL(WALEntryType type,
                  const std::string& txn_id,
                  const nlohmann::json& data);
};

} // namespace themis::sharding
