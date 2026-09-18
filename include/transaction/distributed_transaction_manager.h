/**
 * @file distributed_transaction_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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

#include "transaction/recoverable_two_phase_coordinator.h"
#include "sharding/wal_manager.h"
#include "utils/retry_contract.h"
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

class IDistributedParticipantCallback {
public:
    /**
     * @brief IDistributed Participant Callback.
     * @return Return value.
     */
    virtual ~IDistributedParticipantCallback() = default;

    [[nodiscard]] virtual bool onPrepare(
        const std::string&              txn_id,
        const std::set<std::string>&    affected_keys
    ) = 0;

    /**
     * @brief On Commit.
     * @param[in] txn_id Identifier of the txn.
     */
    virtual void onCommit(const std::string& txn_id) = 0;

    /**
     * @brief On Abort.
     * @param[in] txn_id Identifier of the txn.
     */
    virtual void onAbort(const std::string& txn_id) = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// Participant descriptor
// ─────────────────────────────────────────────────────────────────────────────

struct Participant {
    std::string node_id;

    std::string endpoint;

    std::set<std::string> affected_keys;

    IDistributedParticipantCallback* callback = nullptr;
};

// ─────────────────────────────────────────────────────────────────────────────
// Transaction state
// ─────────────────────────────────────────────────────────────────────────────

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

struct DistributedTransaction {
    using TransactionId = std::string;

    TransactionId              txn_id;
    std::vector<Participant>   participants;
    DistributedTxnState        state = DistributedTxnState::INIT;

    std::chrono::system_clock::time_point timeout;

    std::chrono::system_clock::time_point created_at;

    std::map<std::string, bool> votes;

    std::set<std::string> committed_nodes;

    std::set<std::string> aborted_nodes;

    std::string error_detail;
};

// ─────────────────────────────────────────────────────────────────────────────
// Status
// ─────────────────────────────────────────────────────────────────────────────

struct DistributedTxnStatus {
    bool        ok      = true;
    std::string message;
    std::uint32_t retry_count = 0;
    themis::utils::RetryExhaustionReason exhaustion_reason =
        themis::utils::RetryExhaustionReason::NONE;
    themis::utils::RetryTimeoutSource timeout_source =
        themis::utils::RetryTimeoutSource::NONE;
    std::string correlation_id;

