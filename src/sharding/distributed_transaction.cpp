/**
 * @file distributed_transaction.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=29, M=28, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: distributed_transaction.cpp | Version: 0.0.47 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 93/100 | Lines: 958
 * Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=7, H=20, M=21, L=0
 * PR History (last 5): #4212 fix(chimera/percolator): re... (2026-03-15) | #792 Enhance distributed transac... (2026-03-11) | #1123 Optimize distributed 2PC wi... (2026-03-11)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

// Copyright 2025 ThemisDB
// Licensed under MIT License
//
// Distributed Transaction Coordinator with Two-Phase Commit (2PC)
//
// CC-5 NOTE: ThemisDB contains three independent 2PC implementations with
// different state machines, WAL integration depths, and recovery logic:
//   1. two_phase_commit_coordinator.cpp  — standalone coordinator
//   2. cross_shard_transaction.cpp       — CrossShardTransactionCoordinator
//   3. distributed_transaction.cpp       (this file) — DistributedTransactionCoordinator
// A transaction begun with one coordinator CANNOT be recovered by another.
// Future work: unify under a single 2PC engine (Target: v2.0.0).
//
// This implementation provides ACID guarantees for transactions spanning multiple
// shards using the classical 2PC protocol enhanced with TrueTime for external
// consistency.
//
// Key Features:
// - Two-phase commit protocol (PREPARE → COMMIT/ABORT)
// - TrueTime integration for globally consistent timestamps
// - Parallel participant communication for better performance
// - Snapshot isolation for read-only transactions (wait-free)
// - Configurable timeouts and retry logic
//
// Protocol Flow:
//   1. BEGIN: Create transaction, register participants
//   2. OPERATIONS: Accumulate operations per shard
//   3. COMMIT:
//      a. Phase 1 (PREPARE): All participants vote COMMIT or ABORT
//      b. Assign TrueTime commit timestamp
//      c. Wait until timestamp is in the past (external consistency)
//      d. Phase 2 (COMMIT): Apply changes with timestamp
//   4. Transaction complete (COMMITTED or ABORTED)
//
// For detailed documentation, see docs/DISTRIBUTED_TRANSACTIONS.md

#include "sharding/distributed_transaction.h"
#include "sharding/shard_rpc_client.h"
#include "sharding/metrics_registry.h"
#include "sharding/prometheus_metrics.h"
#include "utils/logger.h"
#include "utils/thread_join_utils.h"
#include <random>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <set>
#include <map>
#include <unordered_map>
#include <chrono>
#include <future>

namespace themis::sharding {

// Helper: return the configured coordinator ID, falling back to "default"
/** @brief Return coordinator label used in metrics, defaulting to "default". */
static inline std::string coordinatorLabel(const DistributedTransactionCoordinator::Config& cfg) {
    return cfg.coordinator_id.empty() ? "default" : cfg.coordinator_id;
}

/** @brief Construct distributed transaction coordinator and initialize recovery WAL. */
DistributedTransactionCoordinator::DistributedTransactionCoordinator(
    std::shared_ptr<TrueTime> truetime,
    const Config& config
)
    : truetime_(truetime)
    , config_(config)
{
    // Initialize WAL manager if recovery logging is enabled
    if (config_.enable_recovery_log) {
        WALManagerConfig wal_config;
        wal_config.wal_directory = "./wal/coordinator";
        wal_config.segment_size = 16 * 1024 * 1024;  // 16 MB
        wal_config.sync_on_write = true;             // Durability
        
        wal_manager_ = std::make_unique<WALManager>(wal_config);
        
        // Recover any in-doubt transactions from WAL
        recoverTransactions();
    }
}

/** @brief Begin new distributed transaction with selected participants and isolation. */
std::string DistributedTransactionCoordinator::beginTransaction(
    const std::vector<std::string>& shard_ids,
    DistributedIsolationLevel isolation_level
) {
    std::lock_guard<std::timed_mutex> lock(mutex_);
    
    // Generate unique transaction ID
    std::string txn_id = generateTransactionId();
    
    // Create transaction
    DistributedTransaction txn;
    txn.transaction_id = txn_id;
    txn.state = TransactionState::ACTIVE;
    txn.isolation_level = isolation_level;
    txn.start_time = truetime_->now().latest;
    
    // Add participants — resolve real gRPC endpoint from registry when available
    for (const auto& shard_id : shard_ids) {
        TransactionParticipant participant;
        participant.shard_id = shard_id;

        auto it = shard_endpoint_map_.find(shard_id);
        if (it != shard_endpoint_map_.end()) {
            participant.endpoint = it->second;
        } else {
            // Fallback: syntactic placeholder — 2PC RPCs will fail at connect time
            // until a real endpoint is registered via setShardEndpointMap().
            participant.endpoint = "shard://" + shard_id;
        }

        participant.prepared  = false;
        participant.committed = false;
        txn.participants.push_back(participant);
    }
    
    transactions_[txn_id] = std::move(txn);
    total_transactions_.fetch_add(1, std::memory_order_relaxed);
    
    THEMIS_DEBUG("Began distributed transaction {} with {} shards, isolation={}",
                txn_id, shard_ids.size(),
                isolation_level == DistributedIsolationLevel::SERIALIZABLE
                    ? "SERIALIZABLE" : "SNAPSHOT_ISOLATION");
    
    return txn_id;
}

