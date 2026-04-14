/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            distributed_transaction_manager.h                  ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-04-14 06:57:49                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     568                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • ff299c514b  2026-04-09  feat(transaction): PERF-D4 batched prepare + lock-free 2P... ║
    • 0f0c408c2f  2026-03-15  feat(transaction): implement Distributed Transaction Coor... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Copyright 2025 ThemisDB
// Licensed under MIT License
//
// DistributedTransactionManager — 2PC coordinator for multi-shard transactions
//
// Implements a Two-Phase Commit (2PC) protocol for ACID distributed
// transactions spanning multiple ThemisDB shards.  Each shard exposes an
// IDistributedParticipantCallback interface; the coordinator drives
// PREPARE → COMMIT/ABORT in lock-step with:
//   - Timeout-based abort for network partition tolerance
//   - Persistent WAL log for coordinator crash recovery
//   - Parallel prepare/commit for lower latency
//   - Participant recovery via WAL replay
//
// Protocol Flow:
//   1. beginDistributed(participants)  – register participants, obtain TXN-ID
//   2. prepareDistributed(txn_id)      – Phase 1: send PREPARE to all participants
//                                        each votes COMMIT or ABORT (with timeout)
//   3. commitDistributed(txn_id)       – Phase 2: all voted YES → send COMMIT
//      OR abortDistributed(txn_id)     – Phase 2: any voted NO  → send ABORT
//
// Participant API (called by each shard on receive):
//   voteOnPrepare(txn_id, node_id, can_commit) – register vote from participant
//   applyCommit(txn_id)                        – apply committed operations
//   applyAbort(txn_id)                         – discard prepared operations
//
// Recovery:
//   recoverInDoubtTransactions() – re-drive in-doubt txns after coordinator restart
//
// Thread-safety:
//   DistributedTransactionManager is thread-safe for concurrent callers.
//   Individual DistributedTransaction records must only be modified under
//   the coordinator's internal lock.

#pragma once

#include "sharding/wal_manager.h"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <set>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace themis::transaction {

// ─────────────────────────────────────────────────────────────────────────────
// Participant callback interface
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Interface each shard participant must implement for 2PC coordination.
 *
 * Concrete implementations may wrap an in-process shard store or a remote
 * RPC stub.  Returning false from onPrepare() votes ABORT for the entire
 * transaction.
 */
class IDistributedParticipantCallback {
public:
    virtual ~IDistributedParticipantCallback() = default;

    /**
     * @brief Phase 1 — validate operations, acquire locks, write PREPARE log.
     *
     * @param txn_id        Globally unique transaction identifier.
     * @param affected_keys Keys this participant must lock for the transaction.
     * @return true → vote COMMIT; false → vote ABORT.
     */
    virtual bool onPrepare(
        const std::string&              txn_id,
        const std::set<std::string>&    affected_keys
    ) = 0;

    /**
     * @brief Phase 2 (commit path) — apply prepared operations, release locks.
     *
     * Called only when every participant voted COMMIT.
     *
     * @param txn_id  Transaction to commit.
     */
    virtual void onCommit(const std::string& txn_id) = 0;

    /**
     * @brief Phase 2 (abort path) — discard prepared operations, release locks.
     *
     * @param txn_id  Transaction to abort.
     */
    virtual void onAbort(const std::string& txn_id) = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// Participant descriptor
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Descriptor for a shard that participates in a distributed transaction.
 */
struct Participant {
    /// Logical node/shard identifier (used in log entries and vote tracking).
    std::string node_id;

    /// Network endpoint ("host:port") — used when sending RPCs remotely.
    /// May be empty for in-process participants.
    std::string endpoint;

    /// Keys on this participant that are touched by the transaction.
    std::set<std::string> affected_keys;

    /// In-process callback; null for remote-only participants.
    IDistributedParticipantCallback* callback = nullptr;
};

// ─────────────────────────────────────────────────────────────────────────────
// Transaction state
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Lifecycle state of a distributed transaction.
 */
enum class DistributedTxnState {
    INIT,       ///< Registered; prepare not yet sent
    PREPARING,  ///< Phase 1 in progress — awaiting all votes
    PREPARED,   ///< All participants voted COMMIT; ready to commit
    COMMITTING, ///< Phase 2 COMMIT in progress
    COMMITTED,  ///< All participants committed successfully
    ABORTING,   ///< Phase 2 ABORT in progress
    ABORTED,    ///< Transaction aborted (vote NO, timeout, or explicit abort)
};

// ─────────────────────────────────────────────────────────────────────────────
// Transaction record
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Internal record for a distributed transaction tracked by the coordinator.
 */
struct DistributedTransaction {
    using TransactionId = std::string;

