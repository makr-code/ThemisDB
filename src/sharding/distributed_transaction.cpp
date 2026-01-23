// Copyright 2025 ThemisDB
// Licensed under MIT License
//
// Distributed Transaction Coordinator with Two-Phase Commit (2PC)
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
#include "utils/logger.h"
#include <random>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace themis::sharding {

DistributedTransactionCoordinator::DistributedTransactionCoordinator(
    std::shared_ptr<TrueTime> truetime,
    const Config& config
)
    : truetime_(truetime)
    , config_(config)
{
}

std::string DistributedTransactionCoordinator::beginTransaction(
    const std::vector<std::string>& shard_ids
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Generate unique transaction ID
    std::string txn_id = generateTransactionId();
    
    // Create transaction
    DistributedTransaction txn;
    txn.transaction_id = txn_id;
    txn.state = TransactionState::ACTIVE;
    txn.start_time = truetime_->now().latest;
    
    // Add participants
    for (const auto& shard_id : shard_ids) {
        TransactionParticipant participant;
        participant.shard_id = shard_id;
        participant.endpoint = "shard://" + shard_id; // Placeholder
        participant.prepared = false;
        participant.committed = false;
        txn.participants.push_back(participant);
    }
    
    transactions_[txn_id] = std::move(txn);
    total_transactions_.fetch_add(1, std::memory_order_relaxed);
    
    return txn_id;
}

bool DistributedTransactionCoordinator::addOperation(
    const std::string& txn_id,
    const std::string& shard_id,
    const nlohmann::json& operation
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
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

bool DistributedTransactionCoordinator::commit(const std::string& txn_id) {
    std::unique_lock<std::mutex> lock(mutex_);
    
    auto it = transactions_.find(txn_id);
    if (it == transactions_.end()) {
        return false;
    }
    
    auto& txn = it->second;
    if (txn.state != TransactionState::ACTIVE) {
        return false;
    }
    
    // Phase 1: Prepare
    txn.state = TransactionState::PREPARING;
    lock.unlock();
    
    bool prepared = preparePhase(txn);
    
    lock.lock();
    if (!prepared) {
        txn.state = TransactionState::ABORTING;
        lock.unlock();
        
        // Abort transaction
        for (auto& participant : txn.participants) {
            sendAbort(participant, txn_id);
        }
        
        lock.lock();
        txn.state = TransactionState::ABORTED;
        aborted_transactions_.fetch_add(1, std::memory_order_relaxed);
        return false;
    }
    
    txn.state = TransactionState::PREPARED;
    
    // Assign commit timestamp using TrueTime
    // Use the latest time to ensure all reads see this transaction
    txn.commit_time = truetime_->now().latest;
    
    // Wait until commit timestamp is definitely in the past
    // This is the key TrueTime operation for external consistency
    lock.unlock();
    truetime_->waitUntil(txn.commit_time);
    lock.lock();
    
    // Phase 2: Commit
    txn.state = TransactionState::COMMITTING;
    lock.unlock();
    
    bool committed = commitPhase(txn);
    
    lock.lock();
    if (committed) {
        txn.state = TransactionState::COMMITTED;
        committed_transactions_.fetch_add(1, std::memory_order_relaxed);
    } else {
        txn.state = TransactionState::ABORTED;
        aborted_transactions_.fetch_add(1, std::memory_order_relaxed);
    }
    
    return committed;
}

bool DistributedTransactionCoordinator::abort(const std::string& txn_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
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
    
    return true;
}

nlohmann::json DistributedTransactionCoordinator::executeReadOnly(
    const std::vector<std::string>& shard_ids,
    const nlohmann::json& operations
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

std::optional<TransactionState> DistributedTransactionCoordinator::getTransactionState(
    const std::string& txn_id
) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = transactions_.find(txn_id);
    if (it == transactions_.end()) {
        return std::nullopt;
    }
    
    return it->second.state;
}

nlohmann::json DistributedTransactionCoordinator::getStatistics() const {
    return nlohmann::json{
        {"total_transactions", total_transactions_.load()},
        {"committed_transactions", committed_transactions_.load()},
        {"aborted_transactions", aborted_transactions_.load()},
        {"readonly_transactions", readonly_transactions_.load()},
        {"active_transactions", transactions_.size()}
    };
}

bool DistributedTransactionCoordinator::preparePhase(DistributedTransaction& txn) {
    // Send prepare to all participants in parallel
    std::vector<std::thread> threads;
    std::atomic<bool> all_prepared{true};
    
    for (auto& participant : txn.participants) {
        threads.emplace_back([this, &participant, &txn, &all_prepared]() {
            if (!sendPrepare(participant, txn.transaction_id)) {
                all_prepared.store(false, std::memory_order_relaxed);
            }
        });
    }
    
    // Wait for all prepare requests to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    return all_prepared.load();
}

bool DistributedTransactionCoordinator::commitPhase(DistributedTransaction& txn) {
    // Send commit to all participants in parallel
    std::vector<std::thread> threads;
    std::atomic<bool> all_committed{true};
    
    for (auto& participant : txn.participants) {
        threads.emplace_back([this, &participant, &txn, &all_committed]() {
            if (!sendCommit(participant, txn.transaction_id, txn.commit_time)) {
                all_committed.store(false, std::memory_order_relaxed);
            }
        });
    }
    
    // Wait for all commit requests to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    return all_committed.load();
}

bool DistributedTransactionCoordinator::sendPrepare(
    TransactionParticipant& participant,
    const std::string& txn_id
) {
    // v1.3.0: Real RPC implementation for 2PC PREPARE
    try {
        ShardRPCClient::Config rpc_config;
        rpc_config.endpoint = participant.endpoint;
        rpc_config.timeout_ms = config_.rpc_timeout_ms;
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

bool DistributedTransactionCoordinator::sendCommit(
    TransactionParticipant& participant,
    const std::string& txn_id,
    std::chrono::nanoseconds commit_timestamp
) {
    // v1.3.0: Real RPC implementation for 2PC COMMIT
    try {
        ShardRPCClient::Config rpc_config;
        rpc_config.endpoint = participant.endpoint;
        rpc_config.timeout_ms = config_.rpc_timeout_ms;
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

bool DistributedTransactionCoordinator::sendAbort(
    TransactionParticipant& participant,
    const std::string& txn_id
) {
    // v1.3.0: Real RPC implementation for 2PC ABORT
    try {
        ShardRPCClient::Config rpc_config;
        rpc_config.endpoint = participant.endpoint;
        rpc_config.timeout_ms = config_.rpc_timeout_ms;
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

std::string DistributedTransactionCoordinator::generateTransactionId() {
    static std::random_device rd;
    static std::mt19937_64 gen(rd());
    static std::uniform_int_distribution<uint64_t> dis;
    
    std::ostringstream oss;
    oss << "txn-" << std::hex << std::setfill('0') << std::setw(16) << dis(gen);
    return oss.str();
}

void DistributedTransactionCoordinator::cleanupOldTransactions() {
    std::lock_guard<std::mutex> lock(mutex_);
    
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

} // namespace themis::sharding
