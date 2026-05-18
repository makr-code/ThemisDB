/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            distributed_transaction_manager.cpp                ║
  Version:         0.0.12                                             ║
  Last Modified:   2026-04-15 18:51:22                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     823                                            ║
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
// DistributedTransactionManager – Two-Phase Commit coordinator
// (see include/transaction/distributed_transaction_manager.h for design notes)
//
// PERF-D4: Batched prepare + lock-free coordination
//   • Thread pool reuses workers → eliminates per-call thread-creation cost.
//   • prepare_batch_window > 0ms: multiple prepareDistributed() callers are
//     queued and flushed together so all their Phase-1 calls hit participants
//     in one parallel wave.
//   • transactions_ is now an std::unordered_map for O(1) lookup.

#include "transaction/distributed_transaction_manager.h"
#include "utils/logger.h"

#include <algorithm>
#include <chrono>
#include <future>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <vector>

namespace themis::transaction {

namespace {
/// Format a system_clock time-point as ISO-8601 for WAL/log data.
std::string formatTimePoint(std::chrono::system_clock::time_point tp) {
    auto tt = std::chrono::system_clock::to_time_t(tp);
    std::tm tm_buf{};
#if defined(_WIN32)
    gmtime_s(&tm_buf, &tt);
#else
    gmtime_r(&tt, &tm_buf);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm_buf, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}
} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// Constructor / Destructor
// ─────────────────────────────────────────────────────────────────────────────

DistributedTransactionManager::DistributedTransactionManager(
    std::string                 coordinator_id,
    DistributedTxnManagerConfig config
)
    : coordinator_id_(std::move(coordinator_id))
    , config_(std::move(config))
{
    if (!config_.wal_directory.empty()) {
        themis::sharding::WALManagerConfig wal_cfg;
        wal_cfg.wal_directory = config_.wal_directory;
        wal_cfg.sync_on_write = config_.sync_wal_writes;
        wal_ = std::make_unique<themis::sharding::WALManager>(wal_cfg);
        THEMIS_INFO("DistributedTransactionManager [{}] WAL initialised at {}",
                    coordinator_id_, config_.wal_directory);
    }

    // Start thread pool (PERF-D4).
    startThreadPool();

    // Start batch-flush thread when batching is enabled (PERF-D4).
    if (config_.prepare_batch_window.count() > 0) {
        batch_stop_.store(false, std::memory_order_relaxed);
        batch_flush_thread_ = std::thread(&DistributedTransactionManager::batchFlushLoop, this);
        THEMIS_INFO("DistributedTransactionManager [{}] batch-prepare enabled: window={}ms workers={}",
                    coordinator_id_, config_.prepare_batch_window.count(),
                    config_.worker_thread_count);
    }
}

DistributedTransactionManager::~DistributedTransactionManager() {
    // Stop batch-flush thread first so it no longer queues tasks.
    if (batch_flush_thread_.joinable()) {
        {
            std::lock_guard<std::mutex> lock(batch_mutex_);
            batch_stop_.store(true, std::memory_order_relaxed);
        }
        batch_cv_.notify_all();
        batch_flush_thread_.join();
    }

    // Drain any remaining batch entries with a "false" result so callers
    // waiting on their futures are unblocked.
    {
        std::lock_guard<std::mutex> lock(batch_mutex_);
        for (auto& entry : batch_queue_) {
            try { entry.result.set_value(false); } catch (...) {}
        }
        batch_queue_.clear();
    }

    // Stop thread pool.
    stopThreadPool();
}

// ─────────────────────────────────────────────────────────────────────────────
// Coordinator API
// ─────────────────────────────────────────────────────────────────────────────

DistributedTransactionManager::TransactionId
DistributedTransactionManager::beginDistributed(
    const std::vector<Participant>& participants
) {
    if (participants.empty()) {
        throw std::invalid_argument(
            "DistributedTransactionManager::beginDistributed: participants must not be empty");
    }

    const TransactionId txn_id = generateTransactionId();

    DistributedTransaction rec;
    rec.txn_id       = txn_id;
    rec.participants = participants;
    rec.state        = DistributedTxnState::INIT;
    rec.created_at   = std::chrono::system_clock::now();
    rec.timeout      = rec.created_at + config_.default_txn_timeout;

    logToWAL(themis::sharding::WALEntryType::BEGIN_TX, txn_id,
             "participants=" + std::to_string(participants.size()));

    {
        std::lock_guard<std::mutex> lock(mutex_);
        transactions_.emplace(txn_id, std::move(rec));
    }

    ++stat_total_;
    THEMIS_DEBUG("DistributedTransactionManager [{}] beginDistributed txn={} participants={}",
                 coordinator_id_, txn_id, participants.size());
    return txn_id;
}

DistributedTxnStatus
DistributedTransactionManager::prepareDistributed(const TransactionId& txn_id) {
    std::unique_lock<std::mutex> lock(mutex_);

    auto* txn = findTransaction(txn_id);
    if (!txn) {
        return DistributedTxnStatus::Error("Transaction not found: " + txn_id);
    }

    if (txn->state != DistributedTxnState::INIT) {
        return DistributedTxnStatus::Error(
            "Transaction " + txn_id + " is not in INIT state");
    }

    txn->state = DistributedTxnState::PREPARING;
    lock.unlock();

    bool all_voted_commit = false;

    if (config_.prepare_batch_window.count() > 0 && batch_flush_thread_.joinable()) {
        // Batched path (PERF-D4): enqueue and wait for the batch-flush thread.
        std::promise<bool> promise;
        std::future<bool>  fut = promise.get_future();
        {
            std::lock_guard<std::mutex> blk(batch_mutex_);
            batch_queue_.push_back({txn_id, std::move(promise)});
        }
        batch_cv_.notify_one();
        all_voted_commit = fut.get();
    } else {
        // Immediate path: run Phase 1 right now.
        all_voted_commit = runPhase1Unlocked(txn_id);
    }

    lock.lock();
    txn = findTransaction(txn_id);  // re-acquire pointer after re-lock
    if (!txn) {
        return DistributedTxnStatus::Error("Transaction removed during prepare: " + txn_id);
    }

    if (all_voted_commit) {
        txn->state = DistributedTxnState::PREPARED;
        logToWAL(themis::sharding::WALEntryType::PREPARE_TX, txn_id);
        THEMIS_DEBUG("DistributedTransactionManager [{}] txn={} PREPARED", coordinator_id_, txn_id);
        return DistributedTxnStatus::OK();
    } else {
        // Abort the transaction – Phase-2 abort sent inside abortUnlocked
        txn->state = DistributedTxnState::ABORTING;
        const std::string err = txn->error_detail.empty()
            ? "One or more participants voted ABORT"
            : txn->error_detail;
        lock.unlock();
        abortDistributed(txn_id);
        return DistributedTxnStatus::Error(err);
    }
}

DistributedTxnStatus
DistributedTransactionManager::commitDistributed(const TransactionId& txn_id) {
    std::unique_lock<std::mutex> lock(mutex_);

    auto* txn = findTransaction(txn_id);
    if (!txn) {
        return DistributedTxnStatus::Error("Transaction not found: " + txn_id);
    }

    if (txn->state != DistributedTxnState::PREPARED) {
        return DistributedTxnStatus::Error(
            "Transaction " + txn_id + " is not in PREPARED state");
    }

    txn->state = DistributedTxnState::COMMITTING;

    // Durably log COMMIT decision before broadcasting to participants.
    // This ensures that if the coordinator crashes after logging but before all
    // ACKs arrive, recoverInDoubtTransactions() will re-send COMMIT.
    logToWAL(themis::sharding::WALEntryType::COMMIT_TX, txn_id);

    // DTM-4 fix: Explicitly flush the WAL to durable storage before Phase-2
    // broadcast.  Without this flush the COMMIT record may still be in a
    // write buffer; a crash between logToWAL() and the Phase-2 broadcast
    // would leave participants in COMMITTED state while the coordinator has no
    // durable record of the COMMIT decision.
    if (wal_) {
        wal_->flush();
    }

    // Collect a snapshot of participants under the lock.
    const std::vector<Participant> parts = txn->participants;
    lock.unlock();

    // Phase 2: send COMMIT to all participants that voted YES.
    runPhase2Unlocked(txn_id, parts, /*do_commit=*/true);

    lock.lock();
    txn = findTransaction(txn_id);
    if (!txn) {
        return DistributedTxnStatus::Error("Transaction removed during commit: " + txn_id);
    }

    txn->state = DistributedTxnState::COMMITTED;
    ++stat_committed_;
    THEMIS_INFO("DistributedTransactionManager [{}] txn={} COMMITTED", coordinator_id_, txn_id);
    return DistributedTxnStatus::OK();
}

void DistributedTransactionManager::abortDistributed(const TransactionId& txn_id) {
    std::unique_lock<std::mutex> lock(mutex_);

    auto* txn = findTransaction(txn_id);
    if (!txn) {
        THEMIS_DEBUG("DistributedTransactionManager [{}] abortDistributed: txn={} not found",
                     coordinator_id_, txn_id);
        return;
    }

    // Idempotent: if already in a terminal state, do nothing.
    const DistributedTxnState st = txn->state;
    if (st == DistributedTxnState::COMMITTED ||
        st == DistributedTxnState::ABORTED) {
        return;
    }

    txn->state = DistributedTxnState::ABORTING;
    logToWAL(themis::sharding::WALEntryType::ABORT_TX, txn_id);

    // DTM-4 fix: Flush WAL before Phase-2 ABORT broadcast (symmetric with
    // commitDistributed) so the ABORT decision is durable before participants
    // are notified.
    if (wal_) {
        wal_->flush();
    }

    // Collect participants.
    const std::vector<Participant> parts = txn->participants;
    lock.unlock();

    // Send ABORT to all participants regardless of whether they voted.
    runPhase2Unlocked(txn_id, parts, /*do_commit=*/false);

    lock.lock();
    txn = findTransaction(txn_id);
    if (!txn) return;

    txn->state = DistributedTxnState::ABORTED;
    ++stat_aborted_;
    THEMIS_INFO("DistributedTransactionManager [{}] txn={} ABORTED", coordinator_id_, txn_id);
}

// ─────────────────────────────────────────────────────────────────────────────
// Participant API
// ─────────────────────────────────────────────────────────────────────────────

DistributedTxnStatus DistributedTransactionManager::voteOnPrepare(
    const TransactionId& txn_id,
    const std::string&   node_id,
    bool                 can_commit
) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto* txn = findTransaction(txn_id);
    if (!txn) {
        return DistributedTxnStatus::Error("Transaction not found: " + txn_id);
    }