    TransactionId              txn_id;
    std::vector<Participant>   participants;
    DistributedTxnState        state = DistributedTxnState::INIT;

    /// Absolute deadline; coordinator aborts on timeout.
    std::chrono::system_clock::time_point timeout;

    /// When the transaction was begun.
    std::chrono::system_clock::time_point created_at;

    /// Per-node vote results collected during Phase 1.
    std::map<std::string, bool> votes;

    /// Nodes that have successfully acknowledged Phase-2 COMMIT.
    std::set<std::string> committed_nodes;

    /// Nodes that have successfully acknowledged Phase-2 ABORT.
    std::set<std::string> aborted_nodes;

    /// Human-readable error detail (set on failure).
    std::string error_detail;
};

// ─────────────────────────────────────────────────────────────────────────────
// Status
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Return type for coordinator and participant operations.
 */
struct DistributedTxnStatus {
    bool        ok      = true;
    std::string message;

    static DistributedTxnStatus OK() { return {}; }
    static DistributedTxnStatus Error(std::string msg) {
        DistributedTxnStatus s;
        s.ok      = false;
        s.message = std::move(msg);
        return s;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Configuration
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Configuration for DistributedTransactionManager.
 */
struct DistributedTxnManagerConfig {
    /// Maximum time allowed for Phase 1 (prepare votes from all participants).
    std::chrono::milliseconds prepare_timeout{5000};

    /// Maximum time allowed for Phase 2 (commit/abort acknowledgement).
    std::chrono::milliseconds commit_timeout{5000};

    /// Default transaction lifetime before timeout-based abort.
    std::chrono::milliseconds default_txn_timeout{30000};

    /// Directory for the WAL (empty = WAL disabled; coordinator crashes are not
    /// recoverable without WAL).
    std::string wal_directory;

    /// Flush WAL synchronously on every write.
    bool sync_wal_writes = true;

    /// Maximum number of concurrent transactions tracked in memory.
    size_t max_active_transactions = 10000;

    // ── Performance / PERF-D4 ────────────────────────────────────────────────

    /// Batched-prepare window.  When > 0ms the coordinator accumulates
    /// prepareDistributed() calls for this window before flushing them all in
    /// one parallel Phase-1 sweep (amortises per-transaction overhead).
    /// 0ms = immediate (no batching); recommended range 10-100ms.
    std::chrono::milliseconds prepare_batch_window{0};

    /// Number of worker threads in the internal thread pool used to dispatch
    /// prepare/commit/abort calls to participants in parallel.
    /// 0 = fall back to std::async per call (legacy behaviour).
    /// Default: 4 (good for typical 2-8 shard deployments).
    size_t worker_thread_count = 4;
};

// ─────────────────────────────────────────────────────────────────────────────
// DistributedTransactionManager
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Two-Phase Commit coordinator for multi-shard distributed transactions
 *        (v1.9.0).
 *
 * Manages participant registration and drives the full 2PC lifecycle across
 * registered IDistributedParticipantCallback instances.
 *
 * Thread-safety: All public methods are thread-safe.
 */
class DistributedTransactionManager {
public:
    using TransactionId = std::string;

    /**
     * @brief Coordinator statistics snapshot (approximate).
     */
    struct Statistics {
        uint64_t total_transactions = 0;
        uint64_t committed          = 0;
        uint64_t aborted            = 0;
        uint64_t timeout_aborts     = 0;
        uint64_t recovered          = 0;
        uint64_t in_doubt           = 0;
    };

    /**
     * @brief Construct a new 2PC coordinator.
     *
     * @param coordinator_id  Unique name for this coordinator instance.
     * @param config          Configuration (timeouts, WAL directory, …).
     */
    explicit DistributedTransactionManager(
        std::string                       coordinator_id,
        DistributedTxnManagerConfig       config = {}
    );

    ~DistributedTransactionManager();

    // Non-copyable, non-moveable
    DistributedTransactionManager(const DistributedTransactionManager&)            = delete;
    DistributedTransactionManager& operator=(const DistributedTransactionManager&) = delete;
    DistributedTransactionManager(DistributedTransactionManager&&)                 = delete;
    DistributedTransactionManager& operator=(DistributedTransactionManager&&)      = delete;

    // ── Coordinator API ───────────────────────────────────────────────────────

    /**
     * @brief Begin a new distributed transaction.
     *
     * Registers the transaction and all its participants.  No messages are
     * sent until prepareDistributed() is called.
     *
     * @param participants  Non-empty list of participating shards.
     * @return              Globally unique transaction ID.
     * @throws std::invalid_argument if participants is empty.
     */
    TransactionId beginDistributed(const std::vector<Participant>& participants);

    /**
     * @brief Run Phase 1 — send PREPARE to all participants and collect votes.
     *
     * Sends PREPARE (via IDistributedParticipantCallback::onPrepare) to all
     * registered participants in parallel, then waits for all votes up to
     * config.prepare_timeout.  If all vote COMMIT the transaction advances to
     * PREPARED state and the method returns OK.  If any participant votes ABORT
     * or the timeout fires the transaction is immediately aborted.
     *
     * @param txn_id  Transaction returned by beginDistributed().
     * @return        OK if all participants voted COMMIT; Error otherwise.
     */
    DistributedTxnStatus prepareDistributed(const TransactionId& txn_id);

    /**
     * @brief Run Phase 2 (commit path) — send COMMIT to all prepared participants.
     *
     * Must only be called after prepareDistributed() returns OK.  Sends COMMIT
     * to all participants that voted YES and waits for acknowledgement up to
     * config.commit_timeout.
     *
     * Durably logs COMMIT_TX to the WAL before broadcasting to participants so
     * the decision survives coordinator crashes.
     *
     * @param txn_id  Transaction in PREPARED state.
     * @return        OK if all participants committed; Error if any failed.
     */
    DistributedTxnStatus commitDistributed(const TransactionId& txn_id);

    /**
     * @brief Abort a transaction — send ABORT to all participants.
     *
     * May be called from any state (INIT, PREPARING, PREPARED, or after a
     * partial commit failure).  No-op if the transaction is already COMMITTED
     * or ABORTED.
     *
     * Durably logs ABORT_TX to the WAL before broadcasting to participants.
     *
     * @param txn_id  Transaction to abort.
     */
    void abortDistributed(const TransactionId& txn_id);

    // ── Participant API ───────────────────────────────────────────────────────

    /**
     * @brief Register a vote from a participant in response to a PREPARE.
     *
     * Used in asynchronous/remote deployments where a participant calls back
     * the coordinator with its vote.  For in-process participants the vote is
     * registered automatically by prepareDistributed().
     *
     * @param txn_id    Transaction being voted on.
     * @param node_id   Participant node identifier.
     * @param can_commit true → COMMIT vote; false → ABORT vote.
     * @return          OK on success; Error if the transaction is not found or
     *                  the vote arrives too late.
     */
    DistributedTxnStatus voteOnPrepare(
        const TransactionId& txn_id,
        const std::string&   node_id,
        bool                 can_commit
    );

    /**
     * @brief Acknowledge a COMMIT decision from the coordinator.
     *
     * Marks the given transaction as committed in the coordinator's record.
     * Typically called by remote participants after they have applied changes.
     *
     * @param txn_id  Transaction being acknowledged.
     * @return        OK on success; Error if the transaction is not found.
     */
    DistributedTxnStatus applyCommit(const TransactionId& txn_id);

    /**
     * @brief Acknowledge an ABORT decision from the coordinator.
     *
     * @param txn_id  Transaction being acknowledged.
     * @return        OK on success; Error if the transaction is not found.
     */
    DistributedTxnStatus applyAbort(const TransactionId& txn_id);

    // ── Recovery ─────────────────────────────────────────────────────────────

    /**
     * @brief Re-drive in-doubt transactions after a coordinator restart.
     *
     * Reads the WAL from the beginning, identifies transactions that were in
     * the PREPARED state when the coordinator crashed, and re-sends the Phase-2
     * decision (COMMIT or ABORT) to all participants that have registered
     * callbacks.
     *
     * Must be called once after construction, before accepting new transactions,
     * when WAL recovery is desired.
     *
     * @return Number of in-doubt transactions resolved.
     */
    size_t recoverInDoubtTransactions();

    // ── Timeout handling ──────────────────────────────────────────────────────

    /**
     * @brief Scan active transactions and abort any that have exceeded their
     *        deadline.
     *
     * This method is non-blocking and returns the number of transactions
     * aborted.  Callers should invoke it periodically (e.g. from a background
     * thread or a heartbeat).
     *
     * @return Number of transactions aborted due to timeout.
     */
    size_t checkTimeouts();

    // ── Failure detection ─────────────────────────────────────────────────────

    /**
     * @brief Check whether a participant appears to be reachable.
     *
     * For in-process participants with a non-null callback the check is always
     * true.  For remote participants (callback == nullptr) the method attempts
     * a lightweight ping via the endpoint (not implemented in this release —
     * returns true by default).
     *
     * @param node_id  Participant node identifier.
     * @return         true if the participant appears healthy.
     */
    bool isParticipantAlive(const std::string& node_id) const;

    // ── Introspection ─────────────────────────────────────────────────────────

    /**
     * @brief Return a copy of the transaction record, or std::nullopt if not found.
     */
    std::optional<DistributedTransaction> getTransaction(const TransactionId& txn_id) const;

    /**
     * @brief Return approximate coordinator statistics.
     */
    Statistics getStatistics() const;

    /**
     * @brief Return the number of currently active (non-terminal) transactions.
     */
    size_t activeTransactionCount() const;

private:
    // ── Internal helpers ──────────────────────────────────────────────────────

    /// Generate a globally-unique transaction identifier.
    std::string generateTransactionId();

    /// Persist a WAL entry; no-op when WAL is disabled.
    void logToWAL(
        themis::sharding::WALEntryType type,
        const std::string&             txn_id,
        const std::string&             data = ""
    );

    /// Run Phase 1: send PREPARE to all participants (mutex NOT held).
    bool runPhase1Unlocked(const TransactionId& txn_id);

    /// Run Phase 2: send COMMIT or ABORT to all participants (mutex NOT held).
    void runPhase2Unlocked(
        const TransactionId&            txn_id,
        const std::vector<Participant>& parts,
        bool                            do_commit
    );

    /// Find a transaction record; returns nullptr if not found (caller must hold mutex_).
    DistributedTransaction* findTransaction(const TransactionId& txn_id);
    const DistributedTransaction* findTransaction(const TransactionId& txn_id) const;

    // ── Thread pool (PERF-D4) ─────────────────────────────────────────────────

    /// Submit a callable to the thread pool; returns a future for the result.
    /// Falls back to std::async when worker_thread_count == 0.
    template<class F>
    auto submitTask(F&& f) -> std::future<decltype(f())> {
        using R = decltype(f());
        if (config_.worker_thread_count == 0) {
            // Legacy: one-shot thread via std::async.
            return std::async(std::launch::async, std::forward<F>(f));
        }
        auto task_ptr = std::make_shared<std::packaged_task<R()>>(std::forward<F>(f));
        std::future<R> fut = task_ptr->get_future();
        {
            std::lock_guard<std::mutex> lock(pool_mutex_);
            if (!pool_stop_) {
                task_queue_.push([task_ptr]() { (*task_ptr)(); });
            } else {
                // Pool is stopping – run inline.
                (*task_ptr)();
                return fut;
            }
        }
        pool_cv_.notify_one();
        return fut;
    }

    /// Start the background worker threads (called from constructor).
    void startThreadPool();

    /// Stop the background worker threads (called from destructor).
    void stopThreadPool();

    // ── Batch-prepare flush (PERF-D4) ─────────────────────────────────────────

    /// Entry in the pending-prepare batch queue.
    struct BatchPrepareEntry {
        TransactionId          txn_id;
        std::promise<bool>     result;
    };

    /// Background loop that drains the batch queue every prepare_batch_window.
    void batchFlushLoop();

    const std::string             coordinator_id_;
    DistributedTxnManagerConfig   config_;

    mutable std::mutex                                        mutex_;
    std::condition_variable                                   vote_cv_;
    std::unordered_map<TransactionId, DistributedTransaction> transactions_;

    std::unique_ptr<themis::sharding::WALManager>        wal_;

    // Approximate statistics (relaxed ordering — monitoring only).
    std::atomic<uint64_t> stat_total_{0};
    std::atomic<uint64_t> stat_committed_{0};
    std::atomic<uint64_t> stat_aborted_{0};
    std::atomic<uint64_t> stat_timeout_aborts_{0};
    std::atomic<uint64_t> stat_recovered_{0};

    std::atomic<uint64_t> txn_counter_{0};

    // ── Thread pool state ─────────────────────────────────────────────────────
    std::vector<std::thread>              worker_threads_;
    std::queue<std::function<void()>>     task_queue_;
    std::mutex                            pool_mutex_;
    std::condition_variable               pool_cv_;
    bool                                  pool_stop_{false};

    // ── Batch-prepare state ───────────────────────────────────────────────────
    std::mutex                            batch_mutex_;
    std::condition_variable               batch_cv_;
    std::vector<BatchPrepareEntry>        batch_queue_;
    std::thread                           batch_flush_thread_;
    std::atomic<bool>                     batch_stop_{false};
};

} // namespace themis::transaction
