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
#include "sharding/wal_logging_helper.h"
#include "sharding/shard_rpc_client.h"
#include "sharding/metrics_registry.h"
#include "sharding/prometheus_metrics.h"
#include "transaction/two_phase_commit_wal_recovery.h"
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

static inline bool isUsableShardEndpoint(const std::string& endpoint) {
    return !endpoint.empty() && endpoint.rfind("shard://", 0) != 0;
}

/** @brief Return stable WAL helper component ID for distributed coordinator logs. */
static inline std::string_view recoveryWalComponentId(
    const DistributedTransactionCoordinator::Config& cfg
) {
    return cfg.coordinator_id.empty() ? std::string_view{"default"} : std::string_view{cfg.coordinator_id};
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
        wal_config.wal_directory = config_.wal_directory;
        wal_config.segment_size = 16 * 1024 * 1024;  // 16 MB
        wal_config.sync_on_write = true;             // Durability
        
        wal_manager_ = std::make_unique<WALManager>(wal_config);
        
        // Recover any in-doubt transactions from WAL
        [[maybe_unused]] const auto recovered_count = recoverInDoubtTransactions();
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
        // W2-S06: Consensus validation — validate shard_id before adding as participant
        if (shard_id.empty()) {
            THEMIS_ERROR("begin: empty shard_id in participant list, rejecting transaction");
            return "";
        }
        
        TransactionParticipant participant;
        participant.shard_id = shard_id;

        auto it = shard_endpoint_map_.find(shard_id);
        if (it != shard_endpoint_map_.end() && isUsableShardEndpoint(it->second)) {
            participant.endpoint = it->second;
        } else {
            participant.endpoint.clear();
            THEMIS_WARN("beginTransaction: shard {} has no registered real endpoint; commit path will fail closed until setShardEndpointMap() is provided",
                        shard_id);
        }

        participant.prepared  = false;
        participant.committed = false;
        txn.participants.push_back(participant);
    }
    
    transactions_[txn_id] = std::move(txn);
    total_transactions_.fetch_add(1, std::memory_order_relaxed);

    if (config_.enable_recovery_log) {
        logBeginStateForRecovery(transactions_.at(txn_id));
    }
    
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
    
    // W2-S06: Consensus validation — ensure shard_id is valid before adding operation
    if (shard_id.empty()) {
        THEMIS_ERROR("addOperation: shard_id is empty for txn {}, rejecting operation", txn_id);
        return false;
    }
    
    // W2-S06: Operation validation — ensure operation is not malformed
    if (!operation.is_object() && !operation.is_array()) {
        THEMIS_ERROR("addOperation: operation must be object or array for txn {}, rejecting", txn_id);
        return false;
    }
    
    // Add operation to transaction
    // CONSENSUS-AWARE: Log operation addition for durability
    if (!txn.operations.contains(shard_id)) {
        txn.operations[shard_id] = nlohmann::json::array();
    }
    txn.operations[shard_id].push_back(operation);
    
    // Log to recovery WAL if enabled (ensures durability before returning)
    if (config_.enable_recovery_log) {
        try {
            logOperationAddedForRecovery(txn_id, shard_id, operation);
        } catch (const std::exception& e) {
            THEMIS_WARN("Failed to log operation to WAL: {}, proceeding with in-memory state", e.what());
            // Fall-through: in-memory state is already updated, accept reduced durability guarantee
        }
    }
    
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
        
        bool committed = false;
        try {
            committed = percolatorCommit(txn);
        } catch (const std::exception& e) {
            THEMIS_ERROR("Exception during percolatorCommit for txn {}: {}", txn_id, e.what());
            // Try to re-acquire lock to update transaction state
            if (!lock.try_lock_for(std::chrono::milliseconds(config_.rpc_timeout_ms))) {
                spdlog::error("Lock acquisition timeout after percolatorCommit exception for txn {}", txn_id);
                return false;
            }
            txn.state = TransactionState::ABORTED;
            aborted_transactions_.fetch_add(1, std::memory_order_relaxed);
            return false;
        }
        
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
    bool prepared = false;
    try {
        prepared = preparePhase(txn);
    } catch (const std::exception& e) {
        THEMIS_ERROR("Exception during preparePhase for txn {}: {}", txn_id, e.what());
        if (!lock.try_lock_for(std::chrono::milliseconds(config_.rpc_timeout_ms))) {
            spdlog::error("Lock acquisition timeout after preparePhase exception for txn {}", txn_id);
            return false;
        }
        txn.state = TransactionState::ABORTED;
        aborted_transactions_.fetch_add(1, std::memory_order_relaxed);
        return false;
    }
    
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
        if (config_.enable_recovery_log) {
            (void)logDecisionStateForRecovery(txn, false, "decision", "prepare_phase_failed");
        }
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
        if (config_.enable_recovery_log) {
            logTransactionForRecovery(txn);
        }

        if (auto m = ShardingMetricsRegistry::instance().getMetrics()) {
            m->record2PCAbort(coordinator_id, "prepare_phase_failed");
            m->record2PCTransaction(coordinator_id, false);
        }
        return false;
    }
    
    txn.state = TransactionState::PREPARED;
    
    // Log PREPARED state for recovery (in case coordinator crashes before commit)
    if (config_.enable_recovery_log && !logPreparedStateForRecovery(txn)) {
        txn.state = TransactionState::ABORTING;
        txn.error_detail = "Failed to durably log PREPARED state before COMMIT decision";
        lock.unlock();
        for (auto& participant : txn.participants) {
            sendAbort(participant, txn_id);
        }
        if (!lock.try_lock_for(std::chrono::milliseconds(config_.rpc_timeout_ms))) {
            spdlog::error("Lock acquisition timeout after PREPARED WAL failure for txn {}", txn_id);
            return false;
        }
        txn.state = TransactionState::ABORTED;
        if (config_.enable_recovery_log) {
            (void)logDecisionStateForRecovery(txn, false, "decision", "prepared_wal_logging_failed");
            logTransactionForRecovery(txn);
        }
        aborted_transactions_.fetch_add(1, std::memory_order_relaxed);
        if (auto m = ShardingMetricsRegistry::instance().getMetrics()) {
            m->record2PCAbort(coordinator_id, "prepared_wal_logging_failed");
            m->record2PCTransaction(coordinator_id, false);
        }
        return false;
    }
    
    // Assign commit timestamp using TrueTime
    // Use the latest time to ensure all reads see this transaction
    txn.commit_time = truetime_->now().latest;
    
    // Wait until commit timestamp is definitely in the past
    // This is the key TrueTime operation for external consistency
    lock.unlock();
    
    try {
        truetime_->waitUntil(txn.commit_time);
    } catch (const std::exception& e) {
        THEMIS_ERROR("Exception during TrueTime wait for txn {}: {}", txn_id, e.what());
        if (!lock.try_lock_for(std::chrono::milliseconds(config_.rpc_timeout_ms))) {
            spdlog::error("Lock acquisition timeout after TrueTime wait exception for txn {}", txn_id);
            return false;
        }
        txn.state = TransactionState::ABORTED;
        aborted_transactions_.fetch_add(1, std::memory_order_relaxed);
        return false;
    }
    
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
    if (wal_manager_ && config_.enable_recovery_log &&
        !logDecisionStateForRecovery(txn, true, "decision")) {
            THEMIS_ERROR("DTM: Failed to flush COMMIT WAL entry for txn '{}'. "
                         "Aborting to prevent unsafe state.", txn.transaction_id);
            if (!lock.try_lock_for(std::chrono::milliseconds(config_.rpc_timeout_ms))) {
                spdlog::error("Lock acquisition timeout in WAL flush error handler for txn {}",
                              txn_id);
                return false;
            }
            txn.state = TransactionState::ABORTING;
            txn.error_detail = "WAL flush failed before Phase 2 COMMIT";
            if (config_.enable_recovery_log) {
                (void)logDecisionStateForRecovery(
                    txn, false, "decision", "commit_decision_flush_failed");
            }
            lock.unlock();
            for (auto& participant : txn.participants) {
                sendAbort(participant, txn_id);
            }
            if (!lock.try_lock_for(std::chrono::milliseconds(config_.rpc_timeout_ms))) {
                spdlog::error("Lock acquisition timeout completing abort after WAL flush failure "
                              "for txn {}", txn_id);
                return false;
            }
            txn.state = TransactionState::ABORTED;
            if (config_.enable_recovery_log) {
                logTransactionForRecovery(txn);
            }
            aborted_transactions_.fetch_add(1, std::memory_order_relaxed);
            return false;
    }

    auto commit_start = std::chrono::steady_clock::now();
    bool committed = false;
    try {
        committed = retryCommitPhase(txn);
    } catch (const std::exception& e) {
        THEMIS_ERROR("Exception during retryCommitPhase for txn {}: {}", txn_id, e.what());
        if (!lock.try_lock_for(std::chrono::milliseconds(config_.rpc_timeout_ms))) {
            spdlog::error("Lock acquisition timeout after retryCommitPhase exception for txn {}", txn_id);
            return false;
        }
        txn.state = TransactionState::ABORTED;
        aborted_transactions_.fetch_add(1, std::memory_order_relaxed);
        return false;
    }
    
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
        txn.state = TransactionState::COMMITTING;
        txn.error_detail =
            "Commit phase incomplete after retries; durable COMMIT decision will be retried by recovery";
        
        THEMIS_ERROR("Transaction {} commit failed after {} retries", 
                    txn_id, txn.commit_retry_count);
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

    if (config_.enable_recovery_log) {
        (void)logDecisionStateForRecovery(txn, false, "decision", "explicit_abort");
    }
    
    // Send abort to all participants
    for (auto& participant : txn.participants) {
        sendAbort(participant, txn_id);
    }
    
    txn.state = TransactionState::ABORTED;
    aborted_transactions_.fetch_add(1, std::memory_order_relaxed);
    if (config_.enable_recovery_log) {
        logTransactionForRecovery(txn);
    }
    
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
            const auto endpoint_it = shard_endpoint_map_.find(shard_id);
            if (endpoint_it == shard_endpoint_map_.end() || !isUsableShardEndpoint(endpoint_it->second)) {
                THEMIS_ERROR("Snapshot read rejected for shard {}: no registered real endpoint", shard_id);
                results[shard_id] = {
                    {"status", "error"},
                    {"error", "missing_registered_endpoint"}
                };
                continue;
            }
            ShardRPCClient::Config rpc_config;
            rpc_config.endpoint = endpoint_it->second;
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

