/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            two_phase_commit_participant.h                     ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:47:08                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     270                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • e963d4e9ba  2026-04-14  fix(concurrency): eliminate deadlocks, blocking I/O under... ║
    • 71d99c4f28  2026-04-14  fix(concurrency): eliminate deadlocks, blocking I/O under... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
    PREPARED,   // Voted COMMIT, waiting for coordinator decision
    COMMITTED,  // Coordinator said COMMIT, changes applied
    ABORTED     // Coordinator said ABORT (or timed out), changes discarded
};

/**
 * @brief Metadata tracked per prepared transaction at the participant
 */
struct ParticipantTransaction {
    std::string             transaction_id;
    ParticipantTxnState     state = ParticipantTxnState::PREPARED;
    nlohmann::json          operations;          // Buffered operations to apply on commit
    std::chrono::steady_clock::time_point prepared_at;
    int64_t                 prepare_timestamp = 0; // Coordinator timestamp
    int64_t                 commit_timestamp  = 0; // MVCC commit timestamp (set on COMMIT)
    bool                    vote_committed    = true; // Our vote in PREPARE phase
};

/**
 * @brief Callback for applying committed operations to the storage engine
 *
 * @param txn_id    Transaction ID
 * @param ops       JSON array of operations to apply
 * @param commit_ts MVCC commit timestamp
 * @return true if operations were applied successfully
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
 */
class TwoPhaseCommitParticipant : public ShardRPCServer::RequestHandler {
public:
    /**
     * @brief Configuration for the participant
     */
    struct Config {
        // WAL directory for durable logging; leave empty to disable WAL
        std::string wal_directory;

        // How long a PREPARED transaction may wait before being auto-aborted
        std::chrono::milliseconds prepare_timeout{60000};

        // Flush WAL synchronously on every write (safest; slightly slower)
        bool sync_wal_writes = true;
    };

    /**
     * @brief Construct a participant with optional storage callbacks
     *
     * If validate_and_lock / apply / release are left nullptr the participant
     * will accept all operations (useful in tests).
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
     * If validate_and_lock / apply / release are left nullptr the participant
     * will accept all operations (useful in tests).
     *
     * @param shard_id   Identifier of this shard
     * @param config     Participant configuration
     * @param validate   Callback to validate & lock ops in PREPARE phase
     * @param apply      Callback to apply ops in COMMIT phase
     * @param release    Callback to release locks in ABORT/after COMMIT
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
     * and returns VOTE_COMMIT (true) or VOTE_ABORT (false).
     * Idempotent: a duplicate PREPARE for the same txn_id returns the
     * same vote as the original call without re-locking.
     */
    bool onPrepare(
        const std::string& transaction_id,
        const std::string& coordinator_shard_id,
        const std::string& transaction_data
    ) override;

    /**
     * @brief Handle COMMIT request (2PC Phase 2)
     *
     * Applies buffered operations using the MVCC commit timestamp,
     * releases locks, and writes COMMIT_TX to WAL.
     * Idempotent: duplicate COMMIT messages return true immediately.
     */
    bool onCommit(const std::string& transaction_id) override;

    /**
     * @brief Handle ABORT request
     *
     * Releases locks and writes ABORT_TX to WAL.
     * Idempotent: duplicate ABORT messages return true immediately.
     */
    bool onAbort(const std::string& transaction_id) override;

    /**
     * @brief Handle health-check request
     */
    ShardRPCServer::HealthInfo onHealthCheck() override;

    // ── Participant-specific API ─────────────────────────────────────────────

    /**
     * @brief Return the current state of a prepared transaction, or nullopt
     */
    std::optional<ParticipantTxnState> getTransactionState(
        const std::string& transaction_id
    ) const;

    /**
     * @brief Abort prepared transactions that have exceeded prepare_timeout
     *
     * Should be called periodically (e.g. from a background thread).
     * @return Number of transactions timed out and aborted
     */
    size_t abortTimedOutTransactions();

    /**
     * @brief Recover state from WAL after a crash
     *
     * Must be called before the participant starts handling requests.
     * Re-populates the in-memory transaction map from durable WAL entries.
     * In-doubt transactions (PREPARED with no COMMIT/ABORT decision) are
     * left in PREPARED state; the coordinator should re-resolve them.
     *
     * @return Number of in-doubt transactions found
     */
    size_t recoverFromWAL();

    /**
     * @brief Statistics about this participant
     */
    nlohmann::json getStatistics() const;

private:
    const std::string shard_id_;
    Config            config_;

    // Storage callbacks (may be nullptr → accept-all default)
    ValidateAndLockCallback validate_and_lock_;
    ApplyOperationsCallback apply_operations_;
    ReleaseLockCallback     release_locks_;

    // In-memory transaction state
    mutable std::mutex                         mutex_;
    std::map<std::string, ParticipantTransaction> transactions_;

    // WAL for durable logging
    std::unique_ptr<WALManager> wal_;

    // Statistics
    std::atomic<uint64_t> total_prepares_{0};
    std::atomic<uint64_t> total_commits_{0};
    std::atomic<uint64_t> total_aborts_{0};
    std::atomic<uint64_t> total_timeouts_{0};

    // Startup time for uptime reporting
    const std::chrono::steady_clock::time_point start_time_{std::chrono::steady_clock::now()};

    // ── Internal helpers ────────────────────────────────────────────────────

    void logToWAL(WALEntryType type,
                  const std::string& txn_id,
                  const nlohmann::json& data);
};

} // namespace themis::sharding