    /**
     * @brief OK.
     * @return Return value.
     * @details Implements OK without additional internal calls.
     */
    static DistributedTxnStatus OK() { return {}; }
    static DistributedTxnStatus Error(
        std::string msg,
        std::uint32_t retry_count = 0,
        themis::utils::RetryExhaustionReason exhaustion_reason =
            themis::utils::RetryExhaustionReason::NONE,
        themis::utils::RetryTimeoutSource timeout_source =
            themis::utils::RetryTimeoutSource::NONE,
        std::string correlation_id = {}) {
        DistributedTxnStatus s;
        s.ok      = false;
        s.message = std::move(msg);
        s.retry_count = retry_count;
        s.exhaustion_reason = exhaustion_reason;
        s.timeout_source = timeout_source;
        s.correlation_id = std::move(correlation_id);
        return s;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Configuration
// ─────────────────────────────────────────────────────────────────────────────

struct DistributedTxnManagerConfig {
    std::chrono::milliseconds prepare_timeout{5000};

    std::chrono::milliseconds commit_timeout{5000};

    std::chrono::milliseconds default_txn_timeout{30000};

    std::string wal_directory;

    bool sync_wal_writes = true;

    bool enable_recovery_log = false;

    size_t max_active_transactions = 10000;

    std::function<bool(
        const std::string& txn_id,
        const std::string& node_id,
        const std::string& endpoint,
        const std::set<std::string>& affected_keys
    )> remote_phase1_dispatch;

    std::function<bool(
        const std::string& txn_id,
        const std::string& node_id,
        const std::string& endpoint,
        bool do_commit
    )> remote_phase2_dispatch;

    // ── Performance / PERF-D4 ────────────────────────────────────────────────

    std::chrono::milliseconds prepare_batch_window{0};

    size_t worker_thread_count = 4;

    using Phase2RpcFn = std::function<void(
        const std::string& endpoint,
        const std::string& txn_id,
        bool               do_commit
    )>;
    std::optional<Phase2RpcFn> phase2_rpc_fn;

    using Phase1RpcFn = std::function<bool(
        const std::string&            endpoint,
        const std::string&            txn_id,
        const std::set<std::string>&  affected_keys
    )>;
    std::optional<Phase1RpcFn> phase1_rpc_fn;

    using LivenessCheckFn = std::function<bool(
        const std::string& endpoint,
        const std::string& node_id
    )>;
    std::optional<LivenessCheckFn> liveness_check_fn;
};

// ─────────────────────────────────────────────────────────────────────────────
// DistributedTransactionManager
// ─────────────────────────────────────────────────────────────────────────────

class DistributedTransactionManager : public IRecoverableTwoPhaseCoordinator {
public:
    using TransactionId = std::string;

    struct Statistics {
        uint64_t total_transactions = 0;
        uint64_t committed          = 0;
        uint64_t aborted            = 0;
        uint64_t timeout_aborts     = 0;
        uint64_t recovered          = 0;
        uint64_t in_doubt           = 0;
    };

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


    /**
     * @brief Begin Distributed.
     * @param[in] participants Input parameter.
     * @return Return value.
     */
    TransactionId beginDistributed(const std::vector<Participant>& participants);

    /**
     * @brief Prepare Distributed.
     * @param[in] txn_id Identifier of the txn.
     * @return Return value.
     */
    DistributedTxnStatus prepareDistributed(const TransactionId& txn_id);

    /**
     * @brief Commit Distributed.
     * @param[in] txn_id Identifier of the txn.
     * @return Return value.
     */
    DistributedTxnStatus commitDistributed(const TransactionId& txn_id);

    /**
     * @brief Abort Distributed.
     * @param[in] txn_id Identifier of the txn.
     */
    void abortDistributed(const TransactionId& txn_id);


    /**
     * @brief Vote On Prepare.
     * @param[in] txn_id Identifier of the txn.
     * @param[in] node_id Identifier of the node.
     * @param[in] can_commit Input parameter.
     * @return Return value.
     */
    DistributedTxnStatus voteOnPrepare(
        const TransactionId& txn_id,
        const std::string&   node_id,
        bool                 can_commit
    );

    /**
     * @brief Apply Commit.
     * @param[in] txn_id Identifier of the txn.
     * @return Return value.
     */
    DistributedTxnStatus applyCommit(const TransactionId& txn_id);

    /**
     * @brief Apply Abort.
     * @param[in] txn_id Identifier of the txn.
     * @return Return value.
     */
    DistributedTxnStatus applyAbort(const TransactionId& txn_id);

    // ── Recovery ─────────────────────────────────────────────────────────────

    size_t recoverInDoubtTransactions() override;

    [[nodiscard]] std::string recoveryCoordinatorName() const override;

    [[nodiscard]] std::string recoveryBackendName() const override;

    [[nodiscard]] std::vector<RecoverableTwoPhaseTransaction>
    getRecoverableTransactions() const override;


    /**
     * @brief Check Timeouts.
     * @return Return value.
     */
    size_t checkTimeouts();

    // ── Remote phase-2 transport bridge ──────────────────────────────────────

    using RemotePhase2Fn = std::function<void(
        const std::string& endpoint,
        const TransactionId& txn_id,
        bool do_commit
    )>;


    /**
     * @brief Set Remote Phase2 Fn.
     * @param[in] fn Input parameter.
     * @details Calls: lock(), std::move().
     */
    void setRemotePhase2Fn(RemotePhase2Fn fn) {
        std::lock_guard<std::mutex> lock(mutex_);
        remote_phase2_fn_ = std::move(fn);
    }


    /**
     * @brief Is Participant Alive.
     * @param[in] node_id Identifier of the node.
     * @return True when the operation succeeds.
     */
    bool isParticipantAlive(const std::string& node_id) const;


    /**
     * @brief Get Transaction.
     * @param[in] txn_id Identifier of the txn.
     * @return Return value.
     */
    std::optional<DistributedTransaction> getTransaction(const TransactionId& txn_id) const;

    /**
     * @brief Return access control statistics.
     * @return Access control statistics.
     */
    Statistics getStatistics() const;

    /**
     * @brief Active Transaction Count.
     * @return Return value.
     */
    size_t activeTransactionCount() const;

    // ─── RPC phase-2 bridge (stub #279) ──────────────────────────────────────

    using RpcPhase2Fn = std::function<void(const std::string& node_id,
                                           const std::string& txn_id,
                                           bool               do_commit)>;

    /**
     * @brief Set Rpc Phase2 Fn.
     * @param[in] fn Input parameter.
     */
    static void setRpcPhase2Fn(RpcPhase2Fn fn);

    /**
     * @brief Clear Rpc Phase2 Fn.
     */
    static void clearRpcPhase2Fn();

    // ─── RPC phase-1 bridge (stub #279 — Phase-1 PREPARE) ────────────────────

    using RpcPhase1Fn = std::function<bool(const std::string& node_id,
                                           const std::string& txn_id,
                                           const std::set<std::string>& affected_keys)>;

    /**
     * @brief Set Rpc Phase1 Fn.
     * @param[in] fn Input parameter.
     */
    static void setRpcPhase1Fn(RpcPhase1Fn fn);

    /**
     * @brief Clear Rpc Phase1 Fn.
     */
    static void clearRpcPhase1Fn();

    // ─── Liveness check bridge (DTM-3) ───────────────────────────────────────

    using StaticLivenessCheckFn = std::function<bool(const std::string& node_id,
                                                      const std::string& endpoint)>;

    /**
     * @brief Set Liveness Check Fn.
     * @param[in] fn Input parameter.
     */
    static void setLivenessCheckFn(StaticLivenessCheckFn fn);

    /**
     * @brief Clear Liveness Check Fn.
     */
    static void clearLivenessCheckFn();

private:

    /**
     * @brief Generate Transaction Id.
     * @return Return value.
     */
    std::string generateTransactionId();

    void logToWAL(
        themis::sharding::WALEntryType type,
        const std::string&             txn_id,
        const std::string&             data = ""
    );

    /**
     * @brief Run Phase1 Unlocked.
     * @param[in] txn_id Identifier of the txn.
     * @return True when the operation succeeds.
     */
    bool runPhase1Unlocked(const TransactionId& txn_id);

    /**
     * @brief Run Phase2 Unlocked.
     * @param[in] txn_id Identifier of the txn.
     * @param[in] parts Input parameter.
     * @param[in] do_commit Input parameter.
     * @return True when the operation succeeds.
     */
    bool runPhase2Unlocked(
        const TransactionId&            txn_id,
        const std::vector<Participant>& parts,
        bool                            do_commit
    );

    /**
     * @brief Find Transaction.
     * @param[in] txn_id Identifier of the txn.
     * @return Pointer to the result.
     */
    DistributedTransaction* findTransaction(const TransactionId& txn_id);
    /**
     * @brief Find Transaction.
     * @param[in] txn_id Identifier of the txn.
     * @return Pointer to the result.
     */
    const DistributedTransaction* findTransaction(const TransactionId& txn_id) const;

    // ── Thread pool (PERF-D4) ─────────────────────────────────────────────────

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
            /**
             * @brief Lock.
             * @param[in] pool_mutex_ Input parameter.
             * @return Return value.
             */
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

    /**
     * @brief Start Thread Pool.
     */
    void startThreadPool();

    /**
     * @brief Stop Thread Pool.
     */
    void stopThreadPool();

    // ── Batch-prepare flush (PERF-D4) ─────────────────────────────────────────

    struct BatchPrepareEntry {
        TransactionId          txn_id;
        std::promise<bool>     result;
    };

    /**
     * @brief Batch Flush Loop.
     */
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

    // ── Remote phase-2 bridge (stub #279) ─────────────────────────────────────
    std::optional<RemotePhase2Fn>         remote_phase2_fn_;
};

} // namespace themis::transaction