    if (txn->state != DistributedTxnState::PREPARING) {
        return DistributedTxnStatus::Error(
            "Transaction " + txn_id + " is not in PREPARING state; vote ignored");
    }

    txn->votes[node_id] = can_commit;
    if (!can_commit) {
        txn->error_detail = "Participant " + node_id + " voted ABORT";
    }

    vote_cv_.notify_all();
    return DistributedTxnStatus::OK();
}

DistributedTxnStatus DistributedTransactionManager::applyCommit(const TransactionId& txn_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto* txn = findTransaction(txn_id);
    if (!txn) {
        return DistributedTxnStatus::Error("Transaction not found: " + txn_id);
    }

    // The commit has been applied; no state change needed here — the
    // coordinator's runPhase2 already advanced to COMMITTED.  This method
    // exists primarily for remote participants to acknowledge the decision.
    THEMIS_DEBUG("DistributedTransactionManager [{}] applyCommit ack txn={}", coordinator_id_, txn_id);
    return DistributedTxnStatus::OK();
}

DistributedTxnStatus DistributedTransactionManager::applyAbort(const TransactionId& txn_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto* txn = findTransaction(txn_id);
    if (!txn) {
        return DistributedTxnStatus::Error("Transaction not found: " + txn_id);
    }

    THEMIS_DEBUG("DistributedTransactionManager [{}] applyAbort ack txn={}", coordinator_id_, txn_id);
    return DistributedTxnStatus::OK();
}

