/**
 * @file distributed_transaction_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=5; TODO=1, Stub=3, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=28, M=7, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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

// ============================================================================
// Sprint 8 Phase 1: Use-After-Move Safety (GAP A-1)
// ============================================================================
// Helper structure to capture transaction state before move operations
// Ensures transaction metadata remains accessible even after async pipeline moves
struct TransactionStateSnapshot {
    std::string txn_id;
    DistributedTxnState state;
    std::vector<std::string> participants;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point timeout;
    std::string error_detail;
    
    explicit TransactionStateSnapshot(const DistributedTransaction& txn)
        : txn_id(txn.txn_id)
        , state(txn.state)
        , created_at(txn.created_at)
        , timeout(txn.timeout)
        , error_detail(txn.error_detail)
    {
        participants.reserve(txn.participants.size());
        for (const auto& p : txn.participants) participants.push_back(p.node_id);
    }
    
};

// ============================================================================
// RPC phase-2 bridge (STUB #279)
//
// STUB/SIMULATION NOTE:
// Purpose: RPC transport injection point — Phase-1/Phase-2 participant communication requires external transport binding
// Activation: Always active until a concrete RpcTransport implementation is injected via constructor/setter
// Production Delta: In-process mock calls replace real RPC; network partitions and timeouts are not exercised
// Removal Plan: Q4 2026 — bind gRPC transport in production wiring; remove stub after integration tests pass
// ============================================================================

namespace {
static std::mutex s_rpc_phase2_fn_mutex;
static DistributedTransactionManager::RpcPhase2Fn s_rpc_phase2_fn;
} // namespace

void DistributedTransactionManager::setRpcPhase2Fn(RpcPhase2Fn fn) {
    std::lock_guard<std::mutex> lock(s_rpc_phase2_fn_mutex);
    s_rpc_phase2_fn = std::move(fn);
}

void DistributedTransactionManager::clearRpcPhase2Fn() {
    std::lock_guard<std::mutex> lock(s_rpc_phase2_fn_mutex);
    s_rpc_phase2_fn = nullptr;
}

static DistributedTransactionManager::RpcPhase2Fn getRpcPhase2Fn() {
    std::lock_guard<std::mutex> lock(s_rpc_phase2_fn_mutex);
    return s_rpc_phase2_fn;
}

// ============================================================================
// RPC phase-1 bridge (STUB #279 — Phase-1 PREPARE extension)
//
// STUB/SIMULATION NOTE:
// Purpose: RPC transport injection point — Phase-1/Phase-2 participant communication requires external transport binding
// Activation: Always active until a concrete RpcTransport implementation is injected via constructor/setter
// Production Delta: In-process mock calls replace real RPC; network partitions and timeouts are not exercised
// Removal Plan: Q4 2026 — bind gRPC transport in production wiring; remove stub after integration tests pass
// ============================================================================

namespace {
static std::mutex s_rpc_phase1_fn_mutex;
static DistributedTransactionManager::RpcPhase1Fn s_rpc_phase1_fn;
} // namespace

void DistributedTransactionManager::setRpcPhase1Fn(RpcPhase1Fn fn) {
    std::lock_guard<std::mutex> lock(s_rpc_phase1_fn_mutex);
    s_rpc_phase1_fn = std::move(fn);
}

void DistributedTransactionManager::clearRpcPhase1Fn() {
    std::lock_guard<std::mutex> lock(s_rpc_phase1_fn_mutex);
    s_rpc_phase1_fn = nullptr;
}

static DistributedTransactionManager::RpcPhase1Fn getRpcPhase1Fn() {
    std::lock_guard<std::mutex> lock(s_rpc_phase1_fn_mutex);
    return s_rpc_phase1_fn;
}

// ============================================================================
// Liveness check bridge (DTM-3)
// ============================================================================

namespace {
static std::mutex s_liveness_check_fn_mutex;
static DistributedTransactionManager::StaticLivenessCheckFn s_liveness_check_fn;
} // namespace

void DistributedTransactionManager::setLivenessCheckFn(StaticLivenessCheckFn fn) {
    std::lock_guard<std::mutex> lock(s_liveness_check_fn_mutex);
    s_liveness_check_fn = std::move(fn);
}

void DistributedTransactionManager::clearLivenessCheckFn() {
    std::lock_guard<std::mutex> lock(s_liveness_check_fn_mutex);
    s_liveness_check_fn = nullptr;
}

static DistributedTransactionManager::StaticLivenessCheckFn getLivenessCheckFn() {
    std::lock_guard<std::mutex> lock(s_liveness_check_fn_mutex);
    return s_liveness_check_fn;
}

std::chrono::milliseconds computeDeterministicRetryBackoff(
    const std::string& txn_id,
    const std::string& node_id,
    size_t attempt)
{
    constexpr std::uint64_t kBaseBackoffMs = 100;
    constexpr std::uint64_t kJitterPercent = 20;
    constexpr std::uint64_t kJitterBuckets = (kJitterPercent * 2) + 1; // [-20, +20]

    const std::uint64_t attempt_factor = 1ULL << (attempt - 1);
    const std::uint64_t base_backoff_ms = kBaseBackoffMs * attempt_factor;

    const auto txn_hash = static_cast<std::uint64_t>(std::hash<std::string>{}(txn_id));
    const auto node_hash = static_cast<std::uint64_t>(std::hash<std::string>{}(node_id));
    const std::uint64_t mixed = txn_hash ^ (node_hash << 1U) ^ (attempt * 0x9e3779b97f4a7c15ULL);
    const int jitter = static_cast<int>(mixed % kJitterBuckets) - static_cast<int>(kJitterPercent);

    const std::uint64_t jittered =
        (base_backoff_ms * static_cast<std::uint64_t>(100 + jitter)) / 100ULL;
    return std::chrono::milliseconds(std::max<std::uint64_t>(1ULL, jittered));
}

template <typename Fn>
bool deliverPhase2WithRetry(
    Fn&&               deliver_fn,
    const char*        bridge_name,
    const std::string& node_id,
    const std::string& txn_id,
    const std::string& coordinator_id,
    bool               do_commit)
{
    constexpr size_t kMaxDeliveryAttempts = 3;

    for (size_t attempt = 1; attempt <= kMaxDeliveryAttempts; ++attempt) {
        try {
            if (deliver_fn()) {
                return true;
            }

            if (attempt < kMaxDeliveryAttempts) {
                const auto backoff = computeDeterministicRetryBackoff(txn_id, node_id, attempt);
                THEMIS_WARN("2PC {} {} returned failure for node={} txn={} coordinator={} on "
                            "attempt {}/{} — retrying in {}ms",
                            bridge_name, do_commit ? "COMMIT" : "ABORT", node_id, txn_id,
                            coordinator_id, attempt, kMaxDeliveryAttempts, backoff.count());
                std::this_thread::sleep_for(backoff);
            } else {
                THEMIS_ERROR("2PC {} {} returned failure for node={} txn={} coordinator={} on "
                             "final attempt {}/{}",
                             bridge_name, do_commit ? "COMMIT" : "ABORT", node_id, txn_id,
                             coordinator_id, attempt, kMaxDeliveryAttempts);
            }
        } catch (const std::exception& ex) {
            if (attempt < kMaxDeliveryAttempts) {
                const auto backoff = computeDeterministicRetryBackoff(txn_id, node_id, attempt);
                THEMIS_WARN("2PC {} {} threw for node={} txn={} coordinator={} on attempt {}/{}: "
                            "{} — retrying in {}ms",
                            bridge_name, do_commit ? "COMMIT" : "ABORT", node_id, txn_id,
                            coordinator_id, attempt, kMaxDeliveryAttempts, ex.what(),
                            backoff.count());
                std::this_thread::sleep_for(backoff);
            } else {
                THEMIS_ERROR("2PC {} {} threw for node={} txn={} coordinator={} on final attempt "
                             "{}/{}: {}",
                             bridge_name, do_commit ? "COMMIT" : "ABORT", node_id, txn_id,
                             coordinator_id, attempt, kMaxDeliveryAttempts, ex.what());
            }
        }
    }

    return false;
}
} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// Constructor / Destructor
// ─────────────────────────────────────────────────────────────────────────────

namespace themis::transaction {

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

    // CRITICAL FIX for stub #279: Validate Phase-2 transport configuration
    // Ensures no remote participants are left in PREPARED state without Phase-2 delivery capability.
    // When remote_phase1_dispatch is set, we MUST have a way to deliver Phase-2 messages.
    if (config_.remote_phase1_dispatch) {
        const bool has_phase2_transport =
            static_cast<bool>(config_.phase2_rpc_fn) ||
            static_cast<bool>(config_.remote_phase2_dispatch) ||
            static_cast<bool>(getRpcPhase2Fn());
        if (!has_phase2_transport) {
            throw std::invalid_argument(
                "DistributedTransactionManager [" + coordinator_id_ + "]: "
                "remote_phase1_dispatch is configured but no Phase-2 transport bridge is available "
                "(set phase2_rpc_fn, remote_phase2_dispatch, or setRpcPhase2Fn). "
                "This would cause remote participants to remain PREPARED indefinitely. "
                "stub #279 fix: fail-fast on misconfiguration.");
        }
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
            try {
                entry.result.set_value(false);
            } catch (const std::future_error&) {
                // Promise already satisfied/broken — ignore during shutdown drain.
            }
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

    // CRITICAL FIX for stub #279 (Phase-2): Validate per-transaction Phase-2 requirements.
    // If any participant is remote (callback == nullptr, endpoint != empty) and remote Phase-1
    // dispatch is enabled, ensure Phase-2 bridge is available to prevent participants
    // from remaining indefinitely in PREPARED state.
    for (const auto& part : participants) {
        if (!part.callback && !part.endpoint.empty()) {
            // This is a remote participant. Validate Phase-2 bridge is configured.
            const bool has_phase2_transport =
                static_cast<bool>(config_.phase2_rpc_fn) ||
                static_cast<bool>(config_.remote_phase2_dispatch) ||
                static_cast<bool>(getRpcPhase2Fn());
            
            if (!has_phase2_transport) {
                throw std::invalid_argument(
                    "DistributedTransactionManager [" + coordinator_id_ + "]::beginDistributed: "
                    "Transaction cannot register remote participant node=" + part.node_id + 
                    " endpoint=" + part.endpoint + " — "
                    "no Phase-2 transport bridge is configured (phase2_rpc_fn, "
                    "remote_phase2_dispatch, or setRpcPhase2Fn). "
                    "This would cause the participant to remain PREPARED indefinitely. "
                    "stub #279 fix: fail-fast on misconfiguration.");
            }
        }
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
        const size_t active_txn_count = static_cast<size_t>(std::count_if(
            transactions_.begin(),
            transactions_.end(),
            [](const auto& kv) {
                const DistributedTxnState st = kv.second.state;
                return st != DistributedTxnState::COMMITTED &&
                       st != DistributedTxnState::ABORTED;
            }));
        if (active_txn_count >= config_.max_active_transactions) {
            throw std::runtime_error(
                "DistributedTransactionManager::beginDistributed: max_active_transactions limit reached (" +
                std::to_string(config_.max_active_transactions) + ")");
        }

        // Sprint 8 Phase 1 (GAP A-1): Transaction ID (txn_id) is captured BEFORE move.
        // This ensures all subsequent operations use the copied txn_id, not the moved object.
        // Pattern: Move object, access by ID; never access moved object.
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
        //
        // GAP fix (blocking_no_timeout / no_timeout): the prior bare fut.get()
        // blocked indefinitely if the batch-flush thread was slow or stalled.
        // Replaced with fut.wait_for(prepare_timeout) so callers always get a
        // bounded response. On timeout the transaction is treated as ABORT-voted
        // and the normal abort path is followed.
        std::promise<bool> promise;
        std::future<bool>  fut = promise.get_future();
        {
            std::lock_guard<std::mutex> blk(batch_mutex_);
            batch_queue_.push_back({txn_id, std::move(promise)});
        }
        batch_cv_.notify_one();

        const auto batch_deadline = std::chrono::steady_clock::now() + config_.prepare_timeout;
        const std::future_status fstatus =
            fut.wait_until(batch_deadline);

        if (fstatus == std::future_status::timeout) {
            THEMIS_WARN(
                "DistributedTransactionManager [{}] prepareDistributed: "
                "batch-flush did not respond within {}ms for txn={}; aborting",
                coordinator_id_, config_.prepare_timeout.count(), txn_id);
            // Mark as ABORTING and delegate to the abort path below.
            lock.lock();
            auto* txn_timeout = findTransaction(txn_id);
            if (txn_timeout) {
                txn_timeout->state        = DistributedTxnState::ABORTING;
                txn_timeout->error_detail = "prepareDistributed: batch-flush timeout (" +
                    std::to_string(config_.prepare_timeout.count()) + "ms)";
            }
            lock.unlock();
            abortDistributed(txn_id);
            return DistributedTxnStatus::Error(
                "prepareDistributed: batch-flush timeout for txn " + txn_id,
                0,
                themis::utils::RetryExhaustionReason::NONE,
                themis::utils::RetryTimeoutSource::OVERALL);
        }

        all_voted_commit = fut.get(); // future is ready — non-blocking at this point
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
        if (wal_) {
            wal_->flush();
        }
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
    const bool phase2_ok = runPhase2Unlocked(txn_id, parts, /*do_commit=*/true);

    lock.lock();
    txn = findTransaction(txn_id);
    if (!txn) {
        return DistributedTxnStatus::Error("Transaction removed during commit: " + txn_id);
    }

    if (!phase2_ok) {
        txn->state = DistributedTxnState::COMMITTING;
        txn->error_detail = "Phase-2 COMMIT delivery incomplete; remote participant(s) "
                           "missing Phase-2 transport bridge or delivery failed";
        THEMIS_ERROR("DistributedTransactionManager [{}] txn={} COMMIT decision logged but "
                     "Phase-2 delivery incomplete (possible missing bridge or delivery failure); "
                     "recovery required",
                     coordinator_id_, txn_id);
        return DistributedTxnStatus::Error(
            "Phase-2 COMMIT delivery incomplete; transaction remains in COMMITTING state; "
            "check error_detail field for root cause (missing Phase-2 bridge?)");
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
    const bool phase2_ok = runPhase2Unlocked(txn_id, parts, /*do_commit=*/false);

    lock.lock();
    txn = findTransaction(txn_id);
    if (!txn) return;

    if (!phase2_ok) {
        txn->state = DistributedTxnState::ABORTING;
        if (txn->error_detail.empty()) {
            txn->error_detail = "Phase-2 ABORT delivery incomplete; remote participant(s) "
                               "missing Phase-2 transport bridge or delivery failed";
        }
        THEMIS_ERROR("DistributedTransactionManager [{}] txn={} ABORT decision logged but "
                     "Phase-2 delivery incomplete (possible missing bridge or delivery failure); "
                     "recovery required",
                     coordinator_id_, txn_id);
        return;
    }

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
        THEMIS_DEBUG("DistributedTransactionManager [{}] recoverInDoubtTransactions: WAL disabled; "
                     "recovering in-memory in-doubt transactions only",
                     coordinator_id_);

        struct PendingDecision {
            TransactionId txn_id;
            std::vector<Participant> participants;
            bool do_commit = false;
        };

        std::vector<PendingDecision> pending;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            for (auto& [tid, txn] : transactions_) {
                if (txn.state == DistributedTxnState::PREPARING ||
                    txn.state == DistributedTxnState::PREPARED) {
                    txn.state = DistributedTxnState::ABORTING;
                    pending.push_back(PendingDecision{tid, txn.participants, /*do_commit=*/false});
                    continue;
                }
                if (txn.state == DistributedTxnState::COMMITTING) {
                    pending.push_back(PendingDecision{tid, txn.participants, /*do_commit=*/true});
                    continue;
                }
                if (txn.state != DistributedTxnState::ABORTING) {
                    continue;
                }
                pending.push_back(PendingDecision{tid, txn.participants, /*do_commit=*/false});
            }
        }

        for (const auto& item : pending) {
            const bool phase2_ok =
                runPhase2Unlocked(item.txn_id, item.participants, item.do_commit);
            std::lock_guard<std::mutex> lock(mutex_);
            if (auto* txn = findTransaction(item.txn_id)) {
                if (phase2_ok) {
                    txn->state = item.do_commit
                        ? DistributedTxnState::COMMITTED
                        : DistributedTxnState::ABORTED;
                    ++stat_recovered_;
                    if (item.do_commit) {
                        ++stat_committed_;
                    } else {
                        ++stat_aborted_;
                    }
                } else {
                    txn->state = item.do_commit
                        ? DistributedTxnState::COMMITTING
                        : DistributedTxnState::ABORTING;
                    if (txn->error_detail.empty() || item.do_commit) {
                        txn->error_detail = item.do_commit
                            ? "Recovery COMMIT delivery incomplete; transaction remains COMMITTING"
                            : "Recovery ABORT delivery incomplete; transaction remains ABORTING";
                    }
                }
            }
        }

        size_t resolved = 0;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            for (const auto& item : pending) {
                const auto* txn = findTransaction(item.txn_id);
                if (txn &&
                    (txn->state == DistributedTxnState::ABORTED ||
                     txn->state == DistributedTxnState::COMMITTED)) {
                    ++resolved;
                }
            }
        }
        return resolved;
    }

    THEMIS_INFO("DistributedTransactionManager [{}] starting in-doubt recovery", coordinator_id_);

    const auto oldest_lsn = wal_->getOldestLSN();
    const auto entries     = wal_->readRange(oldest_lsn);

    // Build a map of txn_id → last WAL decision.
    std::map<std::string, themis::sharding::WALEntryType> last_decision;

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
        if (type != themis::sharding::WALEntryType::PREPARE_TX &&
            type != themis::sharding::WALEntryType::COMMIT_TX) {
            continue;
        }

        const bool do_commit = (type == themis::sharding::WALEntryType::COMMIT_TX);

        THEMIS_WARN("DistributedTransactionManager [{}] recovery: in-doubt txn={} → {}",
                    coordinator_id_, tid, do_commit ? "COMMIT" : "ABORT");

        // For PREPARE_TX (no final decision) we choose conservative ABORT and record
        // the decision before broadcasting. COMMIT_TX is already durable and only needs
        // delivery replay to in-memory participants.
        if (!do_commit) {
            // DTM-2 fix: Log ABORT decision first, then broadcast ABORT to any
            // in-memory participants so they can release their locks.  Without this
            // broadcast, participants remained PREPARED indefinitely while holding
            // row-level locks.
            logToWAL(themis::sharding::WALEntryType::ABORT_TX, tid, "recovery=true");
        }

        // Collect in-memory participant list (if the transaction is still live
        // in this coordinator process; may be empty after a restart).
        std::vector<Participant> parts;
        bool has_live_txn = false;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto* txn = findTransaction(tid);
            if (txn) {
                has_live_txn = true;
                txn->state = do_commit
                    ? DistributedTxnState::COMMITTING
                    : DistributedTxnState::ABORTING;
                parts      = txn->participants;
            }
        }

        // Idempotence hardening: COMMIT_TX is already a durable final decision.
        // If a restarted coordinator no longer has in-memory participants for this
        // txn, there is nothing to re-deliver locally, so do not count it as
        // newly "resolved" on every recovery pass.
        if (do_commit && !has_live_txn) {
            THEMIS_DEBUG("DistributedTransactionManager [{}] recovery: COMMIT_TX for txn={} has "
                         "no in-memory participants after restart; skipping replay",
                         coordinator_id_, tid);
            continue;
        }

        if (!parts.empty()) {
            THEMIS_INFO("DistributedTransactionManager [{}] recovery: broadcasting {} for "
                        "in-doubt txn={} to {} in-memory participants",
                        coordinator_id_, do_commit ? "COMMIT" : "ABORT", tid, parts.size());
            const bool phase2_ok = runPhase2Unlocked(tid, parts, do_commit);

            std::lock_guard<std::mutex> lock(mutex_);
            auto* txn = findTransaction(tid);
            if (txn) {
                if (phase2_ok) {
                    txn->state = do_commit
                        ? DistributedTxnState::COMMITTED
                        : DistributedTxnState::ABORTED;
                } else {
                    txn->state = do_commit
                        ? DistributedTxnState::COMMITTING
                        : DistributedTxnState::ABORTING;
                    if (txn->error_detail.empty()) {
                        txn->error_detail = do_commit
                            ? "Recovery COMMIT delivery incomplete; transaction remains COMMITTING"
                            : "Recovery ABORT delivery incomplete; transaction remains ABORTING";
                    }
                }
            }
            if (!phase2_ok) {
                continue;
            }
        }

        ++resolved;
        ++stat_recovered_;
        if (do_commit) {
            ++stat_committed_;
        } else {
            ++stat_aborted_;
        }
    }

    THEMIS_INFO("DistributedTransactionManager [{}] recovery complete: {} in-doubt txns resolved",
                coordinator_id_, resolved);
    return resolved;
}