/** @brief Append one shard-targeted operation to an active transaction. */
bool DistributedTransactionCoordinator::addOperation(
    const std::string& txn_id,
    const std::string& shard_id,
    const nlohmann::json& operation
) {
    std::lock_guard<std::timed_mutex> lock(mutex_);
    
    auto it = transactions_.find(txn_id);
    if (it == transactions_.end()) {
        return false;
    }
    
    auto& txn = it->second;
    if (txn.state != TransactionState::ACTIVE) {
        return false;
    }
    
    // Add operation to transaction
    if (!txn.operations.contains(shard_id)) {
        txn.operations[shard_id] = nlohmann::json::array();
    }
    txn.operations[shard_id].push_back(operation);
    
    return true;
}

/** @brief Commit transaction via Percolator or 2PC depending on configuration. */
bool DistributedTransactionCoordinator::commit(const std::string& txn_id) {
    std::unique_lock<std::timed_mutex> lock(mutex_);
    
    auto it = transactions_.find(txn_id);
    if (it == transactions_.end()) {
        return false;
    }
    
    auto& txn = it->second;
    if (txn.state != TransactionState::ACTIVE) {
        return false;
    }
    
    const std::string coordinator_id = coordinatorLabel(config_);

    // -----------------------------------------------------------------------
    // Protocol selection:
    //   SNAPSHOT_ISOLATION + use_percolator_for_snapshot → Percolator path
    //   SERIALIZABLE (or flag disabled)               → 2PC path
    // -----------------------------------------------------------------------
    const bool use_percolator =
        config_.use_percolator_for_snapshot &&
        (txn.isolation_level == DistributedIsolationLevel::SNAPSHOT_ISOLATION);

    if (use_percolator) {
        THEMIS_DEBUG("Transaction {} using Percolator commit path (SNAPSHOT_ISOLATION)",
                     txn_id);
        lock.unlock();
        const bool committed = percolatorCommit(txn);
        if (!lock.try_lock_for(std::chrono::milliseconds(config_.rpc_timeout_ms))) {
            spdlog::error("Lock acquisition timeout after percolatorCommit for txn {}", txn_id);
            return false;
        }

        if (committed) {
            txn.state = TransactionState::COMMITTED;
            committed_transactions_.fetch_add(1, std::memory_order_relaxed);
            if (config_.enable_recovery_log) {
                logTransactionForRecovery(txn);
            }
        } else {
            txn.state = TransactionState::ABORTED;
            aborted_transactions_.fetch_add(1, std::memory_order_relaxed);
            if (auto m = ShardingMetricsRegistry::instance().getMetrics()) {
                m->record2PCAbort(coordinator_id, "percolator_commit_failed");
                m->record2PCTransaction(coordinator_id, false);
            }
        }

        return committed;
    }

    // -----------------------------------------------------------------------
    // 2PC path (SERIALIZABLE or Percolator disabled).
    // -----------------------------------------------------------------------
    
    // Phase 1: Prepare
    txn.state = TransactionState::PREPARING;
    lock.unlock();
    
    auto prepare_start = std::chrono::steady_clock::now();
    bool prepared = preparePhase(txn);
    auto prepare_ms = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - prepare_start).count();
    
    if (auto m = ShardingMetricsRegistry::instance().getMetrics()) {
        m->record2PCPreparePhase(coordinator_id, prepare_ms, prepared);
    }
    
    if (!lock.try_lock_for(std::chrono::milliseconds(config_.rpc_timeout_ms))) {
        spdlog::error("Lock acquisition timeout after preparePhase for txn {}", txn_id);
        return false;
    }
    if (!prepared) {
        txn.state = TransactionState::ABORTING;
        txn.error_detail = "Prepare phase failed - one or more participants could not prepare";
        lock.unlock();
        
        // Abort transaction on all participants
        THEMIS_WARN("Transaction {} aborting - prepare phase failed", txn_id);
        for (auto& participant : txn.participants) {
            if (!sendAbort(participant, txn_id)) {
                THEMIS_ERROR("Failed to abort participant {} for transaction {}",
                           participant.shard_id, txn_id);
            }
        }
        
        if (!lock.try_lock_for(std::chrono::milliseconds(config_.rpc_timeout_ms))) {
            spdlog::error("Lock acquisition timeout updating abort state after prepare "
                          "failure for txn {}", txn_id);
            return false;
        }
        txn.state = TransactionState::ABORTED;
        aborted_transactions_.fetch_add(1, std::memory_order_relaxed);

        if (auto m = ShardingMetricsRegistry::instance().getMetrics()) {
            m->record2PCAbort(coordinator_id, "prepare_phase_failed");
            m->record2PCTransaction(coordinator_id, false);
        }
        return false;
    }
    
    txn.state = TransactionState::PREPARED;
    
    // Log PREPARED state for recovery (in case coordinator crashes before commit)
    if (config_.enable_recovery_log) {
        logPreparedStateForRecovery(txn);
    }
    
    // Assign commit timestamp using TrueTime
    // Use the latest time to ensure all reads see this transaction
    txn.commit_time = truetime_->now().latest;
    
    // Wait until commit timestamp is definitely in the past
    // This is the key TrueTime operation for external consistency
    lock.unlock();
    truetime_->waitUntil(txn.commit_time);
    if (!lock.try_lock_for(std::chrono::milliseconds(config_.rpc_timeout_ms))) {
        spdlog::error("Lock acquisition timeout after TrueTime wait for txn {}", txn_id);
        return false;
    }
    
    // Phase 2: Commit
    txn.state = TransactionState::COMMITTING;
    lock.unlock();

    // DTM-4: Write and durably flush the COMMIT decision to WAL *before*
    // broadcasting Phase 2 to participants.  A coordinator crash after flush
    // but before broadcast is recoverable; a crash before flush is not.
    if (wal_manager_ && config_.enable_recovery_log) {
        try {
            WALEntry commit_intent;
            commit_intent.type = WALEntryType::COMMIT_TX;
            commit_intent.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            commit_intent.transaction_id = txn.transaction_id;
            commit_intent.data = {{"state", "COMMITTING"},
                                  {"commit_time", txn.commit_time.count()}};
            wal_manager_->append(commit_intent);
            wal_manager_->flush(); // must be durable before Phase 2
        } catch (const std::exception& e) {
            THEMIS_ERROR("DTM: Failed to flush COMMIT WAL entry for txn '{}': {}. "
                         "Aborting to prevent unsafe state.", txn.transaction_id, e.what());
            if (!lock.try_lock_for(std::chrono::milliseconds(config_.rpc_timeout_ms))) {
                spdlog::error("Lock acquisition timeout in WAL flush error handler for txn {}",
                              txn_id);
                return false;
            }
            txn.state = TransactionState::ABORTED;
            txn.error_detail = "WAL flush failed before Phase 2 COMMIT";
            aborted_transactions_.fetch_add(1, std::memory_order_relaxed);
            return false;
        }
    }

    auto commit_start = std::chrono::steady_clock::now();
    bool committed = retryCommitPhase(txn);
    auto commit_ms = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - commit_start).count();
    
    if (auto m = ShardingMetricsRegistry::instance().getMetrics()) {
        m->record2PCCommitPhase(coordinator_id, commit_ms, committed);
        m->record2PCTransaction(coordinator_id, committed);
    }
    
    if (!lock.try_lock_for(std::chrono::milliseconds(config_.rpc_timeout_ms))) {
        spdlog::error("Lock acquisition timeout updating commit result for txn {}", txn_id);
        return false;
    }
    if (committed) {
        txn.state = TransactionState::COMMITTED;
        committed_transactions_.fetch_add(1, std::memory_order_relaxed);
        
        // Log successful commit for recovery
        if (config_.enable_recovery_log) {
            logTransactionForRecovery(txn);
        }
    } else {
        txn.state = TransactionState::ABORTED;
        txn.error_detail = "Commit phase failed after retries";
        aborted_transactions_.fetch_add(1, std::memory_order_relaxed);
        
        THEMIS_ERROR("Transaction {} commit failed after {} retries", 
                    txn_id, txn.commit_retry_count);
        
        if (auto m = ShardingMetricsRegistry::instance().getMetrics()) {
            m->record2PCAbort(coordinator_id, "commit_phase_failed_after_retries");
        }
    }
    
    return committed;
}