// ─────────────────────────────────────────────────────────────────────────────
// Recovery
// ─────────────────────────────────────────────────────────────────────────────

size_t DistributedTransactionManager::recoverInDoubtTransactions() {
    if (!wal_) {
        THEMIS_DEBUG("DistributedTransactionManager [{}] recoverInDoubtTransactions: WAL disabled",
                     coordinator_id_);
        return 0;
    }

    THEMIS_INFO("DistributedTransactionManager [{}] starting in-doubt recovery", coordinator_id_);

    const auto oldest_lsn = wal_->getOldestLSN();
    const auto entries     = wal_->readRange(oldest_lsn);

    // Build a map of txn_id → last WAL decision.
    std::map<std::string, themis::sharding::WALEntryType> last_decision;
    std::map<std::string, size_t>                         participant_counts;

    for (const auto& entry : entries) {
        const std::string& tid = entry.transaction_id;
        if (tid.empty()) continue;

        switch (entry.type) {
        case themis::sharding::WALEntryType::BEGIN_TX:
            last_decision[tid]     = entry.type;
            break;
        case themis::sharding::WALEntryType::PREPARE_TX:
            last_decision[tid]     = entry.type;
            break;
        case themis::sharding::WALEntryType::COMMIT_TX:
            last_decision[tid]     = entry.type;
            break;
        case themis::sharding::WALEntryType::ABORT_TX:
            last_decision[tid]     = entry.type;
            break;
        default:
            break;
        }
    }

    // Transactions that have a PREPARE_TX but no COMMIT_TX or ABORT_TX are
    // in-doubt.  Re-drive them with ABORT (safe conservative choice when we
    // cannot contact participants to determine their individual states).
    size_t resolved = 0;
    for (const auto& [tid, type] : last_decision) {
        if (type != themis::sharding::WALEntryType::PREPARE_TX) continue;

        THEMIS_WARN("DistributedTransactionManager [{}] recovery: in-doubt txn={} → ABORT",
                    coordinator_id_, tid);

        // DTM-2 fix: Log ABORT decision first, then broadcast ABORT to any
        // in-memory participants so they can release their locks.  Without this
        // broadcast, participants remained PREPARED indefinitely while holding
        // row-level locks.
        logToWAL(themis::sharding::WALEntryType::ABORT_TX, tid, "recovery=true");

        // Collect in-memory participant list (if the transaction is still live
        // in this coordinator process; may be empty after a restart).
        std::vector<Participant> parts;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto* txn = findTransaction(tid);
            if (txn) {
                txn->state = DistributedTxnState::ABORTING;
                parts      = txn->participants;
            }
        }

        if (!parts.empty()) {
            THEMIS_INFO("DistributedTransactionManager [{}] recovery: broadcasting ABORT for "
                        "in-doubt txn={} to {} in-memory participants",
                        coordinator_id_, tid, parts.size());
            runPhase2Unlocked(tid, parts, /*do_commit=*/false);

            std::lock_guard<std::mutex> lock(mutex_);
            auto* txn = findTransaction(tid);
            if (txn) txn->state = DistributedTxnState::ABORTED;
        }

        ++resolved;
        ++stat_recovered_;
        ++stat_aborted_;
    }

    THEMIS_INFO("DistributedTransactionManager [{}] recovery complete: {} in-doubt txns resolved",
                coordinator_id_, resolved);
    return resolved;
}