size_t DistributedTransactionCoordinator::recoverInDoubtTransactions() {
    return recoverTransactions();
}

std::string DistributedTransactionCoordinator::recoveryCoordinatorName() const {
    return "DistributedTransactionCoordinator";
}

std::string DistributedTransactionCoordinator::recoveryBackendName() const {
    return (config_.enable_recovery_log && wal_manager_) ? "WAL" : "disabled";
}

std::vector<themis::transaction::RecoverableTwoPhaseTransaction>
DistributedTransactionCoordinator::getRecoverableTransactions() const {
    std::vector<themis::transaction::RecoverableTwoPhaseTransaction> recoverable;

    std::lock_guard<std::timed_mutex> lock(mutex_);
    recoverable.reserve(transactions_.size());
    for (const auto& [txn_id, txn] : transactions_) {
        if (txn.state == TransactionState::COMMITTED ||
            txn.state == TransactionState::ABORTED) {
            continue;
        }

        themis::transaction::RecoverableTwoPhaseTransaction info;
        info.transaction_id = txn_id;
        switch (txn.state) {
            case TransactionState::ACTIVE:
                info.state = themis::transaction::RecoverableTwoPhaseState::ACTIVE;
                break;
            case TransactionState::PREPARING:
                info.state = themis::transaction::RecoverableTwoPhaseState::PREPARING;
                break;
            case TransactionState::PREPARED:
                info.state = themis::transaction::RecoverableTwoPhaseState::PREPARED;
                break;
            case TransactionState::COMMITTING:
                info.state = themis::transaction::RecoverableTwoPhaseState::COMMITTING;
                info.decision_recorded = true;
                info.decision_commit = true;
                break;
            case TransactionState::ABORTING:
                info.state = themis::transaction::RecoverableTwoPhaseState::ABORTING;
                info.decision_recorded = true;
                info.decision_commit = false;
                break;
            case TransactionState::COMMITTED:
            [[fallthrough]];\n            case TransactionState::ABORTED:
                info.state = themis::transaction::RecoverableTwoPhaseState::COMPLETED;
                break;
        }
        recoverable.push_back(std::move(info));
    }

    return recoverable;
}