/** @brief Abort transaction and propagate abort to all participants. */
bool DistributedTransactionCoordinator::abort(const std::string& txn_id) {
    std::lock_guard<std::timed_mutex> lock(mutex_);
    
    auto it = transactions_.find(txn_id);
    if (it == transactions_.end()) {
        return false;
    }
    
    auto& txn = it->second;
    txn.state = TransactionState::ABORTING;
    
    // Send abort to all participants
    for (auto& participant : txn.participants) {
        sendAbort(participant, txn_id);
    }
    
    txn.state = TransactionState::ABORTED;
    aborted_transactions_.fetch_add(1, std::memory_order_relaxed);
    
    const std::string coordinator_id = coordinatorLabel(config_);
    if (auto m = ShardingMetricsRegistry::instance().getMetrics()) {
        m->record2PCAbort(coordinator_id, "explicit_abort");
        m->record2PCTransaction(coordinator_id, false);
    }
    
    return true;
}

/** @brief Execute read-only snapshot operations without 2PC locking. */
nlohmann::json DistributedTransactionCoordinator::executeReadOnly(
    const std::vector<std::string>& shard_ids,
    [[maybe_unused]] const nlohmann::json& operations
) {
    // Read-only transactions use TrueTime for snapshot isolation
    // 1. Get snapshot timestamp (latest bound ensures we see all committed data)
    auto snapshot_ts = truetime_->now().latest;
    
    // 2. Execute reads at snapshot timestamp (no locking needed)
    nlohmann::json results = nlohmann::json::object();
    
    for (const auto& shard_id : shard_ids) {
        // v1.3.0: Real RPC implementation for snapshot reads
        try {
            ShardRPCClient::Config rpc_config;
            rpc_config.endpoint = "shard://" + shard_id;
            rpc_config.timeout_ms = 5000;
            
            ShardRPCClient client(rpc_config);
            
            // Execute snapshot read at specific timestamp
            nlohmann::json query = nlohmann::json::object({
                {"shard_id", shard_id},
                {"snapshot_timestamp", snapshot_ts.count()}
            });
            
            auto shard_results = client.snapshotRead(snapshot_ts.count(), query);
            
            results[shard_id] = {
                {"status", "success"},
                {"snapshot_timestamp", snapshot_ts.count()},
                {"data", shard_results}
            };
            
        } catch (const std::exception& e) {
            THEMIS_ERROR("Snapshot read from shard {} failed: {}", shard_id, e.what());
            results[shard_id] = {
                {"status", "error"},
                {"error", e.what()}
            };
        }
    }
    
    readonly_transactions_.fetch_add(1, std::memory_order_relaxed);
    
    return results;
}