// ─────────────────────────────────────────────────────────────────────────────
// Timeout handling
// ─────────────────────────────────────────────────────────────────────────────

size_t DistributedTransactionManager::checkTimeouts() {
    const auto now = std::chrono::system_clock::now();

    std::vector<TransactionId> timed_out;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& [tid, txn] : transactions_) {
            if (txn.state == DistributedTxnState::INIT ||
                txn.state == DistributedTxnState::PREPARING ||
                txn.state == DistributedTxnState::PREPARED) {
                if (now > txn.timeout) {
                    timed_out.push_back(tid);
                }
            }
        }
    }

    for (const auto& tid : timed_out) {
        THEMIS_WARN("DistributedTransactionManager [{}] txn={} timed out — aborting",
                    coordinator_id_, tid);

        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto* txn = findTransaction(tid);
            if (txn) {
                txn->error_detail = "Transaction timed out (network partition or slow participant)";
            }
        }
        abortDistributed(tid);
        ++stat_timeout_aborts_;
    }

    return timed_out.size();
}

// ─────────────────────────────────────────────────────────────────────────────
// Failure detection
// ─────────────────────────────────────────────────────────────────────────────

bool DistributedTransactionManager::isParticipantAlive(const std::string& node_id) const {
    // DTM-3 fix: Distinguish between in-process and remote participants.
    //
    // In-process participants (callback != nullptr) are always reachable — the
    // object lives in the same address space and the pointer is valid.
    //
    // Remote participants (callback == nullptr, endpoint non-empty) require a
    // network health check.  A real implementation would issue a ping / gRPC
    // health-check RPC to part.endpoint.  Until that transport is wired, we
    // conservatively report remote nodes as *dead* so that callers (e.g.
    // checkTimeouts) can take an appropriate ABORT action rather than waiting
    // indefinitely.
    //
    // If node_id is not registered in any active transaction we return true
    // (unknown participants are treated as alive to avoid spurious aborts).
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& [tid, txn] : transactions_) {
        for (const auto& part : txn.participants) {
            if (part.node_id != node_id) continue;
            if (part.callback != nullptr) {
                return true;   // in-process — always alive
            }
            // Remote participant: conservative default is *not alive* until
            // a real RPC health check is implemented.
            return false;
        }
    }
    // Not found in any active transaction — treat as alive.
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// Introspection
// ─────────────────────────────────────────────────────────────────────────────

