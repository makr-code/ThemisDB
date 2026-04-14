/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            two_phase_commit_coordinator.h                     ║
  Version:         0.0.29                                             ║
  Last Modified:   2026-04-14 18:43:28                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     303                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • e963d4e9ba  2026-04-14  fix(concurrency): eliminate deadlocks, blocking I/O under... ║
    • 71d99c4f28  2026-04-14  fix(concurrency): eliminate deadlocks, blocking I/O under... ║
    • 67965456c8  2026-03-22  Add constructors with default config for various classes ... ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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

#include "sharding/shard_rpc_server.h"
#include "sharding/shard_rpc_client.h"
#include "sharding/wal_manager.h"
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

    [[nodiscard]] bool committed() const {
        return result == CoordinatorTxnResult::COMMITTED;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Coordinator-side transaction state (persisted in WAL)
// ─────────────────────────────────────────────────────────────────────────────

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
class TwoPhaseCommitCoordinator {
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
     * @param config          Configuration (WAL, timeouts, …)
     */
    explicit TwoPhaseCommitCoordinator(
        const std::string& coordinator_id
    );

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
    size_t recoverInDoubtTransactions();

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
    const std::string coordinator_id_;
    Config            config_;

    mutable std::mutex mutex_;
    std::map<std::string, ShardRPCServer::RequestHandler*> participants_;
    std::map<std::string, CoordinatorTxnRecord>            transactions_;

    // Adapters created by registerParticipantByEndpoint() – owned by the coordinator
    std::map<std::string, std::unique_ptr<ShardRPCServer::RequestHandler>> owned_adapters_;

    // WAL for coordinator durability
    std::unique_ptr<WALManager> wal_;

    // Statistics
    std::atomic<uint64_t> total_transactions_{0};
    std::atomic<uint64_t> total_commits_{0};
    std::atomic<uint64_t> total_aborts_{0};
    std::atomic<uint64_t> total_errors_{0};

    // Startup time
    const std::chrono::steady_clock::time_point start_time_{
        std::chrono::steady_clock::now()
    };

    // ── Internal helpers ──────────────────────────────────────────────────────

    /// Run Phase 1: send PREPARE to all participants; return true if all agreed
    bool runPhase1(CoordinatorTxnRecord& rec);

    /// Run Phase 2: broadcast COMMIT or ABORT to all participants
    void runPhase2(CoordinatorTxnRecord& rec, bool commit);

    /// Build the serialised payload for a single shard
    static std::string buildPayload(const nlohmann::json& ops);

    /// Persist a coordinator WAL entry
    void logToWAL(WALEntryType type,
                  const std::string& txn_id,
                  const nlohmann::json& data);
};

} // namespace themis::sharding