/** @brief Return current state for transaction ID when present. */
std::optional<TransactionState> DistributedTransactionCoordinator::getTransactionState(
    const std::string& txn_id
) const {
    std::lock_guard<std::timed_mutex> lock(mutex_);
    
    auto it = transactions_.find(txn_id);
    if (it == transactions_.end()) {
        return std::nullopt;
    }
    
    return it->second.state;
}

/** @brief Return coordinator counters and active transaction count as JSON. */
nlohmann::json DistributedTransactionCoordinator::getStatistics() const {
    return nlohmann::json{
        {"total_transactions", total_transactions_.load()},
        {"committed_transactions", committed_transactions_.load()},
        {"aborted_transactions", aborted_transactions_.load()},
        {"readonly_transactions", readonly_transactions_.load()},
        {"active_transactions", transactions_.size()}
    };
}

/** @brief Register/replace shard ID to endpoint mapping used for RPC routing. */
void DistributedTransactionCoordinator::setShardEndpointMap(
    std::unordered_map<std::string, std::string> map)
{
    std::lock_guard<std::timed_mutex> lock(mutex_);
    shard_endpoint_map_ = std::move(map);
}

/** @brief Execute prepare phase in parallel across all participants. */
bool DistributedTransactionCoordinator::preparePhase(DistributedTransaction& txn) {
    // Send prepare to all participants in parallel with an explicit timeout so
    // that a slow or stuck shard cannot block the coordinator indefinitely.
    std::vector<std::future<void>> futures;
    std::atomic<bool> all_prepared{true};
    std::mutex error_mutex;
    std::vector<std::string> error_details;

    for (auto& participant : txn.participants) {
        futures.push_back(std::async(std::launch::async,
            [this, &participant, &txn, &all_prepared, &error_mutex, &error_details]() {
                if (!sendPrepare(participant, txn.transaction_id)) {
                    all_prepared.store(false, std::memory_order_relaxed);
                    std::lock_guard<std::mutex> lock(error_mutex);
                    error_details.push_back("Shard " + participant.shard_id +
                                            " failed to prepare: " + participant.error_msg);
                }
            }));
    }

    const auto deadline = std::chrono::milliseconds(config_.prepare_timeout_ms);
    for (auto& f : futures) {
        if (f.wait_for(deadline) == std::future_status::timeout) {
            all_prepared.store(false, std::memory_order_relaxed);
            std::lock_guard<std::mutex> lock(error_mutex);
            error_details.push_back("prepare timed out after " +
                                    std::to_string(config_.prepare_timeout_ms) + "ms");
        }
    }

    if (!all_prepared.load()) {
        std::lock_guard<std::mutex> lock(error_mutex);
        txn.error_detail = "Prepare failures: ";
        for (const auto& err : error_details) {
            txn.error_detail += err + "; ";
        }
    }

    return all_prepared.load();
}