/**
 * @brief Return stable coordinator type name for global recovery reports.
 * @return "DistributedTransactionManager".
 */
std::string DistributedTransactionManager::recoveryCoordinatorName() const {
    return "DistributedTransactionManager";
}

/**
 * @brief Return name of the durable backend used by this coordinator.
 * @return "WAL" when a WAL directory is configured, "disabled" otherwise.
 */
std::string DistributedTransactionManager::recoveryBackendName() const {
    return wal_ ? "WAL" : "disabled";
}

/**
 * @brief Return normalized snapshot of non-final transactions for global recovery.
 *
 * Maps each DistributedTxnState to the canonical RecoverableTwoPhaseState so
 * the GlobalTwoPhaseCommitRecoveryManager can compute aggregated in-doubt counts.
 *
 * @return List of non-final transactions (excludes COMMITTED and ABORTED).
 */
std::vector<RecoverableTwoPhaseTransaction>
DistributedTransactionManager::getRecoverableTransactions() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<RecoverableTwoPhaseTransaction> result;
    for (const auto& [txn_id, rec] : transactions_) {
        if (rec.state == DistributedTxnState::COMMITTED ||
            rec.state == DistributedTxnState::ABORTED) {
            continue;
        }

        RecoverableTwoPhaseTransaction rt;
        rt.transaction_id = txn_id;

        switch (rec.state) {
        case DistributedTxnState::INIT:
            rt.state = RecoverableTwoPhaseState::ACTIVE;
            break;
        case DistributedTxnState::PREPARING:
            rt.state = RecoverableTwoPhaseState::PREPARING;
            break;
        case DistributedTxnState::PREPARED:
            rt.state = RecoverableTwoPhaseState::PREPARED;
            break;
        case DistributedTxnState::COMMITTING:
            rt.state             = RecoverableTwoPhaseState::COMMITTING;
            rt.decision_recorded = true;
            rt.decision_commit   = true;
            break;
        case DistributedTxnState::ABORTING:
            rt.state             = RecoverableTwoPhaseState::ABORTING;
            rt.decision_recorded = true;
            rt.decision_commit   = false;
            break;
        default:
            rt.state = RecoverableTwoPhaseState::UNKNOWN;
            break;
        }

        result.push_back(std::move(rt));
    }

    return result;
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

    size_t fully_aborted = 0;
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
        bool is_fully_aborted = false;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            const auto* txn = findTransaction(tid);
            is_fully_aborted = (txn && txn->state == DistributedTxnState::ABORTED);
        }
        if (is_fully_aborted) {
            ++stat_timeout_aborts_;
            ++fully_aborted;
        } else {
            THEMIS_WARN("DistributedTransactionManager [{}] txn={} timeout ABORT delivery "
                        "incomplete; transaction remains in non-terminal state",
                        coordinator_id_, tid);
        }
    }

    return fully_aborted;
}