std::optional<DistributedTransaction>
DistributedTransactionManager::getTransaction(const TransactionId& txn_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto it = transactions_.find(txn_id);
    if (it == transactions_.end()) return std::nullopt;
    return it->second;
}

DistributedTransactionManager::Statistics
DistributedTransactionManager::getStatistics() const {
    Statistics s;
    s.total_transactions = stat_total_.load(std::memory_order_relaxed);
    s.committed          = stat_committed_.load(std::memory_order_relaxed);
    s.aborted            = stat_aborted_.load(std::memory_order_relaxed);
    s.timeout_aborts     = stat_timeout_aborts_.load(std::memory_order_relaxed);
    s.recovered          = stat_recovered_.load(std::memory_order_relaxed);

    // Count in-doubt (PREPARING or PREPARED) transactions.
    std::lock_guard<std::mutex> lock(mutex_);
    s.in_doubt = static_cast<uint64_t>(std::count_if(
        transactions_.begin(), transactions_.end(),
        [](const auto& kv) {
            return kv.second.state == DistributedTxnState::PREPARING ||
                   kv.second.state == DistributedTxnState::PREPARED;
        }
    ));
    return s;
}

size_t DistributedTransactionManager::activeTransactionCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return std::count_if(
        transactions_.begin(), transactions_.end(),
        [](const auto& kv) {
            const DistributedTxnState st = kv.second.state;
            return st != DistributedTxnState::COMMITTED &&
                   st != DistributedTxnState::ABORTED;
        }
    );
}

// ─────────────────────────────────────────────────────────────────────────────
// Internal helpers
// ─────────────────────────────────────────────────────────────────────────────

std::string DistributedTransactionManager::generateTransactionId() {
    const uint64_t counter = ++txn_counter_;
    // Encode coordinator_id + monotonic counter + millisecond timestamp for
    // global uniqueness even across coordinator restarts.
    const auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    std::ostringstream oss;
    oss << "dtx2pc-" << coordinator_id_ << "-" << now_ms << "-" << counter;
    return oss.str();
}

void DistributedTransactionManager::logToWAL(
    themis::sharding::WALEntryType type,
    const std::string&             txn_id,
    const std::string&             data
) {
    if (!wal_) return;

    themis::sharding::WALEntry entry;
    entry.type           = type;
    entry.transaction_id = txn_id;
    entry.timestamp      = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );
    if (!data.empty()) {
        entry.data = {{"detail", data}};
    }

    try {
        wal_->append(entry);
    } catch (const std::exception& ex) {
        // CC-1 fix: WAL write failure is a hard error.  Silently continuing
        // after a failed WAL append would allow Phase-2 broadcasts without a
        // durable record of the decision, breaking crash-recovery guarantees.
        // Re-throw so callers can abort the transaction and surface the error.
        THEMIS_ERROR("DistributedTransactionManager [{}] WAL write failed for txn={}: {}",
                     coordinator_id_, txn_id, ex.what());
        throw;
    }
}

DistributedTransaction*
DistributedTransactionManager::findTransaction(const TransactionId& txn_id) {
    auto it = transactions_.find(txn_id);
    return (it != transactions_.end()) ? &it->second : nullptr;
}

const DistributedTransaction*
DistributedTransactionManager::findTransaction(const TransactionId& txn_id) const {
    auto it = transactions_.find(txn_id);
    return (it != transactions_.end()) ? &it->second : nullptr;
}

// ─────────────────────────────────────────────────────────────────────────────
// Phase 1: send PREPARE to all participants (without holding the mutex)
// ─────────────────────────────────────────────────────────────────────────────