/** @brief Execute commit phase in parallel across all participants. */
bool DistributedTransactionCoordinator::commitPhase(DistributedTransaction& txn) {
    // Send commit to all participants in parallel with an explicit timeout so
    // that a slow or stuck shard cannot block the coordinator indefinitely.
    std::vector<std::future<void>> futures;
    std::atomic<bool> all_committed{true};
    std::mutex error_mutex;
    std::vector<std::string> error_details;

    for (auto& participant : txn.participants) {
        futures.push_back(std::async(std::launch::async,
            [this, &participant, &txn, &all_committed, &error_mutex, &error_details]() {
                if (!sendCommit(participant, txn.transaction_id, txn.commit_time)) {
                    all_committed.store(false, std::memory_order_relaxed);
                    std::lock_guard<std::mutex> lock(error_mutex);
                    error_details.push_back("Shard " + participant.shard_id +
                                            " failed to commit: " + participant.error_msg);
                }
            }));
    }

    const auto deadline = std::chrono::milliseconds(config_.commit_timeout_ms);
    for (auto& f : futures) {
        if (f.wait_for(deadline) == std::future_status::timeout) {
            all_committed.store(false, std::memory_order_relaxed);
            std::lock_guard<std::mutex> lock(error_mutex);
            error_details.push_back("commit timed out after " +
                                    std::to_string(config_.commit_timeout_ms) + "ms");
        }
    }

    if (!all_committed.load()) {
        std::lock_guard<std::mutex> lock(error_mutex);
        txn.error_detail = "Commit failures: ";
        for (const auto& err : error_details) {
            txn.error_detail += err + "; ";
        }
    }

    return all_committed.load();
}

/** @brief Send PREPARE RPC to one participant and record vote result. */
bool DistributedTransactionCoordinator::sendPrepare(
    TransactionParticipant& participant,
    const std::string& txn_id
) {
    // v1.3.0: Real RPC implementation for 2PC PREPARE
    try {
        ShardRPCClient::Config rpc_config;
        rpc_config.endpoint = participant.endpoint;
        rpc_config.timeout_ms = static_cast<int>(config_.rpc_timeout_ms);
        rpc_config.max_retries = config_.max_retries;
        
        ShardRPCClient client(rpc_config);
        
        // Get operations for this shard
        nlohmann::json operations = nlohmann::json::array();
        auto it = transactions_.find(txn_id);
        if (it != transactions_.end()) {
            auto& txn = it->second;
            if (txn.operations.contains(participant.shard_id)) {
                operations = txn.operations[participant.shard_id];
            }
        }
        
        // Send PREPARE request
        bool vote_commit = client.prepare(txn_id, operations);
        participant.prepared = vote_commit;
        
        THEMIS_DEBUG("PREPARE shard {}: vote={}", 
                    participant.shard_id, vote_commit ? "COMMIT" : "ABORT");
        
        return vote_commit;
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("PREPARE RPC to shard {} failed: {}", 
                    participant.shard_id, e.what());
        participant.prepared = false;
        return false;
    }
}

/** @brief Send COMMIT RPC with commit timestamp to one participant. */
bool DistributedTransactionCoordinator::sendCommit(
    TransactionParticipant& participant,
    const std::string& txn_id,
    std::chrono::nanoseconds commit_timestamp
) {
    // v1.3.0: Real RPC implementation for 2PC COMMIT
    try {
        ShardRPCClient::Config rpc_config;
        rpc_config.endpoint = participant.endpoint;
        rpc_config.timeout_ms = static_cast<int>(config_.rpc_timeout_ms);
        rpc_config.max_retries = config_.max_retries;
        
        ShardRPCClient client(rpc_config);
        
        // Send COMMIT request with timestamp for MVCC
        bool committed = client.commit(txn_id, commit_timestamp.count());
        participant.committed = committed;
        
        THEMIS_DEBUG("COMMIT shard {}: success={}", 
                    participant.shard_id, committed);
        
        return committed;
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("COMMIT RPC to shard {} failed: {}", 
                    participant.shard_id, e.what());
        participant.committed = false;
        return false;
    }
}

/** @brief Send ABORT RPC to one participant. */
bool DistributedTransactionCoordinator::sendAbort(
    TransactionParticipant& participant,
    const std::string& txn_id
) {
    // v1.3.0: Real RPC implementation for 2PC ABORT
    try {
        ShardRPCClient::Config rpc_config;
        rpc_config.endpoint = participant.endpoint;
        rpc_config.timeout_ms = static_cast<int>(config_.rpc_timeout_ms);
        rpc_config.max_retries = config_.max_retries;
        
        ShardRPCClient client(rpc_config);
        
        // Send ABORT request
        bool aborted = client.abort(txn_id);
        participant.prepared = false;
        participant.committed = false;
        
        THEMIS_DEBUG("ABORT shard {}: success={}", 
                    participant.shard_id, aborted);
        
        return aborted;
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("ABORT RPC to shard {} failed: {}", 
                    participant.shard_id, e.what());
        return false;
    }
}