/** @brief Register/replace shard ID to endpoint mapping used for RPC routing. */
void DistributedTransactionCoordinator::setShardEndpointMap(
    std::unordered_map<std::string, std::string> map)
{
    std::lock_guard<std::timed_mutex> lock(mutex_);
    for (auto it = map.begin(); it != map.end();) {
        if (it->first.empty() || !isUsableShardEndpoint(it->second)) {
            THEMIS_WARN("Ignoring invalid shard endpoint registration: shard_id='{}' endpoint='{}'",
                        it->first, it->second);
            it = map.erase(it);
        } else {
            ++it;
        }
    }
    shard_endpoint_map_ = std::move(map);
}

/** @brief Execute prepare phase in parallel across all participants. */
bool DistributedTransactionCoordinator::preparePhase(DistributedTransaction& txn) {
    // Send prepare to all participants in parallel with an explicit timeout so
    // that a slow or stuck shard cannot block the coordinator indefinitely.
    //
    // EXCEPTION SAFETY: This method guarantees strong exception safety by
    // using RAII to ensure all futures are properly waited on, even if an
    // exception is thrown during collection or processing of results.
    
    std::vector<std::future<void>> futures;
    futures.reserve(txn.participants.size());
    
    std::atomic<bool> all_prepared{true};
    std::mutex error_mutex;
    std::vector<std::string> error_details;

    // Phase 1: Launch all prepare tasks in parallel
    try {
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
    } catch (const std::exception& e) {
        THEMIS_ERROR("Exception launching prepare phase: {}", e.what());
        // All futures already destructed; futures that were launched will complete
        // and drop their results automatically
        return false;
    }

    // Phase 2: Wait for all tasks to complete with timeout
    const auto deadline = std::chrono::milliseconds(config_.prepare_timeout_ms);
    try {
        for (auto& f : futures) {
            if (f.wait_for(deadline) == std::future_status::timeout) {
                all_prepared.store(false, std::memory_order_relaxed);
                std::lock_guard<std::mutex> lock(error_mutex);
                error_details.push_back("prepare timed out after " +
                                        std::to_string(config_.prepare_timeout_ms) + "ms");
            }
        }
    } catch (const std::exception& e) {
        THEMIS_ERROR("Exception waiting for prepare futures: {}", e.what());
        return false;
    }

    // Phase 3: Collect error details
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
    //
    // EXCEPTION SAFETY: This method guarantees strong exception safety by
    // using RAII to ensure all futures are properly waited on, even if an
    // exception is thrown during collection or processing of results.
    
    std::vector<std::future<void>> futures;
    futures.reserve(txn.participants.size());
    
    std::atomic<bool> all_committed{true};
    std::mutex error_mutex;
    std::vector<std::string> error_details;

    // Phase 1: Launch all commit tasks in parallel
    try {
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
    } catch (const std::exception& e) {
        THEMIS_ERROR("Exception launching commit phase: {}", e.what());
        // All futures already destructed; futures that were launched will complete
        // and drop their results automatically
        return false;
    }

    // Phase 2: Wait for all tasks to complete with timeout
    const auto deadline = std::chrono::milliseconds(config_.commit_timeout_ms);
    try {
        for (auto& f : futures) {
            if (f.wait_for(deadline) == std::future_status::timeout) {
                all_committed.store(false, std::memory_order_relaxed);
                std::lock_guard<std::mutex> lock(error_mutex);
                error_details.push_back("commit timed out after " +
                                        std::to_string(config_.commit_timeout_ms) + "ms");
            }
        }
    } catch (const std::exception& e) {
        THEMIS_ERROR("Exception waiting for commit futures: {}", e.what());
        return false;
    }

    // Phase 3: Collect error details
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
    if (!isUsableShardEndpoint(participant.endpoint)) {
        THEMIS_ERROR("PREPARE rejected for shard {}: no registered real endpoint", participant.shard_id);
        participant.prepared = false;
        return false;
    }
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
    if (!isUsableShardEndpoint(participant.endpoint)) {
        THEMIS_ERROR("COMMIT rejected for shard {}: no registered real endpoint", participant.shard_id);
        participant.committed = false;
        return false;
    }
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
    if (!isUsableShardEndpoint(participant.endpoint)) {
        THEMIS_ERROR("ABORT rejected for shard {}: no registered real endpoint", participant.shard_id);
        participant.prepared = false;
        participant.committed = false;
        return false;
    }
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
void DistributedTransactionCoordinator::logBeginStateForRecovery(
    const DistributedTransaction& txn
) {
    if (!wal_manager_) {
        return;
    }

    nlohmann::json begin_data = {
        {"transaction_id", txn.transaction_id},
        {"phase", "begin"},
        {"start_time", txn.start_time.count()},
        {"participants", nlohmann::json::array()}
    };

    for (const auto& participant : txn.participants) {
        begin_data["participants"].push_back({
            {"shard_id", participant.shard_id},
            {"endpoint", participant.endpoint}
        });
    }

    WALLoggingHelper::appendEntry(
        wal_manager_.get(),
        WALEntryType::BEGIN_TX,
        txn.transaction_id,
        begin_data,
        /*sync=*/true,
        "distributed-coordinator-recovery",
        recoveryWalComponentId(config_)
    );
}

void DistributedTransactionCoordinator::logOperationAddedForRecovery(
    const std::string& txn_id,
    const std::string& shard_id,
    const nlohmann::json& operation
) {
    if (!wal_manager_) {
        return;
    }

    const nlohmann::json operation_data = {
        {"transaction_id", txn_id},
        {"phase", "operation_append"},
        {"shard_id", shard_id},
        {"operation", operation}
    };

    (void)WALLoggingHelper::appendEntryWithResult(
        wal_manager_.get(),
        WALEntryType::UPDATE,
        txn_id,
        operation_data,
        /*sync=*/true,
        "distributed-coordinator-recovery",
        recoveryWalComponentId(config_)
    );
}

bool DistributedTransactionCoordinator::logDecisionStateForRecovery(
    const DistributedTransaction& txn,
    bool commit,
    std::string_view phase,
    std::string_view reason
) {
    if (!wal_manager_) {
        return false;
    }

    nlohmann::json recovery_data = {
        {"transaction_id", txn.transaction_id},
        {"phase", std::string(phase)},
        {"decision", commit ? "commit" : "abort"},
        {"commit_timestamp_ns", txn.commit_time.count()},
        {"participants", nlohmann::json::array()}
    };

    if (!reason.empty()) {
        recovery_data["reason"] = std::string(reason);
    }

    for (const auto& participant : txn.participants) {
        recovery_data["participants"].push_back({
            {"shard_id", participant.shard_id},
            {"endpoint", participant.endpoint},
            {"prepared", participant.prepared},
            {"committed", participant.committed}
        });
    }

    return WALLoggingHelper::appendEntryWithResult(
        wal_manager_.get(),
        commit ? WALEntryType::COMMIT_TX : WALEntryType::ABORT_TX,
        txn.transaction_id,
        recovery_data,
        /*sync=*/true,
        "distributed-coordinator-recovery",
        recoveryWalComponentId(config_)
    ).has_value();
}

void DistributedTransactionCoordinator::logTransactionForRecovery(
    const DistributedTransaction& txn
) {
    if (!wal_manager_) {
        return; // WAL not enabled
    }
    
    // Create recovery log entry
    nlohmann::json recovery_data = {
        {"transaction_id", txn.transaction_id},
        {"phase", "complete"},
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
    
    const auto lsn = WALLoggingHelper::appendEntryWithResult(
        wal_manager_.get(),
        txn.state == TransactionState::ABORTED
            ? WALEntryType::ABORT_TX
            : WALEntryType::COMMIT_TX,
        txn.transaction_id,
        recovery_data,
        /*sync=*/true,
        "distributed-coordinator-recovery",
        recoveryWalComponentId(config_)
    );
    if (lsn.has_value()) {
        THEMIS_INFO("Transaction {} logged for recovery at LSN {}",
                    txn.transaction_id, lsn->toString());
    }
}

/** @brief Persist PREPARED marker to recover in-doubt transactions after crash. */
bool DistributedTransactionCoordinator::logPreparedStateForRecovery(
    const DistributedTransaction& txn
) {
    if (!wal_manager_) {
        return false; // WAL not enabled
    }
    
    // Log PREPARED state with minimal metadata for audit trail
    // Operations are not included as they're already at participants
    nlohmann::json prepared_data = {
        {"transaction_id", txn.transaction_id},
        {"phase", "prepared"},
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
    
    const auto lsn = WALLoggingHelper::appendEntryWithResult(
        wal_manager_.get(),
        WALEntryType::PREPARE_TX,
        txn.transaction_id,
        prepared_data,
        /*sync=*/true,
        "distributed-coordinator-recovery",
        recoveryWalComponentId(config_)
    );
    if (!lsn.has_value()) {
        return false;
    }

    THEMIS_DEBUG("Transaction {} PREPARED state logged at LSN {}",
                 txn.transaction_id, lsn->toString());
    return true;
}

/** @brief Recover in-doubt transactions by replaying WAL entries. */
size_t DistributedTransactionCoordinator::recoverTransactions() {
    if (!wal_manager_) {
        return 0; // WAL not enabled
    }
    
    THEMIS_INFO("Starting transaction recovery from WAL");
    
    try {
        // Read all entries from WAL
        LSN oldest_lsn = wal_manager_->getOldestLSN();
        LSN current_lsn = wal_manager_->getCurrentLSN();
        
        if (oldest_lsn > current_lsn) {
            THEMIS_INFO("No transactions to recover");
            return 0;
        }
        
        std::vector<WALEntry> entries = wal_manager_->readRange(oldest_lsn, current_lsn);
        
        THEMIS_INFO("Found {} WAL entries to process", entries.size());

        const auto recovered =
            themis::transaction::TwoPhaseCommitWALRecovery::reconstruct(entries);

        size_t recovered_count = 0;
        for (const auto& [txn_id, replay_txn] : recovered) {
            DistributedTransaction recovery_txn;
            recovery_txn.transaction_id = txn_id;
            // commit_timestamp_ns is optional
            if (replay_txn.commit_timestamp_ns.has_value()) {
                recovery_txn.commit_time = std::chrono::nanoseconds(replay_txn.commit_timestamp_ns.value());
            }

            // Recovered participants are simple identifiers (node/shard ids)
            for (const auto& pid : replay_txn.participants) {
                // W2-S06: Consensus validation — validate participant shard ID
                if (pid.empty()) {
                    THEMIS_WARN("recoverInDoubtTransactions: empty participant ID in recovery txn {}, skipping", txn_id);
                    continue;
                }
                
                TransactionParticipant participant;
                participant.shard_id = pid;
                auto endpoint_it = shard_endpoint_map_.find(pid);
                if (endpoint_it == shard_endpoint_map_.end() || !isUsableShardEndpoint(endpoint_it->second)) {
                    THEMIS_WARN("recoverInDoubtTransactions: shard {} in txn {} has no registered real endpoint, skipping participant",
                                pid, txn_id);
                    continue;
                }
                participant.endpoint = endpoint_it->second;
                // Durable WAL does not include per-participant prepared/committed flags
                participant.prepared = false;
                participant.committed = false;
                recovery_txn.participants.push_back(std::move(participant));
            }

            if (replay_txn.completed) {
                // Completed transactions: prefer recorded decision if present
                if (replay_txn.has_decision) {
                    recovery_txn.state = replay_txn.decision_commit ? TransactionState::COMMITTED : TransactionState::ABORTED;
                } else {
                    recovery_txn.state = TransactionState::ABORTED;
                }
                std::lock_guard<std::timed_mutex> lock(mutex_);
                transactions_[txn_id] = std::move(recovery_txn);
                continue;
            }

            bool success = true;
            if (replay_txn.has_decision && replay_txn.decision_commit) {
                recovery_txn.state = TransactionState::COMMITTING;
                THEMIS_WARN("Recovery: re-driving COMMIT for in-doubt txn {}", txn_id);
                success = commitPhase(recovery_txn);
                if (success) {
                    recovery_txn.state = TransactionState::COMMITTED;
                } else {
                    recovery_txn.error_detail = "Recovery COMMIT replay failed; transaction remains in-doubt";
                }
            } else {
                recovery_txn.state = TransactionState::ABORTING;
                if (!replay_txn.has_decision) {
                    THEMIS_WARN("Recovery: txn {} has no durable final decision; aborting conservatively", txn_id);
                } else {
                    THEMIS_WARN("Recovery: re-driving ABORT for in-doubt txn {}", txn_id);
                }

                for (auto& participant : recovery_txn.participants) {
                    if (!sendAbort(participant, txn_id)) {
                        success = false;
                        THEMIS_ERROR("Recovery: failed to send ABORT to shard {} for in-doubt txn {}",
                                    participant.shard_id, txn_id);
                    }
                }
                if (success) {
                    recovery_txn.state = TransactionState::ABORTED;
                } else {
                    recovery_txn.error_detail =
                        "Recovery ABORT replay failed; transaction remains in-doubt";
                }
            }

            {
                std::lock_guard<std::timed_mutex> lock(mutex_);
                transactions_[txn_id] = recovery_txn;
            }

            if (!success) {
                continue;
            }

            logTransactionForRecovery(recovery_txn);
            ++recovered_count;
        }
        
        THEMIS_INFO("Transaction recovery complete - processed {} transactions", 
                   recovered_count);
        return recovered_count;
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Transaction recovery failed: {}", e.what());
    }

    return 0;
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
        const bool joined = themis::utils::joinThreadWithin(t);
        if (!joined) {
            all_committed.store(false, std::memory_order_relaxed);
            std::lock_guard<std::mutex> lk(error_mutex);
            error_details.push_back("commit worker thread join timed out");
        }
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