bool DistributedTransactionManager::runPhase1Unlocked(const TransactionId& txn_id) {
    // Snapshot participants under lock, then release before calling callbacks.
    std::vector<Participant> parts;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto* txn = findTransaction(txn_id);
        if (!txn) return false;
        parts = txn->participants;
    }

    const auto deadline =
        std::chrono::steady_clock::now() + config_.prepare_timeout;

    // Launch prepare calls in parallel via the thread pool (PERF-D4).
    struct VoteResult {
        std::string node_id;
        bool        voted;
        bool        can_commit;
    };

    std::vector<std::future<VoteResult>> futures;
    futures.reserve(parts.size());

    for (const auto& part : parts) {
        if (!part.callback) {
            // DTM-1 fix: Remote participant without a registered callback cannot
            // be contacted for a PREPARE vote.  Treat as ABORT vote (safe
            // conservative choice) rather than unconditionally granting COMMIT.
            // Once a real RPC transport is wired, replace this branch with an
            // actual Phase-1 RPC call to part.endpoint.
            THEMIS_WARN("DistributedTransactionManager [{}] txn={} participant {} has no callback "
                        "(remote) — voting ABORT (DTM-1: real RPC not yet wired)",
                        coordinator_id_, txn_id, part.node_id);
            const std::string nid = part.node_id;
            futures.push_back(submitTask([nid]() -> VoteResult {
                return {nid, true, /*can_commit=*/false};
            }));
            continue;
        }

        IDistributedParticipantCallback* cb  = part.callback;
        const std::string                nid = part.node_id;
        const std::set<std::string>      keys = part.affected_keys;
        const std::string                tid  = txn_id;

        futures.push_back(submitTask([cb, nid, keys, tid]() -> VoteResult {
            try {
                const bool vote = cb->onPrepare(tid, keys);
                return {nid, true, vote};
            } catch (const std::exception& ex) {
                THEMIS_ERROR("2PC prepare threw for node={} txn={}: {}", nid, tid, ex.what());
                return {nid, true, false};  // treat exception as ABORT vote
            }
        }));
    }

    // Collect votes with deadline.
    bool all_commit = true;
    std::string abort_reason;

    for (auto& fut : futures) {
        const auto remaining = deadline - std::chrono::steady_clock::now();
        if (remaining <= std::chrono::milliseconds(0)) {
            all_commit   = false;
            abort_reason = "Prepare phase timed out";
            break;
        }

        const auto status = fut.wait_for(remaining);
        if (status == std::future_status::timeout) {
            all_commit   = false;
            abort_reason = "Prepare phase timed out waiting for participant";
            break;
        }

        const VoteResult result = fut.get();
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto* txn = findTransaction(txn_id);
            if (txn) {
                txn->votes[result.node_id] = result.can_commit;
            }
        }
        vote_cv_.notify_all();

        if (!result.can_commit) {
            all_commit   = false;
            abort_reason = "Participant " + result.node_id + " voted ABORT";
        }
    }

    if (!all_commit) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto* txn = findTransaction(txn_id);
        if (txn && txn->error_detail.empty()) {
            txn->error_detail = abort_reason;
        }
    }

    return all_commit;
}

// ─────────────────────────────────────────────────────────────────────────────
// Phase 2: send COMMIT or ABORT to all participants (without holding the mutex)
// ─────────────────────────────────────────────────────────────────────────────