/** @brief Generate random hexadecimal transaction identifier. */
std::string DistributedTransactionCoordinator::generateTransactionId() {
    static std::random_device rd;
    static std::mt19937_64 gen(rd());
    static std::uniform_int_distribution<uint64_t> dis;
    
    std::ostringstream oss;
    oss << "txn-" << std::hex << std::setfill('0') << std::setw(16) << dis(gen);
    return oss.str();
}

/** @brief Remove completed transactions older than configured retention horizon. */
void DistributedTransactionCoordinator::cleanupOldTransactions() {
    std::lock_guard<std::timed_mutex> lock(mutex_);
    
    // Remove completed transactions older than 1 hour
    auto cutoff = truetime_->now().earliest - std::chrono::hours(1);
    
    auto it = transactions_.begin();
    while (it != transactions_.end()) {
        auto& txn = it->second;
        if ((txn.state == TransactionState::COMMITTED || 
             txn.state == TransactionState::ABORTED) &&
            txn.start_time < cutoff) {
            it = transactions_.erase(it);
        } else {
            ++it;
        }
    }
}

/** @brief Compute capped exponential backoff delay for retry attempt. */
uint64_t DistributedTransactionCoordinator::calculateBackoffDelay(uint32_t retry_count) const {
    // Exponential backoff: base_ms * 2^retry_count, capped at max_backoff_ms
    uint64_t delay = config_.retry_backoff_base_ms * (1ULL << retry_count);
    return std::min(delay, config_.max_backoff_ms);
}

/** @brief Retry commit phase with exponential backoff until success or cap. */
bool DistributedTransactionCoordinator::retryCommitPhase(DistributedTransaction& txn) {
    // First attempt
    bool committed = commitPhase(txn);
    
    if (committed) {
        return true;
    }
    
    // Retry with exponential backoff
    txn.commit_retry_count = 1;
    while (txn.commit_retry_count <= config_.max_commit_retries) {
        uint64_t backoff_ms = calculateBackoffDelay(txn.commit_retry_count - 1);
        
        THEMIS_WARN("Transaction {} commit failed, retrying in {}ms (attempt {}/{})",
                   txn.transaction_id, backoff_ms, txn.commit_retry_count, 
                   config_.max_commit_retries);
        
        // Wait before retry
        std::this_thread::sleep_for(std::chrono::milliseconds(backoff_ms));
        
        // Retry commit phase
        committed = commitPhase(txn);
        
        if (committed) {
            THEMIS_INFO("Transaction {} committed successfully after {} retries",
                       txn.transaction_id, txn.commit_retry_count);
            return true;
        }
        
        txn.commit_retry_count++;
    }
    
    // All retries exhausted
    THEMIS_ERROR("Transaction {} commit failed after {} retries",
                txn.transaction_id, txn.commit_retry_count);
    return false;
}

/** @brief Persist terminal transaction state for crash recovery and audit. */
void DistributedTransactionCoordinator::logTransactionForRecovery(
    const DistributedTransaction& txn
) {
    if (!wal_manager_) {
        return; // WAL not enabled
    }
    
    // Create recovery log entry
    nlohmann::json recovery_data = {
        {"transaction_id", txn.transaction_id},
        {"state", static_cast<int>(txn.state)},
        {"commit_time", txn.commit_time.count()},
        {"start_time", txn.start_time.count()},
        {"participants", nlohmann::json::array()}
    };
    
    for (const auto& participant : txn.participants) {
        recovery_data["participants"].push_back({
            {"shard_id", participant.shard_id},
            {"endpoint", participant.endpoint},
            {"prepared", participant.prepared},
            {"committed", participant.committed}
        });
    }
    
    // Write to WAL for durability
    try {
        WALEntry entry;
        entry.type = WALEntryType::COMMIT_TX;
        entry.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
        entry.transaction_id = txn.transaction_id;
        entry.data = recovery_data;
        
        LSN lsn = wal_manager_->append(entry);
        wal_manager_->flush(); // Ensure durability
        
        THEMIS_INFO("Transaction {} logged for recovery at LSN {}", 
                   txn.transaction_id, lsn.toString());
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Failed to log transaction {} to WAL: {}", 
                    txn.transaction_id, e.what());
    }
}

