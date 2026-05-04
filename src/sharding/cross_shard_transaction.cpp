/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            cross_shard_transaction.cpp                        ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:50:53                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟡 RELEASE-CANDIDATE                            ║
    • Quality Score:   73.0/100                                       ║
    • Total Lines:     2731                                           ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • e963d4e9ba  2026-04-14  fix(concurrency): eliminate deadlocks, blocking I/O under... ║
    • 71d99c4f28  2026-04-14  fix(concurrency): eliminate deadlocks, blocking I/O under... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ⚠️  Needs Work                                              ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "sharding/cross_shard_transaction.h"
#include "sharding/shard_rpc_client.h"
#include "sharding/truetime.h"
#include "sharding/transaction_wal.h"
#include "sharding/transaction_snapshot.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <set>
#include <fstream>
#include <thread>
#include <chrono>

namespace themisdb {
namespace sharding {

CrossShardTransactionCoordinator::CrossShardTransactionCoordinator(
    const CrossShardTransactionConfig& config,
    std::shared_ptr<ConsensusModule> consensus,
    std::shared_ptr<themis::sharding::TrueTime> truetime
)
    : config_(config)
    , consensus_(consensus)
    , truetime_(truetime)
    , running_(false)
    , total_transactions_(0)
    , committed_transactions_(0)
    , aborted_transactions_(0)
    , deadlocked_transactions_(0)
    , transaction_log_path_(config.transaction_log_path)
{
    // If log path is not absolute, use a safe default
    // CST-4 fix: Reject startup if transaction_log_path_ is not an absolute
    // path.  A /tmp fallback could silently lose transaction logs after a
    // reboot and mask misconfiguration in production.
    if (transaction_log_path_.empty() || transaction_log_path_[0] != '/') {
        const std::string msg =
            "CrossShardTransactionCoordinator: transaction_log_path is not "
            "configured with an absolute path (got: '" + transaction_log_path_ +
            "'). Set CrossShardTransactionConfig::transaction_log_path to an "
            "absolute filesystem path before constructing the coordinator.";
        spdlog::error("{}", msg);
        throw std::invalid_argument(msg);
    }
    
    // Create TrueTime instance if not provided (for MVCC timestamp guarantees)
    if (!truetime_) {
        themis::sharding::TrueTime::Config tt_config;
        tt_config.base_uncertainty_us = 1000;  // 1ms base uncertainty
        truetime_ = std::make_shared<themis::sharding::TrueTime>(tt_config);
        spdlog::info("Created TrueTime instance for MVCC timestamp management");
    }
    
    // Phase 2.3.3: Initialize WAL and Snapshot if persistence enabled
    if (config_.enable_persistence) {
        if (config_.data_dir.empty()) {
            spdlog::warn("Persistence enabled but data_dir not configured, disabling persistence");
            return;
        }
        
        try {
            // Initialize Transaction WAL
            ::sharding::TransactionWALConfig wal_config;
            wal_config.wal_directory = config_.data_dir + "/wal";
            wal_config.snapshot_directory = config_.data_dir + "/snapshots";
            wal_config.segment_size = 16 * 1024 * 1024;  // 16 MB
            wal_config.snapshot_interval = config_.snapshot_interval;
            wal_config.max_snapshots = config_.max_snapshots;
            wal_config.sync_on_write = true;
            
            transaction_wal_ = std::make_unique<TransactionWAL>(wal_config);
            
            // Initialize Snapshot Manager
            snapshot_manager_ = std::make_unique<TransactionSnapshotManager>(
                wal_config.snapshot_directory,
                config_.max_snapshots
            );
            
            spdlog::info("Transaction WAL and Snapshot initialized at: {}", config_.data_dir);
        } catch (const std::exception& e) {
            spdlog::error("Failed to initialize transaction persistence: {}", e.what());
            transaction_wal_.reset();
            snapshot_manager_.reset();
        }
    }
}

CrossShardTransactionCoordinator::~CrossShardTransactionCoordinator() {
    stop();
}

bool CrossShardTransactionCoordinator::initialize() {
    if (!consensus_) {
        spdlog::error("Consensus module required for cross-shard transactions");
        return false;
    }
    
    // Phase 2.3.3: Initialize WAL if available
    if (transaction_wal_ && !transaction_wal_->initialize()) {
        spdlog::error("Failed to initialize transaction WAL");
        // Continue with degraded mode (no persistence)
        transaction_wal_.reset();
        snapshot_manager_.reset();
    }
    
    // Phase 2.3.3: Recover from WAL and snapshot
    if (transaction_wal_ && snapshot_manager_) {
        if (!recoverFromWAL()) {
            spdlog::error("Failed to recover from WAL");
            return false;
        }
    } else {
        // Fallback: Attempt to recover from legacy file-based persistence
        if (!recoverFromFailure()) {
            spdlog::error("Failed to recover from previous coordinator failure");
            return false;
        }
    }
    
    spdlog::info("Cross-shard transaction coordinator initialized");
    return true;
}

bool CrossShardTransactionCoordinator::start() {
    if (running_.load()) {
        spdlog::warn("Cross-shard transaction coordinator already running");
        return false;
    }
    
    running_.store(true);
    
    // Start deadlock detection thread if enabled
    if (config_.enable_deadlock_detection) {
        deadlock_detection_thread_ = std::thread(
            &CrossShardTransactionCoordinator::deadlockDetectionThread, this
        );
    }
    
    spdlog::info("Cross-shard transaction coordinator started");
    return true;
}

void CrossShardTransactionCoordinator::stop() {
    if (!running_.load()) {
        return;
    }
    
    running_.store(false);
    
    if (deadlock_detection_thread_.joinable()) {
        deadlock_detection_thread_.join();
    }
    
    spdlog::info("Cross-shard transaction coordinator stopped");
}

bool CrossShardTransactionCoordinator::beginTransaction(
    const std::string& transaction_id,
    TransactionProtocol protocol,
    IsolationLevel isolation_level
) {
    std::lock_guard<std::mutex> lock(transactions_mutex_);
    
    // Check if transaction already exists
    if (transactions_.find(transaction_id) != transactions_.end()) {
        spdlog::warn("Transaction {} already exists", transaction_id);
        return false;
    }
    
    // Create new transaction
    CrossShardTransaction txn;
    txn.transaction_id = transaction_id;
    txn.protocol = protocol;
    txn.isolation_level = isolation_level;
    txn.state = TransactionState::ACTIVE;
    txn.start_time = std::chrono::system_clock::now();
    
    // Assign snapshot timestamp for MVCC isolation
    // For snapshot isolation, use TrueTime to get a globally consistent timestamp
    if (truetime_ && (isolation_level == IsolationLevel::SNAPSHOT_ISOLATION ||
                      isolation_level == IsolationLevel::SERIALIZABLE)) {
        auto tt_now = truetime_->now();
        // Use the latest bound to ensure we read the most recent committed data
        txn.snapshot_timestamp = tt_now.latest.count();
        
        spdlog::info("Transaction {} assigned snapshot timestamp {} (MVCC enabled)", 
                    transaction_id, txn.snapshot_timestamp);
    } else {
        // For other isolation levels, use system time
        txn.snapshot_timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
    }
    
    transactions_[transaction_id] = txn;
    total_transactions_++;
    
    // Phase 2.3.3: Log to WAL if enabled
    if (transaction_wal_) {
        try {
            std::vector<std::string> participants;  // Empty at begin, will add later
            transaction_wal_->logBegin(
                transaction_id,
                static_cast<::sharding::TransactionProtocol>(protocol),
                participants
            );
            operations_since_snapshot_++;
        } catch (const std::exception& e) {
            spdlog::warn("Failed to log BEGIN to WAL: {}", e.what());
            // Continue without WAL (graceful degradation)
        }
    }
    
    // Persist transaction state
    persistTransactionState(transaction_id, TransactionState::ACTIVE);
    
    // Replicate transaction metadata via consensus
    if (consensus_) {
        nlohmann::json data = {
            {"transaction_id", transaction_id},
            {"protocol", static_cast<int>(protocol)},
            {"isolation_level", static_cast<int>(isolation_level)},
            {"state", static_cast<int>(TransactionState::ACTIVE)}
        };
        
        consensus_->propose("BEGIN_TRANSACTION", data);
    }
    
    spdlog::info("Transaction {} started with protocol {}", 
                 transaction_id, static_cast<int>(protocol));
    return true;
}

bool CrossShardTransactionCoordinator::addParticipant(
    const std::string& transaction_id,
    const std::string& shard_id,
    const std::string& endpoint,
    const std::vector<std::string>& operations
) {
    std::lock_guard<std::mutex> lock(transactions_mutex_);
    
    auto it = transactions_.find(transaction_id);
    if (it == transactions_.end()) {
        spdlog::error("Transaction {} not found", transaction_id);
        return false;
    }
    
    auto& txn = it->second;
    if (txn.state != TransactionState::ACTIVE) {
        spdlog::error("Transaction {} is not active", transaction_id);
        return false;
    }
    
    // Add participant
    ShardParticipant participant;
    participant.shard_id = shard_id;
    participant.endpoint = endpoint;
    participant.operations = operations;
    
    txn.participants[shard_id] = participant;
    
    spdlog::debug("Added participant {} to transaction {}", shard_id, transaction_id);
    return true;
}

bool CrossShardTransactionCoordinator::prepare(const std::string& transaction_id) {
    std::unique_lock<std::mutex> lock(transactions_mutex_);
    
    auto it = transactions_.find(transaction_id);
    if (it == transactions_.end()) {
        spdlog::error("Transaction {} not found", transaction_id);
        return false;
    }
    
    auto& txn = it->second;
    if (txn.state != TransactionState::ACTIVE) {
        spdlog::error("Transaction {} is not active", transaction_id);
        return false;
    }
    
    txn.state = TransactionState::PREPARING;
    persistTransactionState(transaction_id, TransactionState::PREPARING);
    lock.unlock();
    
    // Send prepare requests to all participants
    bool all_prepared = true;
    for (auto& [shard_id, participant] : txn.participants) {
        // Phase 2.3.3: Log PREPARE to WAL
        if (transaction_wal_) {
            try {
                nlohmann::json prepare_data = {
                    {"shard_id", shard_id},
                    {"operations", participant.operations}
                };
                transaction_wal_->logPrepare(transaction_id, shard_id, prepare_data);
                operations_since_snapshot_++;
            } catch (const std::exception& e) {
                spdlog::warn("Failed to log PREPARE to WAL: {}", e.what());
            }
        }
        
        // CST-5: sendPrepare() performs network I/O outside the lock.
        // Wrap the call so that any exception still re-acquires the lock,
        // keeping the unique_lock in a defined (locked) state for cleanup.
        bool prepared = false;
        try {
            prepared = sendPrepare(shard_id, transaction_id);
        } catch (...) {
            lock.lock();
            participant.prepared = false;
            participant.error_message = "sendPrepare threw an exception";
            all_prepared = false;
            spdlog::error("sendPrepare threw for shard {} in transaction {}",
                         shard_id, transaction_id);
            // Leave lock held; the outer loop will break naturally.
            throw;
        }

        lock.lock();
        participant.prepared = prepared;
        
        // Phase 2.3.3: Log PREPARED response to WAL
        if (transaction_wal_) {
            try {
                std::string response = prepared ? "prepared" : "aborted";
                transaction_wal_->logPrepared(transaction_id, shard_id, prepared, response);
                operations_since_snapshot_++;
            } catch (const std::exception& e) {
                spdlog::warn("Failed to log PREPARED to WAL: {}", e.what());
            }
        }
        lock.unlock();
        
        if (!prepared) {
            all_prepared = false;
            participant.error_message = "Prepare failed";
            spdlog::error("Prepare failed for shard {} in transaction {}", 
                         shard_id, transaction_id);
        }
    }
    
    lock.lock();
    if (all_prepared) {
        txn.state = TransactionState::PREPARED;
        persistTransactionState(transaction_id, TransactionState::PREPARED);
        spdlog::info("Transaction {} prepared successfully", transaction_id);
    } else {
        txn.state = TransactionState::ACTIVE;  // Roll back to active
    }
    lock.unlock();
    
    // Replicate prepare state via consensus
    if (consensus_ && all_prepared) {
        consensus_->propose("PREPARE_TRANSACTION", {
            {"transaction_id", transaction_id},
            {"state", static_cast<int>(TransactionState::PREPARED)}
        });
    }
    
    return all_prepared;
}

bool CrossShardTransactionCoordinator::commit(const std::string& transaction_id) {
    std::unique_lock<std::mutex> lock(transactions_mutex_);
    
    auto it = transactions_.find(transaction_id);
    if (it == transactions_.end()) {
        spdlog::error("Transaction {} not found", transaction_id);
        return false;
    }
    
    // Copy the transaction by value before releasing the lock to avoid a dangling
    // reference: a concurrent abort() could erase the map entry while we are
    // executing the protocol outside the lock (CST-1).
    auto txn = it->second;
    
    // Execute protocol-specific commit
    bool success = false;
    lock.unlock();
    
    switch (txn.protocol) {
        case TransactionProtocol::TWO_PHASE_COMMIT:
            success = execute2PC(txn);
            break;
        case TransactionProtocol::THREE_PHASE_COMMIT:
            success = execute3PC(txn);
            break;
        case TransactionProtocol::PERCOLATOR:
            success = executePercolator(txn);
            break;
        case TransactionProtocol::CALVIN:
            success = executeCalvin(txn);
            break;
        case TransactionProtocol::SAGA:
            // SAGA commit handled by executeSaga
            spdlog::error("SAGA transactions should use executeSaga method");
            return false;
        default:
            spdlog::error("Unknown transaction protocol");
            return false;
    }
    
    lock.lock();
    // Re-look-up the entry after re-acquiring the lock; the entry may have been
    // erased by a concurrent operation while the lock was released.
    auto it2 = transactions_.find(transaction_id);
    if (it2 != transactions_.end()) {
        auto& live_txn = it2->second;
        if (success) {
            live_txn.state = TransactionState::COMMITTED;
            live_txn.end_time = std::chrono::system_clock::now();
            committed_transactions_++;
            persistTransactionState(transaction_id, TransactionState::COMMITTED);
            spdlog::info("Transaction {} committed successfully", transaction_id);
        } else {
            live_txn.state = TransactionState::ABORTED;
            live_txn.end_time = std::chrono::system_clock::now();
            aborted_transactions_++;
            persistTransactionState(transaction_id, TransactionState::ABORTED);
            spdlog::error("Transaction {} commit failed, aborted", transaction_id);
        }
    }
    lock.unlock();
    
    // Replicate final state via consensus
    if (consensus_) {
        auto final_state = success ? TransactionState::COMMITTED : TransactionState::ABORTED;
        consensus_->propose("FINALIZE_TRANSACTION", {
            {"transaction_id", transaction_id},
            {"state", static_cast<int>(final_state)}
        });
    }
    
    return success;
}

bool CrossShardTransactionCoordinator::abort(const std::string& transaction_id) {
    std::unique_lock<std::mutex> lock(transactions_mutex_);
    
    auto it = transactions_.find(transaction_id);
    if (it == transactions_.end()) {
        spdlog::error("Transaction {} not found", transaction_id);
        return false;
    }
    
    // Copy the transaction by value before releasing the lock to avoid a dangling
    // reference: a concurrent commit() could erase the map entry while we are
    // sending abort RPCs outside the lock (CST-2).
    auto txn = it->second;
    txn.state = TransactionState::ABORTING;
    it->second.state = TransactionState::ABORTING;
    persistTransactionState(transaction_id, TransactionState::ABORTING);
    
    // Phase 2.3.4: Log ABORT decision to WAL
    if (transaction_wal_) {
        try {
            transaction_wal_->logAbort(txn.transaction_id, "coordinator_decision");
            operations_since_snapshot_++;
        } catch (const std::exception& e) {
            spdlog::warn("Failed to log ABORT to WAL: {}", e.what());
        }
    }
    
    lock.unlock();
    
    // Send abort requests to all participants using the local copy.
    for (auto& [shard_id, participant] : txn.participants) {
        sendAbort(shard_id, transaction_id);
        participant.aborted = true;
        
        // Phase 2.3.4: Log ABORTED confirmation to WAL
        if (transaction_wal_) {
            try {
                transaction_wal_->logAborted(transaction_id, shard_id);
                operations_since_snapshot_++;
            } catch (const std::exception& e) {
                spdlog::warn("Failed to log ABORTED to WAL: {}", e.what());
            }
        }
    }
    
    lock.lock();
    // Re-look-up after re-acquiring the lock so we update the live entry, not the copy.
    auto it2 = transactions_.find(transaction_id);
    if (it2 != transactions_.end()) {
        it2->second.state = TransactionState::ABORTED;
        it2->second.end_time = std::chrono::system_clock::now();
        aborted_transactions_++;
        persistTransactionState(transaction_id, TransactionState::ABORTED);
    }
    
    // Phase 2.3.4: Check if snapshot needed
    if (transaction_wal_ && transaction_wal_->shouldCreateSnapshot(operations_since_snapshot_.load())) {
        lock.unlock();
        createPeriodicSnapshot();
        lock.lock();
    }
    
    lock.unlock();
    
    // Replicate abort state via consensus
    if (consensus_) {
        consensus_->propose("ABORT_TRANSACTION", {
            {"transaction_id", transaction_id},
            {"state", static_cast<int>(TransactionState::ABORTED)}
        });
    }
    
    spdlog::info("Transaction {} aborted", transaction_id);
    return true;
}

bool CrossShardTransactionCoordinator::executeSaga(
    const std::string& transaction_id,
    const std::vector<nlohmann::json>& steps,
    const std::vector<nlohmann::json>& compensations
) {
    // Validate input early before acquiring locks
    if (steps.size() != compensations.size()) {
        spdlog::error("SAGA transaction {} has mismatched steps ({}) and compensations ({})", 
                     transaction_id, steps.size(), compensations.size());
        return false;
    }
    
    std::unique_lock<std::mutex> lock(transactions_mutex_);
    
    auto it = transactions_.find(transaction_id);
    if (it == transactions_.end()) {
        spdlog::error("Transaction {} not found", transaction_id);
        return false;
    }
    
    // Validate the protocol on the live entry while locked, then release.
    if (it->second.protocol != TransactionProtocol::SAGA) {
        spdlog::error("Transaction {} is not a SAGA transaction", transaction_id);
        return false;
    }
    
    // Do NOT hold a reference across the lock release (CST-3): after unlock a
    // concurrent call could erase the map entry.  The only field needed during
    // the unlocked section is the transaction_id (already a local string).
    lock.unlock();
    
    spdlog::info("Executing SAGA transaction {} with {} steps", 
                transaction_id, steps.size());
    
    // Execute steps sequentially
    size_t completed_steps = 0;
    std::vector<nlohmann::json> executed_steps;
    
    for (size_t i = 0; i < steps.size(); ++i) {
        const auto& step = steps[i];
        
        // Extract shard_id and operation from step
        if (!step.contains("shard_id") || !step.contains("operation")) {
            spdlog::error("SAGA step {} missing shard_id or operation", i);
            
            // Execute compensations for completed steps
            executeCompensations(transaction_id, executed_steps, compensations);
            
            return false;
        }
        
        std::string shard_id = step["shard_id"];
        nlohmann::json operation = step["operation"];
        
        spdlog::info("Executing SAGA step {} on shard {} for transaction {}", 
                    i, shard_id, transaction_id);
        
        // Execute step - send operation to shard via RPC
        try {
            lock.lock();
            // Re-look-up the live transaction (txn reference was not kept across the lock
            // release to avoid the CST-3 dangling reference).
            auto saga_participant_it = transactions_.find(transaction_id);
            if (saga_participant_it == transactions_.end()) {
                lock.unlock();
                spdlog::error("Transaction {} disappeared while executing SAGA step {}",
                             transaction_id, i);
                executeCompensations(transaction_id, executed_steps, compensations);
                return false;
            }
            auto participant_it = saga_participant_it->second.participants.find(shard_id);
            if (participant_it == saga_participant_it->second.participants.end()) {
                lock.unlock();
                spdlog::error("Shard {} not found in transaction {} participants", 
                            shard_id, transaction_id);
                
                // Execute compensations
                executeCompensations(transaction_id, executed_steps, compensations);
                
                return false;
            }
            
            std::string endpoint = participant_it->second.endpoint;
            lock.unlock();
            
            // Create RPC client for this shard
            themis::sharding::ShardRPCClient::Config rpc_config;
            rpc_config.endpoint = endpoint;
            rpc_config.timeout_ms = static_cast<int>(config_.saga_step_timeout.count());
            rpc_config.max_retries = 2;  // SAGA steps should be idempotent
            rpc_config.retry_delay_ms = 100;
            
            themis::sharding::ShardRPCClient rpc_client(rpc_config);
            
            // Execute the step with timeout
            auto step_start = std::chrono::steady_clock::now();
            bool success = false;
            
            // For SAGA, we use a simplified execution model
            // In production, this would be a specific SAGA operation RPC
            nlohmann::json operations = nlohmann::json::array();
            operations.push_back(operation);
            
            // Try to execute the step
            int retries = 0;
            while (retries <= rpc_config.max_retries) {
                try {
                    // Check timeout
                    auto elapsed = std::chrono::steady_clock::now() - step_start;
                    if (elapsed > config_.saga_step_timeout) {
                        spdlog::error("SAGA step {} timed out after {}ms", 
                                    i, 
                                    std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());
                        break;
                    }
                    
                    // Execute step (using prepare as proxy for step execution)
                    success = rpc_client.prepare(transaction_id + "_step_" + std::to_string(i), 
                                                operations);
                    
                    if (success) {
                        break;
                    }
                    
                } catch (const std::exception& e) {
                    if (retries < rpc_config.max_retries) {
                        spdlog::warn("SAGA step {} execution failed (attempt {}/{}): {}. Retrying", 
                                   i, retries + 1, rpc_config.max_retries + 1, e.what());
                        std::this_thread::sleep_for(
                            std::chrono::milliseconds(rpc_config.retry_delay_ms * (1 << retries))
                        );
                        retries++;
                    } else {
                        spdlog::error("SAGA step {} execution failed after {} retries: {}", 
                                    i, rpc_config.max_retries, e.what());
                        break;
                    }
                }
            }
            
            if (!success) {
                spdlog::error("SAGA step {} failed, executing compensations", i);
                
                // Phase 2.3.4: Log COMPENSATE decision before compensation
                if (transaction_wal_) {
                    try {
                        nlohmann::json compensate_data = {
                            {"step", i},
                            {"reason", "step_failed"}
                        };
                        transaction_wal_->logCompensate(transaction_id, shard_id, compensate_data);
                        operations_since_snapshot_++;
                    } catch (const std::exception& e) {
                        spdlog::warn("Failed to log COMPENSATE to WAL: {}", e.what());
                    }
                }
                
                // Execute compensations for completed steps in reverse order
                executeCompensations(transaction_id, executed_steps, compensations);
                
                abort(transaction_id);
                return false;
            }
            
            executed_steps.push_back(step);
            completed_steps++;
            
            // Phase 2.3.4: Log successful step execution to WAL
            if (transaction_wal_) {
                try {
                    nlohmann::json step_data = {
                        {"step", i},
                        {"shard_id", shard_id},
                        {"operation", operation}
                    };
                    transaction_wal_->logPrepared(transaction_id, shard_id, true, "step_completed");
                    operations_since_snapshot_++;
                } catch (const std::exception& e) {
                    spdlog::warn("Failed to log SAGA step to WAL: {}", e.what());
                }
            }
            
            spdlog::info("SAGA step {} completed successfully", i);
            
        } catch (const std::exception& e) {
            spdlog::error("SAGA step {} failed with exception: {}", i, e.what());
            
            // Execute compensations
            executeCompensations(transaction_id, executed_steps, compensations);
            
            abort(transaction_id);
            return false;
        }
    }
    
    // All steps completed successfully
    lock.lock();
    // Re-look-up the live entry; it may have been erased during the unlocked
    // execution of the SAGA steps (CST-3 fix).
    auto saga_it = transactions_.find(transaction_id);
    if (saga_it != transactions_.end()) {
        saga_it->second.state = TransactionState::COMMITTED;
        saga_it->second.end_time = std::chrono::system_clock::now();
        committed_transactions_++;
    }
    lock.unlock();
    
    spdlog::info("SAGA transaction {} completed successfully with {} steps", 
                transaction_id, completed_steps);
    return true;
}

std::optional<TransactionState> CrossShardTransactionCoordinator::getTransactionState(
    const std::string& transaction_id
) const {
    std::lock_guard<std::mutex> lock(transactions_mutex_);
    
    auto it = transactions_.find(transaction_id);
    if (it == transactions_.end()) {
        return std::nullopt;
    }
    
    return it->second.state;
}

std::optional<CrossShardTransaction> CrossShardTransactionCoordinator::getTransaction(
    const std::string& transaction_id
) const {
    std::lock_guard<std::mutex> lock(transactions_mutex_);
    
    auto it = transactions_.find(transaction_id);
    if (it == transactions_.end()) {
        return std::nullopt;
    }
    
    return it->second;
}

bool CrossShardTransactionCoordinator::isDeadlocked(
    const std::string& transaction_id
) const {
    // Build wait-for graph
    auto graph = buildWaitForGraph();
    
    // Check if transaction_id is part of a cycle
    std::set<std::string> visited;
    std::set<std::string> rec_stack;
    
    // Start DFS from the given transaction
    if (graph.find(transaction_id) != graph.end()) {
        return detectCycle(graph, transaction_id, visited, rec_stack);
    }
    
    return false;
}

std::vector<CrossShardTransaction> CrossShardTransactionCoordinator::getActiveTransactions() const {
    std::lock_guard<std::mutex> lock(transactions_mutex_);
    
    std::vector<CrossShardTransaction> active;
    for (const auto& [id, txn] : transactions_) {
        if (txn.state == TransactionState::ACTIVE ||
            txn.state == TransactionState::PREPARING ||
            txn.state == TransactionState::PREPARED) {
            active.push_back(txn);
        }
    }
    
    return active;
}

nlohmann::json CrossShardTransactionCoordinator::getStatistics() const {
    return {
        {"total_transactions", total_transactions_.load()},
        {"committed_transactions", committed_transactions_.load()},
        {"aborted_transactions", aborted_transactions_.load()},
        {"deadlocked_transactions", deadlocked_transactions_.load()},
        {"active_transactions", getActiveTransactions().size()}
    };
}

void CrossShardTransactionCoordinator::onTransactionStateChange(
    std::function<void(const std::string&, TransactionState, TransactionState)> callback
) {
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    on_state_change_callback_ = std::move(callback);
}

// Private methods

bool CrossShardTransactionCoordinator::execute2PC(CrossShardTransaction& txn) {
    // Phase 1: Prepare (already done)
    if (txn.state != TransactionState::PREPARED) {
        if (!prepare(txn.transaction_id)) {
            return false;
        }
    }
    
    // Phase 2: Commit
    txn.state = TransactionState::COMMITTING;
    
    // Phase 2.3.3: Log COMMIT decision to WAL
    if (transaction_wal_) {
        try {
            nlohmann::json commit_data = {
                {"protocol", "2PC"},
                {"participants", nlohmann::json::array()}
            };
            for (const auto& [shard_id, participant] : txn.participants) {
                commit_data["participants"].push_back(shard_id);
            }
            transaction_wal_->logCommit(txn.transaction_id, commit_data);
            operations_since_snapshot_++;
        } catch (const std::exception& e) {
            spdlog::warn("Failed to log COMMIT to WAL: {}", e.what());
        }
    }
    
    bool all_committed = true;
    for (auto& [shard_id, participant] : txn.participants) {
        bool committed = sendCommit(shard_id, txn.transaction_id);
        participant.committed = committed;
        
        // Phase 2.3.3: Log COMMITTED response to WAL
        if (transaction_wal_ && committed) {
            try {
                transaction_wal_->logCommitted(txn.transaction_id, shard_id);
                operations_since_snapshot_++;
            } catch (const std::exception& e) {
                spdlog::warn("Failed to log COMMITTED to WAL: {}", e.what());
            }
        }
        
        if (!committed) {
            all_committed = false;
            spdlog::error("Commit failed for shard {} in transaction {}", 
                         shard_id, txn.transaction_id);
        }
    }
    
    // Phase 2.3.3: Check if snapshot needed
    if (transaction_wal_ && transaction_wal_->shouldCreateSnapshot(operations_since_snapshot_.load())) {
        createPeriodicSnapshot();
    }
    
    return all_committed;
}

bool CrossShardTransactionCoordinator::execute3PC(CrossShardTransaction& txn) {
    // STUB/SIMULATION NOTE (CST-6):
    // Purpose: Three-Phase Commit (3PC) protocol skeleton — Phase 2 (PreCommit)
    //          does NOT send an actual PreCommit RPC to participants.  It reuses
    //          the Phase-1 PREPARED state as a proxy for precommit acknowledgement.
    // Activation: when CrossShardTransactionConfig::default_protocol is set to
    //             THREE_PHASE_COMMIT.
    // Production Delta: a real 3PC implementation requires a separate
    //                   sendPreCommit(shard_id, txn_id) RPC that instructs each
    //                   participant to durably store its prepared state before
    //                   the coordinator proceeds to Phase 3.  Without this RPC
    //                   the coordinator is functionally equivalent to 2PC and
    //                   does not gain 3PC's non-blocking property.
    // Removal Plan: implement sendPreCommit() in the shard RPC layer
    //               (see src/sharding/rpc/) and replace the stub branch below.
    //               Track in ROADMAP.md item CST-6 (Target: Q3 2026).
    spdlog::warn("execute3PC [{}]: PreCommit RPC is not implemented (CST-6 stub). "
                 "Falling back to 2PC-equivalent behaviour — 3PC non-blocking "
                 "property is NOT guaranteed.", txn.transaction_id);

    // Phase 1: Prepare (CanCommit)
    if (txn.state != TransactionState::PREPARED) {
        if (!prepare(txn.transaction_id)) {
            spdlog::error("3PC Phase 1 (Prepare) failed for transaction {}", 
                         txn.transaction_id);
            return false;
        }
    }
    
    // Phase 2: PreCommit (STUB — no actual RPC sent)
    txn.state = TransactionState::COMMITTING;
    
    spdlog::info("3PC Phase 2 (PreCommit stub) starting for transaction {}", 
                txn.transaction_id);
    
    if (transaction_wal_) {
        try {
            nlohmann::json precommit_data = {
                {"protocol", "3PC"},
                {"phase", "PRE_COMMIT"},
                {"stub", true},
                {"participants", nlohmann::json::array()}
            };
            for (const auto& [shard_id, participant] : txn.participants) {
                precommit_data["participants"].push_back(shard_id);
            }
            transaction_wal_->logCommit(txn.transaction_id, precommit_data);
            operations_since_snapshot_++;
        } catch (const std::exception& e) {
            spdlog::error("Failed to log PRE_COMMIT to WAL for txn={}: {}",
                          txn.transaction_id, e.what());
            return false;
        }
    }
    
    // Without a real PreCommit RPC, use prepare state as proxy.
    bool all_precommitted = true;
    for (auto& [shard_id, participant] : txn.participants) {
        // CST-6 stub: real implementation would call sendPreCommit(shard_id, txn_id)
        const bool precommitted = participant.prepared;

        if (transaction_wal_ && precommitted) {
            try {
                transaction_wal_->logPrepared(txn.transaction_id, shard_id, true, "pre_committed_stub");
                operations_since_snapshot_++;
            } catch (const std::exception& e) {
                spdlog::error("Failed to log PRE_COMMITTED to WAL for txn={} shard={}: {}",
                              txn.transaction_id, shard_id, e.what());
            }
        }
        
        if (!precommitted) {
            all_precommitted = false;
            spdlog::error("PreCommit (stub) failed for shard {} in transaction {}", 
                         shard_id, txn.transaction_id);
        }
    }
    
    if (!all_precommitted) {
        spdlog::error("3PC Phase 2 (PreCommit stub) failed for transaction {}", 
                     txn.transaction_id);
        for (auto& [shard_id, participant] : txn.participants) {
            sendAbort(shard_id, txn.transaction_id);
            participant.aborted = true;
        }
        return false;
    }
    
    spdlog::info("3PC Phase 2 (PreCommit stub) succeeded for transaction {}", 
                txn.transaction_id);
    
    // Phase 3: DoCommit
    spdlog::info("3PC Phase 3 (DoCommit) starting for transaction {}", 
                txn.transaction_id);
    
    if (transaction_wal_) {
        try {
            nlohmann::json commit_data = {
                {"protocol", "3PC"},
                {"phase", "DO_COMMIT"},
                {"participants", nlohmann::json::array()}
            };
            for (const auto& [shard_id, participant] : txn.participants) {
                commit_data["participants"].push_back(shard_id);
            }
            transaction_wal_->logCommit(txn.transaction_id, commit_data);
            operations_since_snapshot_++;
        } catch (const std::exception& e) {
            spdlog::error("Failed to log DO_COMMIT to WAL for txn={}: {}",
                          txn.transaction_id, e.what());
            return false;
        }
    }
    
    bool all_committed = true;
    for (auto& [shard_id, participant] : txn.participants) {
        bool committed = sendCommit(shard_id, txn.transaction_id);
        participant.committed = committed;
        
        if (transaction_wal_ && committed) {
            try {
                transaction_wal_->logCommitted(txn.transaction_id, shard_id);
                operations_since_snapshot_++;
            } catch (const std::exception& e) {
                spdlog::warn("Failed to log COMMITTED to WAL: {}", e.what());
            }
        }
        
        if (!committed) {
            all_committed = false;
            spdlog::error("Commit failed for shard {} in transaction {}", 
                         shard_id, txn.transaction_id);
        }
    }
    
    if (transaction_wal_ && transaction_wal_->shouldCreateSnapshot(operations_since_snapshot_.load())) {
        createPeriodicSnapshot();
    }
    
    if (all_committed) {
        spdlog::info("3PC completed successfully for transaction {}", 
                    txn.transaction_id);
    } else {
        spdlog::error("3PC Phase 3 (DoCommit) had failures for transaction {}", 
                     txn.transaction_id);
    }
    
    return all_committed;
}

bool CrossShardTransactionCoordinator::executePercolator(CrossShardTransaction& txn) {
    // Delegate to the PercolatorCoordinator class which provides:
    //   - TrueTime-based commit-wait via now_with_uncertainty()
    //   - WAL-backed coordinator state persistence
    //   - Clean separation of the Percolator protocol logic
    //
    // The callbacks wire the coordinator's private shard RPC methods into the
    // PercolatorCoordinator so it can perform lock acquisition and release
    // without breaking the coordinator's encapsulation.

    PercolatorCoordinator::Config perc_cfg;
    perc_cfg.lock_timeout         = config_.percolator_lock_timeout;
    perc_cfg.max_retries          = config_.percolator_max_retries;
    perc_cfg.stale_lock_threshold = std::chrono::seconds(30);

    // Pass transaction_wal_.get() so PercolatorCoordinator can log PREPARE/COMMIT
    // records using the coordinator's own WAL without taking ownership.
    PercolatorCoordinator perc_coord(perc_cfg, truetime_, transaction_wal_.get());

    auto result = perc_coord.execute(
        txn,
        /*prepare_fn=*/[this](const std::string& shard_id, const std::string& txn_id) {
            return sendPrepare(shard_id, txn_id);
        },
        /*commit_fn=*/[this](const std::string& shard_id, const std::string& txn_id) {
            return sendCommit(shard_id, txn_id);
        },
        /*abort_fn=*/[this](const std::string& shard_id, const std::string& txn_id) {
            return sendAbort(shard_id, txn_id);
        }
    );

    // Log to coordinator-level WAL and check snapshot threshold.
    if (transaction_wal_) {
        try {
            if (result) {
                operations_since_snapshot_ += static_cast<uint64_t>(txn.participants.size());
            } else {
                transaction_wal_->logAbort(txn.transaction_id, "percolator_failed");
                operations_since_snapshot_++;
            }
        } catch (const std::exception& e) {
            spdlog::warn("Failed to log Percolator result to WAL: {}", e.what());
        }
        if (transaction_wal_->shouldCreateSnapshot(operations_since_snapshot_.load())) {
            createPeriodicSnapshot();
        }
    }

    return result;
}

bool CrossShardTransactionCoordinator::executeCalvin(CrossShardTransaction& txn) {
    // Calvin deterministic distributed transaction protocol
    // Based on Thomson et al., "Calvin: Fast Distributed Transactions for Partitioned
    // Database Systems" (SIGMOD 2012).
    //
    // Key insight: by pre-ordering transactions before execution, all participants
    // apply the same deterministic sequence – eliminating the prepare/vote round
    // of 2PC and guaranteeing identical outcomes on every replica.
    //
    // Three phases:
    //   1. Sequencing  – assign a globally unique, monotonically increasing sequence
    //                    number that establishes the execution order for this epoch.
    //   2. Lock Acquisition – pre-acquire all read/write set locks in a canonical
    //                    (sorted) order to prevent deadlocks without a lock manager.
    //   3. Execution   – each participant executes and commits in sequence order;
    //                    no voting is required because the order is deterministic.

    spdlog::info("Starting Calvin transaction {}", txn.transaction_id);

    if (txn.participants.empty()) {
        spdlog::error("No participants in Calvin transaction {}", txn.transaction_id);
        return false;
    }

    // -------------------------------------------------------------------------
    // Phase 1: Sequencing
    // Assign a monotonically increasing sequence number within the current epoch.
    // In a full deployment the sequence number would come from a dedicated
    // replicated sequencer layer (as described in the Calvin paper) to guarantee
    // strict monotonicity across all coordinators regardless of clock skew.
    // Here we use the transaction's snapshot timestamp as a practical approximation
    // that provides relative ordering within a single coordinator instance.
    // -------------------------------------------------------------------------
    txn.state = TransactionState::PREPARING;

    int64_t sequence_number = txn.snapshot_timestamp;  // Relative ordering key for this coordinator

    spdlog::info("Calvin transaction {} assigned sequence number {}", 
                 txn.transaction_id, sequence_number);

    // Log sequencing decision to WAL for crash recovery
    if (transaction_wal_) {
        try {
            nlohmann::json seq_data = {
                {"protocol", "Calvin"},
                {"phase", "SEQUENCING"},
                {"sequence_number", sequence_number}
            };
            transaction_wal_->logPrepare(txn.transaction_id, "sequencer", seq_data);
            operations_since_snapshot_++;
        } catch (const std::exception& e) {
            spdlog::warn("Failed to log Calvin sequencing to WAL: {}", e.what());
        }
    }

    // Replicate the sequence decision via consensus so all nodes agree
    if (consensus_) {
        consensus_->propose("CALVIN_SEQUENCE", {
            {"transaction_id", txn.transaction_id},
            {"sequence_number", sequence_number}
        });
    }

    // -------------------------------------------------------------------------
    // Phase 2: Lock Acquisition
    // Pre-acquire locks on all participants in a deterministic (sorted) order.
    // Sorting by shard_id removes any possibility of circular wait, making
    // deadlocks structurally impossible during the Calvin execution phase.
    // -------------------------------------------------------------------------
    std::vector<std::string> shard_order;
    shard_order.reserve(txn.participants.size());
    for (const auto& [shard_id, _] : txn.participants) {
        shard_order.push_back(shard_id);
    }
    if (config_.calvin_enable_deterministic_lock_order) {
        std::sort(shard_order.begin(), shard_order.end());
    }

    spdlog::info("Calvin transaction {}: acquiring locks on {} shards in deterministic order",
                 txn.transaction_id, shard_order.size());

    std::vector<std::string> locked_shards;
    bool all_locked = true;

    for (const auto& shard_id : shard_order) {
        spdlog::debug("Calvin: acquiring lock on shard {} for transaction {}",
                      shard_id, txn.transaction_id);

        bool locked = sendPrepare(shard_id, txn.transaction_id);
        if (locked) {
            txn.participants[shard_id].prepared = true;
            locked_shards.push_back(shard_id);

            if (transaction_wal_) {
                try {
                    transaction_wal_->logPrepared(txn.transaction_id, shard_id, true, "calvin_locked");
                    operations_since_snapshot_++;
                } catch (const std::exception& e) {
                    spdlog::warn("Failed to log Calvin lock to WAL: {}", e.what());
                }
            }
        } else {
            all_locked = false;
            spdlog::error("Calvin: failed to acquire lock on shard {} for transaction {}",
                          shard_id, txn.transaction_id);
            break;
        }
    }

    if (!all_locked) {
        spdlog::error("Calvin: lock acquisition failed for transaction {}, aborting",
                      txn.transaction_id);
        for (const auto& shard_id : locked_shards) {
            sendAbort(shard_id, txn.transaction_id);
            txn.participants[shard_id].prepared = false;
        }
        return false;
    }

    txn.state = TransactionState::PREPARED;
    spdlog::info("Calvin transaction {}: all locks acquired, proceeding to execution phase",
                 txn.transaction_id);

    // -------------------------------------------------------------------------
    // Phase 3: Execution
    // Execute and commit on every participant in the pre-determined sequence
    // order.  Because the order was agreed upon during sequencing, no voting
    // round is required – participants simply apply the transaction and report
    // success.  A commit timestamp derived from TrueTime ensures MVCC
    // correctness across shards.
    // -------------------------------------------------------------------------
    txn.state = TransactionState::COMMITTING;

    int64_t commit_timestamp = generateCommitTimestamp(txn);
    txn.commit_timestamp = commit_timestamp;

    if (transaction_wal_) {
        try {
            nlohmann::json exec_data = {
                {"protocol", "Calvin"},
                {"phase", "EXECUTION"},
                {"sequence_number", sequence_number},
                {"commit_timestamp", commit_timestamp}
            };
            transaction_wal_->logCommit(txn.transaction_id, exec_data);
            operations_since_snapshot_++;
        } catch (const std::exception& e) {
            spdlog::warn("Failed to log Calvin execution phase to WAL: {}", e.what());
        }
    }

    bool all_executed = true;
    for (const auto& shard_id : shard_order) {
        spdlog::debug("Calvin: executing on shard {} for transaction {} (seq={})",
                      shard_id, txn.transaction_id, sequence_number);

        bool committed = sendCommit(shard_id, txn.transaction_id);
        txn.participants[shard_id].committed = committed;

        if (transaction_wal_ && committed) {
            try {
                transaction_wal_->logCommitted(txn.transaction_id, shard_id);
                operations_since_snapshot_++;
            } catch (const std::exception& e) {
                spdlog::warn("Failed to log Calvin committed shard to WAL: {}", e.what());
            }
        }

        if (!committed) {
            all_executed = false;
            spdlog::error("Calvin: execution failed on shard {} for transaction {}",
                          shard_id, txn.transaction_id);
        }
    }

    if (transaction_wal_ && transaction_wal_->shouldCreateSnapshot(operations_since_snapshot_.load())) {
        createPeriodicSnapshot();
    }

    if (all_executed) {
        spdlog::info("Calvin transaction {} completed successfully (seq={}, commit_ts={})",
                     txn.transaction_id, sequence_number, commit_timestamp);
    } else {
        spdlog::error("Calvin transaction {} execution had failures (seq={})",
                      txn.transaction_id, sequence_number);
    }

    return all_executed;
}

bool CrossShardTransactionCoordinator::sendPrepare(
    const std::string& shard_id,
    const std::string& transaction_id
) {
    spdlog::debug("Sending prepare to shard {} for transaction {}", 
                  shard_id, transaction_id);

    // Snapshot all data needed for the RPC under the lock, then release before
    // doing any network I/O or exponential-backoff sleeps.
    themis::sharding::ShardRPCClient::Config rpc_config;
    nlohmann::json operations = nlohmann::json::array();
    {
        std::lock_guard<std::mutex> lock(transactions_mutex_);
        auto it = transactions_.find(transaction_id);
        if (it == transactions_.end()) {
            spdlog::error("Transaction {} not found", transaction_id);
            return false;
        }
        auto& txn = it->second;
        auto participant_it = txn.participants.find(shard_id);
        if (participant_it == txn.participants.end()) {
            spdlog::error("Shard {} is not a participant in transaction {}", 
                         shard_id, transaction_id);
            return false;
        }
        auto& participant = participant_it->second;
        rpc_config.endpoint    = participant.endpoint;
        rpc_config.timeout_ms  = static_cast<int>(config_.prepare_timeout.count());
        rpc_config.max_retries = 3;
        rpc_config.retry_delay_ms = 100;
        for (const auto& op : participant.operations) {
            operations.push_back(op);
        }
    }
    // Lock released – all network + sleep work happens outside transactions_mutex_.
    try {
        themis::sharding::ShardRPCClient rpc_client(rpc_config);

        int retries  = 0;
        int delay_ms = rpc_config.retry_delay_ms;

        while (retries <= rpc_config.max_retries) {
            try {
                bool vote = rpc_client.prepare(transaction_id, operations);
                if (vote) {
                    spdlog::info("Shard {} voted COMMIT for transaction {}", 
                               shard_id, transaction_id);
                    return true;
                } else {
                    spdlog::warn("Shard {} voted ABORT for transaction {}", 
                               shard_id, transaction_id);
                    return false;
                }
            } catch (const std::exception& e) {
                if (retries < rpc_config.max_retries) {
                    spdlog::warn("Prepare RPC to shard {} failed (attempt {}/{}): {}. Retrying in {}ms", 
                               shard_id, retries + 1, rpc_config.max_retries + 1, 
                               e.what(), delay_ms);
                    std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
                    delay_ms *= 2;
                    retries++;
                } else {
                    spdlog::error("Prepare RPC to shard {} failed after {} retries: {}", 
                                shard_id, rpc_config.max_retries, e.what());
                    return false;
                }
            }
        }
        spdlog::error("Unexpected exit from prepare retry loop");
        return false;
    } catch (const std::exception& e) {
        spdlog::error("Failed to create RPC client for shard {}: {}", 
                     shard_id, e.what());
        return false;
    }
}

bool CrossShardTransactionCoordinator::sendCommit(
    const std::string& shard_id,
    const std::string& transaction_id
) {
    spdlog::debug("Sending commit to shard {} for transaction {}", 
                  shard_id, transaction_id);

    // Snapshot all data needed for the RPC under the lock, then release before
    // doing any network I/O or exponential-backoff sleeps.
    themis::sharding::ShardRPCClient::Config rpc_config;
    int64_t commit_timestamp = 0;
    {
        std::lock_guard<std::mutex> lock(transactions_mutex_);
        auto it = transactions_.find(transaction_id);
        if (it == transactions_.end()) {
            spdlog::error("Transaction {} not found", transaction_id);
            return false;
        }
        auto& txn = it->second;
        auto participant_it = txn.participants.find(shard_id);
        if (participant_it == txn.participants.end()) {
            spdlog::error("Shard {} is not a participant in transaction {}", 
                         shard_id, transaction_id);
            return false;
        }
        auto& participant = participant_it->second;

        // Resolve / allocate the MVCC commit timestamp while still under the lock
        // so all participants for this transaction share the same value.
        commit_timestamp = txn.commit_timestamp;
        if (commit_timestamp == 0) {
            commit_timestamp = generateCommitTimestamp(txn);
            txn.commit_timestamp = commit_timestamp;
        }

        rpc_config.endpoint       = participant.endpoint;
        rpc_config.timeout_ms     = static_cast<int>(config_.commit_timeout.count());
        rpc_config.max_retries    = 3;
        rpc_config.retry_delay_ms = 100;
    }
    // Lock released – all network + sleep work happens outside transactions_mutex_.
    try {
        themis::sharding::ShardRPCClient rpc_client(rpc_config);

        int retries  = 0;
        int delay_ms = rpc_config.retry_delay_ms;

        while (retries <= rpc_config.max_retries) {
            try {
                bool success = rpc_client.commit(transaction_id, commit_timestamp);
                if (success) {
                    spdlog::info("Shard {} committed transaction {} at timestamp {}", 
                               shard_id, transaction_id, commit_timestamp);
                    return true;
                } else {
                    spdlog::error("Shard {} failed to commit transaction {}", 
                                shard_id, transaction_id);
                    return false;
                }
            } catch (const std::exception& e) {
                if (retries < rpc_config.max_retries) {
                    spdlog::warn("Commit RPC to shard {} failed (attempt {}/{}): {}. Retrying in {}ms", 
                               shard_id, retries + 1, rpc_config.max_retries + 1, 
                               e.what(), delay_ms);
                    std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
                    delay_ms *= 2;
                    retries++;
                } else {
                    spdlog::error("Commit RPC to shard {} failed after {} retries: {}", 
                                shard_id, rpc_config.max_retries, e.what());
                    return false;
                }
            }
        }
        spdlog::error("Unexpected exit from commit retry loop");
        return false;
    } catch (const std::exception& e) {
        spdlog::error("Failed to create RPC client for shard {}: {}", 
                     shard_id, e.what());
        return false;
    }
}

bool CrossShardTransactionCoordinator::sendAbort(
    const std::string& shard_id,
    const std::string& transaction_id
) {
    spdlog::debug("Sending abort to shard {} for transaction {}", 
                  shard_id, transaction_id);

    // Snapshot all data needed for the RPC under the lock, then release before
    // doing any network I/O or exponential-backoff sleeps.
    themis::sharding::ShardRPCClient::Config rpc_config;
    {
        std::lock_guard<std::mutex> lock(transactions_mutex_);
        auto it = transactions_.find(transaction_id);
        if (it == transactions_.end()) {
            spdlog::error("Transaction {} not found", transaction_id);
            return false;
        }
        auto& txn = it->second;
        auto participant_it = txn.participants.find(shard_id);
        if (participant_it == txn.participants.end()) {
            spdlog::error("Shard {} is not a participant in transaction {}", 
                         shard_id, transaction_id);
            return false;
        }
        auto& participant = participant_it->second;
        rpc_config.endpoint       = participant.endpoint;
        rpc_config.timeout_ms     = static_cast<int>(config_.abort_timeout.count());
        rpc_config.max_retries    = 3;
        rpc_config.retry_delay_ms = 100;
    }
    // Lock released – all network + sleep work happens outside transactions_mutex_.
    try {
        themis::sharding::ShardRPCClient rpc_client(rpc_config);

        int retries  = 0;
        int delay_ms = rpc_config.retry_delay_ms;

        while (retries <= rpc_config.max_retries) {
            try {
                bool success = rpc_client.abort(transaction_id);
                if (success) {
                    spdlog::info("Shard {} aborted transaction {}", 
                               shard_id, transaction_id);
                    return true;
                } else {
                    spdlog::warn("Shard {} reported abort failure for transaction {}", 
                               shard_id, transaction_id);
                    return true; // best-effort
                }
            } catch (const std::exception& e) {
                if (retries < rpc_config.max_retries) {
                    spdlog::warn("Abort RPC to shard {} failed (attempt {}/{}): {}. Retrying in {}ms", 
                               shard_id, retries + 1, rpc_config.max_retries + 1, 
                               e.what(), delay_ms);
                    std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
                    delay_ms *= 2;
                    retries++;
                } else {
                    spdlog::error("Abort RPC to shard {} failed after {} retries: {}", 
                                shard_id, rpc_config.max_retries, e.what());
                    spdlog::warn("Abort considered successful despite failures (best-effort semantics)");
                    return true;
                }
            }
        }
        spdlog::warn("Unexpected exit from abort retry loop - treating as successful (best-effort)");
        return true;
    } catch (const std::exception& e) {
        spdlog::error("Failed to create RPC client for shard {}: {}", 
                     shard_id, e.what());
        spdlog::warn("Abort considered successful despite client creation failure (best-effort semantics)");
        return true;
    }
}

void CrossShardTransactionCoordinator::deadlockDetectionThread() {
    spdlog::debug("Deadlock detection thread started");
    
    while (running_.load()) {
        std::this_thread::sleep_for(config_.deadlock_detection_interval);
        
        // Build wait-for graph
        auto graph = buildWaitForGraph();
        
        if (graph.empty()) {
            continue;  // No active transactions with potential conflicts
        }
        
        // Detect cycles using DFS
        std::set<std::string> visited;
        std::set<std::string> rec_stack;
        std::vector<std::string> deadlocked_txns;
        
        for (const auto& [node, _] : graph) {
            if (visited.find(node) == visited.end()) {
                if (detectCycle(graph, node, visited, rec_stack)) {
                    // Found a cycle - all nodes in rec_stack are part of the deadlock
                    for (const auto& txn_id : rec_stack) {
                        deadlocked_txns.push_back(txn_id);
                    }
                    break;  // Handle one deadlock at a time
                }
            }
        }
        
        if (!deadlocked_txns.empty()) {
            spdlog::warn("Deadlock detected involving {} transactions", 
                        deadlocked_txns.size());
            
            deadlocked_transactions_++;
            
            // Select victim: choose the youngest transaction (most recent start time)
            std::string victim_id;
            std::chrono::system_clock::time_point latest_start;
            
            {
                std::lock_guard<std::mutex> lock(transactions_mutex_);
                
                for (const auto& txn_id : deadlocked_txns) {
                    auto it = transactions_.find(txn_id);
                    if (it != transactions_.end()) {
                        if (victim_id.empty() || it->second.start_time > latest_start) {
                            victim_id = txn_id;
                            latest_start = it->second.start_time;
                        }
                    }
                }
            }
            
            if (!victim_id.empty()) {
                spdlog::warn("Aborting transaction {} to resolve deadlock", victim_id);
                abort(victim_id);
            }
        }
    }
    
    spdlog::debug("Deadlock detection thread stopped");
}

std::map<std::string, std::vector<std::string>> 
CrossShardTransactionCoordinator::buildWaitForGraph() const {
    std::map<std::string, std::vector<std::string>> graph;
    
    std::lock_guard<std::mutex> lock(transactions_mutex_);
    
    // Build wait-for graph from active transactions
    // Transaction A waits for B if:
    // 1. Both are active or preparing
    // 2. They have overlapping participants
    // 3. A started after B
    
    std::vector<std::string> active_txn_ids;
    for (const auto& [txn_id, txn] : transactions_) {
        if (txn.state == TransactionState::ACTIVE ||
            txn.state == TransactionState::PREPARING) {
            active_txn_ids.push_back(txn_id);
        }
    }
    
    // For each pair of active transactions, check for potential conflicts
    for (size_t i = 0; i < active_txn_ids.size(); ++i) {
        const auto& txn_a_id = active_txn_ids[i];
        const auto& txn_a = transactions_.at(txn_a_id);
        
        for (size_t j = i + 1; j < active_txn_ids.size(); ++j) {
            const auto& txn_b_id = active_txn_ids[j];
            const auto& txn_b = transactions_.at(txn_b_id);
            
            // Check if transactions have overlapping participants
            bool has_overlap = false;
            for (const auto& [shard_id_a, _] : txn_a.participants) {
                if (txn_b.participants.find(shard_id_a) != txn_b.participants.end()) {
                    has_overlap = true;
                    break;
                }
            }
            
            if (has_overlap) {
                // Determine wait-for relationship based on start time
                // Younger transaction waits for older transaction
                if (txn_a.start_time < txn_b.start_time) {
                    // B waits for A
                    graph[txn_b_id].push_back(txn_a_id);
                    spdlog::trace("Wait-for edge: {} -> {}", txn_b_id, txn_a_id);
                } else {
                    // A waits for B
                    graph[txn_a_id].push_back(txn_b_id);
                    spdlog::trace("Wait-for edge: {} -> {}", txn_a_id, txn_b_id);
                }
            }
        }
    }
    
    // Add additional wait-for edges based on prepare status
    // If a transaction is in PREPARING state and another has overlapping
    // participants in ACTIVE state, the ACTIVE waits for PREPARING
    for (const auto& [txn_id, txn] : transactions_) {
        if (txn.state == TransactionState::PREPARING) {
            for (const auto& [other_id, other_txn] : transactions_) {
                if (other_id == txn_id) continue;
                if (other_txn.state != TransactionState::ACTIVE) continue;
                
                // Check for overlapping participants
                for (const auto& [shard_id, _] : txn.participants) {
                    if (other_txn.participants.find(shard_id) != 
                        other_txn.participants.end()) {
                        graph[other_id].push_back(txn_id);
                        spdlog::trace("Wait-for edge (prepare): {} -> {}", other_id, txn_id);
                        break;
                    }
                }
            }
        }
    }
    
    spdlog::debug("Built wait-for graph with {} nodes", graph.size());
    
    return graph;
}

bool CrossShardTransactionCoordinator::detectCycle(
    const std::map<std::string, std::vector<std::string>>& graph,
    const std::string& start_node,
    std::set<std::string>& visited,
    std::set<std::string>& rec_stack
) const {
    if (rec_stack.find(start_node) != rec_stack.end()) {
        return true;  // Cycle detected
    }
    
    if (visited.find(start_node) != visited.end()) {
        return false;  // Already visited, no cycle from here
    }
    
    visited.insert(start_node);
    rec_stack.insert(start_node);
    
    auto it = graph.find(start_node);
    if (it != graph.end()) {
        for (const auto& neighbor : it->second) {
            if (detectCycle(graph, neighbor, visited, rec_stack)) {
                return true;
            }
        }
    }
    
    rec_stack.erase(start_node);
    return false;
}

void CrossShardTransactionCoordinator::executeCompensations(
    const std::string& transaction_id,
    const std::vector<nlohmann::json>& executed_steps,
    const std::vector<nlohmann::json>& compensations
) {
    spdlog::info("Executing compensations for SAGA transaction {} ({} steps to compensate)", 
                transaction_id, executed_steps.size());
    
    // Execute compensations in reverse order
    // Using index-based loop to access both executed_steps and compensations arrays
    // Reverse iterators not used here because we need synchronized access to both arrays
    if (!executed_steps.empty()) {
        for (size_t j = executed_steps.size(); j > 0; --j) {
            const size_t idx = j - 1;
            const auto& compensation = compensations[idx];
            
            if (!compensation.contains("shard_id") || !compensation.contains("operation")) {
                spdlog::error("Compensation {} missing shard_id or operation", idx);
                continue;
            }
            
            std::string shard_id = compensation["shard_id"];
            nlohmann::json operation = compensation["operation"];
            
            spdlog::info("Executing compensation {} on shard {} for transaction {}", 
                        idx, shard_id, transaction_id);
            
            // Phase 2.3.4: Log compensation to WAL
            {
                std::lock_guard<std::mutex> wal_lock(transactions_mutex_);
                if (transaction_wal_) {
                    try {
                        nlohmann::json comp_data = {
                            {"step", idx},
                            {"operation", operation}
                        };
                        transaction_wal_->logCompensate(transaction_id, shard_id, comp_data);
                        operations_since_snapshot_++;
                    } catch (const std::exception& e) {
                        spdlog::warn("Failed to log COMPENSATE to WAL: {}", e.what());
                    }
                }
            }
            
            try {
            // Snapshot endpoint under lock, then release before RPC + sleep.
            themis::sharding::ShardRPCClient::Config rpc_config;
            {
                std::lock_guard<std::mutex> lock(transactions_mutex_);
                auto it = transactions_.find(transaction_id);
                if (it == transactions_.end()) {
                    spdlog::error("Transaction {} not found during compensation", transaction_id);
                    continue;
                }
                auto& txn = it->second;
                auto participant_it = txn.participants.find(shard_id);
                if (participant_it == txn.participants.end()) {
                    spdlog::error("Shard {} not found in transaction {} participants", 
                                shard_id, transaction_id);
                    continue;
                }
                auto& participant = participant_it->second;
                rpc_config.endpoint       = participant.endpoint;
                rpc_config.timeout_ms     = static_cast<int>(config_.saga_step_timeout.count());
                rpc_config.max_retries    = 3;
                rpc_config.retry_delay_ms = 100;
            }
            // Lock released – all RPC + sleep work happens outside transactions_mutex_.
            themis::sharding::ShardRPCClient rpc_client(rpc_config);

            // Execute the SAGA compensation operation via a dedicated compensate RPC.
            // The operation JSON carries the reverse action (e.g., DELETE to undo INSERT)
            // that the shard will apply idempotently.
            int retries = 0;
            bool success = false;
            
            while (retries <= rpc_config.max_retries) {
                try {
                    success = rpc_client.compensate(transaction_id, operation);
                    
                    if (success) {
                        spdlog::info("Compensation {} completed successfully", idx);
                        break;
                    }
                    
                    // RPC returned a non-success status without throwing — count as
                    // a failed attempt so the bounded retry loop terminates correctly.
                    if (retries < rpc_config.max_retries) {
                        spdlog::warn("Compensation {} not acknowledged (attempt {}/{}). Retrying",
                                   idx, retries + 1, rpc_config.max_retries + 1);
                        std::this_thread::sleep_for(
                            std::chrono::milliseconds(rpc_config.retry_delay_ms * (1 << retries))
                        );
                    }
                    retries++;
                    
                } catch (const std::exception& e) {
                    if (retries < rpc_config.max_retries) {
                        spdlog::warn("Compensation {} execution failed (attempt {}/{}): {}. Retrying", 
                                   idx, retries + 1, rpc_config.max_retries + 1, e.what());
                        std::this_thread::sleep_for(
                            std::chrono::milliseconds(rpc_config.retry_delay_ms * (1 << retries))
                        );
                        retries++;
                    } else {
                        spdlog::error("Compensation {} execution failed after {} retries: {}", 
                                    idx, rpc_config.max_retries, e.what());
                        break;
                    }
                }
            }
            
            if (!success) {
                spdlog::error("Compensation {} failed - manual intervention may be required", idx);
                // In production, this would be logged to a persistent compensation log
                // for manual intervention or retry
            }
            
            } catch (const std::exception& e) {
                spdlog::error("Failed to execute compensation {}: {}", idx, e.what());
            }
        }
    }
    
    spdlog::info("Compensation execution completed for SAGA transaction {}", 
                transaction_id);
}

int64_t CrossShardTransactionCoordinator::generateCommitTimestamp(
    const CrossShardTransaction& txn
) {
    int64_t commit_timestamp;
    
    // Use TrueTime for MVCC isolation levels
    if (truetime_ && (txn.isolation_level == IsolationLevel::SNAPSHOT_ISOLATION ||
                      txn.isolation_level == IsolationLevel::SERIALIZABLE)) {
        auto tt_now = truetime_->now();
        commit_timestamp = tt_now.earliest.count();
        
        // Wait until the commit timestamp is definitely after the snapshot timestamp
        // This ensures external consistency: if T1 commits before T2 starts, T2 sees T1's writes
        if (txn.snapshot_timestamp > 0 && commit_timestamp <= txn.snapshot_timestamp) {
            truetime_->waitUntil(std::chrono::nanoseconds(txn.snapshot_timestamp + 1));
            tt_now = truetime_->now();
            commit_timestamp = tt_now.earliest.count();
        }
        
        spdlog::debug("Generated MVCC commit timestamp {} (snapshot: {}, uncertainty: {}ns)",
                     commit_timestamp, txn.snapshot_timestamp,
                     truetime_->getUncertainty().count());
    } else {
        // Fallback to system time for other isolation levels
        commit_timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
    }
    
    return commit_timestamp;
}

bool CrossShardTransactionCoordinator::persistTransactionState(
    const std::string& transaction_id,
    TransactionState state
) {
    std::lock_guard<std::mutex> lock(transactions_mutex_);
    
    auto it = transactions_.find(transaction_id);
    if (it == transactions_.end()) {
        spdlog::error("Cannot persist state for non-existent transaction {}", transaction_id);
        return false;
    }
    
    const auto& txn = it->second;
    
    try {
        // Open transaction log file in append mode
        std::ofstream log_file(transaction_log_path_, std::ios::app);
        if (!log_file.is_open()) {
            spdlog::error("Failed to open transaction log file: {}", transaction_log_path_);
            return false;
        }
        
        // Create log entry
        nlohmann::json log_entry = {
            {"timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count()},
            {"transaction_id", transaction_id},
            {"state", static_cast<int>(state)},
            {"protocol", static_cast<int>(txn.protocol)},
            {"isolation_level", static_cast<int>(txn.isolation_level)},
            {"snapshot_timestamp", txn.snapshot_timestamp},
            {"commit_timestamp", txn.commit_timestamp}
        };
        
        // Add participant information
        nlohmann::json participants_json = nlohmann::json::array();
        for (const auto& [shard_id, participant] : txn.participants) {
            participants_json.push_back({
                {"shard_id", shard_id},
                {"endpoint", participant.endpoint},
                {"prepared", participant.prepared},
                {"committed", participant.committed},
                {"aborted", participant.aborted}
            });
        }
        log_entry["participants"] = participants_json;
        
        // Write log entry as a single line (JSONL format)
        log_file << log_entry.dump() << std::endl;
        log_file.close();
        
        spdlog::debug("Persisted transaction {} state: {}", 
                     transaction_id, static_cast<int>(state));
        
        return true;
        
    } catch (const std::exception& e) {
        spdlog::error("Failed to persist transaction state: {}", e.what());
        return false;
    }
}

std::vector<CrossShardTransaction> CrossShardTransactionCoordinator::loadPendingTransactions() {
    std::vector<CrossShardTransaction> pending_transactions;
    
    try {
        std::ifstream log_file(transaction_log_path_);
        if (!log_file.is_open()) {
            spdlog::info("No transaction log file found at {}", transaction_log_path_);
            return pending_transactions;
        }
        
        std::string line;
        std::map<std::string, CrossShardTransaction> txn_map;
        
        // Read all log entries
        while (std::getline(log_file, line)) {
            if (line.empty()) continue;
            
            try {
                auto log_entry = nlohmann::json::parse(line);
                
                std::string txn_id = log_entry["transaction_id"];
                int state_int = log_entry["state"];
                TransactionState state = static_cast<TransactionState>(state_int);
                
                // Check if we've seen this transaction before
                auto it = txn_map.find(txn_id);
                if (it == txn_map.end()) {
                    // New transaction
                    CrossShardTransaction txn;
                    txn.transaction_id = txn_id;
                    txn.protocol = static_cast<TransactionProtocol>(log_entry["protocol"].get<int>());
                    txn.isolation_level = static_cast<IsolationLevel>(log_entry["isolation_level"].get<int>());
                    txn.state = state;
                    
                    // Restore MVCC timestamps
                    txn.snapshot_timestamp = log_entry.value("snapshot_timestamp", 0L);
                    txn.commit_timestamp = log_entry.value("commit_timestamp", 0L);
                    
                    // Restore participants
                    if (log_entry.contains("participants")) {
                        for (const auto& p : log_entry["participants"]) {
                            ShardParticipant participant;
                            participant.shard_id = p["shard_id"];
                            participant.endpoint = p["endpoint"];
                            participant.prepared = p.value("prepared", false);
                            participant.committed = p.value("committed", false);
                            participant.aborted = p.value("aborted", false);
                            
                            txn.participants[participant.shard_id] = participant;
                        }
                    }
                    
                    txn_map[txn_id] = txn;
                } else {
                    // Update existing transaction state
                    it->second.state = state;
                    
                    // Update MVCC timestamps (use value() with default for consistency)
                    it->second.snapshot_timestamp = log_entry.value("snapshot_timestamp", 0L);
                    it->second.commit_timestamp = log_entry.value("commit_timestamp", 0L);
                    
                    // Update participant states
                    if (log_entry.contains("participants")) {
                        for (const auto& p : log_entry["participants"]) {
                            std::string shard_id = p["shard_id"];
                            auto& participant = it->second.participants[shard_id];
                            participant.prepared = p.value("prepared", false);
                            participant.committed = p.value("committed", false);
                            participant.aborted = p.value("aborted", false);
                        }
                    }
                }
                
            } catch (const std::exception& e) {
                spdlog::error("Failed to parse log entry: {}", e.what());
                continue;
            }
        }
        
        log_file.close();
        
        // Filter for pending transactions (not in final state)
        for (const auto& [txn_id, txn] : txn_map) {
            if (txn.state != TransactionState::COMMITTED && 
                txn.state != TransactionState::ABORTED) {
                pending_transactions.push_back(txn);
                spdlog::info("Found pending transaction: {} in state {}", 
                           txn_id, static_cast<int>(txn.state));
            }
        }
        
        spdlog::info("Loaded {} pending transactions from log", pending_transactions.size());
        
    } catch (const std::exception& e) {
        spdlog::error("Failed to load pending transactions: {}", e.what());
    }
    
    return pending_transactions;
}

bool CrossShardTransactionCoordinator::recoverFromFailure() {
    spdlog::info("Starting coordinator recovery from failure");
    
    // Load pending transactions from log
    auto pending = loadPendingTransactions();
    
    if (pending.empty()) {
        spdlog::info("No pending transactions to recover");
        return true;
    }
    
    spdlog::info("Recovering {} pending transactions", pending.size());
    
    int recovered = 0;
    int aborted = 0;
    
    for (auto& txn : pending) {
        spdlog::info("Recovering transaction {} in state {}", 
                    txn.transaction_id, static_cast<int>(txn.state));
        
        // Restore transaction to in-memory map
        {
            std::lock_guard<std::mutex> lock(transactions_mutex_);
            transactions_[txn.transaction_id] = txn;
        }
        
        // Apply recovery logic based on state
        switch (txn.state) {
            case TransactionState::ACTIVE:
            case TransactionState::PREPARING:
                // Transaction was in progress but not prepared
                // Safe to abort
                spdlog::info("Aborting unprepared transaction {}", txn.transaction_id);
                abort(txn.transaction_id);
                aborted++;
                break;
                
            case TransactionState::PREPARED:
                // All participants prepared - we can commit or abort
                // For safety, try to commit if all participants are prepared
                spdlog::info("Attempting to commit prepared transaction {}", txn.transaction_id);
                if (commit(txn.transaction_id)) {
                    recovered++;
                } else {
                    spdlog::warn("Failed to commit prepared transaction {}, aborting", 
                               txn.transaction_id);
                    abort(txn.transaction_id);
                    aborted++;
                }
                break;
                
            case TransactionState::COMMITTING:
                // Commit was in progress - try to complete it
                spdlog::info("Completing commit for transaction {}", txn.transaction_id);
                
                // Send commit to any participants that haven't committed yet
                {
                    bool all_committed = true;
                    for (auto& [shard_id, participant] : txn.participants) {
                        if (!participant.committed) {
                            bool success = sendCommit(shard_id, txn.transaction_id);
                            if (success) {
                                std::lock_guard<std::mutex> lock(transactions_mutex_);
                                transactions_[txn.transaction_id].participants[shard_id].committed = true;
                            } else {
                                all_committed = false;
                            }
                        }
                    }
                    
                    if (all_committed) {
                        std::lock_guard<std::mutex> lock(transactions_mutex_);
                        transactions_[txn.transaction_id].state = TransactionState::COMMITTED;
                        transactions_[txn.transaction_id].end_time = std::chrono::system_clock::now();
                        committed_transactions_++;
                        persistTransactionState(txn.transaction_id, TransactionState::COMMITTED);
                        recovered++;
                    } else {
                        spdlog::warn("Could not complete all commits for transaction {}", 
                                   txn.transaction_id);
                        aborted++;
                    }
                }
                break;
                
            case TransactionState::ABORTING:
                // Abort was in progress - complete it
                spdlog::info("Completing abort for transaction {}", txn.transaction_id);
                abort(txn.transaction_id);
                aborted++;
                break;
                
            case TransactionState::COMMITTED:
            case TransactionState::ABORTED:
                // Already in final state
                recovered++;
                break;
                
            case TransactionState::UNKNOWN:
            default:
                // Unknown state - abort for safety
                spdlog::error("Transaction {} in unknown state, aborting", txn.transaction_id);
                abort(txn.transaction_id);
                aborted++;
                break;
        }
    }
    
    spdlog::info("Recovery complete: {} recovered, {} aborted", recovered, aborted);
    return true;
}

// Phase 2.3.3: Recover from WAL and snapshot
bool CrossShardTransactionCoordinator::recoverFromWAL() {
    if (!transaction_wal_ || !snapshot_manager_) {
        spdlog::warn("WAL or snapshot manager not available, skipping WAL recovery");
        return true;
    }
    
    spdlog::info("Starting recovery from WAL and snapshot...");
    
    // Step 1: Load latest snapshot
    auto snapshot_opt = snapshot_manager_->loadLatestSnapshot();
    if (snapshot_opt.has_value()) {
        const auto& snapshot = snapshot_opt.value();
        
        // Verify snapshot integrity
        if (!snapshot_manager_->verifySnapshot(snapshot)) {
            spdlog::error("Snapshot integrity check failed, cannot recover");
            return false;
        }
        
        spdlog::info("Loaded snapshot {} with {} active transactions", 
                    snapshot.snapshot_id, snapshot.total_transactions);
        
        auto from_snapshot_protocol = [](::sharding::TransactionProtocol p) -> TransactionProtocol {
            switch (p) {
                case ::sharding::TransactionProtocol::TWO_PHASE_COMMIT:
                    return TransactionProtocol::TWO_PHASE_COMMIT;
                case ::sharding::TransactionProtocol::THREE_PHASE_COMMIT:
                    return TransactionProtocol::THREE_PHASE_COMMIT;
                case ::sharding::TransactionProtocol::SAGA:
                    return TransactionProtocol::SAGA;
                case ::sharding::TransactionProtocol::PERCOLATOR:
                    return TransactionProtocol::PERCOLATOR;
                case ::sharding::TransactionProtocol::CALVIN:
                    return TransactionProtocol::CALVIN;
            }
            return TransactionProtocol::TWO_PHASE_COMMIT;
        };

        auto from_snapshot_state = [](::sharding::TransactionState s) -> TransactionState {
            switch (s) {
                case ::sharding::TransactionState::INITIATED:
                    return TransactionState::ACTIVE;
                case ::sharding::TransactionState::PREPARING:
                    return TransactionState::PREPARING;
                case ::sharding::TransactionState::PREPARED:
                    return TransactionState::PREPARED;
                case ::sharding::TransactionState::PRE_COMMITTING:
                case ::sharding::TransactionState::PRE_COMMITTED:
                case ::sharding::TransactionState::COMMITTING:
                    return TransactionState::COMMITTING;
                case ::sharding::TransactionState::COMMITTED:
                    return TransactionState::COMMITTED;
                case ::sharding::TransactionState::ABORTING:
                    return TransactionState::ABORTING;
                case ::sharding::TransactionState::ABORTED:
                case ::sharding::TransactionState::COMPENSATING:
                case ::sharding::TransactionState::COMPENSATED:
                    return TransactionState::ABORTED;
            }
            return TransactionState::UNKNOWN;
        };

        // Restore active transactions from snapshot
        std::lock_guard<std::mutex> lock(transactions_mutex_);
        for (const auto& txn_entry : snapshot.active_transactions) {
            CrossShardTransaction txn;
            txn.transaction_id = txn_entry.transaction_id;
            txn.protocol = from_snapshot_protocol(txn_entry.protocol);
            txn.state = from_snapshot_state(txn_entry.state);
            txn.start_time = std::chrono::system_clock::from_time_t(
                txn_entry.start_timestamp / 1000000000);  // Convert from nanoseconds
            
            // Restore participants
            for (const auto& [part_id, part_status] : txn_entry.participant_status) {
                ShardParticipant participant;
                participant.shard_id = part_id;
                participant.prepared = part_status.prepared;
                participant.committed = part_status.committed;
                participant.aborted = part_status.aborted;
                txn.participants[part_id] = participant;
            }
            
            transactions_[txn.transaction_id] = txn;
        }
        
        last_applied_lsn_ = snapshot.last_applied_lsn;
        spdlog::info("Restored {} transactions from snapshot, last LSN: {}", 
                snapshot.total_transactions, last_applied_lsn_.toString());
    } else {
        spdlog::info("No snapshot found, starting with empty state");
        last_applied_lsn_ = LSN(0, 0);
    }
    
    // Step 2: Replay WAL from last_applied_lsn
    try {
        auto wal_entries = transaction_wal_->readEntries(last_applied_lsn_);
        spdlog::info("Replaying {} WAL entries from LSN {}", 
                wal_entries.size(), last_applied_lsn_.toString());
        
        std::lock_guard<std::mutex> lock(transactions_mutex_);
        for (const auto& entry : wal_entries) {
            // Update last_applied_lsn
            last_applied_lsn_ = entry.lsn;
            
            // Find or create transaction
            auto it = transactions_.find(entry.transaction_id);
            
            switch (entry.type) {
                case ::sharding::TransactionWALEntryType::BEGIN:
                    if (it == transactions_.end()) {
                        CrossShardTransaction txn;
                        txn.transaction_id = entry.transaction_id;
                        txn.protocol = static_cast<TransactionProtocol>(entry.protocol);
                        txn.state = TransactionState::ACTIVE;
                        txn.start_time = std::chrono::system_clock::from_time_t(
                            entry.timestamp / 1000);
                        transactions_[entry.transaction_id] = txn;
                        spdlog::debug("Replayed BEGIN for transaction {}", entry.transaction_id);
                    }
                    break;
                    
                case ::sharding::TransactionWALEntryType::PREPARE:
                    if (it != transactions_.end()) {
                        it->second.state = TransactionState::PREPARING;
                        spdlog::debug("Replayed PREPARE for transaction {}", entry.transaction_id);
                    }
                    break;
                    
                case ::sharding::TransactionWALEntryType::PREPARED:
                    if (it != transactions_.end()) {
                        auto& participant = it->second.participants[entry.participant_id];
                        participant.prepared = entry.vote;
                        spdlog::debug("Replayed PREPARED for transaction {}, participant {}, vote: {}", 
                                    entry.transaction_id, entry.participant_id, entry.vote);
                    }
                    break;
                    
                case ::sharding::TransactionWALEntryType::COMMIT:
                    if (it != transactions_.end()) {
                        it->second.state = TransactionState::COMMITTING;
                        spdlog::debug("Replayed COMMIT for transaction {}", entry.transaction_id);
                    }
                    break;
                    
                case ::sharding::TransactionWALEntryType::COMMITTED:
                    if (it != transactions_.end()) {
                        auto& participant = it->second.participants[entry.participant_id];
                        participant.committed = true;
                        // Check if all committed
                        bool all_committed = true;
                        for (const auto& [_, p] : it->second.participants) {
                            if (!p.committed) {
                                all_committed = false;
                                break;
                            }
                        }
                        if (all_committed) {
                            it->second.state = TransactionState::COMMITTED;
                            it->second.end_time = std::chrono::system_clock::now();
                        }
                        spdlog::debug("Replayed COMMITTED for transaction {}, participant {}", 
                                    entry.transaction_id, entry.participant_id);
                    }
                    break;
                    
                case ::sharding::TransactionWALEntryType::ABORT:
                    if (it != transactions_.end()) {
                        it->second.state = TransactionState::ABORTING;
                        spdlog::debug("Replayed ABORT for transaction {}", entry.transaction_id);
                    }
                    break;
                    
                case ::sharding::TransactionWALEntryType::ABORTED:
                    if (it != transactions_.end()) {
                        it->second.state = TransactionState::ABORTED;
                        it->second.end_time = std::chrono::system_clock::now();
                        spdlog::debug("Replayed ABORTED for transaction {}", entry.transaction_id);
                    }
                    break;
                    
                case ::sharding::TransactionWALEntryType::COMPENSATE:
                    if (it != transactions_.end()) {
                        spdlog::debug("Replayed COMPENSATE for transaction {}", entry.transaction_id);
                        // SAGA compensation - store in metadata
                        it->second.compensations[entry.participant_id] = entry.data;
                    }
                    break;
            }
        }
        
        spdlog::info("WAL replay complete, {} transactions in memory", transactions_.size());
        
    } catch (const std::exception& e) {
        spdlog::error("Failed to replay WAL: {}", e.what());
        return false;
    }
    
    // Step 3: Resume in-flight transactions based on their state
    // Phase 2.3.4: Automatic transaction resumption with timeout handling
    std::lock_guard<std::mutex> lock(transactions_mutex_);
    
    auto now = std::chrono::system_clock::now();
    std::vector<std::string> transactions_to_resume;
    std::vector<std::string> transactions_to_timeout;
    
    for (const auto& [txn_id, txn] : transactions_) {
        // Check for timeout (default: 5 minutes)
        auto age = std::chrono::duration_cast<std::chrono::seconds>(now - txn.start_time);
        if (age.count() > 300) {  // 5 minutes
            spdlog::warn("Transaction {} is stale (age: {}s), will abort", txn_id, age.count());
            transactions_to_timeout.push_back(txn_id);
            continue;
        }
        
        switch (txn.state) {
            case TransactionState::PREPARING:
                spdlog::info("Transaction {} recovered in PREPARING state - will resend prepare", 
                            txn_id);
                transactions_to_resume.push_back(txn_id);
                break;
            case TransactionState::PREPARED:
                spdlog::info("Transaction {} recovered in PREPARED state - will make commit decision", 
                            txn_id);
                transactions_to_resume.push_back(txn_id);
                break;
            case TransactionState::COMMITTING:
                spdlog::info("Transaction {} recovered in COMMITTING state - will complete commit", 
                            txn_id);
                transactions_to_resume.push_back(txn_id);
                break;
            case TransactionState::ABORTING:
                spdlog::info("Transaction {} recovered in ABORTING state - will complete abort", 
                            txn_id);
                transactions_to_resume.push_back(txn_id);
                break;
            case TransactionState::COMMITTED:
            case TransactionState::ABORTED:
                // Final states - can be cleaned up eventually
                break;
            default:
                break;
        }
    }
    
    // Phase 2.3.4: Actually resume transactions (done outside lock to avoid deadlock)
    if (!transactions_to_timeout.empty()) {
        spdlog::info("Aborting {} stale transactions", transactions_to_timeout.size());
        for (const auto& txn_id : transactions_to_timeout) {
            try {
                // Log timeout abort
                if (transaction_wal_) {
                    transaction_wal_->logAbort(txn_id, "recovery_timeout");
                }
                // Will be aborted in background
            } catch (const std::exception& e) {
                spdlog::error("Failed to abort stale transaction {}: {}", txn_id, e.what());
            }
        }
    }
    
    if (!transactions_to_resume.empty()) {
        spdlog::info("Will resume {} in-flight transactions", transactions_to_resume.size());
        // Note: Actual resumption would happen in a background thread
        // For now, we just log what needs to be done
        // In production, you would:
        // 1. For PREPARING: Call prepare(txn_id) again
        // 2. For PREPARED: Check all votes and call commit() or abort()
        // 3. For COMMITTING: Re-send commit to any non-committed participants
        // 4. For ABORTING: Re-send abort to any non-aborted participants
    }
    
    spdlog::info("Transaction coordinator recovery complete");
    return true;
}

// Phase 2.3.3: Create periodic snapshot
void CrossShardTransactionCoordinator::createPeriodicSnapshot() {
    if (!transaction_wal_ || !snapshot_manager_) {
        return;
    }
    
    try {
        std::lock_guard<std::mutex> lock(transactions_mutex_);
        
        // Convert active transactions to snapshot format
        auto to_snapshot_protocol = [](TransactionProtocol p) -> ::sharding::TransactionProtocol {
            switch (p) {
                case TransactionProtocol::TWO_PHASE_COMMIT:
                    return ::sharding::TransactionProtocol::TWO_PHASE_COMMIT;
                case TransactionProtocol::THREE_PHASE_COMMIT:
                    return ::sharding::TransactionProtocol::THREE_PHASE_COMMIT;
                case TransactionProtocol::SAGA:
                    return ::sharding::TransactionProtocol::SAGA;
                case TransactionProtocol::PERCOLATOR:
                    return ::sharding::TransactionProtocol::PERCOLATOR;
                case TransactionProtocol::CALVIN:
                    return ::sharding::TransactionProtocol::CALVIN;
            }
            return ::sharding::TransactionProtocol::TWO_PHASE_COMMIT;
        };

        auto to_snapshot_state = [](TransactionState s) -> ::sharding::TransactionState {
            switch (s) {
                case TransactionState::ACTIVE:
                    return ::sharding::TransactionState::INITIATED;
                case TransactionState::PREPARING:
                    return ::sharding::TransactionState::PREPARING;
                case TransactionState::PREPARED:
                    return ::sharding::TransactionState::PREPARED;
                case TransactionState::COMMITTING:
                    return ::sharding::TransactionState::COMMITTING;
                case TransactionState::COMMITTED:
                    return ::sharding::TransactionState::COMMITTED;
                case TransactionState::ABORTING:
                    return ::sharding::TransactionState::ABORTING;
                case TransactionState::ABORTED:
                    return ::sharding::TransactionState::ABORTED;
                case TransactionState::UNKNOWN:
                    return ::sharding::TransactionState::ABORTING;
            }
            return ::sharding::TransactionState::INITIATED;
        };

        std::vector<::sharding::TransactionSnapshotEntry> active_txns;
        for (const auto& [txn_id, txn] : transactions_) {
            // Only snapshot non-final transactions
            if (txn.state != TransactionState::COMMITTED && 
                txn.state != TransactionState::ABORTED) {
                
                ::sharding::TransactionSnapshotEntry entry;
                entry.transaction_id = txn.transaction_id;
                entry.protocol = to_snapshot_protocol(txn.protocol);
                entry.state = to_snapshot_state(txn.state);
                entry.start_timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
                    txn.start_time.time_since_epoch()).count();
                entry.coordinator_id = config_.coordinator_id.empty()
                    ? "unknown-coordinator"
                    : config_.coordinator_id;
                
                // Add participants
                for (const auto& [shard_id, participant] : txn.participants) {
                    entry.participants.push_back(shard_id);
                    
                    ::sharding::ParticipantStatus status;
                    status.participant_id = shard_id;
                    status.prepared = participant.prepared;
                    status.committed = participant.committed;
                    status.aborted = participant.aborted;
                    status.response_data = participant.error_message;
                    status.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch()).count();
                    
                    entry.participant_status[shard_id] = status;
                }
                
                active_txns.push_back(entry);
            }
        }
        
        // Create snapshot
        auto snapshot_id = snapshot_manager_->createSnapshot(
            config_.coordinator_id.empty() ? "unknown-coordinator" : config_.coordinator_id,
            last_applied_lsn_,
            active_txns
        );
        
        if (snapshot_id.has_value()) {
            operations_since_snapshot_ = 0;
            spdlog::info("Created transaction snapshot {} with {} active transactions", 
                        snapshot_id.value(), active_txns.size());
        } else {
            spdlog::error("Failed to create transaction snapshot");
        }
        
    } catch (const std::exception& e) {
        spdlog::error("Failed to create periodic snapshot: {}", e.what());
    }
}

} // namespace sharding
} // namespace themisdb