void DistributedTransactionManager::runPhase2Unlocked(
    const TransactionId&         txn_id,
    const std::vector<Participant>& parts,
    bool                         do_commit
) {
    const auto deadline =
        std::chrono::steady_clock::now() + config_.commit_timeout;

    std::vector<std::future<void>> futures;
    futures.reserve(parts.size());

    for (const auto& part : parts) {
        if (!part.callback) {
            // STUB/SIMULATION NOTE (stub #279):
            // Purpose: Preserve in-process phase-2 fan-out while the remote
            //          commit/abort RPC path is still missing.
            // Activation: Reached when a participant is registered only by
            //             node/endpoint and provides no local callback.
            // Production Delta: The coordinator durably records COMMIT/ABORT in
            //                   its own WAL, but the final decision is never
            //                   delivered to the remote participant. Remote
            //                   nodes can remain prepared/orphaned until a real
            //                   transport replays the decision.
            // Removal Plan: Route phase-2 decisions through shard RPC / mTLS
            //               transport instead of skipping callback-less
            //               participants (tracked in STUB_INVENTORY #279).
            continue;
        }

        IDistributedParticipantCallback* cb  = part.callback;
        const std::string                nid = part.node_id;
        const std::string                tid = txn_id;
        const std::string                cid = coordinator_id_;

        if (do_commit) {
            futures.push_back(submitTask([cb, nid, tid, cid]() {
                try {
                    cb->onCommit(tid);
                } catch (const std::exception& ex) {
                    THEMIS_ERROR("2PC COMMIT threw for node={} txn={} coordinator={}: {}",
                                 nid, tid, cid, ex.what());
                }
            }));
        } else {
            futures.push_back(submitTask([cb, nid, tid, cid]() {
                try {
                    cb->onAbort(tid);
                } catch (const std::exception& ex) {
                    THEMIS_ERROR("2PC ABORT threw for node={} txn={} coordinator={}: {}",
                                 nid, tid, cid, ex.what());
                }
            }));
        }
    }

    for (auto& fut : futures) {
        const auto remaining = deadline - std::chrono::steady_clock::now();
        if (remaining <= std::chrono::milliseconds(0)) {
            THEMIS_WARN("DistributedTransactionManager [{}] Phase-2 deadline expired for txn={}",
                        coordinator_id_, txn_id);
            break;
        }
        const auto status = fut.wait_for(remaining);
        if (status == std::future_status::timeout) {
            THEMIS_WARN("DistributedTransactionManager [{}] participant timed out in Phase-2 for txn={}",
                        coordinator_id_, txn_id);
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Thread pool (PERF-D4)
// ─────────────────────────────────────────────────────────────────────────────

void DistributedTransactionManager::startThreadPool() {
    const size_t n = config_.worker_thread_count;
    if (n == 0) return;  // Legacy mode: std::async per call.

    pool_stop_ = false;
    worker_threads_.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        worker_threads_.emplace_back([this] {
            while (true) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(pool_mutex_);
                    pool_cv_.wait(lock, [this] {
                        return pool_stop_ || !task_queue_.empty();
                    });
                    if (pool_stop_ && task_queue_.empty()) return;
                    task = std::move(task_queue_.front());
                    task_queue_.pop();
                }
                task();
            }
        });
    }

    THEMIS_DEBUG("DistributedTransactionManager [{}] thread pool started: {} workers",
                 coordinator_id_, n);
}

void DistributedTransactionManager::stopThreadPool() {
    {
        std::lock_guard<std::mutex> lock(pool_mutex_);
        pool_stop_ = true;
    }
    pool_cv_.notify_all();
    for (auto& t : worker_threads_) {
        if (t.joinable()) t.join();
    }
    worker_threads_.clear();
}

// ─────────────────────────────────────────────────────────────────────────────
// Batch-prepare flush loop (PERF-D4)
// ─────────────────────────────────────────────────────────────────────────────

void DistributedTransactionManager::batchFlushLoop() {
    while (!batch_stop_.load(std::memory_order_relaxed)) {
        std::vector<BatchPrepareEntry> batch;
        {
            std::unique_lock<std::mutex> lock(batch_mutex_);
            // Wait for the window or until stopped.
            batch_cv_.wait_for(lock, config_.prepare_batch_window, [this] {
                return !batch_queue_.empty() || batch_stop_.load(std::memory_order_relaxed);
            });
            if (batch_stop_.load(std::memory_order_relaxed) && batch_queue_.empty()) break;
            batch.swap(batch_queue_);
        }

        if (batch.empty()) continue;

        THEMIS_DEBUG("DistributedTransactionManager [{}] batch-flush: {} transactions",
                     coordinator_id_, batch.size());

        // Launch Phase 1 for every queued transaction in parallel.
        // Each Phase-1 internally dispatches participant calls to the thread
        // pool, so this outer layer of parallelism batches across transactions.
        std::vector<std::future<bool>> phase1_futures;
        phase1_futures.reserve(batch.size());

        for (auto& entry : batch) {
            const TransactionId tid = entry.txn_id;
            phase1_futures.push_back(submitTask([this, tid]() -> bool {
                return runPhase1Unlocked(tid);
            }));
        }

        // Deliver results back to the waiting callers.
        for (size_t i = 0; i < batch.size(); ++i) {
            bool result = false;
            try {
                result = phase1_futures[i].get();
            } catch (...) {
                result = false;
            }
            try {
                batch[i].result.set_value(result);
            } catch (...) {}
        }
    }
}

} // namespace themis::transaction