/** @brief Persist PREPARED marker to recover in-doubt transactions after crash. */
void DistributedTransactionCoordinator::logPreparedStateForRecovery(
    const DistributedTransaction& txn
) {
    if (!wal_manager_) {
        return; // WAL not enabled
    }
    
    // Log PREPARED state with minimal metadata for audit trail
    // Operations are not included as they're already at participants
    nlohmann::json prepared_data = {
        {"transaction_id", txn.transaction_id},
        {"state", static_cast<int>(TransactionState::PREPARED)},
        {"start_time", txn.start_time.count()},
        {"participants", nlohmann::json::array()}
    };
    
    for (const auto& participant : txn.participants) {
        prepared_data["participants"].push_back({
            {"shard_id", participant.shard_id},
            {"endpoint", participant.endpoint},
            {"prepared", participant.prepared}
        });
    }
    
    // Write PREPARED state to WAL for in-doubt recovery
    try {
        WALEntry entry;
        entry.type = WALEntryType::PREPARE_TX;
        entry.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
        entry.transaction_id = txn.transaction_id;
        entry.data = prepared_data;
        
        LSN lsn = wal_manager_->append(entry);
        wal_manager_->flush();
        
        THEMIS_DEBUG("Transaction {} PREPARED state logged at LSN {}", 
                    txn.transaction_id, lsn.toString());
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Failed to log PREPARED state for transaction {} to WAL: {}", 
                    txn.transaction_id, e.what());
    }
}

/** @brief Recover in-doubt transactions by replaying WAL entries. */
void DistributedTransactionCoordinator::recoverTransactions() {
    if (!wal_manager_) {
        return; // WAL not enabled
    }
    
    THEMIS_INFO("Starting transaction recovery from WAL");
    
    try {
        // Read all entries from WAL
        LSN oldest_lsn = wal_manager_->getOldestLSN();
        LSN current_lsn = wal_manager_->getCurrentLSN();
        
        if (oldest_lsn > current_lsn) {
            THEMIS_INFO("No transactions to recover");
            return;
        }
        
        std::vector<WALEntry> entries = wal_manager_->readRange(oldest_lsn, current_lsn);
        
        THEMIS_INFO("Found {} WAL entries to process", entries.size());
        
        // Two passes: first build the committed set, then find in-doubt transactions
        std::set<std::string> committed_ids;
        std::set<std::string> aborted_ids;
        std::map<std::string, nlohmann::json> prepared_entries; // txn_id -> prepared data
        
        int recovered_count = 0;
        for (const auto& entry : entries) {
            try {
                if (entry.type == WALEntryType::COMMIT_TX) {
                    std::string txn_id = entry.data["transaction_id"];
                    committed_ids.insert(txn_id);
                } else if (entry.type == WALEntryType::ABORT_TX) {
                    std::string txn_id = entry.data["transaction_id"];
                    aborted_ids.insert(txn_id);
                } else if (entry.type == WALEntryType::PREPARE_TX) {
                    std::string txn_id = entry.data["transaction_id"];
                    prepared_entries[txn_id] = entry.data;
                }
            } catch (const std::exception& e) {
                THEMIS_ERROR("Failed to parse WAL entry: {}", e.what());
            }
        }
        
        // Recover in-doubt transactions: prepared but no commit/abort decision logged
        for (const auto& [txn_id, prepared_data] : prepared_entries) {
            if (committed_ids.count(txn_id)) {
                THEMIS_INFO("Recovered committed transaction: {}", txn_id);
                recovered_count++;
            } else if (aborted_ids.count(txn_id)) {
                THEMIS_INFO("In-doubt transaction {} was aborted before crash", txn_id);
            } else {
                // In-doubt transaction: prepared but no decision recorded
                // Safe default is to abort (participants will clean up on timeout)
                THEMIS_WARN("In-doubt transaction {} found (prepared, no commit/abort) - aborting for safety", txn_id);
                
                DistributedTransaction recovery_txn;
                recovery_txn.transaction_id = txn_id;
                recovery_txn.state = TransactionState::ABORTING;
                
                if (prepared_data.contains("participants")) {
                    for (const auto& p : prepared_data["participants"]) {
                        TransactionParticipant participant;
                        participant.shard_id = p.value("shard_id", "");
                        participant.endpoint = p.value("endpoint", "");
                        recovery_txn.participants.push_back(participant);
                    }
                }
                
                for (auto& participant : recovery_txn.participants) {
                    if (!sendAbort(participant, txn_id)) {
                        THEMIS_ERROR("Recovery: failed to send ABORT to shard {} for in-doubt txn {}",
                                    participant.shard_id, txn_id);
                    }
                }
                
                // Log abort decision
                WALEntry abort_entry;
                abort_entry.type = WALEntryType::ABORT_TX;
                abort_entry.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()
                ).count();
                abort_entry.transaction_id = txn_id;
                abort_entry.data = {
                    {"transaction_id", txn_id},
                    {"state", static_cast<int>(TransactionState::ABORTED)},
                    {"reason", "in-doubt recovery on coordinator restart"}
                };
                
                try {
                    wal_manager_->append(abort_entry);
                    wal_manager_->flush();
                } catch (const std::exception& e) {
                    THEMIS_ERROR("Failed to log abort for in-doubt transaction {}: {}", txn_id, e.what());
                }
                
                recovered_count++;
            }
        }
        
        THEMIS_INFO("Transaction recovery complete - processed {} transactions", 
                   recovered_count);
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Transaction recovery failed: {}", e.what());
    }
}