// ─────────────────────────────────────────────────────────────────────────────
// Failure detection
// ─────────────────────────────────────────────────────────────────────────────

bool DistributedTransactionManager::isParticipantAlive(const std::string& node_id) const {
    // DTM-3 fix: Distinguish between in-process and remote participants, and
    // support an injectable liveness check bridge for remote nodes.
    //
    // Priority:
    //   1. In-process participant (callback != nullptr) → always alive.
    //   2. Remote participant: try per-instance config_.liveness_check_fn.
    //   3. Remote participant: try process-wide static getLivenessCheckFn().
    //   4. Remote participant with no bridge: conservatively return false.
    //
    // Unknown node_id (not in any active transaction) → return true to avoid
    // spurious aborts.
    bool      found_remote    = false;
    std::string endpoint_found;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& [tid, txn] : transactions_) {
            if (found_remote) break;
            for (const auto& part : txn.participants) {
                if (part.node_id != node_id) continue;
                if (part.callback != nullptr) {
                    return true;   // in-process — always alive
                }
                // Remote participant found.
                endpoint_found = part.endpoint;
                found_remote   = true;
                break; // exit inner loop; outer loop exits via found_remote check
            }
        }
    }

    if (!found_remote) {
        // Not found in any active transaction — treat as alive.
        return true;
    }

    // Remote participant: consult bridges in priority order.

    // 1. Per-instance liveness_check_fn.
    if (config_.liveness_check_fn) {
        try {
            return (*config_.liveness_check_fn)(endpoint_found, node_id);
        } catch (const std::exception& e) {
            THEMIS_WARN("DistributedTransactionManager [{}] liveness_check_fn threw for "
                        "node={}: {} — treating as not alive",
                        coordinator_id_, node_id, e.what());
            return false;
        } catch (const std::string& e) {
            THEMIS_WARN("DistributedTransactionManager [{}] liveness_check_fn threw for "
                        "node={}: {} — treating as not alive",
                        coordinator_id_, node_id, e);
            return false;
        } catch (const char* e) {
            THEMIS_WARN("DistributedTransactionManager [{}] liveness_check_fn threw for "
                        "node={}: {} — treating as not alive",
                        coordinator_id_, node_id, (e ? e : "<null>"));
            return false;
        }
    }

    // 2. Process-wide static liveness check.
    if (auto static_fn = getLivenessCheckFn()) {
        try {
            return static_fn(node_id, endpoint_found);
        } catch (const std::exception& e) {
            THEMIS_WARN("DistributedTransactionManager [{}] static liveness check threw for "
                        "node={}: {} — treating as not alive",
                        coordinator_id_, node_id, e.what());
            return false;
        } catch (const std::string& e) {
            THEMIS_WARN("DistributedTransactionManager [{}] static liveness check threw for "
                        "node={}: {} — treating as not alive",
                        coordinator_id_, node_id, e);
            return false;
        } catch (const char* e) {
            THEMIS_WARN("DistributedTransactionManager [{}] static liveness check threw for "
                        "node={}: {} — treating as not alive",
                        coordinator_id_, node_id, (e ? e : "<null>"));
            return false;
        }
    }

    // 3. No bridge configured: conservatively report remote node as not alive.
    return false;
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
            const std::string ep  = part.endpoint;
            const std::string nid = part.node_id;
            const std::string tid = txn_id;
            const std::string cid = coordinator_id_;
            const std::set<std::string> keys = part.affected_keys;

            if (ep.empty()) {
                THEMIS_ERROR("DistributedTransactionManager [{}] cannot send Phase-1 PREPARE for "
                             "remote participant node={} — empty endpoint; voting ABORT",
                             coordinator_id_, part.node_id);
                futures.push_back(submitTask([nid]() -> VoteResult {
                    return {nid, true, /*can_commit=*/false};
                }));
                continue;
            }

            if (!isParticipantAlive(nid)) {
                THEMIS_WARN("DistributedTransactionManager [{}] txn={} participant {} failed "
                            "liveness check before Phase-1 PREPARE; voting ABORT",
                            coordinator_id_, txn_id, nid);
                futures.push_back(submitTask([nid]() -> VoteResult {
                    return {nid, true, /*can_commit=*/false};
                }));
                continue;
            }

            // Phase-1 RPC bridge — three layers of injection mirroring Phase-2.
            // Preference order: phase1_rpc_fn > remote_phase1_dispatch > static RpcPhase1Fn.
            if (config_.phase1_rpc_fn) {
                auto rpc_fn = *config_.phase1_rpc_fn;
                futures.push_back(submitTask([rpc_fn, ep, nid, tid, cid, keys]() -> VoteResult {
                    try {
                        const bool vote = rpc_fn(ep, tid, keys);
                        if (!vote) {
                            THEMIS_WARN("2PC Phase-1 RPC ABORT vote from node={} txn={} coordinator={}",
                                        nid, tid, cid);
                        }
                        return {nid, true, vote};
                    } catch (const std::exception& ex) {
                        THEMIS_ERROR("2PC Phase-1 RPC threw for node={} txn={} coordinator={}: {}",
                                     nid, tid, cid, ex.what());
                        return {nid, true, /*can_commit=*/false};
                    }
                }));
                continue;
            }

            if (config_.remote_phase1_dispatch) {
                auto dispatch = config_.remote_phase1_dispatch;
                futures.push_back(submitTask([dispatch, ep, nid, tid, cid, keys]() -> VoteResult {
                    try {
                        const bool vote = dispatch(tid, nid, ep, keys);
                        if (!vote) {
                            THEMIS_WARN("2PC remote_phase1_dispatch ABORT vote from node={} txn={} coordinator={}",
                                        nid, tid, cid);
                        }
                        return {nid, true, vote};
                    } catch (const std::exception& ex) {
                        THEMIS_ERROR("2PC remote_phase1_dispatch threw for node={} txn={} coordinator={}: {}",
                                     nid, tid, cid, ex.what());
                        return {nid, true, /*can_commit=*/false};
                    }
                }));
                continue;
            }

            if (auto legacy_p1_fn = getRpcPhase1Fn()) {
                futures.push_back(submitTask([legacy_p1_fn, ep, nid, tid, cid, keys]() -> VoteResult {
                    try {
                        const bool vote = legacy_p1_fn(nid, tid, keys);
                        if (!vote) {
                            THEMIS_WARN("2PC legacy Phase-1 RPC ABORT vote from node={} txn={} coordinator={}",
                                        nid, tid, cid);
                        }
                        return {nid, true, vote};
                    } catch (const std::exception& ex) {
                        THEMIS_ERROR("2PC legacy Phase-1 RPC threw for node={} txn={} coordinator={}: {}",
                                     nid, tid, cid, ex.what());
                        return {nid, true, /*can_commit=*/false};
                    }
                }));
                continue;
            }

            // No Phase-1 bridge: fail-closed with ABORT vote.
            THEMIS_WARN("DistributedTransactionManager [{}] txn={} participant {} has no callback "
                        "(remote) — voting ABORT (no Phase-1 RPC bridge configured)",
                        coordinator_id_, txn_id, part.node_id);
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
    size_t definitive_votes = 0;
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
            if (txn && result.voted) {
                txn->votes[result.node_id] = result.can_commit;
            }
        }
        vote_cv_.notify_all();

        if (result.voted) {
            ++definitive_votes;
        }

        if (result.voted && !result.can_commit) {
            all_commit   = false;
            abort_reason = "Participant " + result.node_id + " voted ABORT";
        }
    }

    if (all_commit && definitive_votes == 0) {
        all_commit = false;
        abort_reason = "No participant returned a definitive PREPARE vote";
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

bool DistributedTransactionManager::runPhase2Unlocked(
    const TransactionId&         txn_id,
    const std::vector<Participant>& parts,
    bool                         do_commit
) {
    const auto deadline =
        std::chrono::steady_clock::now() + config_.commit_timeout;

    std::vector<std::future<bool>> futures;
    futures.reserve(parts.size());
    bool all_delivered = true;

    for (const auto& part : parts) {
        if (!part.callback) {
            // Remote participant: deliver Phase-2 decision via one of the configured
            // bridges (new Phase2RpcFn, legacy static RpcPhase2Fn, or
            // remote_phase2_dispatch).
            const std::string ep  = part.endpoint;
            const std::string nid = part.node_id;
            const std::string tid = txn_id;
            const std::string cid = coordinator_id_;
            const bool        dc  = do_commit;

            if (ep.empty()) {
                THEMIS_ERROR("DistributedTransactionManager [{}] cannot deliver Phase-2 {} for "
                             "remote participant node={} — empty endpoint",
                             coordinator_id_, do_commit ? "COMMIT" : "ABORT", part.node_id);
                all_delivered = false;
                continue;
            }

            if (config_.phase2_rpc_fn) {
                auto rpc_fn = *config_.phase2_rpc_fn;
                futures.push_back(submitTask([rpc_fn, ep, nid, tid, cid, dc]() {
                    return deliverPhase2WithRetry(
                        [&]() {
                            rpc_fn(ep, tid, dc);
                            return true;
                        },
                        "Phase-2 RPC", nid, tid, cid, dc);
                }));
                continue;
            }

            if (config_.remote_phase2_dispatch) {
                auto remote_dispatch = config_.remote_phase2_dispatch;
                futures.push_back(submitTask([remote_dispatch, ep, nid, tid, cid, dc]() {
                    return deliverPhase2WithRetry(
                        [&]() {
                            return remote_dispatch(tid, nid, ep, dc);
                        },
                        "remote_phase2_dispatch", nid, tid, cid, dc);
                }));
                continue;
            }

            if (auto legacy_rpc_fn = getRpcPhase2Fn()) {
                futures.push_back(submitTask([legacy_rpc_fn, nid, tid, cid, dc]() {
                    return deliverPhase2WithRetry(
                        [&]() {
                            legacy_rpc_fn(nid, tid, dc);
                            return true;
                        },
                        "legacy Phase-2 RPC", nid, tid, cid, dc);
                }));
                continue;
            }

            // CRITICAL FIX for stub #279: No Phase-2 transport available.
            // This should have been caught during initialization if remote_phase1_dispatch is set,
            // or during beginDistributed for per-transaction participants.
            // If we reach here, either: (a) configuration is incomplete, or (b) Phase-2 bridge
            // was cleared after initialization/beginDistributed (production safety violation).
            // 
            // FIX: Implement fail-closed behavior - fail the entire Phase-2 delivery
            // instead of silently skipping this participant, which would leave them
            // in PREPARED state indefinitely.
            THEMIS_ERROR("DistributedTransactionManager [{}] SECURITY/CONSISTENCY VIOLATION: "
                         "cannot deliver Phase-2 {} for remote participant node={} endpoint='{}' — "
                         "no remote dispatcher configured. Transaction {} will be marked for "
                         "manual recovery. This indicates misconfiguration or a security violation.",
                         coordinator_id_, do_commit ? "COMMIT" : "ABORT", part.node_id, part.endpoint,
                         txn_id);
            all_delivered = false;
            // Do NOT continue processing — mark failure and stop trying to deliver to other
            // participants in this batch. The caller will handle transaction abort/retry.
            break;
        }
        IDistributedParticipantCallback* cb  = part.callback;
        const std::string                nid = part.node_id;
        const std::string                tid = txn_id;
        const std::string                cid = coordinator_id_;

        if (do_commit) {
            futures.push_back(submitTask([cb, nid, tid, cid]() {
                try {
                    cb->onCommit(tid);
                    return true;
                } catch (const std::exception& ex) {
                    THEMIS_ERROR("2PC COMMIT threw for node={} txn={} coordinator={}: {}",
                                 nid, tid, cid, ex.what());
                    return false;
                }
            }));
        } else {
            futures.push_back(submitTask([cb, nid, tid, cid]() {
                try {
                    cb->onAbort(tid);
                    return true;
                } catch (const std::exception& ex) {
                    THEMIS_ERROR("2PC ABORT threw for node={} txn={} coordinator={}: {}",
                                 nid, tid, cid, ex.what());
                    return false;
                }
            }));
        }
    }

    for (auto& fut : futures) {
        const auto remaining = deadline - std::chrono::steady_clock::now();
        if (remaining <= std::chrono::milliseconds(0)) {
            THEMIS_WARN("DistributedTransactionManager [{}] Phase-2 deadline expired for txn={}",
                        coordinator_id_, txn_id);
            all_delivered = false;
            break;
        }
        const auto status = fut.wait_for(remaining);
        if (status == std::future_status::timeout) {
            THEMIS_ERROR("DistributedTransactionManager [{}] participant timed out in Phase-2 for txn={}",
                         coordinator_id_, txn_id);
            all_delivered = false;
            continue;
        }
        try {
            if (!fut.get()) {
                all_delivered = false;
            }
        } catch (const std::exception& ex) {
            THEMIS_ERROR("DistributedTransactionManager [{}] Phase-2 future failed for txn={}: {}",
                         coordinator_id_, txn_id, ex.what());
            all_delivered = false;
        }
    }

    return all_delivered;
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
                    // Use wait_for with timeout (30s) to prevent indefinite blocking.
                    // If timeout occurs, loop will re-acquire lock and re-check conditions.
                    const auto timeout_result = pool_cv_.wait_for(
                        lock,
                        std::chrono::seconds(30),
                        [this] { return pool_stop_ || !task_queue_.empty(); }
                    );
                     
                    if (!timeout_result && !pool_stop_) {
                        // Timeout occurred and pool is not stopping; log warning and continue waiting.
                        THEMIS_WARN("DistributedTransactionManager [{}] thread pool worker timeout "
                                  "after 30s waiting for tasks", coordinator_id_);
                        continue;
                    }
                     
                    if (pool_stop_ && task_queue_.empty()) return;
                     
                    if (task_queue_.empty()) {
                        // Spurious wakeup or timeout with empty queue; loop again.
                        continue;
                    }
                     
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

        // Execute Phase-1 directly in the flush thread.
        // runPhase1Unlocked() already parallelizes participant calls via the
        // worker pool (or std::async in legacy mode). Submitting the whole
        // Phase-1 call into the same worker pool can deadlock under load when
        // workers block while waiting on nested pool tasks.
        for (auto& entry : batch) {
            bool result = false;
            try {
                result = runPhase1Unlocked(entry.txn_id);
            } catch (const std::exception& ex) {
                THEMIS_WARN("DistributedTransactionManager [{}] batch Phase-1 failed for txn {}: {}",
                            coordinator_id_, entry.txn_id, ex.what());
                result = false;
            } catch (const std::string& ex) {
                THEMIS_WARN("DistributedTransactionManager [{}] batch Phase-1 failed for txn {}: {}",
                            coordinator_id_, entry.txn_id, ex);
                result = false;
            } catch (const char* ex) {
                THEMIS_WARN("DistributedTransactionManager [{}] batch Phase-1 failed for txn {}: {}",
                            coordinator_id_, entry.txn_id, (ex ? ex : "<null>"));
                result = false;
            }
            try {
                entry.result.set_value(result);
            } catch (const std::future_error&) {
                // Promise already satisfied/broken — ignore in flush loop.
            }
        }
    }
}

} // namespace themis::transaction