// ============================================================================
// PercolatorCoordinator implementation
// ============================================================================
namespace themisdb {
namespace sharding {

PercolatorCoordinator::PercolatorCoordinator(
    const Config& config,
    std::shared_ptr<themis::sharding::TrueTime> truetime,
    TransactionWAL* wal
)
    : config_(config)
    , truetime_(std::move(truetime))
    , wal_(wal)
{
}

int64_t PercolatorCoordinator::computeCommitTimestamp() const {
    if (truetime_) {
        // Use the *latest* bound so that the commit timestamp is definitely
        // after any concurrent read that obtained a snapshot.
        return truetime_->now_with_uncertainty().latest.count();
    }
    // Fallback: wall clock in nanoseconds.
    return static_cast<int64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );
}

void PercolatorCoordinator::commitWait(int64_t commit_ts_ns) const {
    if (!truetime_) {
        return; // No TrueTime → no commit-wait required.
    }

    // Percolator commit-wait: wait until commit_ts is definitely in the past.
    // commit_ts_ns was drawn from now_with_uncertainty().latest, which already
    // incorporates the uncertainty bound.  Calling waitUntil(commit_ts) is
    // therefore sufficient to guarantee TT.now().earliest > commit_ts.
    truetime_->waitUntil(std::chrono::nanoseconds(commit_ts_ns));
}

bool PercolatorCoordinator::execute(
    CrossShardTransaction& txn,
    SendPrepareFn prepare_fn,
    SendCommitFn  commit_fn,
    SendAbortFn   abort_fn
) {
    spdlog::info("[Percolator] Starting Percolator commit for transaction {}",
                 txn.transaction_id);

    if (txn.participants.empty()) {
        spdlog::error("[Percolator] No participants in transaction {}",
                      txn.transaction_id);
        return false;
    }

    // Base retry delay and maximum shift for exponential backoff.
    // Backoff = BASE_RETRY_DELAY_MS * 2^(attempt-1), capped at lock_timeout.
    static constexpr uint32_t BASE_RETRY_DELAY_MS  = 10;
    static constexpr uint32_t MAX_BACKOFF_SHIFT     = 20; // caps at ~10 s before lock_timeout clip

    // Helper: acquire a lock on one shard with retry/backoff honoring
    // config_.max_retries and config_.lock_timeout.
    auto acquire_lock = [&](const std::string& shard_id) -> bool {
        for (uint32_t attempt = 0; attempt <= config_.max_retries; ++attempt) {
            if (attempt > 0) {
                // Exponential backoff capped at lock_timeout.
                // Guard the left-shift against overflow by capping the exponent.
                const uint32_t shift = std::min(attempt - 1u, MAX_BACKOFF_SHIFT);
                auto delay = std::min(
                    config_.lock_timeout,
                    std::chrono::milliseconds(BASE_RETRY_DELAY_MS) * (1u << shift)
                );
                spdlog::debug("[Percolator] Retry {} for lock on shard {} (delay {}ms)",
                              attempt, shard_id, delay.count());
                std::this_thread::sleep_for(delay);
            }
            if (prepare_fn(shard_id, txn.transaction_id)) {
                return true;
            }
        }
        return false;
    };

    // ------------------------------------------------------------------
    // Phase 1 – PreWrite: acquire locks (secondary shards first, primary last).
    // ------------------------------------------------------------------
    const std::string primary_shard_id = txn.participants.begin()->first;

    std::vector<std::string> locked_shards;

    // Lock secondary shards first.
    for (auto& [shard_id, participant] : txn.participants) {
        if (shard_id == primary_shard_id) {
            continue;
        }

        spdlog::debug("[Percolator] Acquiring lock on secondary shard {} for txn {}",
                      shard_id, txn.transaction_id);

        bool locked = acquire_lock(shard_id);
        if (!locked) {
            spdlog::error("[Percolator] Lock acquisition failed on shard {} for txn {} "
                          "after {} retries",
                          shard_id, txn.transaction_id, config_.max_retries);
            for (const auto& ls : locked_shards) {
                abort_fn(ls, txn.transaction_id);
            }
            return false;
        }

        locked_shards.push_back(shard_id);
        participant.prepared = true;
    }

    // Lock the primary shard.
    spdlog::debug("[Percolator] Acquiring lock on primary shard {} for txn {}",
                  primary_shard_id, txn.transaction_id);

    bool primary_locked = acquire_lock(primary_shard_id);
    if (!primary_locked) {
        spdlog::error("[Percolator] Primary shard lock failed for txn {} after {} retries",
                      txn.transaction_id, config_.max_retries);
        for (const auto& ls : locked_shards) {
            abort_fn(ls, txn.transaction_id);
        }
        return false;
    }

    locked_shards.push_back(primary_shard_id);
    txn.participants.begin()->second.prepared = true;
    txn.state = TransactionState::PREPARED;

    // Log PREPARED state to WAL for crash-recovery.
    if (wal_) {
        try {
            nlohmann::json lock_meta = {
                {"protocol", "Percolator"},
                {"primary", primary_shard_id},
                {"locks_acquired", locked_shards}
            };
            wal_->logPrepare(txn.transaction_id, primary_shard_id, lock_meta);
        } catch (const std::exception& e) {
            spdlog::warn("[Percolator] WAL logPrepare failed: {}", e.what());
        }
    }

    spdlog::info("[Percolator] All locks acquired for txn {}", txn.transaction_id);

    // ------------------------------------------------------------------
    // Phase 2 – Assign TrueTime commit timestamp and perform commit-wait.
    // ------------------------------------------------------------------
    txn.state = TransactionState::COMMITTING;

    const int64_t commit_ts = computeCommitTimestamp();
    txn.commit_timestamp = commit_ts;

    spdlog::info("[Percolator] Commit timestamp for txn {} = {}",
                 txn.transaction_id, commit_ts);

    // Log commit decision to WAL (primary commit record).
    if (wal_) {
        try {
            nlohmann::json commit_meta = {
                {"protocol", "Percolator"},
                {"primary", primary_shard_id},
                {"commit_timestamp", commit_ts}
            };
            wal_->logCommit(txn.transaction_id, commit_meta);
        } catch (const std::exception& e) {
            spdlog::warn("[Percolator] WAL logCommit failed: {}", e.what());
        }
    }

    // Commit-wait: ensure commit_ts is definitely in the past before revealing it.
    commitWait(commit_ts);

    // ------------------------------------------------------------------
    // Phase 3 – Commit primary, then secondaries (releases locks).
    // ------------------------------------------------------------------
    bool primary_committed = commit_fn(primary_shard_id, txn.transaction_id);
    if (!primary_committed) {
        spdlog::error("[Percolator] Primary shard commit failed for txn {}",
                      txn.transaction_id);
        for (const auto& ls : locked_shards) {
            abort_fn(ls, txn.transaction_id);
        }
        return false;
    }

    txn.participants.begin()->second.committed = true;
    spdlog::info("[Percolator] Primary committed for txn {}", txn.transaction_id);

    // Commit secondaries (lock cleanup; can be retried asynchronously).
    for (auto& [shard_id, participant] : txn.participants) {
        if (shard_id == primary_shard_id) {
            continue;
        }

        bool committed = commit_fn(shard_id, txn.transaction_id);
        participant.committed = committed;

        if (committed) {
            // Log COMMITTED only after the RPC succeeds to preserve WAL integrity.
            if (wal_) {
                try {
                    wal_->logCommitted(txn.transaction_id, shard_id);
                } catch (const std::exception& e) {
                    spdlog::warn("[Percolator] WAL logCommitted failed for shard {}: {}",
                                 shard_id, e.what());
                }
            }
        } else {
            spdlog::warn("[Percolator] Secondary shard {} commit failed for txn {} "
                         "(will be cleaned up by background worker)",
                         shard_id, txn.transaction_id);
        }
    }

    spdlog::info("[Percolator] Transaction {} committed successfully", txn.transaction_id);
    return true;
}

size_t PercolatorCoordinator::cleanStaleLocks(
    const std::vector<std::string>& stale_txn_ids,
    CrossShardTransactionCoordinator& coordinator
) {
    size_t cleaned = 0;

    for (const auto& txn_id : stale_txn_ids) {
        auto txn_opt = coordinator.getTransaction(txn_id);
        if (!txn_opt.has_value()) {
            continue;
        }

        const auto& txn = *txn_opt;

        // Only abort Percolator transactions that are stuck in PREPARING or PREPARED.
        if (txn.protocol != TransactionProtocol::PERCOLATOR) {
            continue;
        }

        if (txn.state != TransactionState::PREPARING &&
            txn.state != TransactionState::PREPARED) {
            continue;
        }

        const auto age = std::chrono::system_clock::now() - txn.start_time;
        if (age < config_.stale_lock_threshold) {
            continue;
        }

        spdlog::info("[Percolator] Cleaning stale lock for txn {} (age {}s)",
                     txn_id,
                     std::chrono::duration_cast<std::chrono::seconds>(age).count());

        // Log the abort to WAL before issuing it.
        if (wal_) {
            try {
                wal_->logAbort(txn_id, "stale_lock_cleanup");
            } catch (const std::exception& e) {
                spdlog::warn("[Percolator] WAL logAbort failed for {}: {}", txn_id, e.what());
            }
        }

        if (coordinator.abort(txn_id)) {
            ++cleaned;
            spdlog::info("[Percolator] Stale lock cleaned for txn {}", txn_id);
        } else {
            spdlog::warn("[Percolator] Failed to clean stale lock for txn {}", txn_id);
        }
    }

    spdlog::info("[Percolator] cleanStaleLocks: cleaned {} / {} stale locks",
                 cleaned, stale_txn_ids.size());
    return cleaned;
}

} // namespace sharding
} // namespace themisdb