// ---------------------------------------------------------------------------
// Percolator-style commit path (used for SNAPSHOT_ISOLATION transactions)
// ---------------------------------------------------------------------------
/** @brief Execute Percolator-style commit path for snapshot-isolated transaction.
 *
 * The protocol performs cross-shard write-write conflict detection via a full
 * prepare phase before assigning a commit timestamp.  Skipping the prepare
 * phase would allow write-skew anomalies: two concurrent transactions can each
 * read a consistent snapshot, write to disjoint shards, and both commit without
 * detecting the mutual conflict.  Running the prepare vote here ensures every
 * participant performs its local conflict check before the coordinator proceeds
 * to the commit timestamp / commit-wait / send-COMMIT sequence.
 *
 * Protocol (with conflict detection):
 *   0. PREPARE phase — all participants vote COMMIT or ABORT.
 *      If any vote ABORT, send ABORT to all prepared participants and return false.
 *   1. Assign commit timestamp from TrueTime::now_with_uncertainty().latest
 *   2. Commit-wait: spin until TT.now().earliest > commit_ts
 *   3. Send COMMIT to all participants with the agreed timestamp
 */
bool DistributedTransactionCoordinator::percolatorCommit(DistributedTransaction& txn) {
    // Step 0: Cross-shard write-write conflict detection via the prepare phase.
    // Every participant must vote before the coordinator assigns a commit
    // timestamp.  This prevents write-skew anomalies where two overlapping
    // snapshot-isolation transactions both observe a consistent state and both
    // commit conflicting writes to different shards.
    txn.state = TransactionState::PREPARING;
    if (!preparePhase(txn)) {
        THEMIS_WARN("Percolator conflict detected for txn {} — aborting: {}",
                    txn.transaction_id, txn.error_detail);
        // Send ABORT to every participant that already voted COMMIT so they
        // can release any locks or tentative writes they acquired.
        for (auto& participant : txn.participants) {
            if (participant.prepared) {
                sendAbort(participant, txn.transaction_id);
            }
        }
        return false;
    }

    // Step 1: Derive commit timestamp.
    // Use the *latest* bound so every concurrent snapshot read that started
    // before this commit is guaranteed to see either the old or new version.
    const auto tt_interval = truetime_->now_with_uncertainty();
    txn.commit_time         = tt_interval.latest;

    THEMIS_DEBUG("Percolator txn {} commit_ts={} (TrueTime [earliest={}, latest={}])",
                 txn.transaction_id,
                 txn.commit_time.count(),
                 tt_interval.earliest.count(),
                 tt_interval.latest.count());

    // Step 2: Commit-wait.
    // commit_time was drawn from now_with_uncertainty().latest which already
    // incorporates the uncertainty bound; waiting until commit_time is
    // sufficient to guarantee TT.now().earliest > commit_time.
    truetime_->waitUntil(txn.commit_time);

    // Step 3: Send COMMIT to all participants.
    txn.state = TransactionState::COMMITTING;

    std::vector<std::thread> threads;
    std::atomic<bool> all_committed{true};
    std::mutex error_mutex;
    std::vector<std::string> error_details;

    for (auto& participant : txn.participants) {
        // Capture a stable pointer rather than the loop variable reference,
        // which would be reused across iterations and cause data races.
        TransactionParticipant* p_ptr = &participant;
        threads.emplace_back([this, p_ptr, &txn, &all_committed,
                              &error_mutex, &error_details]() {
            if (!sendCommit(*p_ptr, txn.transaction_id, txn.commit_time)) {
                all_committed.store(false, std::memory_order_relaxed);
                std::lock_guard<std::mutex> lk(error_mutex);
                error_details.push_back("Shard " + p_ptr->shard_id +
                                        " failed Percolator commit: " +
                                        p_ptr->error_msg);
            }
        });
    }

    for (auto& t : threads) {
        themis::utils::joinThreadWithin(t);
    }

    if (!all_committed.load()) {
        std::lock_guard<std::mutex> lk(error_mutex);
        txn.error_detail = "Percolator commit failures: ";
        for (const auto& e : error_details) {
            txn.error_detail += e + "; ";
        }
        THEMIS_ERROR("Percolator commit failed for txn {}: {}",
                     txn.transaction_id, txn.error_detail);
    }

    return all_committed.load();
}

} // namespace themis::sharding

